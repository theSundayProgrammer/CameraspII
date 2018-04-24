
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
//
//////////////////////////////////////////////////////////////////////////////

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <camerasp/logger.hpp>
#include <camerasp/utils.hpp>
#include <camerasp/mot_detect.hpp>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <jpeg/jpgconvert.hpp>
#include <gsl/gsl_util>
using namespace std;
using namespace cv;
extern asio::io_service frame_grabber_service;
const int BMP_HEADER_SIZE = 54;

// Check if there is motion in the result matrix
// count the number of changes and return.
std::tuple<cv::Rect, int>
detect_motion(const Mat &motion, const Rect &bounding_box)
{
  int number_of_changes = 0;
  int min_x = motion.cols, max_x = 0;
  int min_y = motion.rows, max_y = 0;
  // loop over image and detect changes
  for (int j = 0; j < bounding_box.height; j += 2)
  { // height
    for (int i = 0; i < bounding_box.width; i += 2)
    { // height
      // check if at pixel (j,i) intensity is equal to 255
      // this means that the pixel is different in the sequence
      // of images (prev_frame, current_frame, next_frame)
      if (static_cast<int>(motion.at<int>(bounding_box.y + j, bounding_box.x + i)) == 255)
      {
        number_of_changes++;
        if (min_x > i)
          min_x = bounding_box.x + i;
        if (max_x < i)
          max_x = bounding_box.x + i;
        if (min_y > j)
          min_y = bounding_box.y + j;
        if (max_y < j)
          max_y = bounding_box.y + j;
      }
    }
  }
  if (number_of_changes)
  {
    // draw rectangle round the changed pixel
    Point x(min_x, min_y);
    Point y(max_x, max_y);
    Rect rect(x, y);
    return make_tuple(rect, number_of_changes);
  }
  return make_tuple(cv::Rect(), 0);
}

namespace camerasp
{

motion_detector::motion_detector(
  std::function<bool(std::string const&)> archive_)
       :number_of_sequence (0),
        archive(archive_)
{
  // Erode kernel -- used in motion detection
  leveldb::Options options;
  options.create_if_missing = true;
  kernel_ero = getStructuringElement(MORPH_RECT, Size(2, 2));
}
void motion_detector::handle_motion(img_info const& img)
{

  int number_of_changes;
  // Detect motion in window
  int x_start = 10;
  int y_start = 10;
  switch (current_state)
  {
    case 0:
      current_frame = Mat(img.height, img.width, CV_8UC3, const_cast<char*>(img.buffer.data()) + BMP_HEADER_SIZE);
      cvtColor(current_frame,current_frame, CV_BGR2GRAY);
      current_state = 1;
      return;
    case 1:
      next_frame = Mat(img.height, img.width, CV_8UC3, const_cast<char*>(img.buffer.data()) + BMP_HEADER_SIZE);
      cvtColor(next_frame, next_frame, CV_BGR2GRAY);
      current_state = 2;
      return;
    case 2:
      {

        // Take a new image
        Mat prev_frame(img.height, img.width, CV_8UC3, const_cast<char*>(img.buffer.data()) + BMP_HEADER_SIZE);
        cvtColor(prev_frame, prev_frame, CV_BGR2GRAY);
        cv::swap(prev_frame, current_frame);
        cv::swap(current_frame, next_frame);
        if (!next_frame.data ) 
        {
          return;
        }

        // Calc differences between the images and do AND-operation
        // threshold image, low differences are ignored (ex. contrast change due to sunlight)
        Mat d1, d2, motion;
        absdiff(prev_frame, next_frame, d1);
        absdiff(next_frame, current_frame, d2);
        bitwise_and(d1, d2, motion);
        threshold(motion, motion, 35, 255, CV_THRESH_BINARY);
        erode(motion, motion, kernel_ero);
        Rect rect;
        int width = current_frame.cols - 20;
        int height = current_frame.rows - 20;
        std::tie(rect, number_of_changes) = detect_motion(motion, cv::Rect(x_start, y_start, width, height));

        // If a lot of changes happened, we assume something changed.
        if (number_of_changes >= there_is_motion && db_ok)
        {
          console->debug("Top Left:{0},{1} ", rect.x, rect.y);
          console->info("Number of Changes in image = {0}", number_of_changes);
	  auto buffer = write_JPEG_dat(img);
          auto status = archive(buffer);
          db_ok =  status;
        }
        else if (number_of_sequence) {

          //To Do: remove this variable
          number_of_sequence = 0;
        }
      }
      break;
  }
  return;
}
}
