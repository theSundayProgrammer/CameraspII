//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////

#include <camerasp/timer.hpp>
#include <jpeg/jpgconvert.hpp>
namespace camerasp
{
  
  periodic_frame_grabber::periodic_frame_grabber(
    asio::io_context& io_service,
    Json::Value const& backup)
    : timer_(io_service),
      cur_img(0),
      current_count(0)
  {
      max_file_count = backup["count"].asInt();
      int secs = backup["sample_period"].asInt();
      sampling_period = std::chrono::seconds(secs);
      pathname_prefix = backup["path_prefix"].asString();
      quit_flag =0;
      pending_count=0;
     camera_.set_width(640);
     camera_.set_height(480);
     camera_.setISO(400);
     camera_.open();

  }
  void periodic_frame_grabber::save_image(buffer_t const& buffer, std::string const& fName)
  {
    FILE *fp = nullptr;
    fopen_s(&fp, fName.c_str(), "wb");
    if (fp)
    {
      for (auto c : buffer) putc(c, fp);
      fclose(fp);
    }
  }
  //need a better way of handling file save
  void periodic_frame_grabber::save_file(buffer_t& buffer, unsigned int file_number) {
    
    char intstr[8];
    sprintf(intstr, "%04d", file_number);
    save_image(buffer, pathname_prefix + intstr + ".jpg");
    --pending_count;
  }

  buffer_t periodic_frame_grabber::grab_picture() {

    //  At any point in time only one instance of this function will be running
    img_info info;
    //  console->debug("Height = {0}, Width= {1}", camera_.get_height(), camera_.get_width());
    auto siz = camera_.image_buffer_size();
    info.buffer.resize(siz);
    camera_.take_picture((unsigned char*)(&info.buffer[0]), &siz);

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
    else
    {
      return buffer_t();

    }
  }
  void periodic_frame_grabber::handle_timeout(const asio::error_code&)
  {
    //At any point in time only one instance of this function will be running
    using namespace std::placeholders;

    if (!quit_flag) {
    //console->debug("timer active");
      auto current = high_resolution_timer::clock_type::now();
      auto diff = current - prev;
      auto next = cur_img;
      auto buffer = grab_picture();
      
      int files_in_queue = ++pending_count;
      if (files_in_queue < max_file_count / 10 || files_in_queue < 10)
      {
        save_file(buffer, next);
      }
      else
      {
        --pending_count;
      }
      {
        std::lock_guard<std::mutex> lock(image_buffers[next].m);
        image_buffers[next].buffer.swap(buffer);
      }
      if (current_count < max_size) ++current_count;
      cur_img = (cur_img + 1) % max_size;
      //console->info("Prev Data Size {0}; time elapse {1}s..", buffer.size(), diff.count());
      timer_.expires_at(prev + 2 * sampling_period);
      prev = current;
      timer_.async_wait(std::bind(&periodic_frame_grabber::handle_timeout, this, _1));
    }
  }


  void periodic_frame_grabber::set_timer() {
    using namespace std::placeholders;
    try {
      prev = high_resolution_timer::clock_type::now();
      timer_.expires_at(prev + sampling_period);
      timer_.async_wait(std::bind(&periodic_frame_grabber::handle_timeout, this, _1));
    }
    catch (std::exception& e) {
      console->error("Error: {}..", e.what());
    }
  }
  buffer_t  periodic_frame_grabber::get_image(unsigned int k) {

    auto next =  (k > current_count && current_count < max_size)?
        0:
       (cur_img + max_size - 1 - k) % max_size;
    console->info("Image number = {0}", next);
    std::lock_guard<std::mutex> lock(image_buffers[next].m);
    auto imagebuffer = image_buffers[next].buffer;
    return buffer_t(imagebuffer.begin(), imagebuffer.end());
  }
  void periodic_frame_grabber::start_capture()
  {
    if (quit_flag) {
      quit_flag = 0;
      set_timer();
    }
  }
  void periodic_frame_grabber::stop_capture()
  {
    if (0 == quit_flag)  quit_flag = 1;
  }

  errno_t periodic_frame_grabber::set_vertical_flip(bool val)
  {
    camera_.set_vertical_flip(val);
    return 0;
  }
  errno_t  periodic_frame_grabber::set_horizontal_flip(bool val)
  {
     camera_.set_horizontal_flip(val);
    return 0;
  }
}
