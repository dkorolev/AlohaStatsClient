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
#include "../../../util/make_scope_guard.h"

using bricks::MakePointerScopeGuard;

#ifndef ANDROID
#define PLATFORM_SPECIFIC_CAST (void**)
#else
#define PLATFORM_SPECIFIC_CAST
#endif

namespace {

static std::unique_ptr<aloha::Stats> g_stats;

static JavaVM* g_jvm = 0;
// Cached class and methods for faster access from native code
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
