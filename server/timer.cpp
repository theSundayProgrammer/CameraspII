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
void handle_motion(const char* fName);
namespace camerasp
{
  periodic_frame_grabber::periodic_frame_grabber(
      asio::io_context& io_service,
      Json::Value const& root)
    : timer_(io_service)
    ,cur_img(0)
    ,current_count(0)
    ,file_saver_(root["Data"], root["home_path"].asString())
  {
    auto backup=root["Data"];
    auto secs = backup["sample_period"].asInt();
    sampling_period = std::chrono::seconds(secs);
    quit_flag =1;
    auto camera=root["Camera"];
    camera_.set_width(camera["width"].asInt());
    camera_.set_height(camera["height"].asInt());
    camera_.setISO(camera["iso"].asInt());
    camera_.set_vertical_flip(camera["vertical"].asInt());
    camera_.set_horizontal_flip(camera["horizontal"].asInt());
  }


  buffer_t periodic_frame_grabber::grab_picture() {

    //  At any point in time only one instance of this function will be running
    img_info info;
    //  console->debug("Height = {0}, Width= {1}", camera_.get_height(), camera_.get_width());
    auto siz = camera_.image_buffer_size();
    info.buffer.resize(siz);
    if(camera_.take_picture((unsigned char*)(&info.buffer[0]), &siz)==0)
    {

    info.image_height = camera_.get_height();
    info.image_width = camera_.get_width();
    info.quality = 100;
    info.row_stride = info.image_width * 3;


    if (info.image_height > 0 && info.image_width > 0)
    {
      info.quality = 100;
#ifndef RASPICAM_MOCK
      info.xformbgr2rgb();
      return write_JPEG_dat(info);
#else
      info.buffer.resize(siz);
      return info.buffer;
#endif // !RASPICAM_MOCK
    
}
}
      return buffer_t();

  }
  void periodic_frame_grabber::handle_timeout(const asio::error_code&)
  {
    //At any point in time only one instance of this function will be running
    using namespace std::placeholders;

    if (!quit_flag) {
      auto current = high_resolution_timer::clock_type::now();
      auto next = cur_img;
      auto buffer = grab_picture();
      if (!buffer.empty())
      {
         auto fName = file_saver_.save_image(buffer);
	std::lock_guard<std::mutex> lock(image_buffers[next].m);
	image_buffers[next].buffer.swap(buffer);
         handle_motion(fName.c_str());
      }
      if (current_count < max_size) ++current_count;
      cur_img = (cur_img + 1) % max_size;
      timer_.expires_at(current+ sampling_period);
      timer_.async_wait(std::bind(&periodic_frame_grabber::handle_timeout, this, _1));
    }
    else
    {
      camera_.release();
    }
  }


  void periodic_frame_grabber::set_timer() {
    using namespace std::placeholders;
    try {
      auto prev = high_resolution_timer::clock_type::now();
      timer_.expires_at(prev + sampling_period);
      timer_.async_wait(std::bind(&periodic_frame_grabber::handle_timeout, this, _1));
    }
    catch (std::exception& e) {
      console->error("Error: {}..", e.what());
    }
  }
  buffer_t  periodic_frame_grabber::get_image(unsigned int k) 
  {
    //precondition there is at least one image in the buffer
    //Tha is, current_count>0
    if(current_count ==0)
      throw std::runtime_error("No image captured");
    if(k<max_size)
    {
      auto next =  (k > current_count && current_count < max_size)?
	0:
	(cur_img + max_size - 1 - k) % max_size;
      console->info("Image number = {0}", next);
      std::lock_guard<std::mutex> lock(image_buffers[next].m);
      auto imagebuffer = image_buffers[next].buffer;
      return buffer_t(imagebuffer.begin(), imagebuffer.end());
    }
    else
    {
      return file_saver_.get_image(k);
    }
  } 
  bool periodic_frame_grabber::resume()
  {
    bool retval=quit_flag;
    if (quit_flag) {
      quit_flag = 0;
      camera_.open();
      set_timer();
    }
    return retval;
  }
  bool periodic_frame_grabber::pause()
  {
    bool retval= 0==quit_flag;
    if (0 == quit_flag)  quit_flag = 1;
    return retval;
  }

  errno_t periodic_frame_grabber::set_vertical_flip(bool val)
  {
      console->debug("vertical flip={0}",val);
    camera_.set_vertical_flip(val);
    return 0;
  }
  errno_t  periodic_frame_grabber::set_horizontal_flip(bool val)
  {
      console->debug("horizontal flip={0}",val);
    camera_.set_horizontal_flip(val);
    return 0;
  }
}
