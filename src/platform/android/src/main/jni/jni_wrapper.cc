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

#include "../../../../../cpp/aloha_stats.h"
#include "../../../../../bricks/make_scope_guard.h"

#define LOG_TAG "AlohaStats_jni_wrapper"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

static std::unique_ptr<aloha::Stats> g_stats;

static JavaVM* g_jvm = 0;
// Cached class and it's method for faster access from native code
static jclass g_httpTransportClass = 0;
static jmethodID g_httpTransportClass_postToDefaultUrl = 0;

// JNI helper function
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
  return g_stats->LogEvent(ToNativeString(env, eventName));
}

JNIEXPORT bool JNICALL
    Java_org_alohastats_lib_Statistics_logEvent__Ljava_lang_String_2Ljava_lang_String_2(JNIEnv* env,
                                                                                        jclass,
                                                                                        jstring eventName,
                                                                                        jstring eventValue) {
  return g_stats->LogEvent(ToNativeString(env, eventName), ToNativeString(env, eventValue));
}

JNIEXPORT void JNICALL Java_org_alohastats_lib_Statistics_setupHttpTransport(JNIEnv* env,
                                                                             jclass,
                                                                             jstring url,
                                                                             jclass httpTransportClass) {
  // Initialize statistics engine
  g_stats.reset(new aloha::Stats(ToNativeString(env, url)));

  if (g_httpTransportClass) {
    env->DeleteGlobalRef(g_httpTransportClass);
  }

  g_httpTransportClass = static_cast<jclass>(env->NewGlobalRef(httpTransportClass));
  g_httpTransportClass_postToDefaultUrl =
      env->GetStaticMethodID(g_httpTransportClass, "postToDefaultUrl", "([B)Z");
}

}  // extern "C"

// @TODO (AlexZ) Temporarily commented out, will be rewritten with fresh implementation soon
//***********************************************************************
// Exported functions implementation
//***********************************************************************
// namespace aloha {
//
// bool HttpPostToDefaultUrl(const std::string& body_data) {
//  JNIEnv* env;
//  if (JNI_OK != GetJVM()->AttachCurrentThread(&env, nullptr)) {
//    LOGE("Can't AttachCurrentThread in HttpPostToDefaultUrl()");
//    return false;
//  }
//
//  const auto thread_detacher = bricks::MakeScopeGuard([]{ GetJVM()->DetachCurrentThread(); });
//
//  auto const jni_body_data = bricks::MakePointerScopeGuard(env->NewByteArray(body_data.size()), [&](jbyteArray
//  arr) { env->DeleteLocalRef(arr); });
//  if (env->ExceptionOccurred()) {
//    env->ExceptionDescribe();
//    return false;
//  }
//
//  env->SetByteArrayRegion(
//      jni_body_data.get(), 0, body_data.size(), reinterpret_cast<const jbyte*>(body_data.data()));
//  if (env->ExceptionCheck()) {
//    env->ExceptionDescribe();
//    return false;
//  }
//
//  const jboolean success = env->CallStaticBooleanMethod(
//      g_httpTransportClass, g_httpTransportClass_postToDefaultUrl, jni_body_data.get());
//  if (env->ExceptionCheck()) {
//    env->ExceptionDescribe();
//    return false;
//  }
//  return success;
//}
//
//}  // namespace aloha
