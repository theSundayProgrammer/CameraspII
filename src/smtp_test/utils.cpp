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
const std::string config_path = "/srv/camerasp/";
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
}
