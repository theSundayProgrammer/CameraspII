#pragma once
#include <camerasp/utils.hpp>
#include <mutex>
#include <jpeg/jpgconvert.h>
#ifdef RASPICAM_MOCK
#include <camerasp/raspicamMock.hpp>
#else
#include <camerasp/cam_still.hpp>
#endif // RASPICAM_MOCK
#include <json/json.h>
namespace camerasp
{

  class periodic_frame_grabber
  {
    struct Imagebuffer
    {
      std::mutex m;
      buffer_t buffer;
    };

  public:
    periodic_frame_grabber(asio::io_context& io_service, Json::Value const&);
    std::string  getImage(unsigned int k);
    void setTimer();
    void stopCapture();

  private:
    static void save_image(buffer_t const& buffer, std::string const& fName);

    //need a better way of handling file save
    void save_file(int file_number);

    buffer_t grabPicture();
    void handle_timeout(const asio::error_code&);
    void startCapture();

  private:
    cam_still camera_;
    high_resolution_timer timer_;
    std::chrono::seconds sampling_period;
    int max_file_count;
    std::string  pathname_prefix;
    enum { maxSize = 100 };
    Imagebuffer image_buffers[maxSize];
    std::atomic<unsigned> pending_count, current_count;
    std::atomic<bool> quit_flag;
    high_resolution_timer::clock_type::time_point prev;
    unsigned cur_img;

  };
}
