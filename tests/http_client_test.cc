#include <string>
#include <iostream>
#include <fstream>

#include "../src/cpp/http_client.hpp"
#include "../src/bricks/make_scope_guard.hpp"
#include "../src/bricks/read_file_as_string.hpp"

#include "../3party/gtest/gtest.h"
#include "../3party/gtest/gtest-main.h"

using std::string;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using aloha::HttpClient;
using bricks::MakeScopeGuard;
using bricks::ReadFileAsString;

static void WriteStringToFile(string file_name, string string_to_write) {
  ofstream file(file_name);
  file << string_to_write;
  ASSERT_TRUE(file.good());
}

TEST(HttpClient, GetIntoMemory) {
  const string url = "http://httpbin.org/drip?numbytes=7";
  HttpClient client(url);
  ASSERT_TRUE(client.Connect());
  EXPECT_EQ(200, client.error_code());
  EXPECT_EQ("*******", client.server_response());
  EXPECT_EQ(url, client.url_received());
  EXPECT_FALSE(client.was_redirected());
}

TEST(HttpClient, GetIntoFile) {
  const char* file_name = "some_test_file_for_http_get";
  const auto file_deleter = MakeScopeGuard([&] { ::remove(file_name); });
  HttpClient client("http://httpbin.org/drip?numbytes=5");
  client.set_received_file(file_name);
  ASSERT_TRUE(client.Connect());
  EXPECT_TRUE(client.server_response().empty());
  EXPECT_EQ("*****", ReadFileAsString(file_name));
}

TEST(HttpClient, PostFromMemoryIntoMemory) {
  string const post_body("\0\1\2\3\4\5\6\7\0", 9);  // Some binary data
  ASSERT_EQ(9, post_body.size());
  const string url = "http://httpbin.org/post";
  HttpClient client(url);
  client.set_post_body(post_body, "application/octet-stream");
  ASSERT_TRUE(client.Connect());
  EXPECT_NE(string::npos,
            client.server_response().find(
                "\"data\": \"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\u0000\""))
      << client.server_response();
}

TEST(HttpClient, PostFromInvalidFile) {
  const string url = "http://httpbin.org/post";
  HttpClient client(url);
  client.set_post_file("this_file_should_not_exist", "text/plain");
  ASSERT_FALSE(client.Connect());
  EXPECT_NE(200, client.error_code());
}

TEST(HttpClient, PostFromFileIntoMemory) {
  // Use it as a file name and as a test contents string.
  const string file_name = "some_input_test_file_for_http_post";
  const auto file_deleter = MakeScopeGuard([&] { ::remove(file_name.c_str()); });
  WriteStringToFile(file_name, file_name);
  const string url = "http://httpbin.org/post";
  HttpClient client(url);
  client.set_post_file(file_name, "text/plain");
  ASSERT_TRUE(client.Connect());
  EXPECT_NE(string::npos, client.server_response().find(file_name));
}

TEST(HttpClient, PostFromMemoryIntoFile) {
  // Use it as a file name and as a test contents string.
  const string file_name = "some_output_test_file_for_http_post";
  const auto file_deleter = MakeScopeGuard([&] { ::remove(file_name.c_str()); });
  HttpClient client("http://httpbin.org/post");
  client.set_received_file(file_name).set_post_body(file_name, "text/plain");
  ASSERT_TRUE(client.Connect());
  EXPECT_TRUE(client.server_response().empty());
  EXPECT_NE(string::npos, ReadFileAsString(file_name).find(file_name));
}

TEST(HttpClient, PostFromFileIntoFile) {
  const string input_file_name = "some_complex_input_test_file_for_http_post";
  const string output_file_name = "some_complex_output_test_file_for_http_post";
  const auto file_deleter = MakeScopeGuard([&] {
    ::remove(input_file_name.c_str());
    ::remove(output_file_name.c_str());
  });
  const string post_body = "Aloha, this text should pass from one file to another. Mahalo!";
  WriteStringToFile(input_file_name, post_body);
  HttpClient client("http://httpbin.org/post");
  client.set_post_file(input_file_name, "text/plain").set_received_file(output_file_name);
  ASSERT_TRUE(client.Connect());
  EXPECT_TRUE(client.server_response().empty());
  {
    const string received_data = ReadFileAsString(output_file_name);
    EXPECT_NE(string::npos, received_data.find(post_body)) << received_data << endl << post_body;
  }
}

TEST(HttpClient, ErrorCodes) {
  HttpClient client("http://httpbin.org/status/403");
  ASSERT_FALSE(client.Connect());
  EXPECT_EQ(403, client.error_code());
}

TEST(HttpClient, Https) {
  HttpClient client("https://httpbin.org/get?Aloha=Mahalo");
  ASSERT_TRUE(client.Connect());
  EXPECT_EQ(200, client.error_code());
  EXPECT_FALSE(client.was_redirected());
  EXPECT_NE(string::npos, client.server_response().find("\"Aloha\": \"Mahalo\""));
}

TEST(HttpClient, HttpRedirect301) {
  HttpClient client("http://github.com");
  ASSERT_TRUE(client.Connect());
  EXPECT_NE(client.url_requested(), client.url_received());
  EXPECT_EQ("https://github.com/", client.url_received());
  EXPECT_TRUE(client.was_redirected());
}

TEST(HttpClient, HttpRedirect302) {
  HttpClient client("http://httpbin.org/redirect/2");
  ASSERT_TRUE(client.Connect());
  EXPECT_EQ("http://httpbin.org/get", client.url_received());
  EXPECT_TRUE(client.was_redirected());
}

TEST(HttpClient, HttpRedirect307) {
  HttpClient client("http://msn.com");
  ASSERT_TRUE(client.Connect());
  EXPECT_EQ("http://www.msn.com/", client.url_received());
  EXPECT_TRUE(client.was_redirected());
}

TEST(HttpClient, InvalidUrl) {
  HttpClient client("http://very.bad.url");
  EXPECT_FALSE(client.Connect());
  EXPECT_NE(200, client.error_code());
}

TEST(HttpClient, UserAgent) {
  HttpClient client("http://httpbin.org/user-agent");
  const string custom_user_agent = "Aloha User Agent";
  client.set_user_agent(custom_user_agent);
  ASSERT_TRUE(client.Connect());
  EXPECT_NE(string::npos, client.server_response().find(custom_user_agent));
}
