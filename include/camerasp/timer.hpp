#pragma once
#include <camerasp/utils.hpp>
#include <mutex>
//#include <jpeg/jpgconvert.h>
#include <camerasp/cam_still.hpp>
#include <json/json.h>
#include <camerasp/types.hpp>
namespace camerasp
{

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
    void start_capture();
    void stop_capture();
    errno_t set_vertical_flip(bool on);
    errno_t set_horizontal_flip(bool on);
  private:
    static void save_image(buffer_t const& buffer, std::string const& fName);

    //need a better way of handling file save
    void save_file(buffer_t&,unsigned int);

    buffer_t grab_picture();
    void handle_timeout(const asio::error_code&);
    void set_timer();

  private:
    cam_still camera_;
    high_resolution_timer timer_;
    std::chrono::seconds sampling_period;
    int max_file_count;
    std::string  pathname_prefix;
    enum { max_size = 100 };
    image_buffer image_buffers[max_size];
    std::atomic<unsigned> pending_count, current_count;
    std::atomic<bool> quit_flag;
    high_resolution_timer::clock_type::time_point prev;
    unsigned cur_img;

  };
}
