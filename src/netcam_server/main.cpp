
//////////////////////////////////////////////////////////////////////////////
// Copyright 2016-2018 (c) Joseph Mariadassou
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
#include <cstdlib>
#include <memory>
#include <camerasp/network.hpp>
#include <camerasp/frame_grabber.hpp>
#include <arpa/inet.h>
#include <asio/signal_set.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <camerasp/utils.hpp>
#include <boost/filesystem.hpp>
#include "./server.hpp"
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

  console = spd::stdout_color_mt("console");
//  console = spd::rotating_logger_mt("fg", logpath.string(), 1048576 * size_mega_bytes, count_files);
  console->set_level(spd::level::info);
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
      std::cerr << "Usage: netcam <path/to/options>\n";
      return 1;
    }
  auto &root = camerasp::get_root(argv[1]);
  //configure console
  home_path = root["home_path"].asString();
  const int no_error=0;
  try
  {
    configure_logger(root);
    asio::io_service frame_grabber_service;
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
    camerasp::frame_grabber frame_grabber(frame_grabber_service, root);
    std::thread thread2([&](){ frame_grabber.begin_data_wait();});
    //console->debug("OK here {0}: {1}", __FILE__, __LINE__);
    bool retval = frame_grabber.resume();
    if (!retval) {
      console->error("Unable to open Camera");
      return 1;
    }

    auto port_no = root["Server"]["port"].asInt();
    server s(frame_grabber_service, frame_grabber, port_no);
    console->debug("OK here {0}: {1}", __FILE__, __LINE__);
    frame_grabber_service.run();
    frame_grabber.stop_data_wait();
    thread2.join();
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
