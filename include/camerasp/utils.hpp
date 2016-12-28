//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <asio.hpp>
#include <string>
#include <vector>
#include <json/reader.h>
#include <spdlog/spdlog.h>
extern std::shared_ptr<spdlog::logger> console;
#ifdef __GNUC__
typedef int errno_t;
errno_t fopen_s(FILE** fp, const char* name, const char* mode);
#endif
namespace camerasp{
  std::string lower_case(std::string const& str);
  Json::Value get_DOM(std::string const& path);
  /// A string to send in responses.
  struct url_parser {
    std::string command;
    std::map<std::string, std::string> queries;
    url_parser(std::string const& s);
  };
  //////image capture timer
  class cam_still;
  typedef asio::basic_waitable_timer<std::chrono::steady_clock>   high_resolution_timer;
  void setTimer(high_resolution_timer& timer, cam_still&);
  std::string  get_image(unsigned int k);

 bool is_integer(const std::string & s, int* k);

}
