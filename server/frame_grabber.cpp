
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
#include <camerasp/ipc.hpp>
std::shared_ptr<spdlog::logger> console;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
#else
const std::string config_path = "/srv/camerasp/";
#endif
#define ASIO_ERROR_CODE asio::error_code
void configure_console()
{
  namespace spd = spdlog;
  console = spd::stdout_color_mt("fg");
  console->set_level(spd::level::debug);
}

int main(int argc, char *argv[])
{
  using namespace boost::interprocess;
  try {
    configure_console();
    // Construct the :shared_request_data.
    shared_memory_object shm(open_only, REQUEST_MEMORY_NAME, read_write);

    mapped_region region(shm, read_write);

    shared_request_data& request = *static_cast<shared_request_data *>(region.get_address());

    console->debug("Line {0}",__LINE__);

    // Construct the :shared_response data.
    shared_memory_object shm_response(open_only, RESPONSE_MEMORY_NAME, read_write);

    mapped_region region_response(shm_response, read_write);

    shared_response_data& response = *static_cast<shared_response_data *>(region_response.get_address());

    console->debug("Line {0}",__LINE__);

    asio::io_service frame_grabber_service;
    // The signal set is used to register termination notifications
    asio::signal_set signals_(frame_grabber_service);
    signals_.add(SIGINT);
    signals_.add(SIGTERM);

    // register the handle_stop callback
    signals_.async_wait([&]
	(ASIO_ERROR_CODE const& error, int signal_number) { 
	request.set("exit");
	console->debug("SIGTERM received");
	});

    //configure frame grabber
    Json::Value root=camerasp::get_DOM(config_path + "options.json");
    camerasp::periodic_frame_grabber timer(frame_grabber_service, root["Data"]);
    timer.resume();
    console->debug("Line {0}",__LINE__);
    // Start worker threads 
    std::thread thread1{ [&]() { 
      for(;;)
      {
	std::string uri=request.get();
	std::smatch m;
	if (std::regex_search(uri, m,std::regex("image\\?prev=([0-9]+)$") ))
	{
	  int k = atoi(m[1].str().c_str());
	  auto image= timer.get_image(k);
	  response.set(image); 
	}
	else if (std::regex_search(uri,m,std::regex("flip\\?vertical=(0|1)$")))
	{
	  int k = atoi(m[1].str().c_str());
	  if(0==timer.set_vertical_flip(k != 0))
	    response.set("Success"); 
	  else
	    response.set("Error Flip failed");
	}
	else if (std::regex_search(uri,m,std::regex("flip\\?horizontal=(0|1)$")))
	{
	  int k = atoi(m[1].str().c_str());
	  if(0==timer.set_horizontal_flip(k != 0))
	    response.set("Success"); 
	  else
	    response.set("Error Flip failed");
	}
	else if (std::regex_search(uri,m,std::regex("resume")))
	{
	  if( timer.resume())
	    response.set("Success"); 
	  else
	    response.set("Running already");
	}        
	else if (std::regex_search(uri,m,std::regex("pause")))
	{
	  if( timer.pause())
	    response.set("Success"); 
	  else
	    response.set("Stopped already");
	}        
	else if (std::regex_search(uri,m,std::regex("image")))
	{
	  auto image= timer.get_image(0);
	  response.set(image); 
	}        
	else if (std::regex_search(uri,m,std::regex("exit")))
	{
	  timer.pause();
	  frame_grabber_service.stop();
	  response.set(std::string("stopping"));
	  console->debug("SIGTERM handled");
	  return;
	}        
      }

    } };
    console->debug("Line {0}",__LINE__);
    frame_grabber_service.run();
    thread1.join();
    //std::string uri=request.get();
    console->info("frame_grabber_service.run complete, shutdown successful");
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
