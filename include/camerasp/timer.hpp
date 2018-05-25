#pragma once
#include <camerasp/utils.hpp>
#include <mutex>
#include <camerasp/cam_still.hpp>
#include <json/json.h>
#include <camerasp/types.hpp>
#include <memory>
#include <jpeg/jpgconvert.hpp>
#include <camerasp/file_saver.hpp>
#include <camerasp/basic_frame_grabber.hpp>
#include <camerasp/smtp_client.hpp>
namespace camerasp
{

  class periodic_frame_grabber:
        public basic_frame_grabber
  {

    public:
    periodic_frame_grabber(asio::io_context& io_service, Json::Value const&);
    ~periodic_frame_grabber(){}

    private:
    void on_image_acquire(img_info&);

  asio::ssl::context ctx;
  smtp_client smtp;
  asio::ip::tcp::resolver::iterator socket_address;
  };
}
