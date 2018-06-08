//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
// 
// detect motion and save in level db
//////////////////////////////////////////////////////////////////////////////

//#include <camerasp/timer.hpp>
#include <jpeg/jpgconvert.hpp>
#include <fstream>
#include <sstream>
#include <camerasp/mot_detect.hpp>
#include <camerasp/save_ldb.hpp>

namespace camerasp
{
  save_ldb::save_ldb(std::string const& db_location):
    dB(db_location)
  {
  }
  void save_ldb::operator()(img_info const& img)
  {
    static motion_detector detector;
    if(detector.handle_motion(img))
    {
      dB.save_img(current_GMT_time(), img);
    }
  }
}
