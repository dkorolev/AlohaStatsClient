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

#ifdef LINK_JAVA_ON_LOAD_INTO_SOURCE
// For the sake of this sample Makefile, if the above define is set to tell Bricks
// it's a single-source-file binary, then make it a single-source-file binary.
#include "../../src/bricks/net/api/test.cc"
#endif

#include <iostream>

#include <jni.h>

#include "../../src/cpp/aloha_stats.cc"

extern int main(int, char**);

extern "C" {
// The purpose of this wrapper function is simple: launch gflags unit tests from another module.
JNIEXPORT void JNICALL Java_org_alohastats_lib_JNITest_runTests(JNIEnv*, jclass) {
  const char* cmd_line[] = {"http_client_test_jni", "--expected_arch", "Java"};
  (void)main(3, const_cast<char**>(cmd_line));
}

}  // extern "C"
