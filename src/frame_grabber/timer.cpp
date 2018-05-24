//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////

#include <camerasp/timer.hpp>
#include <jpeg/jpgconvert.hpp>
#include <fstream>
#include <sstream>
#include <camerasp/mot_detect.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
namespace camerasp
{
  periodic_frame_grabber::periodic_frame_grabber(
      asio::io_context &io_service,
      Json::Value const &root) :
    basic_frame_grabber(io_service,root),
    file_saver_(root["Data"], root["home_path"].asString())
  {
  }
  void periodic_frame_grabber::on_image_acquire(img_info& img)
  {
    static motion_detector detector;
    auto buffer = write_JPEG_dat(img);
    auto fName = file_saver_.save_image(buffer);
    detector.handle_motion(fName.c_str(),img);
  }
}
