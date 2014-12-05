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
#include <android/log.h>

#include "aloha_stats.hpp"

#define LOG_TAG "AlohaStats_jni_wrapper"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// jni helper functions
namespace {

static JavaVM* g_jvm = 0;
// Cached class and it's method for faster access from native code
static jclass g_httpTransportClass = 0;
static jmethodID g_httpTransportClass_postToDefaultUrl = 0;

std::string ToNativeString(JNIEnv* env, jstring str) {
  std::string result;
  char const* utfBuffer = env->GetStringUTFChars(str, 0);
  if (utfBuffer) {
    result = utfBuffer;
    env->ReleaseStringUTFChars(str, utfBuffer);
  } else {
    LOGE("Call to GetStringUTFChars has failed");
  }
  return result;
}

}  // namespace

extern "C" {
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  g_jvm = vm;
  return JNI_VERSION_1_6;
}

JNIEXPORT bool JNICALL
    Java_org_alohastats_lib_Statistics_logEvent__Ljava_lang_String_2(JNIEnv* env, jclass, jstring eventName) {
  return aloha::Stats::Instance().LogEvent(ToNativeString(env, eventName));
}

JNIEXPORT bool JNICALL
    Java_org_alohastats_lib_Statistics_logEvent__Ljava_lang_String_2Ljava_lang_String_2(JNIEnv* env,
                                                                                        jclass,
                                                                                        jstring eventName,
                                                                                        jstring eventValue) {
  return aloha::Stats::Instance().LogEvent(ToNativeString(env, eventName), ToNativeString(env, eventValue));
}

JNIEXPORT void JNICALL
    Java_org_alohastats_lib_Statistics_cacheHttpTransport(JNIEnv* env, jclass, jclass httpTransportClass) {
  if (g_httpTransportClass) env->DeleteGlobalRef(g_httpTransportClass);

  g_httpTransportClass = static_cast<jclass>(env->NewGlobalRef(httpTransportClass));
  g_httpTransportClass_postToDefaultUrl =
      env->GetStaticMethodID(g_httpTransportClass, "postToDefaultUrl", "([B)Z");
}

}  // extern "C"

//***********************************************************************
// Exported functions implementation
//***********************************************************************
namespace aloha {

struct DetachThreadOnScopeExit {
  ~DetachThreadOnScopeExit() {
    g_jvm->DetachCurrentThread();
  }
};

bool HttpPostToDefaultUrl(const std::string& body_data) {
  JNIEnv* env;
  if (JNI_OK != g_jvm->AttachCurrentThread(&env, nullptr)) {
    LOGE("Can't AttachCurrentThread in HttpPostToDefaultUrl()");
    return false;
  }

  DetachThreadOnScopeExit thread_detacher;

  // Swiss-knife style: solving two tasks on scope exit
  auto array_deleter = [&](jbyteArray arr) { env->DeleteLocalRef(arr); };
  std::unique_ptr<_jbyteArray, decltype(array_deleter)> jni_body_data(env->NewByteArray(body_data.size()),
                                                                      array_deleter);
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    return false;
  }

  env->SetByteArrayRegion(
      jni_body_data.get(), 0, body_data.size(), reinterpret_cast<const jbyte*>(body_data.data()));
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    return false;
  }

  const jboolean success = env->CallStaticBooleanMethod(
      g_httpTransportClass, g_httpTransportClass_postToDefaultUrl, jni_body_data.get());
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    return false;
  }
  return success;
}

}  // namespace aloha