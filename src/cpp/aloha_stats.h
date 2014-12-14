#ifndef ALOHA_STATS_H
#define ALOHA_STATS_H

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

#include "../bricks/net/api/api.h"

namespace aloha {

class Stats {
  std::string statistics_server_url_;
  std::string storage_path_;

 public:
  Stats(std::string const& statistics_server_url, std::string const& storage_path_with_a_slash_at_the_end)
      : statistics_server_url_(statistics_server_url), storage_path_(storage_path_with_a_slash_at_the_end) {
  }

  bool LogEvent(std::string const& event_name) const {
    // TODO(dkorolev): Insert real message queue + cereal here.
    std::thread(&SimpleSampleHttpPost, statistics_server_url_, event_name).detach();
    return true;
  }

  bool LogEvent(std::string const& event_name, std::string const& event_value) const {
    // TODO(dkorolev): Insert real message queue + cereal here.
    std::thread(&SimpleSampleHttpPost, statistics_server_url_, event_name + "=" + event_value).detach();
    return true;
  }

 private:
  static void SimpleSampleHttpPost(const std::string& url, const std::string& post_data) {
    using namespace bricks::net::api;
    HTTP(POST(url, post_data, "text/plain"));
  }
};

}  // namespace aloha

#endif  // #ifndef ALOHA_STATS_H
