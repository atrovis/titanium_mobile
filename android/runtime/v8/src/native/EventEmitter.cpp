/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2015 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 *
 * Original code Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
 */
#include <jni.h>
#include <v8.h>

#include "AndroidUtil.h"
#include "EventEmitter.h"
#include "TypeConverter.h"
#include "V8Util.h"
#include "JNIUtil.h"
#include "V8Runtime.h"

#define TAG "EventEmitter"

using namespace v8;

namespace titanium {

Persistent<FunctionTemplate> EventEmitter::constructorTemplate;

static Persistent<String> eventsSymbol;
Persistent<String> EventEmitter::emitSymbol;

void EventEmitter::eventEmitterConstructor(const FunctionCallbackInfo<Value>& args)
{
	HandleScope scope(args.GetIsolate());
	EventEmitter *emitter = new EventEmitter();
	emitter->Wrap(args.This());
}

void EventEmitter::initTemplate(Local<Context> context)
{
	Isolate* isolate = context->GetIsolate();
	HandleScope scope(isolate);
	Local<FunctionTemplate> constructor = FunctionTemplate::New(isolate, eventEmitterConstructor);
	constructor->InstanceTemplate()->SetInternalFieldCount(1);
	constructor->SetClassName(FIXED_ONE_BYTE_STRING(isolate, "EventEmitter"));
	constructorTemplate.Reset(isolate, constructor);

	eventsSymbol.Reset(isolate, FIXED_ONE_BYTE_STRING(isolate, "_events"));
	emitSymbol.Reset(isolate, FIXED_ONE_BYTE_STRING(isolate, "emit"));
}

void EventEmitter::dispose()
{
	constructorTemplate.Reset();
	eventsSymbol.Reset();
	emitSymbol.Reset();
}

bool EventEmitter::emit(Handle<String> event, int argc, Handle<Value> *argv)
{
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	Local<Object> self = handle(); 

	Local<Value> events_v = self->Get(eventsSymbol.Get(isolate));
	if (!events_v->IsObject()) return false;

	Local<Object> events = events_v->ToObject(isolate);

	Local<Value> listeners_v = events->Get(event);
	TryCatch try_catch;

	if (listeners_v->IsFunction()) {
		// Optimized one-listener case
		Local<Function> listener = Local<Function>::Cast(listeners_v);
		listener->Call(self, argc, argv);
		if (try_catch.HasCaught()) {
			V8Util::fatalException(isolate, try_catch);
			return false;
		}
	} else if (listeners_v->IsArray()) {
		Local<Array> listeners = Local<Array>::Cast(listeners_v->ToObject(isolate)->Clone());
		for (uint32_t i = 0; i < listeners->Length(); ++i) {
			Local<Value> listener_v = listeners->Get(i);
			if (!listener_v->IsFunction()) continue;
			Local<Function> listener = Local<Function>::Cast(listener_v);
			listener->Call(self, argc, argv);
			if (try_catch.HasCaught()) {
				V8Util::fatalException(isolate, try_catch);
				return false;
			}
		}
	} else {
		return false;
	}

	return true;
}

} // namespace titanium

