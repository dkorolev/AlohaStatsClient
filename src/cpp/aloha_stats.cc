#include <memory>

#include "aloha_stats.h"

#include "../bricks/port.h"

#if defined(BRICKS_JAVA) || defined(BRICKS_ANDROID)

#include "../bricks/java_wrapper/java_wrapper.h"

static std::unique_ptr<aloha::Stats> g_stats;

extern "C" {

JNIEXPORT void JNICALL Java_org_alohastats_lib_Statistics_setupCPP(JNIEnv* env,
                                                                   jclass,
                                                                   jclass httpTransportClass,
                                                                   jstring serverUrl,
                                                                   jstring storagePath) {
  using bricks::java_wrapper::ToStdString;
  auto& JAVA = bricks::java_wrapper::JavaWrapper::Singleton();

  // Initialize statistics engine.
  g_stats.reset(new aloha::Stats(ToStdString(env, serverUrl), ToStdString(env, storagePath)));

  JAVA.httpTransportClass = static_cast<jclass>(env->NewGlobalRef(httpTransportClass));
  RETURN_ON_EXCEPTION
  JAVA.httpTransportClass_run = env->GetStaticMethodID(
      JAVA.httpTransportClass,
      "run",
      "(Lorg/alohastats/lib/HttpTransport$Params;)Lorg/alohastats/lib/HttpTransport$Params;");
  RETURN_ON_EXCEPTION
  JAVA.httpParamsClass = env->FindClass("org/alohastats/lib/HttpTransport$Params");
  RETURN_ON_EXCEPTION
  JAVA.httpParamsClass = static_cast<jclass>(env->NewGlobalRef(JAVA.httpParamsClass));
  RETURN_ON_EXCEPTION
  JAVA.httpParamsConstructor = env->GetMethodID(JAVA.httpParamsClass, "<init>", "(Ljava/lang/String;)V");
  RETURN_ON_EXCEPTION
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

}  // extern "C"

#endif  // defined(BRICKS_JAVA) || defined(BRICKS_ANDROID)
