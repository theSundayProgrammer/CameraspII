 //////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// adapted from Raspicam sample code
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
//returns the value of a command line param. If not found, defvalue is returned
namespace camerasp{
  
  UrlParser::UrlParser(std::string const& s) {
      std::string uri = s;
      //filter out anything after '#'
      auto pos = uri.find('#');
      if (pos != std::string::npos) {
        uri = uri.erase(pos);
      }
      //detect command which may end with '?' if there are parameters
      std::smatch m;
      std::regex e("([^?]*)(\\??)");
      if (std::regex_search(uri, m, e)) {
        command = m[1];
        //Command detected; now get parameter values "key=value&key=value"
        uri = m.suffix().str();
        std::regex e2("([^&]+)(&?)");
        while (!uri.empty() && std::regex_search(uri, m, e2)) {
          //for each matching key=value save key,value in map
          std::regex e3("([^=]*)(=)");
          std::string keyval = m[1];
          std::smatch m3;
          if (std::regex_search(keyval, m3, e3)) {
            std::string key = m3[1];
            std::string val = m3.suffix().str();
            queries[key] = val;
          }
          uri = m.suffix().str();
        }
      }
      else {
        command = uri;
      }
    }
  
  
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
}
