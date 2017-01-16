#include <json/reader.h>
#include <asio.hpp>

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cstdarg>
#include <regex>
#include <iostream>
#include <fstream>
#include <atomic>
#include <camerasp/utils.hpp>
#include <camerasp/timer.hpp>
#include <camerasp/ipc.hpp>
std::shared_ptr<spdlog::logger> console;

int main(int argc, char *argv[])
{
  using namespace boost::interprocess;
  try
  {
    console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);
    // Construct the shared_request_data.
    shared_memory_object shm(open_only, REQUEST_MEMORY_NAME, read_write);

    mapped_region region(shm, read_write);

    shared_request_data& request = *static_cast<shared_request_data *>(region.get_address());
    if(argc>1 )
    {
      request.set(argv[1]);
      shared_memory_object shm_response(open_only, RESPONSE_MEMORY_NAME, read_write);

      mapped_region region_response(shm_response, read_write);

      shared_response_data& response = *static_cast<shared_response_data *>(region_response.get_address());
      camerasp::buffer_t data = response.get();
      std::ofstream ofs("img.jpg",std::ios::binary);
      ofs<<data;

    }else {
      request.set("exit");
    }
    console->info("{0} executed",argc>1? argv[1]:"exit");
  }
  catch (Json::LogicError& err) 
  {
    std::cerr << "Parse Error: {0}" << err.what() << std::endl;
    return -1;
  }
  catch (std::exception& e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
