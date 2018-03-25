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
namespace camerasp
{

  file_saver::file_saver( Json::Value const& backup):
    current_count(0)
    ,cur(0)
  {
    max_files= backup["count"].asInt();
    image_path= backup["path_prefix"].asString();
    console->debug("path={0},count={1}",image_path,max_files);
  }


  camerasp::buffer_t file_saver::get_image(unsigned int k)
  {
    std::ostringstream ostr;
    unsigned next = current_count < max_files && k >= current_count?
      0:
      (cur+max_files-k)%max_files;
    ostr<<image_path<<next<<std::ends;

    std::ifstream img_file(ostr.str(),std::ios::binary);
    std::ostringstream img_str;
    img_str<<img_file.rdbuf();
    return img_str.str();
  }
  void file_saver::save_image(camerasp::buffer_t const& image)
  {
    std::ostringstream ostr;
    unsigned int k=cur;
    ostr<<image_path<<k<<std::ends;
    std::ofstream(ostr.str(),std::ios::binary) << image;
    ++cur;
    if(cur == max_files) cur=0;
    if(current_count < max_files) ++current_count;
  }
}
