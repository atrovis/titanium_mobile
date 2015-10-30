/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2012 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 */

#include <string.h>

#include <v8.h>
#include "V8Util.h"
#include "JNIUtil.h"
#include "JSException.h"
#include "AndroidUtil.h"
#include "TypeConverter.h"


namespace titanium {
using namespace v8;

#define TAG "V8Util"

Local<String> ImmutableAsciiStringLiteral::CreateFromLiteral(Isolate* isolate, const char *stringLiteral, size_t length)
{
	EscapableHandleScope scope(isolate);
	MaybeLocal<String> result = String::NewExternalOneByte(isolate, new ImmutableAsciiStringLiteral(stringLiteral, length));
	return scope.Escape(result.ToLocalChecked());
}

Local<Value> V8Util::executeString(Isolate* isolate, Local<String> source, Local<Value> filename)
{
	EscapableHandleScope scope(isolate);
	TryCatch tryCatch;

	Local<Script> script = Script::Compile(source, filename->ToString(isolate));
	if (script.IsEmpty()) {
		LOGF(TAG, "Script source is empty");
		reportException(isolate, tryCatch, true);
		return scope.Escape(Undefined(isolate));
	}

	Local<Value> result = script->Run();
	if (result.IsEmpty()) {
		LOGF(TAG, "Script result is empty");
		reportException(isolate, tryCatch, true);
		return scope.Escape(Undefined(isolate));
	}

	return scope.Escape(result);
}

Local<Value> V8Util::newInstanceFromConstructorTemplate(Persistent<FunctionTemplate>& t, const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	EscapableHandleScope scope(isolate);
	const int argc = args.Length();
	Local<Value>* argv = new Local<Value> [argc];

	for (int i = 0; i < argc; ++i) {
		argv[i] = args[i];
	}

	Local<Object> instance = t.Get(isolate)->GetFunction()->NewInstance(argc, argv);
	delete[] argv;
	return scope.Escape(instance);
}

void V8Util::objectExtend(Local<Object> dest, Local<Object> src)
{
	Local<Array> names = src->GetOwnPropertyNames();
	int length = names->Length();

	for (int i = 0; i < length; ++i) {
		Local<Value> name = names->Get(i);
		Local<Value> value = src->Get(name);
		dest->Set(name, value);
	}
}

#define EXC_TAG "V8Exception"

static Persistent<String> nameSymbol, messageSymbol;

void V8Util::reportException(Isolate* isolate, TryCatch &tryCatch, bool showLine)
{
	HandleScope scope(isolate);
	Local<Message> message = tryCatch.Message();

	if (nameSymbol.IsEmpty()) {
		nameSymbol.Reset(isolate, SYMBOL_LITERAL(isolate, "name"));
		messageSymbol.Reset(isolate, SYMBOL_LITERAL(isolate, "message"));
	}

	if (showLine) {
		Local<Message> message = tryCatch.Message();
		if (!message.IsEmpty()) {
			String::Utf8Value filename(message->GetScriptResourceName());
			String::Utf8Value msg(message->Get());
			int linenum = message->GetLineNumber();
			LOGE(EXC_TAG, "Exception occurred at %s:%i: %s", *filename, linenum, *msg);
		}
	}

	Local<Value> stackTrace = tryCatch.StackTrace();
	String::Utf8Value trace(tryCatch.StackTrace());

	if (trace.length() > 0 && !stackTrace->IsUndefined()) {
		LOGD(EXC_TAG, *trace);
	} else {
		Local<Value> exception = tryCatch.Exception();
		if (exception->IsObject()) {
			Local<Object> exceptionObj = exception->ToObject(isolate);
			Local<Value> message = exceptionObj->Get(messageSymbol.Get(isolate));
			Local<Value> name = exceptionObj->Get(nameSymbol.Get(isolate));

			if (!message->IsUndefined() && !name->IsUndefined()) {
				String::Utf8Value nameValue(name);
				String::Utf8Value messageValue(message);
				LOGE(EXC_TAG, "%s: %s", *nameValue, *messageValue);
			}
		} else {
			String::Utf8Value error(exception);
			LOGE(EXC_TAG, *error);
		}
	}
}

void V8Util::openJSErrorDialog(Isolate* isolate, TryCatch &tryCatch)
{
	JNIEnv *env = JNIUtil::getJNIEnv();
	if (!env) {
		return;
	}

	Local<Message> message = tryCatch.Message();

	jstring title = env->NewStringUTF("Runtime Error");
	jstring errorMessage = TypeConverter::jsValueToJavaString(isolate, env, message->Get());
	jstring resourceName = TypeConverter::jsValueToJavaString(isolate, env, message->GetScriptResourceName());
	jstring sourceLine = TypeConverter::jsValueToJavaString(isolate, env, message->GetSourceLine());

	env->CallStaticVoidMethod(
		JNIUtil::krollRuntimeClass,
		JNIUtil::krollRuntimeDispatchExceptionMethod,
		title,
		errorMessage,
		resourceName,
		message->GetLineNumber(),
		sourceLine,
		message->GetEndColumn());

	env->DeleteLocalRef(title);
	env->DeleteLocalRef(errorMessage);
	env->DeleteLocalRef(resourceName);
	env->DeleteLocalRef(sourceLine);
}

static int uncaughtExceptionCounter = 0;

void V8Util::fatalException(Isolate* isolate, TryCatch &tryCatch)
{
	HandleScope scope(isolate);

	// Check if uncaught_exception_counter indicates a recursion
	if (uncaughtExceptionCounter > 0) {
		reportException(isolate, tryCatch, true);
		LOGF(TAG, "Double exception fault");
	}
	reportException(isolate, tryCatch, true);
}

Local<String> V8Util::jsonStringify(Isolate* isolate, Local<Value> value)
{
	HandleScope scope(isolate);

	Local<Object> json = isolate->GetCurrentContext()->Global()->Get(FIXED_ONE_BYTE_STRING(isolate, "JSON"))->ToObject(isolate);
	Local<Function> stringify = Local<Function>::Cast(json->Get(FIXED_ONE_BYTE_STRING(isolate, "stringify")));
	Local<Value> args[] = { value };
	Local<Value> result = stringify->Call(json, 1, args);
    if (result.IsEmpty()) {
        LOGE(TAG, "!!!! JSON.stringify() result is null/undefined.!!!");
        return FIXED_ONE_BYTE_STRING(isolate, "ERROR");
    } else {
        return result->ToString(isolate);
    }
}

bool V8Util::constructorNameMatches(Isolate* isolate, Local<Object> object, const char* name)
{
	HandleScope scope(isolate);
	Local<String> constructorName = object->GetConstructorName();
	return strcmp(*String::Utf8Value(constructorName), name) == 0;
}

static Persistent<Function> isNaNFunction;

bool V8Util::isNaN(Isolate* isolate, Local<Value> value)
{
	HandleScope scope(isolate);
	Local<Object> global = isolate->GetCurrentContext()->Global();

	if (isNaNFunction.IsEmpty()) {
		Local<Value> isNaNValue = global->Get(SYMBOL_LITERAL(isolate, "isNaN"));
		isNaNFunction.Reset(isolate, isNaNValue.As<Function>());
	}

	Local<Value> args[] = { value };

	return isNaNFunction.Get(isolate)->Call(global, 1, args)->BooleanValue();
}

void V8Util::dispose()
{
	nameSymbol.Reset();
	messageSymbol.Reset();
	isNaNFunction.Reset();
}

}
