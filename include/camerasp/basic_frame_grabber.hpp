#pragma once
#include <camerasp/utils.hpp>
#include <mutex>
#include <camerasp/cam_still.hpp>
#include <json/json.h>
#include <camerasp/types.hpp>
#include <memory>
#include <jpeg/jpgconvert.hpp>
namespace camerasp
{

  struct img_info;
  class basic_frame_grabber
  {
    struct image_buffer
    {
      std::mutex m;
      buffer_t buffer;
    };

    public:
    basic_frame_grabber(asio::io_context& io_service, Json::Value const&);
    ~basic_frame_grabber(){}
    buffer_t  get_image(unsigned int k);
    bool resume();
    bool pause();
    errno_t set_vertical_flip(bool on);
    errno_t set_horizontal_flip(bool on);

    private:

    img_info grab_picture();
    void handle_timeout(const asio::error_code&);
    void set_timer();
    virtual void on_image_aquire(img_info&) {}

    private:
    cam_still camera_;
    high_resolution_timer timer_;
    std::chrono::seconds sampling_period;
    enum { max_size = 100 };
    image_buffer image_buffers[max_size];
    std::atomic<unsigned>  current_count;
    std::atomic<bool> quit_flag;
    unsigned cur_img;
  };
}
