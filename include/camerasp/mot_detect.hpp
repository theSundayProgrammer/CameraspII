#pragma once

#include <opencv2/core/core.hpp>

#include <camerasp/utils.hpp>
#include <jpeg/jpgconvert.hpp>
#include <tuple>
namespace leveldb{
  class DB;
}
namespace camerasp
{
  
class motion_detector
{
public:
  motion_detector();
  ~motion_detector();
  void handle_motion( img_info const&);
  std::tuple<int,std::string> get_image(std::string const& start);
  std::tuple<int,std::string> get_key(std::string const& start);
private:
  int current_state = 0;
  cv::Mat current_frame;
  cv::Mat next_frame;
  int number_of_sequence = 0;
  leveldb::DB *db;
  bool db_ok;
  cv::Mat kernel_ero;
  // If more than 'there_is_motion' pixels are changed, we say there is motion
  // and store an image on disk
  const int there_is_motion = 50;
};
}
