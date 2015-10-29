/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2015 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 */
#include <jni.h>
#include <stdio.h>
#include <v8.h>

#include "AndroidUtil.h"
#include "TypeConverter.h"
#include "JNIUtil.h"
#include "JavaObject.h"
#include "ProxyFactory.h"
#include "V8Runtime.h"

#define TAG "TypeConverter"

using namespace titanium;

/****************************** public methods ******************************/
jshort TypeConverter::jsNumberToJavaShort(v8::Local<v8::Number> jsNumber)
{
	return ((jshort) jsNumber->Value());
}

v8::Local<v8::Number> TypeConverter::javaShortToJsNumber(v8::Isolate* isolate, jshort javaShort)
{
	return v8::Number::New(isolate, (double) javaShort);
}

jint TypeConverter::jsNumberToJavaInt(v8::Local<v8::Number> jsNumber)
{
	return ((jint) jsNumber->Value());
}

v8::Local<v8::Number> TypeConverter::javaIntToJsNumber(v8::Isolate* isolate, jint javaInt)
{
	return v8::Number::New(isolate, (double) javaInt);
}

jlong TypeConverter::jsNumberToJavaLong(v8::Local<v8::Number> jsNumber)
{
	return ((jlong) jsNumber->Value());
}

v8::Local<v8::Number> TypeConverter::javaLongToJsNumber(v8::Isolate* isolate, jlong javaLong)
{
	return v8::Number::New(isolate, (double) javaLong);
}

jfloat TypeConverter::jsNumberToJavaFloat(v8::Local<v8::Number> jsNumber)
{
	return ((jfloat) jsNumber->Value());
}

v8::Local<v8::Number> TypeConverter::javaFloatToJsNumber(v8::Isolate* isolate, jfloat javaFloat)
{
	return v8::Number::New(isolate, (double) javaFloat);
}

jdouble TypeConverter::jsNumberToJavaDouble(v8::Local<v8::Number> jsNumber)
{
	return ((jdouble) jsNumber->Value());
}

v8::Local<v8::Number> TypeConverter::javaDoubleToJsNumber(v8::Isolate* isolate, jdouble javaDouble)
{
	return v8::Number::New(isolate, javaDouble);
}

jboolean TypeConverter::jsBooleanToJavaBoolean(v8::Local<v8::Boolean> jsBoolean)
{
	return (jsBoolean->Value()) == JNI_TRUE;
}

v8::Local<v8::Boolean> TypeConverter::javaBooleanToJsBoolean(v8::Isolate* isolate, jboolean javaBoolean)
{
	return v8::Boolean::New(isolate, (bool) javaBoolean);
}

jstring TypeConverter::jsStringToJavaString(v8::Local<v8::String> jsString)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsStringToJavaString(env, jsString);
}

jstring TypeConverter::jsStringToJavaString(JNIEnv *env, v8::Local<v8::String> jsString)
{
	v8::String::Value javaString(jsString);
	return env->NewString(*javaString, javaString.length());
}

jstring TypeConverter::jsValueToJavaString(v8::Isolate* isolate, v8::Local<v8::Value> jsValue)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsValueToJavaString(isolate, env, jsValue);
}

jstring TypeConverter::jsValueToJavaString(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Value> jsValue)
{
	if (jsValue->IsNull()) {
		return NULL;
	}

	v8::Local<v8::String> jsString = jsValue->ToString(isolate);
	v8::String::Value javaString(jsString);
	return env->NewString(*javaString, javaString.length());
}

v8::Local<v8::Value> TypeConverter::javaStringToJsString(v8::Isolate* isolate, jstring javaString)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::String::Empty(isolate);
	}
	return TypeConverter::javaStringToJsString(isolate, env, javaString);
}

v8::Local<v8::Value> TypeConverter::javaStringToJsString(v8::Isolate* isolate, JNIEnv *env, jstring javaString)
{
	if (!javaString) {
		return v8::Null(isolate);
	}

	int nativeStringLength = env->GetStringLength(javaString);
	const jchar *nativeString = env->GetStringChars(javaString, NULL);
	v8::Local<v8::String> jsString = v8::String::New(isolate, nativeString, nativeStringLength);
	env->ReleaseStringChars(javaString, nativeString);

	return jsString;
}

jobject TypeConverter::jsDateToJavaDate(v8::Isolate* isolate, v8::Local<v8::Date> jsDate)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsDateToJavaDate(isolate, env, jsDate);
}

jobject TypeConverter::jsDateToJavaDate(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Date> jsDate)
{
	return env->NewObject(JNIUtil::dateClass, JNIUtil::dateInitMethod, (jlong) jsDate->NumberValue(isolate));
}

jlong TypeConverter::jsDateToJavaLong(v8::Isolate* isolate, v8::Local<v8::Date> jsDate)
{
	return (jlong) jsDate->NumberValue(isolate);
}

v8::Local<v8::Date> TypeConverter::javaDateToJsDate(v8::Isolate* isolate, jobject javaDate)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Date::New(isolate, 0);
	}
	return TypeConverter::javaDateToJsDate(isolate, env, javaDate);
}

v8::Local<v8::Date> TypeConverter::javaDateToJsDate(v8::Isolate* isolate, JNIEnv *env, jobject javaDate)
{
	jlong epochTime = env->CallLongMethod(javaDate, JNIUtil::dateGetTimeMethod);
	return v8::Date::New(isolate, (double) epochTime);
}

v8::Local<v8::Date> TypeConverter::javaLongToJsDate(v8::Isolate* isolate, jlong javaLong)
{
	return v8::Date::New(isolate, (double) javaLong);
}

jobject TypeConverter::jsObjectToJavaFunction(v8::Isolate* isolate, v8::Local<v8::Object> jsObject)
{
	JNIEnv *env = JNIScope::getEnv();
	if (!env) {
		return NULL;
	}
	return TypeConverter::jsObjectToJavaFunction(isolate, env, jsObject);
}

jobject TypeConverter::jsObjectToJavaFunction(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Object> jsObject)
{
	Persistent<Function> jsFunction(isolate, v8::Local<Function>::Cast(jsObject));
	jsFunction.MarkIndependent();

	jlong ptr = (jlong) *jsFunction;
	return env->NewObject(JNIUtil::v8FunctionClass, JNIUtil::v8FunctionInitMethod, ptr);
}

v8::Local<v8::Function> TypeConverter::javaObjectToJsFunction(v8::Isolate* isolate, jobject javaObject)
{
	JNIEnv *env = JNIScope::getEnv();
	if (!env) {
		return v8::Function::New(isolate);
	}
	return TypeConverter::javaObjectToJsFunction(isolate, env, javaObject);
}

v8::Local<v8::Function> TypeConverter::javaObjectToJsFunction(v8::Isolate* isolate, JNIEnv *env, jobject javaObject)
{
	jlong v8ObjectPointer = env->GetLongField(javaObject, JNIUtil::v8ObjectPtrField);
	return v8::Local<v8::Function>(reinterpret_cast<v8::Function*>(v8ObjectPointer));
}

jobjectArray TypeConverter::jsArgumentsToJavaArray(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	JNIEnv *env = JNIScope::getEnv();
	if (!env) {
		return NULL;
	}
	return TypeConverter::jsArgumentsToJavaArray(env, args);
}

jobjectArray TypeConverter::jsArgumentsToJavaArray(JNIEnv *env, const v8::FunctionCallbackInfo<v8::Value>& args)
{
	HandleScope scope;
	int argCount = args.Length();
	jobjectArray javaArgs = env->NewObjectArray(argCount, JNIUtil::objectClass, NULL);

	for (int i = 0; i < argCount; ++i)
	{
		Local<Value> v8Arg = args[i];
		bool isNew;
		jobject javaArg = jsValueToJavaObject(v8Arg, &isNew);
		env->SetObjectArrayElement(javaArgs, i, javaArg);

		if (isNew) {
			env->DeleteLocalRef(javaArg);
		}
	}

	return javaArgs;
}

// call "delete" on the return value otherwise the memory will never be released
v8::Local<v8::Value> * TypeConverter::javaObjectArrayToJsArguments(jobjectArray javaObjectArray, int *length)
{
	JNIEnv *env = JNIScope::getEnv();
	if (!env) {
		return NULL;
	}
	return TypeConverter::javaObjectArrayToJsArguments(env, javaObjectArray, length);
}

v8::Local<v8::Value> * TypeConverter::javaObjectArrayToJsArguments(JNIEnv *env, jobjectArray javaObjectArray, int *length)
{
	jsize javaArrayLength = env->GetArrayLength(javaObjectArray);
	v8::Local<v8::Value> *jsArguments = new v8::Local<v8::Value>[javaArrayLength];
	for (int i = 0; i < javaArrayLength; i++)
	{
		jobject arrayElement = env->GetObjectArrayElement(javaObjectArray, i);
		jsArguments[i] = TypeConverter::javaObjectToJsValue(env, arrayElement);
		env->DeleteLocalRef(arrayElement);
	}

	*length = javaArrayLength;
	return jsArguments;
}

jarray TypeConverter::jsArrayToJavaArray(v8::Isolate* isolate, v8::Local<v8::Array> jsArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsArrayToJavaArray(isolate, env, jsArray);
}

jarray TypeConverter::jsArrayToJavaArray(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Array> jsArray)
{
	int arrayLength = jsArray->Length();
	jobjectArray javaArray = env->NewObjectArray(arrayLength, JNIUtil::objectClass, NULL);
	if (javaArray == NULL) {
		LOGE(TAG, "unable to create new jobjectArray");
		return NULL;
	}

	for (int i = 0; i < arrayLength; i++) {
		v8::Local<v8::Value> element = jsArray->Get(i);
		bool isNew;

		jobject javaObject = jsValueToJavaObject(isolate, element, &isNew);
		env->SetObjectArrayElement(javaArray, i, javaObject);

		if (isNew) {
			env->DeleteLocalRef(javaObject);
		}
	}

	return javaArray;
}

jobjectArray TypeConverter::jsArrayToJavaStringArray(v8::Local<v8::Array> jsArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsArrayToJavaStringArray(env, jsArray);
}

jobjectArray TypeConverter::jsArrayToJavaStringArray(JNIEnv *env, v8::Local<v8::Array> jsArray)
{
	int arrayLength = jsArray->Length();
	jobjectArray javaArray = env->NewObjectArray(arrayLength, JNIUtil::stringClass, NULL);
	if (javaArray == NULL) {
		LOGE(TAG, "unable to create new jobjectArray");
		return NULL;
	}

	for (int i = 0; i < arrayLength; i++) {
		v8::Local<v8::Value> element = jsArray->Get(i);
		jstring javaObject = jsStringToJavaString(env, element->ToString());
		env->SetObjectArrayElement(javaArray, i, javaObject);

		env->DeleteLocalRef(javaObject);
	}

	return javaArray;
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(v8::Isolate* isolate, jbooleanArray javaBooleanArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Array::New(isolate);
	}
	return TypeConverter::javaArrayToJsArray(isolate, env, javaBooleanArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(v8::Isolate* isolate, JNIEnv *env, jbooleanArray javaBooleanArray)
{
	int arrayLength = env->GetArrayLength(javaBooleanArray);
	v8::Local<v8::Array> jsArray = v8::Array::New(isolate, arrayLength);

	jboolean *arrayElements = env->GetBooleanArrayElements(javaBooleanArray, 0);
	for (int i = 0; i < arrayLength; i++) {
		jsArray->Set((uint32_t) i, v8::Boolean::New(isolate, arrayElements[i]));
	}

	return jsArray;
}

jshortArray TypeConverter::jsArrayToJavaShortArray(v8::Isolate* isolate, v8::Local<v8::Array> jsArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsArrayToJavaShortArray(isolate, env, jsArray);
}

jshortArray TypeConverter::jsArrayToJavaShortArray(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Array> jsArray)
{
	int arrayLength = jsArray->Length();
	jshortArray javaShortArray = env->NewShortArray(arrayLength);
	if (javaShortArray == NULL) {
		LOGE(TAG, "unable to create new jshortArray");
		return NULL;
	}

	jshort* shortBuffer = new jshort[arrayLength];
	for (int i = 0; i < arrayLength; i++) {
		v8::Local<v8::Value> element = jsArray->Get(i);
		shortBuffer[i] = TypeConverter::jsNumberToJavaShort(element->ToNumber(isolate));
	}
	env->SetShortArrayRegion(javaShortArray, 0, arrayLength, shortBuffer);

	return javaShortArray;
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(jshortArray javaShortArray)
{
	return javaShortArrayToJsNumberArray(javaShortArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(JNIEnv *env, jshortArray javaShortArray)
{
	return javaShortArrayToJsNumberArray(env, javaShortArray);
}

jintArray TypeConverter::jsArrayToJavaIntArray(v8::Local<v8::Array> jsArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsArrayToJavaIntArray(env, jsArray);
}

jintArray TypeConverter::jsArrayToJavaIntArray(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Array> jsArray)
{
	int arrayLength = jsArray->Length();
	jintArray javaIntArray = env->NewIntArray(arrayLength);
	if (javaIntArray == NULL) {
		LOGE(TAG, "unable to create new jintArray");
		return NULL;
	}

	jint* intBuffer = new jint[arrayLength];
	for (int i = 0; i < arrayLength; i++) {
		v8::Local<v8::Value> element = jsArray->Get(i);
		intBuffer[i] = TypeConverter::jsNumberToJavaInt(element->ToNumber(isolate));
	}
	env->SetIntArrayRegion(javaIntArray, 0, arrayLength, intBuffer);

	return javaIntArray;
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(v8::Isolate* isolate, jintArray javaIntArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Array::New(isolate);
	}
	return TypeConverter::javaArrayToJsArray(isolate, env, javaIntArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(v8::Isolate* isolate, JNIEnv *env, jintArray javaIntArray)
{
	int arrayLength = env->GetArrayLength(javaIntArray);
	v8::Local<v8::Array> jsArray = v8::Array::New(isolate, arrayLength);

	jint *arrayElements = env->GetIntArrayElements(javaIntArray, 0);
	for (int i = 0; i < arrayLength; i++) {
		jsArray->Set((uint32_t) i, v8::Integer::New(isolate, arrayElements[i]));
	}

	return jsArray;
}

jlongArray TypeConverter::jsArrayToJavaLongArray(v8::Isolate* isolate, v8::Local<v8::Array> jsArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsArrayToJavaLongArray(isolate, env, jsArray);
}

jlongArray TypeConverter::jsArrayToJavaLongArray(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Array> jsArray)
{
	int arrayLength = jsArray->Length();
	jlongArray javaLongArray = env->NewLongArray(arrayLength);
	if (javaLongArray == NULL) {
		LOGE(TAG, "unable to create new jlongArray");
		return NULL;
	}

	jlong* longBuffer = new jlong[arrayLength];
	for (int i = 0; i < arrayLength; i++) {
		v8::Local<v8::Value> element = jsArray->Get(i);
		longBuffer[i] = TypeConverter::jsNumberToJavaLong(element->ToNumber(isolate));
	}
	env->SetLongArrayRegion(javaLongArray, 0, arrayLength, longBuffer);

	return javaLongArray;
}

jfloatArray TypeConverter::jsArrayToJavaFloatArray(v8::Isolate* isolate, v8::Local<v8::Array> jsArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
    return TypeConverter::jsArrayToJavaFloatArray(isolate, env, jsArray);
}
  
jfloatArray TypeConverter::jsArrayToJavaFloatArray(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Array> jsArray)
{
	int arrayLength = jsArray->Length();
	jfloatArray javaFloatArray = env->NewFloatArray(arrayLength);
	if (javaFloatArray == NULL) {
		LOGE(TAG, "unable to create new jfloatArray");
		return NULL;
	}
    
	jfloat* floatBuffer = new jfloat[arrayLength];
	for (int i = 0; i < arrayLength; i++) {
		v8::Local<v8::Value> element = jsArray->Get(i);
		floatBuffer[i] = TypeConverter::jsNumberToJavaFloat(element->ToNumber(isolate));
	}
	env->SetFloatArrayRegion(javaFloatArray, 0, arrayLength, floatBuffer);
    
	return javaFloatArray;
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(jlongArray javaLongArray)
{
	return javaLongArrayToJsNumberArray(javaLongArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(JNIEnv *env, jlongArray javaLongArray)
{
	return javaLongArrayToJsNumberArray(env, javaLongArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(jfloatArray javaFloatArray)
{
	return javaFloatArrayToJsNumberArray(javaFloatArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(JNIEnv *env, jfloatArray javaFloatArray)
{
	return javaFloatArrayToJsNumberArray(env, javaFloatArray);
}

jdoubleArray TypeConverter::jsArrayToJavaDoubleArray(v8::Local<v8::Array> jsArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsArrayToJavaDoubleArray(env,jsArray);
}

jdoubleArray TypeConverter::jsArrayToJavaDoubleArray(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Array> jsArray)
{
	int arrayLength = jsArray->Length();
	jdoubleArray javaDoubleArray = env->NewDoubleArray(arrayLength);
	if (javaDoubleArray == NULL) {
		LOGE(TAG, "unable to create new jdoubleArray");
		return NULL;
	}

	jdouble* doubleBuffer = new jdouble[arrayLength];
	for (int i = 0; i < arrayLength; i++) {
		v8::Local<v8::Value> element = jsArray->Get(i);
		doubleBuffer[i] = TypeConverter::jsNumberToJavaDouble(element->ToNumber(isolate));
	}
	env->SetDoubleArrayRegion(javaDoubleArray, 0, arrayLength, doubleBuffer);

	return javaDoubleArray;
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(jdoubleArray javaDoubleArray)
{
	return javaDoubleArrayToJsNumberArray(javaDoubleArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(JNIEnv *env, jdoubleArray javaDoubleArray)
{
	return javaDoubleArrayToJsNumberArray(env, javaDoubleArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(v8::Isolate* isolate, jobjectArray javaObjectArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Array::New(isolate);
	}
	return TypeConverter::javaArrayToJsArray(isolate, env, javaObjectArray);
}

v8::Local<v8::Array> TypeConverter::javaArrayToJsArray(v8::Isolate* isolate, JNIEnv *env, jobjectArray javaObjectArray)
{
	int arrayLength = env->GetArrayLength(javaObjectArray);
	v8::Local<v8::Array> jsArray = v8::Array::New(isolate, arrayLength);

	for (int i = 0; i < arrayLength; i++) {
		jobject javaArrayElement = env->GetObjectArrayElement(javaObjectArray, i);
		v8::Local<v8::Value> jsArrayElement = TypeConverter::javaObjectToJsValue(env, javaArrayElement);
		jsArray->Set((uint32_t) i, jsArrayElement);
		env->DeleteLocalRef(javaArrayElement);
	}

	return jsArray;
}

// converts js value to java object and recursively converts sub objects if this
// object is a container type
jobject TypeConverter::jsValueToJavaObject(v8::Isolate* isolate, v8::Local<v8::Value> jsValue, bool *isNew)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsValueToJavaObject(isolate, env, jsValue, isNew);
}

jobject TypeConverter::jsValueToJavaObject(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Value> jsValue, bool *isNew)
{
	if (jsValue->IsNumber()) {
		*isNew = true;
		if (jsValue->IsInt32()) {
			jint javaInt = TypeConverter::jsNumberToJavaInt(jsValue->ToNumber(isolate));
			return env->NewObject(JNIUtil::integerClass, JNIUtil::integerInitMethod, javaInt);
		}
		jdouble javaDouble = TypeConverter::jsNumberToJavaDouble(jsValue->ToNumber(isolate));
		return env->NewObject(JNIUtil::doubleClass, JNIUtil::doubleInitMethod, javaDouble);

	} else if (jsValue->IsBoolean()) {
		jboolean javaBoolean = TypeConverter::jsBooleanToJavaBoolean(jsValue->ToBoolean(isolate));
		*isNew = true;
		return env->NewObject(JNIUtil::booleanClass, JNIUtil::booleanInitMethod, javaBoolean);

	} else if (jsValue->IsString()) {
		*isNew = true;
		return TypeConverter::jsStringToJavaString(env, jsValue->ToString(isolate));

	} else if (jsValue->IsDate()) {
		Local<Date> date = Local<Date>::Cast<Value>(jsValue);
		return TypeConverter::jsDateToJavaDate(env, date);

	} else if (jsValue->IsArray()) {
		*isNew = true;
		return TypeConverter::jsArrayToJavaArray(env, v8::Local<v8::Array>::Cast(jsValue));

	} else if (jsValue->IsFunction()) {
		*isNew = true;
		return TypeConverter::jsObjectToJavaFunction(env, jsValue->ToObject(isolate));

	} else if (jsValue->IsObject()) {
		v8::Local<v8::Object> jsObject = jsValue->ToObject(isolate);

		if (JavaObject::isJavaObject(jsObject)) {
			*isNew = JavaObject::useGlobalRefs ? false : true;
			JavaObject *javaObject = JavaObject::Unwrap<JavaObject>(jsObject);
			return javaObject->getJavaObject();
		} else {
			v8::Local<v8::Array> objectKeys = jsObject->GetOwnPropertyNames();
			int numKeys = objectKeys->Length();
			*isNew = true;
			jobject javaHashMap = env->NewObject(JNIUtil::hashMapClass, JNIUtil::hashMapInitMethod, numKeys);

			for (int i = 0; i < numKeys; i++) {
				v8::Local<v8::Value> jsObjectPropertyKey = objectKeys->Get((uint32_t) i);
				bool keyIsNew, valueIsNew;
				jobject javaObjectPropertyKey = TypeConverter::jsValueToJavaObject(env, jsObjectPropertyKey, &keyIsNew);
				v8::Local<v8::Value> jsObjectPropertyValue = jsObject->Get(jsObjectPropertyKey);
				jobject javaObjectPropertyValue = TypeConverter::jsValueToJavaObject(env, jsObjectPropertyValue, &valueIsNew);

				jobject result = env->CallObjectMethod(javaHashMap,
				                                       JNIUtil::hashMapPutMethod,
				                                       javaObjectPropertyKey,
				                                       javaObjectPropertyValue);
				env->DeleteLocalRef(result);

				if (keyIsNew) {
					env->DeleteLocalRef(javaObjectPropertyKey);
				}
				if (valueIsNew) {
					env->DeleteLocalRef(javaObjectPropertyValue);
				}
			}

			return javaHashMap;
		}
	}

	if (!jsValue->IsNull() && !jsValue->IsUndefined()) {
		LOGW(TAG, "jsValueToJavaObject returning null.");
	}
	return NULL;
}

// converts js object to kroll dict and recursively converts sub objects if this
// object is a container type
jobject TypeConverter::jsObjectToJavaKrollDict(v8::Isolate* isolate, v8::Local<v8::Value> jsValue, bool *isNew)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsObjectToJavaKrollDict(isolate, env,jsValue,isNew);
}

jobject TypeConverter::jsObjectToJavaKrollDict(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Value> jsValue, bool *isNew)
{
	if (jsValue->IsObject())
	{
		v8::Local<v8::Object> jsObject = jsValue->ToObject(isolate);
		v8::Local<v8::Array> objectKeys = jsObject->GetOwnPropertyNames();
		int numKeys = objectKeys->Length();
		*isNew = true;
		jobject javaKrollDict = env->NewObject(JNIUtil::krollDictClass, JNIUtil::krollDictInitMethod, numKeys);

		for (int i = 0; i < numKeys; i++) {
			v8::Local<v8::Value> jsObjectPropertyKey = objectKeys->Get((uint32_t) i);
			bool keyIsNew, valueIsNew;
			jobject javaObjectPropertyKey = TypeConverter::jsValueToJavaObject(env, jsObjectPropertyKey, &keyIsNew);
			v8::Local<v8::Value> jsObjectPropertyValue = jsObject->Get(jsObjectPropertyKey);
			jobject javaObjectPropertyValue = TypeConverter::jsValueToJavaObject(env, jsObjectPropertyValue, &valueIsNew);

			jobject result = env->CallObjectMethod(javaKrollDict,
				                                   JNIUtil::krollDictPutMethod,
				                                   javaObjectPropertyKey,
				                                   javaObjectPropertyValue);
			env->DeleteLocalRef(result);

			if (keyIsNew) {
				env->DeleteLocalRef(javaObjectPropertyKey);
			}
			if (valueIsNew) {
				env->DeleteLocalRef(javaObjectPropertyValue);
			}
		}

		return javaKrollDict;
	}

	if (!jsValue->IsNull() && !jsValue->IsUndefined()) {
		LOGW(TAG, "jsObjectToJavaKrollDict returning null.");
	}
	return NULL;
}


// converts js value to java error
jobject TypeConverter::jsValueToJavaError(v8::Local<v8::Value> jsValue, bool* isNew)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return NULL;
	}
	return TypeConverter::jsValueToJavaError(env,jsValue,isNew);
}

jobject TypeConverter::jsValueToJavaError(v8::Isolate* isolate, JNIEnv *env, v8::Local<v8::Value> jsValue, bool* isNew)
{
	if (jsValue->IsObject()) {
		v8::Local<v8::Object> jsObject = jsValue->ToObject(isolate);

		// If it's a java object, we just return null for now.
		if (!JavaObject::isJavaObject(jsObject)) {

			Local<String> stackString = String::New(isolate, "stack"), messageString = String::New(isolate, "message");
			if (jsObject->HasOwnProperty(stackString) || jsObject->HasOwnProperty(messageString)) {
				bool keyIsNew, valueIsNew;
				*isNew = true;
				v8::Local<v8::Value> jsObjectMessageProperty = jsObject->GetRealNamedProperty(messageString);
				v8::Local<v8::Value> jsObjectStackProperty = jsObject->GetRealNamedProperty(stackString);

				return env->NewObject(JNIUtil::krollExceptionClass, JNIUtil::krollExceptionInitMethod,
							TypeConverter::jsValueToJavaString(env, jsObjectMessageProperty), TypeConverter::jsValueToJavaString(env, jsObjectStackProperty));
			}
		}

	} else  {
		*isNew = true;
		return env->NewObject(JNIUtil::krollExceptionClass, JNIUtil::krollExceptionInitMethod,
			TypeConverter::jsValueToJavaString(env, jsValue), NULL);
	} 

	if (!jsValue->IsNull() && !jsValue->IsUndefined()) {
		LOGW(TAG, "jsValueToJavaObject returning null.");
	}
	return NULL;
}

// converts java hashmap to js value and recursively converts sub objects if this
// object is a container type. If javaObject is NULL, an empty object is created.
v8::Local<v8::Object> TypeConverter::javaHashMapToJsValue(v8::Isolate* isolate, JNIEnv *env, jobject javaObject)
{
	v8::Local<v8::Object> jsObject = v8::Object::New(isolate);
	if (!javaObject || !env) {
		return jsObject;
	}

	jobject hashMapSet = env->CallObjectMethod(javaObject, JNIUtil::hashMapKeySetMethod);
	jobjectArray hashMapKeys = (jobjectArray) env->CallObjectMethod(hashMapSet, JNIUtil::setToArrayMethod);
	env->DeleteLocalRef(hashMapSet);

	int hashMapKeysLength = env->GetArrayLength(hashMapKeys);
	bool isStringHashMap = env->IsInstanceOf(hashMapKeys, JNIUtil::stringArrayClass);

	for (int i = 0; i < hashMapKeysLength; i++) {
		jobject javaPairKey = env->GetObjectArrayElement(hashMapKeys, i);
		v8::Local<v8::Value> jsPairKey;
		if (isStringHashMap) {
			jstring javaString = (jstring)javaPairKey;
			int nativeStringLength = env->GetStringLength(javaString);
			const jchar *nativeString = env->GetStringChars(javaString, NULL);
			jsPairKey = v8::String::New(isolate, nativeString, nativeStringLength);
			env->ReleaseStringChars(javaString, nativeString);
		} else {
			jsPairKey = TypeConverter::javaObjectToJsValue(isolate, env, javaPairKey);
		}

		jobject javaPairValue = env->CallObjectMethod(javaObject, JNIUtil::hashMapGetMethod, javaPairKey);
		env->DeleteLocalRef(javaPairKey);

		jsObject->Set(jsPairKey, TypeConverter::javaObjectToJsValue(isolate, env, javaPairValue));
		env->DeleteLocalRef(javaPairValue);
	}

	env->DeleteLocalRef(hashMapKeys);

	return jsObject;
}

// converts java object to js value and recursively converts sub objects if this
// object is a container type
v8::Local<v8::Value> TypeConverter::javaObjectToJsValue(v8::Isolate* isolate, jobject javaObject)
{
	if (!javaObject) {
		return v8::Null(isolate);
	}

	JNIEnv *env = JNIScope::getEnv();
	if (!env) {
		return v8::Undefined(isolate);
	}
	return TypeConverter::javaObjectToJsValue(isolate, env, javaObject);
}

v8::Local<v8::Value> TypeConverter::javaObjectToJsValue(v8::Isolate* isolate, JNIEnv *env, jobject javaObject)
{
	if (!javaObject) {
		return v8::Null(isolate);
	}

	if (env->IsInstanceOf(javaObject, JNIUtil::booleanClass)) {
		jboolean javaBoolean = env->CallBooleanMethod(javaObject, JNIUtil::booleanBooleanValueMethod);
		return javaBoolean ? v8::True(isolate) : v8::False(isolate);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::numberClass)) {
		jdouble javaDouble = env->CallDoubleMethod(javaObject, JNIUtil::numberDoubleValueMethod);
		return v8::Number::New(isolate, (double) javaDouble);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::stringClass)) {
		return TypeConverter::javaStringToJsString(isolate, env, (jstring) javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::dateClass)) {
		return TypeConverter::javaDateToJsDate(isolate, env, javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::hashMapClass)) {
		return TypeConverter::javaHashMapToJsValue(isolate, env, javaObject);
	} else if (env->IsInstanceOf(javaObject, JNIUtil::krollProxyClass)) {
		jobject krollObject = env->GetObjectField(javaObject, JNIUtil::krollProxyKrollObjectField);
		if (krollObject) {
			jlong v8ObjectPointer = env->GetLongField(krollObject, JNIUtil::v8ObjectPtrField);
			env->DeleteLocalRef(krollObject);

			if (v8ObjectPointer != 0) {
				Persistent<Object> v8Object = Persistent<Object>((Object *) v8ObjectPointer);
				JavaObject *jo = NativeObject::Unwrap<JavaObject>(v8Object);
				jo->getJavaObject();
				return v8Object;
			}
		}

		jclass javaObjectClass = env->GetObjectClass(javaObject);
		v8::Local<v8::Object> proxyHandle = ProxyFactory::createV8Proxy(isolate, javaObjectClass, javaObject);
		env->DeleteLocalRef(javaObjectClass);
		return proxyHandle;

	} else if (env->IsInstanceOf(javaObject, JNIUtil::v8FunctionClass)) {
		return javaObjectToJsFunction(isolate, javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::objectArrayClass)) {
		return javaArrayToJsArray(isolate, (jobjectArray) javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::shortArrayClass)) {
		return javaArrayToJsArray(isolate, (jshortArray) javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::intArrayClass)) {
		return javaArrayToJsArray(isolate, (jintArray) javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::longArrayClass)) {
		return javaArrayToJsArray(isolate, (jlongArray) javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::floatArrayClass)) {
		return javaArrayToJsArray(isolate, (jfloatArray) javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::doubleArrayClass)) {
		return javaArrayToJsArray(isolate, (jdoubleArray) javaObject);

	} else if (env->IsInstanceOf(javaObject, JNIUtil::booleanArrayClass)) {
		return javaArrayToJsArray(isolate, (jbooleanArray) javaObject);

	} else if (env->IsSameObject(JNIUtil::undefinedObject, javaObject)) {
		return v8::Undefined(isolate);
	}

	JNIUtil::logClassName("!!! Unable to convert unknown Java object class '%s' to Js value !!!",
	                      env->GetObjectClass(javaObject),
	                      true);
	return v8::Undefined(isolate);
}

jobjectArray TypeConverter::jsObjectIndexPropsToJavaArray(v8::Local<v8::Object> jsObject, int start, int length)
{
	JNIEnv *env = JNIScope::getEnv();
	if (!env) {
		return NULL;
	}
	return TypeConverter::jsObjectIndexPropsToJavaArray(env, jsObject, start, length);
}

jobjectArray TypeConverter::jsObjectIndexPropsToJavaArray(JNIEnv *env, v8::Local<v8::Object> jsObject, int start, int length)
{
	HandleScope scope;

	int arrayLength = length == 0 ? 0 : length - start;
	jobjectArray javaArray = env->NewObjectArray(arrayLength, JNIUtil::objectClass, NULL);
	int index = 0;

	for (int index = start; index < length; ++index) {
		v8::Local<Value> prop = jsObject->Get(index);
		bool isNew;

		jobject javaObject = jsValueToJavaObject(prop, &isNew);
		env->SetObjectArrayElement(javaArray, index - start, javaObject);

		if (isNew) {
			env->DeleteLocalRef(javaObject);
		}
	}

	return javaArray;
}

/****************************** private methods ******************************/

// used mainly by the array conversion methods when converting java numeric types 
// arrays to to the generic js number type 
v8::Local<v8::Array> TypeConverter::javaDoubleArrayToJsNumberArray(isolate, jdoubleArray javaDoubleArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Array::New(isolate);
	}
	return TypeConverter::javaDoubleArrayToJsNumberArray(isolate, env, javaDoubleArray);
}

v8::Local<v8::Array> TypeConverter::javaDoubleArrayToJsNumberArray(v8::Isolate* isolate, JNIEnv *env, jdoubleArray javaDoubleArray)
{
	int arrayLength = env->GetArrayLength(javaDoubleArray);
	v8::Local<v8::Array> jsArray = v8::Array::New(isolate, arrayLength);

	jdouble *arrayElements = env->GetDoubleArrayElements(javaDoubleArray, 0);
	for (int i = 0; i < arrayLength; i++) {
		jsArray->Set((uint32_t) i, v8::Number::New(isolate, arrayElements[i]));
	}
	env->ReleaseDoubleArrayElements(javaDoubleArray, arrayElements, JNI_ABORT);
	//Since we were only reading, there is no need to copy back. Thus, Abort.
	return jsArray;
}

v8::Local<v8::Array> TypeConverter::javaLongArrayToJsNumberArray(v8::Isolate* isolate, jlongArray javaLongArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Array::New(isolate);
	}
	return TypeConverter::javaLongArrayToJsNumberArray(isolate, env, javaLongArray);
}

v8::Local<v8::Array> TypeConverter::javaLongArrayToJsNumberArray(v8::Isolate* isolate, JNIEnv *env, jlongArray javaLongArray)
{    
	int arrayLength = env->GetArrayLength(javaLongArray);
	v8::Local<v8::Array> jsArray = v8::Array::New(isolate, arrayLength);
    
	jlong *arrayElements = env->GetLongArrayElements(javaLongArray, 0);
	for (int i = 0; i < arrayLength; i++) {
		jsArray->Set((uint32_t) i, v8::Number::New(isolate, arrayElements[i]));
	}
	return jsArray;
}

v8::Local<v8::Array> TypeConverter::javaFloatArrayToJsNumberArray(v8::Isolate* isolate, jfloatArray javaFloatArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Array::New(isolate);
	}
	return TypeConverter::javaFloatArrayToJsNumberArray(isolate, env, javaFloatArray);
}

v8::Local<v8::Array> TypeConverter::javaFloatArrayToJsNumberArray(v8::Isolate* isolate, JNIEnv *env, jfloatArray javaFloatArray)
{
	int arrayLength = env->GetArrayLength(javaFloatArray);
	v8::Local<v8::Array> jsArray = v8::Array::New(isolate, arrayLength);
    
	jfloat *arrayElements = env->GetFloatArrayElements(javaFloatArray, 0);
	for (int i = 0; i < arrayLength; i++) {
		jsArray->Set((uint32_t) i, v8::Number::New(isolate, arrayElements[i]));
	}
	env->ReleaseFloatArrayElements(javaFloatArray, arrayElements, JNI_ABORT);
	return jsArray;
}

v8::Local<v8::Array> TypeConverter::javaShortArrayToJsNumberArray(v8::Isolate* isolate, jshortArray javaShortArray)
{
	JNIEnv *env = JNIScope::getEnv();
	if (env == NULL) {
		return v8::Array::New(isolate);
	}
	return TypeConverter::javaShortArrayToJsNumberArray(isolate, env, javaShortArray);
}

v8::Local<v8::Array> TypeConverter::javaShortArrayToJsNumberArray(v8::Isolate* isolate, JNIEnv *env, jshortArray javaShortArray)
{
	int arrayLength = env->GetArrayLength(javaShortArray);
	v8::Local<v8::Array> jsArray = v8::Array::New(isolate, arrayLength);

	jshort *arrayElements = env->GetShortArrayElements(javaShortArray, 0);
	for (int i = 0; i < arrayLength; i++) {
		jsArray->Set((uint32_t) i, v8::Number::New(isolate, arrayElements[i]));
	}
	env->ReleaseShortArrayElements(javaShortArray, arrayElements, JNI_ABORT);
	return jsArray;
}

