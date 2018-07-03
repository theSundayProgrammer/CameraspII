//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <atomic>
#include <string>
#include <json/json.h>
#include <camerasp/types.hpp>
#include <memory>
namespace camerasp
{
/** \brief A class that saves a sequence of files
 *
 * To allow for run time configuration, a Json subtree
 * is used
 */
  class file_saver
  {
    public:
/**
 * Constructor 
 */
      file_saver(Json::Value const& root, std::string const& path);
      buffer_t get_image(unsigned int k);
      std::string save_image(buffer_t const& image);
    private:
      unsigned int cur;
      unsigned int max_files;
      std::atomic<unsigned>  current_count;
      std::string const image_path;
  };
}
