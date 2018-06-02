//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////

#include <camerasp/frame_grabber.hpp>
#include <jpeg/jpgconvert.hpp>
#include <fstream>
#include <sstream>

namespace camerasp
{
frame_grabber::frame_grabber(
    asio::io_context &io_service,
    Json::Value const &root) :
        camera(io_service),
	timer(io_service),
	cur_img(0), 
	current_count(0) ,
  running(false)
{
  auto backup = root["Data"];
  auto secs = backup["sample_period"].asInt();
  sampling_period = std::chrono::seconds(secs);
  quit_flag = 1;
  auto camera = root["Camera"];
  camera.set_width(camera["width"].asInt());
  camera.set_height(camera["height"].asInt());
  camera.setISO(camera["iso"].asInt());
  camera.set_vertical_flip(camera["vertical"].asInt());
  camera.set_horizontal_flip(camera["horizontal"].asInt());
  camera.connect([this](img_info& info) { grab_picture(info);});
}

void frame_grabber::grab_picture(img_info& info)
{

  //  At any point in time only one instance of this function will be running
  auto next = cur_img;

  running = false;
  auto buffer = write_JPEG_dat(info);
  {
    std::lock_guard<std::mutex> lock(image_buffers[next].m);
    image_buffers[next].buffer.swap(buffer);
  }
  on_image_capture(info);
  if (current_count < max_size)
    ++current_count;
  cur_img = (cur_img + 1) % max_size;
}
void frame_grabber::handle_timeout(const asio::error_code &)
{
  //At any point in time only one instance of this function will be running
  if (!quit_flag)
    if(!running)
    {
      running = true;
      auto current = high_resolution_timer::clock_type::now();
      timer.expires_at(current + sampling_period);
      timer.async_wait(std::bind(&frame_grabber::handle_timeout, this, std::placeholders::_1));
      camera.take_picture();
    }
    else
    {
      camera.release();
      throw std::runtime_error("camera not responding");
    }
}
void frame_grabber::begin_data_wait()
{
  camera.await_data_ready();
}
void frame_grabber::set_timer()
{
  try
  {
    auto prev = high_resolution_timer::clock_type::now();
    timer.expires_at(prev + sampling_period);
    timer.async_wait(std::bind(&frame_grabber::handle_timeout, this, std::placeholders::_1));
  }
  catch (std::exception &e)
  {
    console->error("Error: {}..", e.what());
  }
}
buffer_t frame_grabber::get_image(unsigned int k)
{
  //precondition there is at least one image in the buffer
  //That is, current_count>0
  if (current_count == 0)
    throw std::runtime_error("No image captured");
  {
    unsigned n = current_count; 
    auto next =  (cur_img + n - 1 - k) % n;
    console->info("Image number = {0}", next);
    std::lock_guard<std::mutex> lock(image_buffers[next].m);
    auto imagebuffer = image_buffers[next].buffer;
    return buffer_t(imagebuffer.begin(), imagebuffer.end());
  }
}
bool frame_grabber::resume()
{
  bool retval = quit_flag;
  if (quit_flag)
  {
    quit_flag = 0;
    if(retval = camera.open())
        set_timer();
  }
  return retval;
}
bool frame_grabber::pause()
{
  bool retval = 0 == quit_flag;
  if (0 == quit_flag)
    quit_flag = 1;
  return retval;
}

errno_t frame_grabber::set_vertical_flip(bool val)
{
  console->debug("vertical flip={0}", val);
  camera.set_vertical_flip(val);
  return 0;
}
errno_t frame_grabber::set_horizontal_flip(bool val)
{
  console->debug("horizontal flip={0}", val);
  camera.set_horizontal_flip(val);
  return 0;
}
}
