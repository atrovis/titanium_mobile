/**
 * Appcelerator Titanium Mobile
 * Copyright (c) 2011-2015 by Appcelerator, Inc. All Rights Reserved.
 * Licensed under the terms of the Apache Public License
 * Please see the LICENSE included with this distribution for details.
 */
#ifndef V8_UTIL_H
#define V8_UTIL_H

#include <stdio.h>
#include <v8.h>

#define ENTER_V8(context) \
	v8::HandleScope scope(context.GetIsolate());

#define IMMUTABLE_STRING_LITERAL(isolate, string_literal) \
	::titanium::ImmutableAsciiStringLiteral::CreateFromLiteral( \
		isolate, string_literal "", sizeof(string_literal) - 1)

#define IMMUTABLE_STRING_LITERAL_FROM_ARRAY(isolate, string_literal, length) \
	::titanium::ImmutableAsciiStringLiteral::CreateFromLiteral( \
	isolate, string_literal, length)

#define SYMBOL_LITERAL(isolate, string_literal) \
	v8::String::NewFromUtf8(isolate, string_literal "", v8::String::kInternalizedString)

#define FIXED_ONE_BYTE_STRING(isolate, string)                                \
  (titanium::OneByteString((isolate), (string), sizeof(string) - 1))

#ifdef TI_DEBUG
# define LOG_HEAP_STATS(TAG) \
{ \
	v8::HeapStatistics stats; \
	v8::V8::GetHeapStatistics(&stats); \
	LOGE(TAG, "Heap stats:"); \
	LOGE(TAG, "   Total heap size:            %dk", stats.total_heap_size() / 1024); \
	LOGE(TAG, "   Total heap size executable: %dk", stats.total_heap_size_executable() / 1024); \
	LOGE(TAG, "   Used heap size:             %dk", stats.used_heap_size() / 1024); \
	LOGE(TAG, "   Heap size limit:            %dk", stats.heap_size_limit() / 1024); \
}
# define LOG_STACK_TRACE(TAG, ...) \
{ \
	v8::Local<v8::StackTrace> stackTrace = v8::StackTrace::CurrentStackTrace(16); \
	uint32_t frameCount = stackTrace->GetFrameCount(); \
	LOGV(TAG, __VA_ARGS__); \
	for (uint32_t i = 0; i < frameCount; i++) { \
		v8::Local<v8::StackFrame> frame = stackTrace->GetFrame(i); \
		v8::String::Utf8Value fnName(frame->GetFunctionName()); \
		v8::String::Utf8Value scriptUrl(frame->GetScriptName()); \
		LOGV(TAG, "    at %s [%s:%d:%d]", *fnName, *scriptUrl, frame->GetLineNumber(), frame->GetColumn()); \
	} \
}
#else
# define LOG_HEAP_STATS(TAG)
# define LOG_STACK_TRACE(TAG)
#endif

namespace titanium {

inline v8::Local<v8::FunctionTemplate>
    NewFunctionTemplate(v8::Isolate* isolate,
    					v8::FunctionCallback callback,
                        v8::Local<v8::Signature> signature = v8::Local<v8::Signature>()) {
  return v8::FunctionTemplate::New(isolate, callback, v8::Local<v8::Value>(), signature);
}

inline void SetMethod(v8::Isolate* isolate,
					  v8::Local<v8::Object> that,
                      const char* name,
                      v8::FunctionCallback callback) {
  v8::Local<v8::Function> function =
      NewFunctionTemplate(isolate, callback)->GetFunction();
  // kInternalized strings are created in the old space.
  const v8::NewStringType type = v8::NewStringType::kInternalized;
  v8::Local<v8::String> name_string =
      v8::String::NewFromUtf8(isolate, name, type).ToLocalChecked();
  that->Set(name_string, function);
  function->SetName(name_string);  // NODE_SET_METHOD() compatibility.
}

inline void SetProtoMethod(v8::Isolate* isolate,
						   v8::Local<v8::FunctionTemplate> that,
                           const char* name,
                           v8::FunctionCallback callback) {
  v8::Local<v8::Signature> signature = v8::Signature::New(isolate, that);
  v8::Local<v8::Function> function =
      NewFunctionTemplate(isolate, callback, signature)->GetFunction();
  // kInternalized strings are created in the old space.
  const v8::NewStringType type = v8::NewStringType::kInternalized;
  v8::Local<v8::String> name_string =
      v8::String::NewFromUtf8(isolate, name, type).ToLocalChecked();
  that->PrototypeTemplate()->Set(name_string, function);
  function->SetName(name_string);  // NODE_SET_PROTOTYPE_METHOD() compatibility.
}

inline void SetTemplateMethod(v8::Isolate* isolate,
							  v8::Local<v8::FunctionTemplate> that,
                              const char* name,
                              v8::FunctionCallback callback) {
  v8::Local<v8::Function> function =
      NewFunctionTemplate(isolate, callback)->GetFunction();
  // kInternalized strings are created in the old space.
  const v8::NewStringType type = v8::NewStringType::kInternalized;
  v8::Local<v8::String> name_string =
      v8::String::NewFromUtf8(isolate, name, type).ToLocalChecked();
  that->Set(name_string, function);
  function->SetName(name_string);  // NODE_SET_METHOD() compatibility.
}

inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate,
                                           const char* data,
                                           int length) {
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(data),
                                    v8::String::kNormalString,
                                    length);
}

inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate,
                                           const signed char* data,
                                           int length) {
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(data),
                                    v8::String::kNormalString,
                                    length);
}

inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate,
                                           const unsigned char* data,
                                           int length) {
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(data),
                                    v8::String::kNormalString,
                                    length);
}

class ImmutableAsciiStringLiteral: public v8::String::ExternalOneByteStringResource
{
public:
	static v8::Local<v8::String> CreateFromLiteral(v8::Isolate* isolate, const char *stringLiteral, size_t length);

	ImmutableAsciiStringLiteral(const char *src, size_t src_len)
			: buffer_(src), buf_len_(src_len)
	{
	}

	virtual ~ImmutableAsciiStringLiteral()
	{
	}

	const char *data() const
	{
		return buffer_;
	}

	size_t length() const
	{
		return buf_len_;
	}

private:
	const char *buffer_;
	size_t buf_len_;
};

class V8Util {
public:
	static v8::Local<v8::Value> executeString(v8::Isolate* isolate, v8::Local<v8::String> source, v8::Local<v8::Value> filename);
	static v8::Local<v8::Value> newInstanceFromConstructorTemplate(v8::Persistent<v8::FunctionTemplate>& t,
		const v8::FunctionCallbackInfo<v8::Value>& args);
	static void objectExtend(v8::Local<v8::Object> dest, v8::Local<v8::Object> src);
	static void reportException(v8::Isolate* isolate, v8::TryCatch &tryCatch, bool showLine = true);
	static void openJSErrorDialog(v8::Isolate* isolate, v8::TryCatch &tryCatch);
	static void fatalException(v8::Isolate* isolate, v8::TryCatch &tryCatch);
	static v8::Local<v8::String> jsonStringify(v8::Isolate* isolate, v8::Local<v8::Value> value);
	static bool constructorNameMatches(v8::Isolate* isolate, v8::Local<v8::Object>, const char* name);
	static bool isNaN(v8::Isolate* isolate, v8::Local<v8::Value> value);
	static void dispose();
};

}

#endif
