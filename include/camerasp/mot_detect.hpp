//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <opencv2/core/core.hpp>

#include <camerasp/utils.hpp>
#include <jpeg/jpgconvert.hpp>
namespace camerasp
{
  
// When motion is detected we write the image to disk
//    - Check if the directory exists where the image will be stored.
//    - Build the directory and image names.
class motion_detector
{
public:
  motion_detector();
  bool handle_motion( img_info const&);

private:
  int current_state = 0;
  cv::Mat current_frame;
  cv::Mat next_frame;
  int number_of_sequence = 0;
  cv::Mat kernel_ero;
  // If more than 'there_is_motion' pixels are changed, we say there is motion
  // and store an image on disk
  const int there_is_motion = 50;
};
}
