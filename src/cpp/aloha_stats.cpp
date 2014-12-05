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

#include "aloha_stats.hpp"
#include "platform_http.hpp"

// TODO
#include <thread>

namespace aloha {

class Stats::Impl {
  // TODO
};

Stats::Stats() : impl_(new Impl()) {
}

bool Stats::LogEvent(std::string const& event_name) const {
  // TODO
  std::thread(&HttpPostToDefaultUrl, std::string(event_name)).detach();
  return true;
}

bool Stats::LogEvent(std::string const& event_name, std::string const& event_value) const {
  // TODO
  std::thread(&HttpPostToDefaultUrl, event_name + "=" + event_value).detach();
  return true;
}

Stats& Stats::Instance() {
  // C++11 guarantees that it should work correctly in multi-threading environment:
  // §6.7 [stmt.dcl] p4
  // If control enters the declaration concurrently while the variable is being initialized,
  // the concurrent execution shall wait for completion of the initialization.
  static Stats instance;
  return instance;
}

}  // namespace aloha
