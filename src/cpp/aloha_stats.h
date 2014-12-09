// TODO (dkorolev) add header guards when header's name and location become stable.
#pragma once

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

#include <string>
#include <thread>
#include <iostream>

#include "http_client.h"

namespace aloha {

class Stats {
  std::string statistics_server_url_;

 public:
  Stats(std::string const& statistics_server_url) : statistics_server_url_(statistics_server_url) {
  }

  bool LogEvent(std::string const& event_name) const {
    std::thread(&SimpleSampleHttpPost, statistics_server_url_, event_name).detach();
  }

  bool LogEvent(std::string const& event_name, std::string const& event_value) const {
    std::thread(&SimpleSampleHttpPost, statistics_server_url_, event_name + "=" + event_value).detach();
  }

 private:
  // TODO temporary stub function
  static void SimpleSampleHttpPost(const std::string& url, const std::string& post_data) {
    if (!HttpClient(url).set_post_body(post_data, "text/plain").Connect())
      std::cerr << "Error while sending data to the server " << url << std::endl;
  }
};

std::string HelloWorld();

}  // namespace aloha
