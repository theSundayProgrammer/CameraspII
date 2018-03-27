#pragma once
#include <atomic>
#include <string>
#include <json/json.h>
#include <camerasp/types.hpp>
#include <memory>
namespace camerasp
{

  class file_saver
  {
    public:
      file_saver(Json::Value const&, std::string const&);
      camerasp::buffer_t get_image(unsigned int k);
      std::string save_image(camerasp::buffer_t const& image);
    private:
      unsigned int cur;
      unsigned int max_files;
      std::atomic<unsigned>  current_count;
      std::string image_path;
  };
}
