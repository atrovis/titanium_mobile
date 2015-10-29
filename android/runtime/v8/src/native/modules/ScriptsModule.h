/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2015 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 */
#ifndef SCRIPTS_MODULE_H
#define SCRIPTS_MODULE_H

#include <v8.h>
#include "../NativeObject.h"

namespace titanium {

using v8::Local;
using v8::Object;
using v8::Context;
using v8::Persistent;
using v8::FunctionTemplate;
using v8::FunctionCallbackInfo;
using v8::Value;
using v8::ObjectTemplate;
using v8::Script;

class ScriptsModule
{
public:
	static void Initialize(Local<Object> target, Local<Context> context);
	static void Dispose();
};

class WrappedContext: NativeObject
{
public:
	WrappedContext(Persistent<Context> context);
	virtual ~WrappedContext();

	static void Initialize(Local<Object> target, Local<Context> context);

	// Unwrap a context from the given global proxy object.
	static WrappedContext* Unwrap(Local<Object> global);

	Local<Context> GetV8Context();

	static Persistent<ObjectTemplate> global_template;

protected:
	Persistent<Context> context_;
};

class WrappedScript: NativeObject
{
public:
	static void Initialize(Local<Object> target, Local<Context> context);

	enum EvalInputFlags
	{
		compileCode, unwrapExternal
	};
	enum EvalContextFlags
	{
		thisContext, newContext, userContext
	};
	enum EvalOutputFlags
	{
		returnResult, wrapExternal
	};

	template<EvalInputFlags input_flag, EvalContextFlags context_flag, EvalOutputFlags output_flag>
	static void EvalMachine(const FunctionCallbackInfo<Value>& args);

	static Persistent<FunctionTemplate> constructor_template;

	static void New(const FunctionCallbackInfo<Value>& args);
	static void CreateContext(const FunctionCallbackInfo<Value>& args);
	static void DisposeContext(const FunctionCallbackInfo<Value>& args);
	static void RunInContext(const FunctionCallbackInfo<Value>& args);
	static void RunInThisContext(const FunctionCallbackInfo<Value>& args);
	static void RunInNewContext(const FunctionCallbackInfo<Value>& args);
	static void CompileRunInContext(const FunctionCallbackInfo<Value>& args);
	static void CompileRunInThisContext(const FunctionCallbackInfo<Value>& args);
	static void CompileRunInNewContext(const FunctionCallbackInfo<Value>& args);

protected:

	WrappedScript()
		: NativeObject()
	{
	}
	virtual ~WrappedScript();

	Persistent<Script> script_;
};

}

#endif
