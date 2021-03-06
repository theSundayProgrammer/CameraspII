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
#include <mutex>
#include <fstream>
#include <sstream>
#include <camerasp/raspicamtypes.h>
#include <camerasp/utils.hpp>
#include <mmal/mmal_types.h>
#include <mmal/mmal_parameters_camera.h>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
#else
#endif
namespace camerasp{
  void write_file_content(std::string const& path, std::string const& dat)    {
    std::ofstream ofs(path);
    ofs<<dat;
  }



std::string current_GMT_time()
{
  auto now = std::chrono::system_clock::now();                // system time
  auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&in_time_t), "%a, %b %d %Y %X GMT");
  return ss.str();
}

std::string current_date_time()
{
  auto now = std::chrono::system_clock::now();                // system time
  auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%a, %b %d %Y %X %z");
  return ss.str();
}
std::string EncodeBase64(const std::string &data)
{
  using namespace boost::archive::iterators;
  std::stringstream os;
  typedef insert_linebreaks<
      base64_from_binary<  // convert binary values to base64 characters
          transform_width< // retrieve 6 bit integers from a sequence of 8 bit bytes
              const char *, 6, 8>>,
      72 // insert line breaks every 72 characters
      >
      base64_text; // compose all the above operations in to a new iterator

  std::copy(
      base64_text(data.c_str()),
      base64_text(data.c_str() + data.size()),
      ostream_iterator<char>(os));

  return os.str();
}
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

  Json::Value& get_root(std::string const& config_path)
{
  static Json::Value  root=get_DOM(config_path);
  return root;
}
  Json::Value& get_root()
{
  const std::string config_path = "/srv/camerasp/";
  static Json::Value  root=get_DOM(config_path+"options.json");
  return root;
}
MMAL_PARAM_EXPOSUREMODE_T get_exposure_from_string (const  std::string& str ) {
    if ( str=="OFF" ) return MMAL_PARAM_EXPOSUREMODE_OFF;
    if ( str=="AUTO" ) return MMAL_PARAM_EXPOSUREMODE_AUTO;
    if ( str=="NIGHT" ) return MMAL_PARAM_EXPOSUREMODE_NIGHT;
    if ( str=="NIGHTPREVIEW" ) return MMAL_PARAM_EXPOSUREMODE_NIGHTPREVIEW;
    if ( str=="BACKLIGHT" ) return MMAL_PARAM_EXPOSUREMODE_BACKLIGHT;
    if ( str=="SPOTLIGHT" ) return MMAL_PARAM_EXPOSUREMODE_SPOTLIGHT;
    if ( str=="SPORTS" ) return MMAL_PARAM_EXPOSUREMODE_SPORTS;
    if ( str=="SNOW" ) return MMAL_PARAM_EXPOSUREMODE_SNOW;
    if ( str=="BEACH" ) return MMAL_PARAM_EXPOSUREMODE_BEACH;
    if ( str=="VERYLONG" ) return MMAL_PARAM_EXPOSUREMODE_VERYLONG;
    if ( str=="FIXEDFPS" ) return MMAL_PARAM_EXPOSUREMODE_FIXEDFPS;
    if ( str=="ANTISHAKE" ) return MMAL_PARAM_EXPOSUREMODE_ANTISHAKE;
    if ( str=="FIREWORKS" ) return MMAL_PARAM_EXPOSUREMODE_FIREWORKS;
    return MMAL_PARAM_EXPOSUREMODE_AUTO;
  }

  MMAL_PARAM_AWBMODE_T get_awb_from_string ( const std::string& str ) {
    if ( str=="OFF" ) return MMAL_PARAM_AWBMODE_OFF;
    if ( str=="AUTO" ) return MMAL_PARAM_AWBMODE_AUTO;
    if ( str=="SUNLIGHT" ) return MMAL_PARAM_AWBMODE_SUNLIGHT;
    if ( str=="CLOUDY" ) return MMAL_PARAM_AWBMODE_CLOUDY;
    if ( str=="SHADE" ) return MMAL_PARAM_AWBMODE_SHADE;
    if ( str=="TUNGSTEN" ) return MMAL_PARAM_AWBMODE_TUNGSTEN;
    if ( str=="FLUORESCENT" ) return MMAL_PARAM_AWBMODE_FLUORESCENT;
    if ( str=="INCANDESCENT" ) return MMAL_PARAM_AWBMODE_INCANDESCENT;
    if ( str=="FLASH" ) return MMAL_PARAM_AWBMODE_FLASH;
    if ( str=="HORIZON" ) return MMAL_PARAM_AWBMODE_HORIZON;
    return MMAL_PARAM_AWBMODE_AUTO;
  }

}

