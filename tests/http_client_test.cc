#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <functional>

#include "../src/cpp/http_client.h"

#include "../src/bricks/make_scope_guard.h"
#include "../src/bricks/read_file_as_string.h"

#include "../src/bricks/net/posix_http_server.h"  // TODO(dkorolev): Structure `bricks`, I know how!

#include "../src/bricks/dflags/dflags.h"

#include "../3party/gtest/gtest.h"
#include "../3party/gtest/gtest-main.h"

using std::string;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::to_string;
using std::thread;
using std::function;

using aloha::HttpClient;

using bricks::MakeScopeGuard;

using bricks::ReadFileAsString;

using bricks::net::Socket;
using bricks::net::HTTPConnection;
using bricks::net::HTTPHeadersType;
using bricks::net::HTTPResponseCode;

DEFINE_int32(port, 8080, "Local port to use for the test HTTP server.");

// TODO(dkorolev): 1) add path to local files == ".tmp/" to flags, 2) mkdir .tmp / rm -rf .tmp in the Makefile.

class UseHTTPBinTestServer {
 public:
  struct DummyTypeWithNonTrivialDestructor {
    ~DummyTypeWithNonTrivialDestructor() {
      // To avoid the `unused variable: server_scope` warning down in the tests.
    }
  };
  static string BaseURL() {
    return "http://httpbin.org";
  }
  static DummyTypeWithNonTrivialDestructor SpawnServer() {
    return DummyTypeWithNonTrivialDestructor{};
  }

  // TODO(dkorolev): Get rid of this.
  static bool SupportsExternalURLs() {
    return true;
  }
};

class UseLocalHTTPTestServer {
 public:
  static string BaseURL() {
    return string("http://localhost:") + to_string(FLAGS_port);
  }

  class ThreadForSingleServerRequest {
   public:
    ThreadForSingleServerRequest(function<void(Socket)> server_impl)
        : server_thread_(server_impl, std::move(Socket(FLAGS_port))) {
    }
    ThreadForSingleServerRequest(ThreadForSingleServerRequest&& rhs)
        : server_thread_(std::move(rhs.server_thread_)) {
    }
    ~ThreadForSingleServerRequest() {
      server_thread_.join();
    }

   private:
    thread server_thread_;
  };

  static ThreadForSingleServerRequest SpawnServer() {
    return ThreadForSingleServerRequest(UseLocalHTTPTestServer::TestServerHandler);
  }

  // TODO(dkorolev): Get rid of this.
  static bool SupportsExternalURLs() {
    return false;
  }

 private:
  // TODO(dkorolev): This code should use our real bricks::HTTPServer, once it's coded.
  static void TestServerHandler(Socket socket) {
    bool serve_more_requests = true;
    while (serve_more_requests) {
      serve_more_requests = false;
      HTTPConnection connection(socket.Accept());
      ASSERT_TRUE(connection);
      const string method = connection.Method();
      const string url = connection.URL();
      if (method == "GET") {
        if (url == "/get") {
          connection.SendHTTPResponse("DIMA");
        } else if (url == "/drip?numbytes=7") {
          connection.SendHTTPResponse("*******");
        } else if (url == "/drip?numbytes=5") {
          connection.SendHTTPResponse("*****");
        } else if (url == "/status/403") {
          connection.SendHTTPResponse("", HTTPResponseCode::Forbidden);
        } else if (url == "/get?Aloha=Mahalo") {
          connection.SendHTTPResponse("{\"Aloha\": \"Mahalo\"}");
        } else if (url == "/user-agent") {
          // TODO(dkorolev): Add parsing User-Agent to Bricks' HTTP headers parser.
          connection.SendHTTPResponse("Aloha User Agent");
        } else if (url == "/redirect-to?url=/get") {
          HTTPHeadersType headers;
          headers.push_back(std::make_pair("Location", "/get"));
          connection.SendHTTPResponse("", HTTPResponseCode::Found, "text/html", headers);
          serve_more_requests = true;
        } else {
          ASSERT_TRUE(false) << "GET not implemented for: " << connection.URL();
        }
      } else if (method == "POST") {
        if (url == "/post") {
          ASSERT_TRUE(connection.HasBody());
          connection.SendHTTPResponse("\"data\": \"" + connection.Body() + "\"");
        } else {
          ASSERT_TRUE(false) << "POST not implemented for: " << connection.URL();
        }
      } else {
        ASSERT_TRUE(false) << "Method not implemented: " << connection.Method();
      }
    }
  }
};

static void WriteStringToFile(string file_name, string string_to_write) {
  ofstream file(file_name);
  file << string_to_write;
  ASSERT_TRUE(file.good());
}

template <typename T>
class HTTPClient : public ::testing::Test {};

typedef ::testing::Types<UseLocalHTTPTestServer, UseHTTPBinTestServer> HTTPClientTestTypeList;
TYPED_TEST_CASE(HTTPClient, HTTPClientTestTypeList);

TYPED_TEST(HTTPClient, GetIntoMemory) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/drip?numbytes=7";
  HttpClient client(url);
  ASSERT_TRUE(client.Connect());
  EXPECT_EQ(200, client.error_code());
  EXPECT_EQ("*******", client.server_response());
  EXPECT_EQ(url, client.url_received());
  EXPECT_FALSE(client.was_redirected());
}

TYPED_TEST(HTTPClient, GetIntoFile) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/drip?numbytes=5";

  const char* file_name = "some_test_file_for_http_get";
  ::remove(file_name);
  const auto file_deleter = MakeScopeGuard([&] { ::remove(file_name); });
  HttpClient client(url);
  client.set_received_file(file_name);
  ASSERT_TRUE(client.Connect());
  EXPECT_TRUE(client.server_response().empty());
  EXPECT_EQ("*****", ReadFileAsString(file_name));
}

TYPED_TEST(HTTPClient, PostFromMemoryIntoMemory) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/post";
  string const post_body = "Hello, World!";
  HttpClient client(url);
  client.set_post_body(post_body, "application/octet-stream");
  ASSERT_TRUE(client.Connect());
  EXPECT_NE(string::npos, client.server_response().find("\"data\": \"" + post_body + "\""))
      << client.server_response();
}

TYPED_TEST(HTTPClient, PostFromInvalidFile) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/post";
  HttpClient client(url);
  client.set_post_file("this_file_should_not_exist", "text/plain");
  ASSERT_FALSE(client.Connect());
  EXPECT_NE(200, client.error_code());

  // Still do one request since local HTTP server is waiting for it.
  ASSERT_TRUE(HttpClient(TypeParam::BaseURL() + "/get").Connect());
}

TYPED_TEST(HTTPClient, PostFromFileIntoMemory) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/post";
  // Use it as a file name and as a test contents string.
  const string file_name = "some_input_test_file_for_http_post";
  const auto file_deleter = MakeScopeGuard([&] { ::remove(file_name.c_str()); });
  WriteStringToFile(file_name, file_name);
  HttpClient client(url);
  client.set_post_file(file_name, "text/plain");
  ASSERT_TRUE(client.Connect());
  EXPECT_NE(string::npos, client.server_response().find(file_name));
}

TYPED_TEST(HTTPClient, PostFromMemoryIntoFile) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/post";
  // Use it as a file name and as a test contents string.
  const string file_name = "some_output_test_file_for_http_post";
  const auto file_deleter = MakeScopeGuard([&] { ::remove(file_name.c_str()); });
  HttpClient client(url);
  client.set_received_file(file_name).set_post_body(file_name, "text/plain");
  ASSERT_TRUE(client.Connect());
  EXPECT_TRUE(client.server_response().empty());
  EXPECT_NE(string::npos, ReadFileAsString(file_name).find(file_name));
}

TYPED_TEST(HTTPClient, PostFromFileIntoFile) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/post";
  const string input_file_name = "some_complex_input_test_file_for_http_post";
  const string output_file_name = "some_complex_output_test_file_for_http_post";
  const auto file_deleter = MakeScopeGuard([&] {
    ::remove(input_file_name.c_str());
    ::remove(output_file_name.c_str());
  });
  const string post_body = "Aloha, this text should pass from one file to another. Mahalo!";
  WriteStringToFile(input_file_name, post_body);
  HttpClient client(url);
  client.set_post_file(input_file_name, "text/plain").set_received_file(output_file_name);
  ASSERT_TRUE(client.Connect());
  EXPECT_TRUE(client.server_response().empty());
  {
    const string received_data = ReadFileAsString(output_file_name);
    EXPECT_NE(string::npos, received_data.find(post_body)) << received_data << endl << post_body;
  }
}

TYPED_TEST(HTTPClient, ErrorCodes) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/status/403";
  HttpClient client(url);
  ASSERT_FALSE(client.Connect());
  EXPECT_EQ(403, client.error_code());
}

TYPED_TEST(HTTPClient, Https) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/get?Aloha=Mahalo";
  HttpClient client(url);
  ASSERT_TRUE(client.Connect());
  EXPECT_EQ(200, client.error_code());
  EXPECT_FALSE(client.was_redirected());
  EXPECT_NE(string::npos, client.server_response().find("\"Aloha\": \"Mahalo\""));
}

TYPED_TEST(HTTPClient, HttpRedirect302) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/redirect-to?url=/get";
  HttpClient client(url);
  ASSERT_TRUE(client.Connect());
  EXPECT_EQ(TypeParam::BaseURL() + "/get", client.url_received());
  EXPECT_TRUE(client.was_redirected());
}

TYPED_TEST(HTTPClient, UserAgent) {
  auto server_scope = TypeParam::SpawnServer();
  const string url = TypeParam::BaseURL() + "/user-agent";
  HttpClient client(url);
  const string custom_user_agent = "Aloha User Agent";
  client.set_user_agent(custom_user_agent);
  ASSERT_TRUE(client.Connect());
  EXPECT_NE(string::npos, client.server_response().find(custom_user_agent));
}

// TODO(dkorolev): Get rid of the tests involving external URLs.
TYPED_TEST(HTTPClient, HttpRedirect301) {
  if (TypeParam::SupportsExternalURLs()) {
    auto server_scope = TypeParam::SpawnServer();
    HttpClient client("http://github.com");
    ASSERT_TRUE(client.Connect());
    EXPECT_NE(client.url_requested(), client.url_received());
    EXPECT_EQ("https://github.com/", client.url_received());
    EXPECT_TRUE(client.was_redirected());
  }
}

TYPED_TEST(HTTPClient, HttpRedirect307) {
  if (TypeParam::SupportsExternalURLs()) {
    auto server_scope = TypeParam::SpawnServer();
    HttpClient client("http://msn.com");
    ASSERT_TRUE(client.Connect());
    EXPECT_EQ("http://www.msn.com/", client.url_received());
    EXPECT_TRUE(client.was_redirected());
  }
}

TYPED_TEST(HTTPClient, InvalidUrl) {
  if (TypeParam::SupportsExternalURLs()) {
    auto server_scope = TypeParam::SpawnServer();
    HttpClient client("http://very.bad.url");
    EXPECT_FALSE(client.Connect());
    EXPECT_NE(200, client.error_code());
  }
}
