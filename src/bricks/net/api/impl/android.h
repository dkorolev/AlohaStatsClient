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

#ifndef BRICKS_NET_API_IMPL_ANDROID_H
#define BRICKS_NET_API_IMPL_ANDROID_H

#include "../../../port.h"

#if defined(BRICKS_ANDROID)

#include <jni.h>

#include <cassert>
#include <memory>
#include <string>

#include "../../../../cpp/aloha_stats.h"
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

namespace aloha {

class HTTPClientPlatformWrapper {
 public:
  enum {
    kNotInitialized = -1,
  };

 private:
  friend class bricks::net::api::ImplWrapper<HTTPClientPlatformWrapper>;

  std::string url_requested_;
  // Contains final content's url taking redirects (if any) into an account.
  std::string url_received_;
  int error_code_;
  std::string post_file_;
  // Used instead of server_reply_ if set.
  std::string received_file_;
  // Data we received from the server if output_file_ wasn't initialized.
  std::string server_response_;
  std::string content_type_;
  std::string user_agent_;
  std::string post_body_;

  HTTPClientPlatformWrapper(const HTTPClientPlatformWrapper&) = delete;
  HTTPClientPlatformWrapper(HTTPClientPlatformWrapper&&) = delete;
  HTTPClientPlatformWrapper& operator=(const HTTPClientPlatformWrapper&) = delete;

 public:
  HTTPClientPlatformWrapper() : error_code_(kNotInitialized) {
  }
  HTTPClientPlatformWrapper(const std::string& url) : url_requested_(url), error_code_(kNotInitialized) {
  }

  // Synchronous (blocking) call, should be implemented for each platform
  // @returns true only if server answered with HTTP 200 OK
  // @note Implementations should transparently support all needed HTTP redirects
  bool Go() {
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

  std::string const& url_requested() const {
    return url_requested_;
  }
  // @returns empty string in the case of error
  std::string const& url_received() const {
    return url_received_;
  }
  bool was_redirected() const {
    return url_requested_ != url_received_;
  }
  // Mix of HTTP errors (in case of successful connection) and system-dependent error codes,
  // in the simplest success case use 'if (200 == client.error_code())' // 200 means OK in HTTP
  int error_code() const {
    return error_code_;
  }
  std::string const& server_response() const {
    return server_response_;
  }

};  // class HTTPClientPlatformWrapper

}  // namespace aloha

namespace bricks {
namespace net {
namespace api {

template <>
struct ImplWrapper<aloha::HTTPClientPlatformWrapper> {
  // Populating the fields of aloha::HTTPClientPlatformWrapper given request parameters.
  inline static void PrepareInput(const HTTPRequestGET& request, aloha::HTTPClientPlatformWrapper& client) {
    client.url_requested_ = request.url;
    if (!request.custom_user_agent.empty()) {
      client.user_agent_ = request.custom_user_agent;
    }
  }
  inline static void PrepareInput(const HTTPRequestPOST& request, aloha::HTTPClientPlatformWrapper& client) {
    client.url_requested_ = request.url;
    if (!request.custom_user_agent.empty()) {
      client.user_agent_ = request.custom_user_agent;
    }
    client.post_body_ = request.body;
    client.content_type_ = request.content_type;
  }
  inline static void PrepareInput(const HTTPRequestPOSTFromFile& request, aloha::HTTPClientPlatformWrapper& client) {
    client.url_requested_ = request.url;
    if (!request.custom_user_agent.empty()) {
      client.user_agent_ = request.custom_user_agent;
    }
    client.post_file_ = request.file_name;
    client.content_type_ = request.content_type;
  }

  // Populating the fields of aloha::HTTPClientPlatformWrapper given response configuration parameters.
  inline static void PrepareInput(const KeepResponseInMemory&, aloha::HTTPClientPlatformWrapper&) {
  }
  inline static void PrepareInput(const SaveResponseToFile& save_to_file_request, aloha::HTTPClientPlatformWrapper& client) {
    client.received_file_ = save_to_file_request.file_name;
  }

  // Parsing the response from within aloha::HTTPClientPlatformWrapper.
  template <typename T_REQUEST_PARAMS, typename T_RESPONSE_PARAMS>
  inline static void ParseOutput(const T_REQUEST_PARAMS&,
                                 const T_RESPONSE_PARAMS&,
                                 const aloha::HTTPClientPlatformWrapper& response,
                                 HTTPResponse& output) {
    output.url = response.url_requested_;
    output.code = response.error_code_;
    output.url_after_redirects = response.url_received_;
  }

  template <typename T_REQUEST_PARAMS, typename T_RESPONSE_PARAMS>
  inline static void ParseOutput(const T_REQUEST_PARAMS& request_params,
                                 const T_RESPONSE_PARAMS& response_params,
                                 const aloha::HTTPClientPlatformWrapper& response,
                                 HTTPResponseWithBuffer& output) {
    ParseOutput(request_params, response_params, response, static_cast<HTTPResponse&>(output));
    output.body = response.server_response_;
  }
  template <typename T_REQUEST_PARAMS, typename T_RESPONSE_PARAMS>
  inline static void ParseOutput(const T_REQUEST_PARAMS& request_params,
                                 const T_RESPONSE_PARAMS& response_params,
                                 const aloha::HTTPClientPlatformWrapper& response,
                                 HTTPResponseWithResultingFileName& output) {
    ParseOutput(request_params, response_params, response, static_cast<HTTPResponse&>(output));
    output.body_file_name = response_params.file_name;
  }
};

}  // namespace api
}  // namespace net
}  // namespace bricke

#endif  // defined(BRICKS_ANDROID)

#endif  // BRICKS_NET_API_IMPL_ANDROID_H
