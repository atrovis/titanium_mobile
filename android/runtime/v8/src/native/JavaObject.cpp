/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2015 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 */

#include "AndroidUtil.h"
#include "EventEmitter.h"
#include "JavaObject.h"
#include "JNIUtil.h"
#include "ReferenceTable.h"

#include <v8.h>

using namespace v8;

#define TAG "JavaObject"

namespace titanium {

bool JavaObject::useGlobalRefs = true;

static jobject objectMap;

#ifdef TI_DEBUG
static struct {
	int total;
	int detached;
} stats = {0, 0};

#define UPDATE_STATS(_total, _detached) \
	stats.total += _total; \
	stats.detached += _detached; \
	LOGD(TAG, ">>> JavaObject: total=%i detached=%i <<<", stats.total, stats.detached);
#else
#define UPDATE_STATS(total, detached)
#endif

// Callback for V8 letting us know the JavaScript object is no longer reachable.
// Once we receive this callback we can safely release our strong reference
// on the wrapped Java object so it can become eligible for collection.
static void DetachCallback(const v8::WeakCallbackData<v8::Object, JavaObject>& data) {
    JavaObject* javaObject = data.GetParameter();
    javaObject->detach();
}

JavaObject::JavaObject(jobject javaObject)
	: EventEmitter()
	, javaObject_(NULL)
	, refTableKey_(0)
	, isWeakRef_(false)
{
	UPDATE_STATS(1, 1);

	if (javaObject) {
		attach(javaObject);
	}
}

// Create a strong reference to the wrapped Java object
// to prevent it from becoming garbage collected by Dalvik.
void JavaObject::newGlobalRef()
{
	JNIEnv *env = JNIUtil::getJNIEnv();
	ASSERT(env != NULL);

	if (useGlobalRefs) {
		ASSERT(javaObject_ != NULL);
		jobject globalRef = env->NewGlobalRef(javaObject_);
		if (isWeakRef_) {
			env->DeleteWeakGlobalRef(javaObject_);
			isWeakRef_ = false;
		}
		javaObject_ = globalRef;
	} else {
		ASSERT(refTableKey_ == 0);
		refTableKey_ = ReferenceTable::createReference(javaObject_);
		javaObject_ = NULL;
	}
}

// Returns a global reference to the wrapped Java object.
// If the object has become "detached" this will re-attach
// it to ensure the Java object will not get collected.
jobject JavaObject::getJavaObject()
{
	if (useGlobalRefs) {
		ASSERT(javaObject_ != NULL);

		// We must always return a valid Java proxy reference.
		// Otherwise we risk crashing in the calling code.
		// If we are "detached" we will re-attach whenever the Java
		// proxy is requested.
		if (isDetached()) {
			attach(NULL);
		}

		return javaObject_;
	} else {
		if (isWeakRef_) {
			UPDATE_STATS(0, -1);
			jobject javaObject = ReferenceTable::clearWeakReference(refTableKey_);
			if (javaObject == NULL) {
				LOGE(TAG, "Java object reference has been invalidated.");
			}
			isWeakRef_ = false;
			persistent().SetWeak(this, DetachCallback);
			return javaObject;
		}
		return ReferenceTable::getReference(refTableKey_);
	}
}

// Convert our strong reference to the Java object into a weak
// reference to allow it to become eligible for collection by Dalvik.
// This typically happens once V8 has detected the JavaScript object
// that wraps the Java object is no longer reachable.
void JavaObject::weakGlobalRef()
{
	JNIEnv *env = JNIUtil::getJNIEnv();
	ASSERT(env != NULL);

	if (useGlobalRefs) {
		ASSERT(javaObject_ != NULL);
		jweak weakRef = env->NewWeakGlobalRef(javaObject_);
		env->DeleteGlobalRef(javaObject_);
		javaObject_ = weakRef;
	} else {
		ReferenceTable::makeWeakReference(refTableKey_);
	}

	isWeakRef_ = true;
}

// Deletes the reference to the wrapped Java object.
// This should only happen once this object is no longer
// needed and about to be deleted.
void JavaObject::deleteGlobalRef()
{
	JNIEnv *env = JNIUtil::getJNIEnv();
	ASSERT(env != NULL);

	if (useGlobalRefs) {
		ASSERT(javaObject_ != NULL);
		if (isWeakRef_) {
			env->DeleteWeakGlobalRef(javaObject_);
		} else {
			env->DeleteGlobalRef(javaObject_);
		}
		javaObject_ = NULL;
	} else {
		ReferenceTable::destroyReference(refTableKey_);
		refTableKey_ = 0;
	}
}

JavaObject::~JavaObject()
{
	UPDATE_STATS(-1, isDetached() ? -1 : 0);

	if (javaObject_ || refTableKey_ > 0) {
		deleteGlobalRef();
	}
}

// Do we need this special method instead of just using NativeObject::Wrap?
void JavaObject::wrap(Isolate* isolate, Local<Object> jsObject)
{
	ASSERT(persistent().IsEmpty());
	ASSERT(jsObject->InternalFieldCount() > 0);
	jsObject->SetAlignedPointerInInternalField(0, this);
	persistent().Reset(isolate, jsObject);
}

// Attaches the Java object to this native wrapper.
// This wrapper will create a global reference to the
// Java object and keep it from becoming collected by Dalvik
// until it is detached or made weak (weakGlobalRef()).
void JavaObject::attach(jobject javaObject)
{
	ASSERT((javaObject && javaObject_ == NULL) || javaObject == NULL);
	UPDATE_STATS(0, -1);


	persistent().SetWeak(this, DetachCallback);
    persistent().MarkIndependent();

	if (javaObject) {
		javaObject_ = javaObject;
	}
	newGlobalRef();
}

void JavaObject::detach()
{
	persistent().SetWeak(this, DetachCallback);

	if (isDetached()) {
		return;
	}

	UPDATE_STATS(0, 1);

	weakGlobalRef();
}

bool JavaObject::isDetached()
{
	return (javaObject_ == NULL && refTableKey_ == 0) || isWeakRef_;
}

}

