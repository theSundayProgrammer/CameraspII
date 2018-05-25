
//////////////////////////////////////////////////////////////////////////////
// Copyright 2016-2017 (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#include <json/reader.h>
#include <asio.hpp>

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cstdarg>
#include <regex>
#include <iostream>
#include <atomic>
#include <camerasp/utils.hpp>
#include <camerasp/timer.hpp>
#include <camerasp/msgq.hpp>
#include <boost/filesystem.hpp>
std::shared_ptr<spdlog::logger> console;
#define ASIO_ERROR_CODE 
std::string home_path;
void configure_logger(Json::Value &root)
{
  namespace spd = spdlog;
  boost::filesystem::path root_path(home_path);
  auto log_config = root["Logging"];
  auto json_path = log_config["path"];
  auto logpath = root_path / (json_path.asString());
  auto size_mega_bytes = log_config["size"].asInt();
  auto count_files = log_config["count"].asInt();

  console = spd::rotating_logger_mt("fg", logpath.string(), 1048576 * size_mega_bytes, count_files);
  console->set_level(spd::level::debug);
}
int main(int argc, char *argv[])
{
  using namespace boost::interprocess;
  auto &root = camerasp::get_root();
  //configure console
  home_path = root["home_path"].asString();
  const int no_error=0;
  try
  {
    configure_logger(root);
    asio::io_service frame_grabber_service;
    camerasp::client_msg_queues queue(128);
    // The signal set is used to register termination notifications
    asio::signal_set signals_(frame_grabber_service);
    signals_.add(SIGINT);
    signals_.add(SIGTERM);

    // register the handle_stop callback
    signals_.async_wait([&](asio::error_code const &error, int signal_number) {
      frame_grabber_service.stop();
      console->debug("SIGTERM received");
    });

    //configure frame grabber
    camerasp::periodic_frame_grabber frame_grabber(frame_grabber_service, root);
    bool retval = frame_grabber.resume();
    if (!retval) {
      console->error("Unable to open Camera");
      return 1;
  }
    auto capture_frame = [&](int k) {
      try
      {
  
        auto image = frame_grabber.get_image(k);
        queue.send_response(no_error,image);
      }
      catch (std::runtime_error &er)
      {
        queue.send_response(-1,"Failed");
        console->debug("Frame Capture Failed");
      }
    };

    // Start worker threads
    std::thread thread1{[&]() {
      for (;;)
      {
        std::string uri = queue.get_request();
        std::smatch m;
        if (std::regex_search(uri, m, std::regex("image\\?prev=([0-9]+)$")))
        {
          int k = atoi(m[1].str().c_str());
          capture_frame(k);
        }
        else if (std::regex_search(uri, m, std::regex("flip\\?vertical=(0|1)$")))
        {
          int k = atoi(m[1].str().c_str());
          if (0 == frame_grabber.set_vertical_flip(k != 0))
            queue.send_response(0,"Success");
          else
            queue.send_response(-1,"Failed");
        }
        else if (std::regex_search(uri, m, std::regex("flip\\?horizontal=(0|1)$")))
        {
          int k = atoi(m[1].str().c_str());
          if (0 == frame_grabber.set_horizontal_flip(k != 0))
            queue.send_response(0,"Success");
          else
            queue.send_response(-1,"Failed");
        }
        else if (std::regex_search(uri, m, std::regex("resume")))
        {
          if (frame_grabber.resume())
            queue.send_response(0,"Success");
          else
            queue.send_response(-1,"Failed");
        }
        else if (std::regex_search(uri, m, std::regex("pause")))
        {
          if (frame_grabber.pause())
            queue.send_response(0,"Success");
          else
            queue.send_response(-1,"Failed");
        }
        else if (std::regex_search(uri, m, std::regex("image")))
        {
          int k = 0;
          capture_frame(k);
        }
        else if (std::regex_search(uri, m, std::regex("exit")))
        {
          frame_grabber.pause();
          frame_grabber_service.stop();
          queue.send_response(no_error,"Stopping");
          console->debug("SIGTERM handled");
          return;
        }
      }

    }};
    frame_grabber_service.run();
    thread1.join();
    //std::string uri=request.get();
    console->info("frame_grabber_service.run complete, shutdown successful");
  }
  catch (Json::LogicError &err)
  {
    std::cerr << "Parse Error: {0}" << err.what() << std::endl;
    return -1;
  }
  catch (std::exception &e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
