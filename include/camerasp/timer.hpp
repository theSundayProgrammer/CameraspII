#pragma once
#include <camerasp/utils.hpp>
#include <mutex>
//#include <jpeg/jpgconvert.h>
#include <camerasp/cam_still.hpp>
#include <json/json.h>
#include <camerasp/types.hpp>
#include <memory>
namespace camerasp
{

  class file_saver
  {
    public:
      file_saver(Json::Value const&);
      camerasp::buffer_t get_image(unsigned int k);
      void save_image(camerasp::buffer_t const& image);
    private:
      unsigned int cur;
      unsigned int max_files;
    std::atomic<unsigned>  current_count;
      std::string image_path;
  };
  class periodic_frame_grabber
  {
    struct image_buffer
    {
      std::mutex m;
      buffer_t buffer;
    };

    public:
    periodic_frame_grabber(asio::io_context& io_service, Json::Value const&);
    ~periodic_frame_grabber(){}
    buffer_t  get_image(unsigned int k);
    bool resume();
    bool pause();
    errno_t set_vertical_flip(bool on);
    errno_t set_horizontal_flip(bool on);

    private:

    buffer_t grab_picture();
    void handle_timeout(const asio::error_code&);
    void set_timer();

    private:
    cam_still camera_;
    high_resolution_timer timer_;
    std::chrono::seconds sampling_period;
    enum { max_size = 100 };
    image_buffer image_buffers[max_size];
    std::atomic<unsigned>  current_count;
    std::atomic<bool> quit_flag;
    unsigned cur_img;
    file_saver file_saver_;
  };
}
