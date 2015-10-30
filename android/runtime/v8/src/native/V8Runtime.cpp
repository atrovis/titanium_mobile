/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2015 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 */
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <v8.h>
#include <v8-debug.h>

#include "AndroidUtil.h"
#include "EventEmitter.h"
#include "JavaObject.h"
#include "JNIUtil.h"
#include "JSException.h"
#include "KrollBindings.h"
#include "ProxyFactory.h"
#include "ScriptsModule.h"
#include "TypeConverter.h"
#include "V8Util.h"

#include "V8Runtime.h"

#include "org_appcelerator_kroll_runtime_v8_V8Runtime.h"

#define TAG "V8Runtime"

// The port number on which the V8 debugger will listen on.
#define V8_DEBUGGER_PORT 9999

namespace titanium {

Persistent<Context> V8Runtime::globalContext;
Persistent<Object> V8Runtime::krollGlobalObject;
Persistent<Array> V8Runtime::moduleContexts;

jobject V8Runtime::javaInstance;
Isolate* v8_isolate = nullptr;
bool V8Runtime::debuggerEnabled = false;
bool V8Runtime::DBG = false;

/* static */
void V8Runtime::collectWeakRef(Persistent<Value> ref, void *parameter)
{
	jobject v8Object = (jobject) parameter;
	ref.Reset();
	JNIScope::getEnv()->DeleteGlobalRef(v8Object);
}

Local<Object> V8Runtime::Global()
{
	return krollGlobalObject.Get(v8_isolate);
}

Local<Context> V8Runtime::GlobalContext()
{
	return globalContext.Get(v8_isolate);
}

Local<Object> V8Runtime::ModuleObject()
{
	return moduleObject.Get(v8_isolate);
}

Local<Function> V8Runtime::RunModuleFunction()
{
	return runModuleFunction.Get(v8_isolate);
}

// Minimalistic logging function for internal JS
static void krollLog(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);
	uint32_t len = args.Length();

	if (len < 2) {
		JSException::Error(isolate, "log: missing required tag and message arguments");
		return;
	}

	Local<String> tag = args[0]->ToString(isolate);
	Local<String> message = args[1]->ToString(isolate);
	Local<String> space = FIXED_ONE_BYTE_STRING(isolate, " ");
	for (uint32_t i = 2; i < len; ++i) {
		message = String::Concat(String::Concat(message, space), args[i]->ToString(isolate));
	}

	String::Utf8Value tagValue(tag);
	String::Utf8Value messageValue(message);
	__android_log_print(ANDROID_LOG_DEBUG, *tagValue, *messageValue);
}

/* static */
void V8Runtime::bootstrap(Local<Context> context)
{
	Isolate* isolate = context->GetIsolate();
	EventEmitter::initTemplate(context);

	Local<Object> global = Object::New(isolate);
	krollGlobalObject.Reset(isolate, global);
	Local<Array> mc = Array::New(isolate);
	moduleContexts.Reset(isolate, mc);

	KrollBindings::initFunctions(global, context);

	SetMethod(isolate, global, "log", krollLog);
	// Move this into the EventEmitter::initTemplate call?
	Local<FunctionTemplate> eect = Local<FunctionTemplate>::New(isolate, EventEmitter::constructorTemplate);
	global->Set(FIXED_ONE_BYTE_STRING(isolate, "EventEmitter"), eect->GetFunction());

	global->Set(SYMBOL_LITERAL(isolate, "runtime"), FIXED_ONE_BYTE_STRING(isolate, "v8"));
	global->Set(SYMBOL_LITERAL(isolate, "DBG"), v8::Boolean::New(isolate, V8Runtime::DBG));
	global->Set(SYMBOL_LITERAL(isolate, "moduleContexts"), mc);

	LOG_TIMER(TAG, "Executing kroll.js");

	TryCatch tryCatch;
	Local<Value> result = V8Util::executeString(isolate, KrollBindings::getMainSource(isolate), FIXED_ONE_BYTE_STRING(isolate, "ti:/kroll.js"));

	if (tryCatch.HasCaught()) {
		V8Util::reportException(isolate, tryCatch, true);
	}
	if (!result->IsFunction()) {
		LOGF(TAG, "kroll.js result is not a function");
		V8Util::reportException(isolate, tryCatch, true);
	}

	Local<Function> mainFunction = Local<Function>::Cast(result);
	Local<Value> args[] = { global };
	mainFunction->Call(global, 1, args);

	if (tryCatch.HasCaught()) {
		V8Util::reportException(isolate, tryCatch, true);
		LOGE(TAG, "Caught exception while bootstrapping Kroll");
	}
}

static void logV8Exception(Local<Message> msg, Local<Value> data)
{
	HandleScope scope(v8_isolate);

	// Log reason and location of the error.
	LOGD(TAG, *String::Utf8Value(msg->Get()));
	LOGD(TAG, "%s @ %d >>> %s",
		*String::Utf8Value(msg->GetScriptResourceName()),
		msg->GetLineNumber(),
		*String::Utf8Value(msg->GetSourceLine()));
}

static jmethodID dispatchDebugMessage = NULL;

static void dispatchHandler()
{
	static JNIEnv *env = NULL;
	if (!env) {
		titanium::JNIUtil::javaVm->AttachCurrentThread(&env, NULL);
	}

	env->CallVoidMethod(V8Runtime::javaInstance, dispatchDebugMessage);
}

} // namespace titanium

#ifdef __cplusplus
extern "C" {
#endif

using namespace titanium;

/*
 * Class:     org_appcelerator_kroll_runtime_v8_V8Runtime
 * Method:    nativeInit
 * Signature: (Lorg/appcelerator/kroll/runtime/v8/V8Runtime;)J
 */
JNIEXPORT void JNICALL Java_org_appcelerator_kroll_runtime_v8_V8Runtime_nativeInit(JNIEnv *env, jobject self, jboolean useGlobalRefs, jint debuggerPort, jboolean DBG, jboolean profilerEnabled)
{
	if (profilerEnabled) {
		char* argv[] = { const_cast<char*>(""), const_cast<char*>("--expose-gc") };
		int argc = sizeof(argv)/sizeof(*argv);
		V8::SetFlagsFromCommandLine(&argc, argv, false);
	}

	titanium::JNIScope jniScope(env);

	// Log all uncaught V8 exceptions.
	V8::AddMessageListener(logV8Exception);
	V8::SetCaptureStackTraceForUncaughtExceptions(true);

	JavaObject::useGlobalRefs = useGlobalRefs;
	V8Runtime::debuggerEnabled = debuggerPort >= 0;
	V8Runtime::DBG = DBG;

	V8Runtime::javaInstance = env->NewGlobalRef(self);
	JNIUtil::initCache();

	Isolate::CreateParams params;
	Isolate* isolate = Isolate::New(params);
	v8_isolate = isolate;

	Isolate::Scope isolate_scope(isolate);
    HandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
	context->Enter();

	V8Runtime::globalContext.Reset(isolate, context);
	V8Runtime::bootstrap(context);

	if (V8Runtime::debuggerEnabled) {
		jclass v8RuntimeClass = env->FindClass("org/appcelerator/kroll/runtime/v8/V8Runtime");
		dispatchDebugMessage = env->GetMethodID(v8RuntimeClass, "dispatchDebugMessages", "()V");

		// FIXME Fix up debugger!
		//Debug::SetMessageHandler(dispatchHandler);
		//Debug::EnableAgent("titanium", debuggerPort, true);
	}

	LOG_HEAP_STATS(TAG);
}

/*
 * Class:     org_appcelerator_kroll_runtime_v8_V8Runtime
 * Method:    nativeRunModule
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_appcelerator_kroll_runtime_v8_V8Runtime_nativeRunModule
	(JNIEnv *env, jobject self, jstring source, jstring filename, jobject activityProxy)
{
	HandleScope scope(v8_isolate);
	titanium::JNIScope jniScope(env);

	if (V8Runtime::moduleObject.IsEmpty()) {
		Local<Object> module = V8Runtime::Global()->Get(FIXED_ONE_BYTE_STRING(v8_isolate, "Module"))->ToObject(v8_isolate);
		V8Runtime::moduleObject.Reset(v8_isolate, module);

		V8Runtime::runModuleFunction.Reset(v8_isolate,
			Local<Function>::Cast(module->Get(FIXED_ONE_BYTE_STRING(v8_isolate, "runModule"))));
	}

	Local<Value> jsSource = TypeConverter::javaStringToJsString(v8_isolate, env, source);
	Local<Value> jsFilename = TypeConverter::javaStringToJsString(v8_isolate, env, filename);
	Local<Value> jsActivity = TypeConverter::javaObjectToJsValue(v8_isolate, env, activityProxy);

	Local<Value> args[] = { jsSource, jsFilename, jsActivity };
	TryCatch tryCatch;
	V8Runtime::RunModuleFunction()->Call(V8Runtime::ModuleObject(), 3, args);

	if (tryCatch.HasCaught()) {
		V8Util::openJSErrorDialog(v8_isolate, tryCatch);
		V8Util::reportException(v8_isolate, tryCatch, true);
	}
}

JNIEXPORT jobject JNICALL Java_org_appcelerator_kroll_runtime_v8_V8Runtime_nativeEvalString
	(JNIEnv *env, jobject self, jstring source, jstring filename)
{
	HandleScope scope(v8_isolate);
	titanium::JNIScope jniScope(env);

	Local<Value> jsSource = TypeConverter::javaStringToJsString(v8_isolate, env, source);
	if (jsSource.IsEmpty() || !jsSource->IsString()) {
		LOGE(TAG, "Error converting Javascript string, aborting evalString");
		return NULL;
	}

	Local<Value> jsFilename = TypeConverter::javaStringToJsString(v8_isolate, env, filename);

	TryCatch tryCatch;
	Local<Script> script = Script::Compile(jsSource->ToString(v8_isolate), jsFilename->ToString(v8_isolate));
	Local<Value> result = script->Run();

	if (tryCatch.HasCaught()) {
		V8Util::openJSErrorDialog(v8_isolate, tryCatch);
		V8Util::reportException(v8_isolate, tryCatch, true);
		return NULL;
	}

	return TypeConverter::jsValueToJavaObject(v8_isolate, env, result);
}

JNIEXPORT void JNICALL Java_org_appcelerator_kroll_runtime_v8_V8Runtime_nativeProcessDebugMessages(JNIEnv *env, jobject self)
{
	v8::Debug::ProcessDebugMessages();
}

JNIEXPORT jboolean JNICALL Java_org_appcelerator_kroll_runtime_v8_V8Runtime_nativeIdle(JNIEnv *env, jobject self)
{
	return V8Runtime::v8_isolate->IdleNotificationDeadline(100); // FIXME What is a good value to use here?
}

/*
 * Called by V8Runtime.java, this passes a KrollSourceCodeProvider java class instance
 * to KrollBindings, where it's stored and later used to retrieve an external CommonJS module's
 * Javascript code when require(moduleName) occurs in Javascript.
 * "External" CommonJS modules are CommonJS modules stored in external modules.
 */
JNIEXPORT void JNICALL Java_org_appcelerator_kroll_runtime_v8_V8Runtime_nativeAddExternalCommonJsModule
	(JNIEnv *env, jobject self, jstring moduleName, jobject sourceProvider)
{
	const char* mName = env->GetStringUTFChars(moduleName, NULL);
	jclass cls = env->GetObjectClass(sourceProvider);

	if (!cls) {
		LOGE(TAG, "Could not find source code provider class for module: %s", mName);
		return;
	}

	jmethodID method = env->GetMethodID(cls, "getSourceCode", "(Ljava/lang/String;)Ljava/lang/String;");
	if (!method) {
		LOGE(TAG, "Could not find getSourceCode method in source code provider class for module: %s", mName);
		return;
	}

	KrollBindings::addExternalCommonJsModule(mName, env->NewGlobalRef(sourceProvider), method);
}

// This method disposes of all native resources used by V8 when
// all activities have been destroyed by the application.
//
// When a Persistent handle is Dispose()'d in V8, the internal
// pointer is not changed, handle->IsEmpty() returns false. 
// As a consequence, we have to explicitly reset the handle
// to an empty handle using Persistent<Type>()
//
// Since we use lazy initialization in a lot of our code,
// there's probably not an easier way (unless we use boolean flags)

JNIEXPORT void JNICALL Java_org_appcelerator_kroll_runtime_v8_V8Runtime_nativeDispose(JNIEnv *env, jobject runtime)
{
	JNIScope jniScope(env);

	// We use a new scope here so any new handles we create
	// while disposing are cleaned up before our global context
	// is disposed below.
	{
		HandleScope scope(v8_isolate);

		// Any module that has been require()'d or opened via Window URL
		// will be cleaned up here. We setup the initial "moduleContexts"
		// Array and expose it on kroll above in nativeInit, and
		// module.js will insert module contexts into this array in
		// Module.prototype._runScript
		uint32_t length = V8Runtime::ModuleContexts()->Length();
		for (uint32_t i = 0; i < length; ++i) {
			Local<Value> moduleContext = V8Runtime::ModuleContexts()->Get(i);

			// WrappedContext is simply a C++ wrapper for the V8 Context object,
			// and is used to expose the Context to javascript. See ScriptsModule for
			// implementation details
			WrappedContext *wrappedContext = WrappedContext::Unwrap(moduleContext->ToObject(v8_isolate));
			ASSERT(wrappedContext != NULL);

			wrappedContext->Dispose();
		}

		// KrollBindings
		KrollBindings::dispose();
		EventEmitter::dispose();

		V8Runtime::moduleContexts.Reset();

		V8Runtime::GlobalContext()->DetachGlobal();

	}

	// Dispose of each class' static cache / resources

	V8Util::dispose();
	ProxyFactory::dispose();

	V8Runtime::moduleObject.Reset();

	V8Runtime::runModuleFunction.Reset();

	V8Runtime::krollGlobalObject.Reset();

	V8Runtime::GlobalContext()->Exit();
	V8Runtime::globalContext.Reset();

	// Removes the retained global reference to the V8Runtime 
	env->DeleteGlobalRef(V8Runtime::javaInstance);

	V8Runtime::javaInstance = NULL;

	// Whereas most calls to IdleNotification get kicked off via Java (the looper's
	// idle event in V8Runtime.java), we can't count on that running anymore at this point.
	// So as our last act, run IdleNotification until it returns true so we can clean up all
	// the stuff we just released references for above.
	while (!V8Runtime::v8_isolate->IdleNotification(100));
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	JNIUtil::javaVm = vm;
	return JNI_VERSION_1_4;
}

#ifdef __cplusplus
}
#endif
