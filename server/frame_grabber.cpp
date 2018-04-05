
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
#include <boost/filesystem.hpp>
std::shared_ptr<spdlog::logger> console;
#define ASIO_ERROR_CODE asio::error_code
extern    asio::io_service frame_grabber_service;
asio::io_service frame_grabber_service;
std::string home_path;
void configure_console(Json::Value& root)
{
  namespace spd = spdlog;
  boost::filesystem::path root_path(home_path);
  auto log_config = root["Logging"];
  auto json_path = log_config["path"];
  auto logpath = root_path/(json_path.asString());
  auto size_mega_bytes = log_config["size"].asInt();
  auto count_files = log_config["count"].asInt();
  
  console = spd::rotating_logger_mt("fg", logpath.string(), 1048576 * size_mega_bytes, count_files);
  console->set_level(spd::level::debug);
}
int main(int argc, char *argv[])
{
  using namespace boost::interprocess;
  auto&   root = camerasp::get_root();
    //configure console
    home_path = root["home_path"].asString();
  
  try {
    configure_console(root);
    // Construct the :shared_request_data.
    shared_memory_object shm(open_only, REQUEST_MEMORY_NAME, read_write);

    mapped_region region(shm, read_write);

    shared_request_data& request = *static_cast<shared_request_data *>(region.get_address());


    // Construct the :shared_response data.
    shared_memory_object shm_response(open_only, RESPONSE_MEMORY_NAME, read_write);

    mapped_region region_response(shm_response, read_write);

    shared_response_data& response = *static_cast<shared_response_data *>(region_response.get_address());


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
    camerasp::periodic_frame_grabber timer(frame_grabber_service, root);
    timer.resume();
	auto capture_frame = [&](int k){
	  try
	  {
	    std::string error(4,'\0');
            console->debug("OK here {0}", __LINE__); 
	    auto image= timer.get_image(k);
            console->debug("OK here {0}", __LINE__); 
	    response.set(error+image); 
	  }
	  catch(std::runtime_error& er)
	  {
            response.set("EMPT");
            console->debug("Error=EMPTY"); 
	  }	
	};
    // Start worker threads 
    std::thread thread1{ [&]() { 
      for(;;)
      {
	std::string uri=request.get();
	std::smatch m;
	if (std::regex_search(uri, m,std::regex("image\\?prev=([0-9]+)$") ))
	{
	  int k = atoi(m[1].str().c_str());
          capture_frame(k);
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
	  int k = 0;
          capture_frame(k);
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
