//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// adapted from Kenneth Baker's via-http
//
// addpted from http://github.com/kenba/vai-http
// original copyright follows
// Copyright (c) 2014-2015 Ken Barker
// (ken dot barker at via-technology dot co dot uk)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
/// @file cameras_http_server
/// @brief Main file of web server
//////////////////////////////////////////////////////////////////////////////
#include <via/comms/tcp_adaptor.hpp>
#include <via/http_server.hpp>
#include <iostream>
#include <json/reader.h>
#include <camerasp/utils.hpp>
#include <camerasp/handler.hpp>
#include <camerasp/timer.hpp>
std::shared_ptr<spdlog::logger> console;

/// Define an HTTP server using std::string to store message bodies
typedef http_server_type::http_connection_type http_connection;
typedef http_server_type::chunk_type http_chunk_type;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
#else
const std::string config_path = "/srv/camerasp/";
#endif
int main(int argc, char *argv[])
{
  namespace spd = spdlog;
  using camerasp::Handler;
  using namespace std::placeholders;
  std::string app_name(argv[0]);
  unsigned port_number=8088;
  try {
    auto root = camerasp::get_DOM(config_path + "options.json");
    auto server_config = root["Server"];
    if (!server_config.empty())
      port_number = server_config["port"].asInt();
    auto log_config = root["Logging"];
    auto json_path = log_config["path"];
    auto logpath = json_path.asString();
    auto size_mega_bytes = log_config["size"].asInt();
    auto count_files = log_config["count"].asInt();
    console = spd::rotating_logger_mt("camerasp", logpath, 1024 * 1024 * size_mega_bytes, count_files);
    //console = spd::stdout_color_mt("console");
    console->set_level(spd::level::debug);
    console->info("{0} at port {1}", app_name, port_number);
    // create an io_service for the server
    asio::io_service io_service;
    camerasp::periodic_frame_grabber timer(io_service, root["Data"]);
    Handler handler(timer);
    timer.set_timer();
    http_server_type http_server(io_service);
    http_server.request_received_event(std::bind(&Handler::request,std::ref(handler),_1,_2, _3));

    // connect the optional handler callback functions
    http_server.chunk_received_event(std::bind(&Handler::chunk,std::ref(handler),_1,_2, _3));
    http_server.request_expect_continue_event(std::bind(&Handler::expect_continue,std::ref(handler),_1,_2, _3));
    http_server.invalid_request_event(std::bind(&Handler::invalid_request,std::ref(handler),_1,_2, _3));
    http_server.socket_connected_event(std::bind(&Handler::connected,std::ref(handler),_1));
    http_server.socket_disconnected_event(std::bind(&Handler::disconnected,std::ref(handler),_1));
    http_server.message_sent_event(std::bind(&Handler::message_sent,std::ref(handler),_1));

    // set the connection timeout (10 seconds)
    http_server.set_timeout(10000);

    // set the connection buffer sizes
    http_server.set_rx_buffer_size(16384);
    http_server.tcp_server()->set_receive_buffer_size(16384);
    http_server.tcp_server()->set_send_buffer_size(16384);

    // start accepting http connections on the port
    ASIO_ERROR_CODE error(http_server.accept_connections(port_number));
    if (error)
    {
      console->error("Error: {0}", error.message());
      return 1;
    }

    // The signal set is used to register termination notifications
    asio::signal_set signals_(io_service);
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // #if defined(SIGQUIT)

    // register the handle_stop callback
    signals_.async_wait([&]
      (ASIO_ERROR_CODE const& error, int signal_number)
    { 
      handler.stop(error, signal_number, http_server);
      timer.stop_capture();
      io_service.stop();
    });
  // Start the on two  worker threads server
      //std::thread thread1{ [&io_service]() { io_service.run(); } };
      //std::thread thread2{ [&io_service]() { io_service.run(); } };
      io_service.run();
      //thread1.join();
      //thread2.join();
    console->info("io_service.run complete, shutdown successful");
  }
  catch (Json::LogicError& err) {
    std::cerr << "Parse Error: {0}" << err.what() << std::endl;
    return -1;
  }
 catch (std::exception& e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
//////////////////////////////////////////////////////////////////////////////
 
