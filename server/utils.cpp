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
#include <camerasp/raspicamtypes.h>
namespace camerasp{


  std::string upper_case(std::string const& str)    {
    std::string retval(str);
    std::transform(std::begin(str), std::end(str), std::begin(retval), toupper);
    return retval;
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
  bool is_integer(const std::string & s, int* k)
  {
    if (s.empty()) return false;

    char * p;
    *k = strtol(s.c_str(), &p, 10);

    return true;//(*p == 0);
  }
  RASPICAM_EXPOSURE get_exposure_from_string (const  std::string& str ) {
    if ( str=="OFF" ) return RASPICAM_EXPOSURE_OFF;
    if ( str=="AUTO" ) return RASPICAM_EXPOSURE_AUTO;
    if ( str=="NIGHT" ) return RASPICAM_EXPOSURE_NIGHT;
    if ( str=="NIGHTPREVIEW" ) return RASPICAM_EXPOSURE_NIGHTPREVIEW;
    if ( str=="BACKLIGHT" ) return RASPICAM_EXPOSURE_BACKLIGHT;
    if ( str=="SPOTLIGHT" ) return RASPICAM_EXPOSURE_SPOTLIGHT;
    if ( str=="SPORTS" ) return RASPICAM_EXPOSURE_SPORTS;
    if ( str=="SNOW" ) return RASPICAM_EXPOSURE_SNOW;
    if ( str=="BEACH" ) return RASPICAM_EXPOSURE_BEACH;
    if ( str=="VERYLONG" ) return RASPICAM_EXPOSURE_VERYLONG;
    if ( str=="FIXEDFPS" ) return RASPICAM_EXPOSURE_FIXEDFPS;
    if ( str=="ANTISHAKE" ) return RASPICAM_EXPOSURE_ANTISHAKE;
    if ( str=="FIREWORKS" ) return RASPICAM_EXPOSURE_FIREWORKS;
    return RASPICAM_EXPOSURE_AUTO;
  }

  RASPICAM_AWB get_awb_from_string ( const std::string& str ) {
    if ( str=="OFF" ) return RASPICAM_AWB_OFF;
    if ( str=="AUTO" ) return RASPICAM_AWB_AUTO;
    if ( str=="SUNLIGHT" ) return RASPICAM_AWB_SUNLIGHT;
    if ( str=="CLOUDY" ) return RASPICAM_AWB_CLOUDY;
    if ( str=="SHADE" ) return RASPICAM_AWB_SHADE;
    if ( str=="TUNGSTEN" ) return RASPICAM_AWB_TUNGSTEN;
    if ( str=="FLUORESCENT" ) return RASPICAM_AWB_FLUORESCENT;
    if ( str=="INCANDESCENT" ) return RASPICAM_AWB_INCANDESCENT;
    if ( str=="FLASH" ) return RASPICAM_AWB_FLASH;
    if ( str=="HORIZON" ) return RASPICAM_AWB_HORIZON;
    return RASPICAM_AWB_AUTO;
  }
  RASPICAM_FORMAT get_format_from_string ( const std::string& str ) {
    if(str=="GREY") return RASPICAM_FORMAT_GRAY;
    if(str=="RGB") return RASPICAM_FORMAT_RGB;
    if(str=="YUV") return RASPICAM_FORMAT_YUV420;
    return RASPICAM_FORMAT_RGB;
  }

}

