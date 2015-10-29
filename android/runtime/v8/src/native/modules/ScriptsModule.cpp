/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2015 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 */
#include <v8.h>
#include <jni.h>

#include "AndroidUtil.h"
#include "NativeObject.h"
#include "ScriptsModule.h"
#include "V8Runtime.h"
#include "V8Util.h"
#include "JNIUtil.h"
#include "TypeConverter.h"
#include "JSException.h"

#define TAG "ScriptsModule"

namespace titanium {
using namespace v8;

Persistent<FunctionTemplate> WrappedScript::constructor_template;
Persistent<ObjectTemplate> WrappedContext::global_template;

void WrappedContext::Initialize(Local<Object> target, Local<Context> context)
{
	Isolate* isolate = context->GetIsolate();
	HandleScope scope(isolate);

	Local<ObjectTemplate> gt = ObjectTemplate::New(isolate);
	gt->SetInternalFieldCount(1);
	global_template(isolate, gt);
}

WrappedContext* WrappedContext::Unwrap(Local<Object> global)
{
	HandleScope scope;
	return NativeObject::Unwrap<WrappedContext>(global->GetPrototype().As<Object>());
}

WrappedContext::WrappedContext(Isolate* isolate, Local<Context> context)
	: context_(isolate, context)
{
	HandleScope scope(isolate);

	Local<Object> globalProxy = context->Global();
	Local<Object> global = globalProxy->GetPrototype().As<Object>();
	Wrap(global);
}

WrappedContext::~WrappedContext()
{
	if (!context_.IsEmpty()) {
		GetV8Context(Isolate::GetCurrent())->DetachGlobal();
		context_.Reset();
	}
}

Local<Context> WrappedContext::GetV8Context(Isolate* isolate)
{
	return Local<Context>::New(isolate, context_);
}

void WrappedScript::Initialize(Local<Object> target, Local<Context> context)
{
	Isolate* isolate = context->GetIsolate();
	HandleScope scope(isolate);

	Local<FunctionTemplate> constructor = FunctionTemplate::New(isolate, WrappedScript::New);
	constructor->InstanceTemplate()->SetInternalFieldCount(1);
	constructor->SetClassName(FIXED_ONE_BYTE_STRING(isolate, "Script"));

	constructor_template.Reset(constructor, ft);

	SetProtoMethod(constructor, "runInContext", WrappedScript::RunInContext);
	SetProtoMethod(constructor, "runInThisContext", WrappedScript::RunInThisContext);
	SetProtoMethod(constructor, "runInNewContext", WrappedScript::RunInNewContext);

	SetTemplateMethod(constructor, "createContext", WrappedScript::CreateContext);
	SetTemplateMethod(constructor, "disposeContext", WrappedScript::DisposeContext);
	SetTemplateMethod(constructor, "runInContext", WrappedScript::CompileRunInContext);
	SetTemplateMethod(constructor, "runInThisContext", WrappedScript::CompileRunInThisContext);
	SetTemplateMethod(constructor, "runInNewContext", WrappedScript::CompileRunInNewContext);

	target->Set(FIXED_ONE_BYTE_STRING(isolate, "Script"), constructor->GetFunction());
}

void WrappedScript::New(const FunctionCallbackInfo<Value>& args)
{
	if (!args.IsConstructCall()) {
		return V8Util::newInstanceFromConstructorTemplate(constructor_template, args);
	}

	HandleScope scope(args.GetIsolate());
	WrappedScript *t = new WrappedScript();
	t->Wrap(args.Holder());
	args.GetReturnValue().Set(WrappedScript::EvalMachine<compileCode, thisContext, wrapExternal>(args));
}

WrappedScript::~WrappedScript()
{
	script_.Reset();
}

void WrappedScript::CreateContext(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	EscapableHandleScope scope(isolate);

	Persistent<Context> context = Context::New(isolate, WrappedContext::global_template);
	WrappedContext *wrappedContext = new WrappedContext(isolate, context);
	Local<Object> global = context->Global();

	// Allow current context access to newly created context's objects.
	context->SetSecurityToken(Context::GetCurrent()->GetSecurityToken());

	// If a sandbox is provided initial the new context's global with it.
	if (args.Length() > 0) {
		Local<Object> sandbox = args[0]->ToObject(isolate);
		Local<Array> keys = sandbox->GetPropertyNames();

		for (uint32_t i = 0; i < keys->Length(); i++) {
			Local<String> key = keys->Get(Integer::New(isolate, i))->ToString(isolate);
			Local<Value> value = sandbox->Get(key);
			if (value == sandbox) {
				value = global;
			}
			global->Set(key, value);
		}
	}

	return scope.Escape(global);
}

void WrappedScript::DisposeContext(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	if (args.Length() < 1) {
		return JSException::Error(isolate, "Must pass the context as the first argument.");
	}

	WrappedContext* wrappedContext = WrappedContext::Unwrap(args[0]->ToObject(isolate));
	delete wrappedContext;
}

void WrappedScript::RunInContext(const FunctionCallbackInfo<Value>& args)
{
	return WrappedScript::EvalMachine<unwrapExternal, userContext, returnResult>(args);
}

void WrappedScript::RunInThisContext(const FunctionCallbackInfo<Value>& args)
{
	return WrappedScript::EvalMachine<unwrapExternal, thisContext, returnResult>(args);
}

void WrappedScript::RunInNewContext(const FunctionCallbackInfo<Value>& args)
{
	return WrappedScript::EvalMachine<unwrapExternal, newContext, returnResult>(args);
}

void WrappedScript::CompileRunInContext(const FunctionCallbackInfo<Value>& args)
{
	return WrappedScript::EvalMachine<compileCode, userContext, returnResult>(args);
}

void WrappedScript::CompileRunInThisContext(const FunctionCallbackInfo<Value>& args)
{
	return WrappedScript::EvalMachine<compileCode, thisContext, returnResult>(args);
}

void WrappedScript::CompileRunInNewContext(const FunctionCallbackInfo<Value>& args)
{
	return WrappedScript::EvalMachine<compileCode, newContext, returnResult>(args);
}

template<WrappedScript::EvalInputFlags input_flag, WrappedScript::EvalContextFlags context_flag,
	WrappedScript::EvalOutputFlags output_flag>
void WrappedScript::EvalMachine(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	if (input_flag == compileCode && args.Length() < 1) {
		return isolate->ThrowException(Exception::TypeError(String::New(isolate, "needs at least 'code' argument.")));
	}

	const int sandbox_index = input_flag == compileCode ? 1 : 0;
	if (context_flag == userContext && args.Length() < (sandbox_index + 1)) {
		return isolate->ThrowException(Exception::TypeError(String::New("needs a 'context' argument.")));
	}

	Local<String> code;
	if (input_flag == compileCode) code = args[0]->ToString(isolate);

	Local<Object> sandbox;
	if (context_flag == newContext) {
		sandbox = args[sandbox_index]->IsObject() ? args[sandbox_index]->ToObject(isolate) : Object::New(isolate);
	} else if (context_flag == userContext) {
		sandbox = args[sandbox_index]->ToObject(isolate);
	}

	int filename_offset = 1;
	if (context_flag == thisContext) {
		filename_offset = 0;
	}

	const int filename_index = sandbox_index + filename_offset;
	Local<String> filename =
		args.Length() > filename_index ? args[filename_index]->ToString(isolate) : String::New(isolate, "evalmachine.<anonymous>");

	const int display_error_index = args.Length() - 1;
	bool display_error = false;
	if (args.Length() > display_error_index && args[display_error_index]->IsBoolean()
		&& args[display_error_index]->BooleanValue() == true) {
		display_error = true;
	}

	Persistent<Context> context;

	Local<Array> keys;
	unsigned int i;
	WrappedContext *nContext = NULL;
	Local<Object> contextArg;

	if (context_flag == newContext) {
		// Create the new context
		context = Context::New(isolate);

	} else if (context_flag == userContext) {
		// Use the passed in context
		contextArg = args[sandbox_index]->ToObject(isolate);
		nContext = WrappedContext::Unwrap(contextArg);
		context = nContext->GetV8Context();
	}

	// New and user context share code. DRY it up.
	if (context_flag == userContext || context_flag == newContext) {
		// Enter the context
		context->Enter();
	}

	Local<Value> result;
	Local<Script> script;

	if (input_flag == compileCode) {
		// well, here WrappedScript::New would suffice in all cases, but maybe
		// Compile has a little better performance where possible
		script = output_flag == returnResult ? Script::Compile(code, filename) : Script::New(code, filename);
		if (script.IsEmpty()) {
			// Hack because I can't get a proper stacktrace on SyntaxError
			return Undefined(isolate);
		}
	} else {
		WrappedScript *n_script = NativeObject::Unwrap<WrappedScript>(args.Holder());
		if (!n_script) {
			return isolate->ThrowException(Exception::Error(String::New(isolate, "Must be called as a method of Script.")));
		} else if (n_script->script_.IsEmpty()) {
			return isolate->ThrowException(Exception::Error(String::New(isolate, "'this' must be a result of previous "
				"new Script(code) call.")));
		}

		script = n_script->script_;
	}

	if (output_flag == returnResult) {
		result = script->Run();
		if (result.IsEmpty()) {
			if (context_flag == newContext) {
				context->DetachGlobal();
				context->Exit();
				context.Dispose();
			}
			return Undefined();
		}
	} else {
		WrappedScript *n_script = NativeObject::Unwrap<WrappedScript>(args.Holder());
		if (!n_script) {
			return isolate->ThrowException(Exception::Error(String::New(isolate, "Must be called as a method of Script.")));
		}
		n_script->script_ = Persistent<Script>::New(script);
		result = args.This();
	}

	if (context_flag == newContext) {
		// Clean up, clean up, everybody everywhere!
		context->DetachGlobal();
		context->Exit();
		context.Dispose();
	} else if (context_flag == userContext) {
		// Exit the passed in context.
		context->Exit();
	}

	if (result->IsObject()) {
		Local<Context> creation = result->ToObject(isolate)->CreationContext();
	}

	args.GetReturnValue().Set(result);
}

void ScriptsModule::Initialize(Local<Object> target, Local<Context> context)
{
	Isolate* isolate = context->GetIsolate();
	HandleScope scope(isolate);
	WrappedContext::Initialize(target, context);
	WrappedScript::Initialize(target, context);
}

void ScriptsModule::Dispose()
{
	WrappedScript::constructor_template.Reset();
	WrappedContext::global_template.Reset();
}

}
