//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cstdarg>
#include <regex>
#include <fstream>
#include <sstream>
#include <camerasp/utils.hpp>
namespace camerasp{

  std::string lower_case(std::string const& str)    {
    std::string retval(str);
    std::transform(std::begin(str), std::end(str), std::begin(retval), tolower);
    return retval;
  }


  JSONCPP_STRING read_file_content(const char* path)    {
    std::ifstream ifs(path);
    std::stringstream istr;
    istr << ifs.rdbuf();
    return istr.str();
  }

  Json::Value get_DOM(std::string const& path)    {
    JSONCPP_STRING input = read_file_content(path.c_str());
    if (input.empty())      {
      throw std::runtime_error("Empty input file");
    }

    Json::Features mode = Json::Features::strictMode();
    mode.allowComments_ = true;
    Json::Value root;

    Json::Reader reader(mode);
    bool parsingSuccessful = reader.parse(input.data(), input.data() + input.size(), root);
    if (!parsingSuccessful)      {
      Json::throwLogicError(reader.getFormattedErrorMessages());
    }
    return root;
  }
  bool is_integer(const std::string & s, int* k)
  {
    if (s.empty()) return false;

    char * p;
    *k = strtol(s.c_str(), &p, 10);

    return true;//(*p == 0);
  }
}
