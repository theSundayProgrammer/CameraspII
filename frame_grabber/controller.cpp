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
void configure_console(Json::Value& root)
{
  namespace spd = spdlog;
  auto log_config = root["Logging"];
  auto logpath = log_config["path"].asString()+"_fg";
  auto size_mega_bytes = log_config["size"].asInt();
  auto count_files = log_config["count"].asInt();
  console = spd::rotating_logger_mt("camerasp", logpath, 1024 * 1024 * size_mega_bytes, count_files);
  //console = spd::stdout_color_mt("console");
  console->set_level(spd::level::debug);
}

int main(int argc, char *argv[])
{
  using namespace boost::interprocess;
  try {
    // Construct the :shared_request_data.
    shared_memory_object::remove(RESPONSE_MEMORY_NAME );
    shared_memory_object shm_response(create_only, RESPONSE_MEMORY_NAME , read_write);


    shm_response.truncate(sizeof (shared_response_data));

    mapped_region region_response(shm_response, read_write);

    new (region_response.get_address()) shared_response_data;
    shared_response_data& response =*static_cast<shared_response_data*>(region_response.get_address());

    // Construct the :shared_request_data.
    shared_memory_object::remove(REQUEST_MEMORY_NAME);
    shared_memory_object shm_request(create_only, REQUEST_MEMORY_NAME, read_write);


    shm_request.truncate(sizeof (shared_request_data));

    mapped_region region_request(shm_request, read_write);

    new (region_request.get_address()) shared_request_data;
    shared_request_data& request =  *static_cast<shared_request_data *>(region_request.get_address());

    BOOST_SCOPE_EXIT(argc) {
      shared_memory_object::remove(REQUEST_MEMORY_NAME);
      shared_memory_object::remove(RESPONSE_MEMORY_NAME);
    } BOOST_SCOPE_EXIT_END;

    Json::Value root=camerasp::get_DOM(config_path + "options.json");
    configure_console(root);
    asio::io_service frame_grabber_service;
    camerasp::periodic_frame_grabber timer(frame_grabber_service, root["Data"]);
    timer.start_capture();


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
	else if (std::regex_search(uri,m,std::regex("image")))
	{
	  auto image= timer.get_image(0);
	  response.set(image); 
	}        
	else if (std::regex_search(uri,m,std::regex("exit")))
	{
	  timer.stop_capture();
	  frame_grabber_service.stop();
	  console->debug("SIGTERM handled");
	  return;
	}        
      }

    } };
    frame_grabber_service.run();
    thread1.join();
    console->info("frame_grabber_service.run complete, shutdown successful");
  }
  catch (std::exception& e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
