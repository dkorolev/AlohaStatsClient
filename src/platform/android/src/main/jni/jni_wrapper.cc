/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include <jni.h>
#include <string>
#include <memory>

#include "../../../../../cpp/aloha_stats.h"
#include "../../../../../bricks/make_scope_guard.h"

using bricks::MakePointerScopeGuard;

#ifndef ANDROID
#define PLATFORM_SPECIFIC_CAST (void**)
#else
#define PLATFORM_SPECIFIC_CAST
#endif

namespace {

static std::unique_ptr<aloha::Stats> g_stats;

static JavaVM* g_jvm = 0;
// Cached classs and methods for faster access from native code
static jclass g_httpTransportClass = 0;
static jmethodID g_httpTransportClass_run = 0;
static jclass g_httpParamsClass = 0;
static jmethodID g_httpParamsConstructor = 0;

// JNI helper functions
std::string ToStdString(JNIEnv* env, jstring str) {
  std::string result;
  char const* utfBuffer = env->GetStringUTFChars(str, 0);
  if (utfBuffer) {
    result = utfBuffer;
    env->ReleaseStringUTFChars(str, utfBuffer);
  }
  return result;
}

}  // namespace

// Exported for access from C++ code
extern JavaVM* GetJVM() {
  return g_jvm;
}

extern "C" {
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  g_jvm = vm;
  return JNI_VERSION_1_6;
}

JNIEXPORT bool JNICALL
    Java_org_alohastats_lib_Statistics_logEvent__Ljava_lang_String_2(JNIEnv* env, jclass, jstring eventName) {
  return g_stats->LogEvent(ToStdString(env, eventName));
}

JNIEXPORT bool JNICALL
    Java_org_alohastats_lib_Statistics_logEvent__Ljava_lang_String_2Ljava_lang_String_2(JNIEnv* env,
                                                                                        jclass,
                                                                                        jstring eventName,
                                                                                        jstring eventValue) {
  return g_stats->LogEvent(ToStdString(env, eventName), ToStdString(env, eventValue));
}

#define CLEAR_AND_RETURN_FALSE_ON_EXCEPTION \
  if (env->ExceptionCheck()) {              \
    env->ExceptionDescribe();               \
    env->ExceptionClear();                  \
    return false;                           \
  }

#define RETURN_ON_EXCEPTION    \
  if (env->ExceptionCheck()) { \
    env->ExceptionDescribe();  \
    return;                    \
  }

JNIEXPORT void JNICALL Java_org_alohastats_lib_Statistics_setupCPP(JNIEnv* env,
                                                                   jclass,
                                                                   jclass httpTransportClass,
                                                                   jstring serverUrl,
                                                                   jstring storagePath) {
  // Initialize statistics engine
  g_stats.reset(new aloha::Stats(ToStdString(env, serverUrl), ToStdString(env, storagePath)));

  g_httpTransportClass = static_cast<jclass>(env->NewGlobalRef(httpTransportClass));
  RETURN_ON_EXCEPTION
  g_httpTransportClass_run = env->GetStaticMethodID(
      g_httpTransportClass,
      "run",
      "(Lorg/alohastats/lib/HttpTransport$Params;)Lorg/alohastats/lib/HttpTransport$Params;");
  RETURN_ON_EXCEPTION
  g_httpParamsClass = env->FindClass("org/alohastats/lib/HttpTransport$Params");
  RETURN_ON_EXCEPTION
  g_httpParamsClass = static_cast<jclass>(env->NewGlobalRef(g_httpParamsClass));
  RETURN_ON_EXCEPTION
  g_httpParamsConstructor = env->GetMethodID(g_httpParamsClass, "<init>", "(Ljava/lang/String;)V");
  RETURN_ON_EXCEPTION
}

}  // extern "C"

//***********************************************************************
// Exported functions implementation
//***********************************************************************
namespace aloha {

bool HTTPClientPlatformWrapper::RunHTTPRequest() {
  // Attaching multiple times from the same thread is a no-op, which only gets good env for us.
  JNIEnv* env;
  if (JNI_OK != ::GetJVM()->AttachCurrentThread(PLATFORM_SPECIFIC_CAST & env, nullptr)) {
    // TODO(AlexZ): throw some critical exception.
    return false;
  }

  // TODO(AlexZ): May need to refactor if this method will be agressively used from the same thread.
  const auto detachThreadOnScopeExit = bricks::MakeScopeGuard([] { ::GetJVM()->DetachCurrentThread(); });

  // Convenience lambda.
  const auto deleteLocalRef = [&env](jobject o) { env->DeleteLocalRef(o); };

  // Create and fill request params.
  const auto jniUrl = MakePointerScopeGuard(env->NewStringUTF(url_requested_.c_str()), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  const auto httpParamsObject = MakePointerScopeGuard(
      env->NewObject(g_httpParamsClass, g_httpParamsConstructor, jniUrl.get()), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  // Cache it on the first call.
  const static jfieldID dataField = env->GetFieldID(g_httpParamsClass, "data", "[B");
  if (!post_body_.empty()) {
    const auto jniPostData = MakePointerScopeGuard(env->NewByteArray(post_body_.size()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetByteArrayRegion(
        jniPostData.get(), 0, post_body_.size(), reinterpret_cast<const jbyte*>(post_body_.data()));
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), dataField, jniPostData.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  const static jfieldID contentTypeField =
      env->GetFieldID(g_httpParamsClass, "contentType", "Ljava/lang/String;");
  if (!content_type_.empty()) {
    const auto jniContentType = MakePointerScopeGuard(env->NewStringUTF(content_type_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), contentTypeField, jniContentType.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!user_agent_.empty()) {
    const static jfieldID userAgentField =
        env->GetFieldID(g_httpParamsClass, "userAgent", "Ljava/lang/String;");

    const auto jniUserAgent = MakePointerScopeGuard(env->NewStringUTF(user_agent_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), userAgentField, jniUserAgent.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!post_file_.empty()) {
    const static jfieldID inputFilePathField =
        env->GetFieldID(g_httpParamsClass, "inputFilePath", "Ljava/lang/String;");

    const auto jniInputFilePath = MakePointerScopeGuard(env->NewStringUTF(post_file_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), inputFilePathField, jniInputFilePath.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!received_file_.empty()) {
    const static jfieldID outputFilePathField =
        env->GetFieldID(g_httpParamsClass, "outputFilePath", "Ljava/lang/String;");

    const auto jniOutputFilePath =
        MakePointerScopeGuard(env->NewStringUTF(received_file_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), outputFilePathField, jniOutputFilePath.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  // DO ALL MAGIC!
  // Current Java implementation simply reuses input params instance, so we don't need to
  // DeleteLocalRef(response).
  const jobject response =
      env->CallStaticObjectMethod(g_httpTransportClass, g_httpTransportClass_run, httpParamsObject.get());
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    // TODO(AlexZ): think about rethrowing corresponding C++ exceptions.
    env->ExceptionClear();
    return false;
  }

  const static jfieldID httpResponseCodeField = env->GetFieldID(g_httpParamsClass, "httpResponseCode", "I");
  error_code_ = env->GetIntField(response, httpResponseCodeField);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  const static jfieldID receivedUrlField =
      env->GetFieldID(g_httpParamsClass, "receivedUrl", "Ljava/lang/String;");
  const auto jniReceivedUrl = MakePointerScopeGuard(
      static_cast<jstring>(env->GetObjectField(response, receivedUrlField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniReceivedUrl) {
    url_received_ = std::move(ToStdString(env, jniReceivedUrl.get()));
  }

  // contentTypeField is already cached above.
  const auto jniContentType = MakePointerScopeGuard(
      static_cast<jstring>(env->GetObjectField(response, contentTypeField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniContentType) {
    content_type_ = std::move(ToStdString(env, jniContentType.get()));
  }

  // dataField is already cached above.
  const auto jniData =
      MakePointerScopeGuard(static_cast<jbyteArray>(env->GetObjectField(response, dataField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniData) {
    jbyte* buffer = env->GetByteArrayElements(jniData.get(), nullptr);
    if (buffer) {
      server_response_.assign(reinterpret_cast<const char*>(buffer), env->GetArrayLength(jniData.get()));
      env->ReleaseByteArrayElements(jniData.get(), buffer, JNI_ABORT);
    }
  }
  return true;
}

}  // namespace aloha
