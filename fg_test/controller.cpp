
#include <json/reader.h>
#include <asio.hpp>
#include <spawn.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <camerasp/utils.hpp>
#include <camerasp/timer.hpp>
#include <camerasp/ipc.hpp>
std::shared_ptr<spdlog::logger> console;

int main(int argc, char *argv[], char* env[])
{
  using namespace boost::interprocess;
  try
  {
    console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);
    console->debug("Starting");
    // Construct the shared_request_data.
    shared_memory_object::remove(RESPONSE_MEMORY_NAME );
    shared_memory_object shm_response(open_or_create, RESPONSE_MEMORY_NAME , read_write);


    shm_response.truncate(sizeof (shared_response_data));

    mapped_region region_response(shm_response, read_write);

    new (region_response.get_address()) shared_response_data;
    shared_response_data& response =*static_cast<shared_response_data*>(region_response.get_address());
    console->debug("CReated response");
    // Construct the :shared_request_data.
    shared_memory_object::remove(REQUEST_MEMORY_NAME);
    shared_memory_object shm_request(open_or_create, REQUEST_MEMORY_NAME, read_write);


    shm_request.truncate(sizeof (shared_request_data));

    mapped_region region_request(shm_request, read_write);

    new (region_request.get_address()) shared_request_data;
    shared_request_data& request =  *static_cast<shared_request_data *>(region_request.get_address());

    console->debug("CReated request");
    BOOST_SCOPE_EXIT(argc) {
      shared_memory_object::remove(REQUEST_MEMORY_NAME);
      shared_memory_object::remove(RESPONSE_MEMORY_NAME);
    } BOOST_SCOPE_EXIT_END;

    char const *cmd= "/home/chakra/projects/camerasp2/frame_grabber/camerasp";
    int ret;
    pid_t child_pid;
    posix_spawn_file_actions_t child_fd_actions;
    if (ret = posix_spawn_file_actions_init (&child_fd_actions))
      console->error("posix_spawn_file_actions_init"), exit(ret);
    if (ret = posix_spawn_file_actions_addopen (&child_fd_actions, 1, "/tmp/foo-log", 
	  O_WRONLY | O_CREAT | O_TRUNC, 0644))
      console->error("posix_spawn_file_actions_addopen"), exit(ret);
    if (ret = posix_spawn_file_actions_adddup2 (&child_fd_actions, 1, 2))
      console->error("posix_spawn_file_actions_adddup2"), exit(ret);

    std::string str;
    bool started =false;
    while(std::cin>>str)
    {
      if(str=="start")
      {
	started=true;
	if (ret = posix_spawnp (&child_pid, cmd, &child_fd_actions, NULL, argv, env))
	  console->error("posix_spawn"), exit(ret);
	console->info("Child pid: {0}\n", child_pid);
      }
      else if(started && str=="exit")
      {
	started=false;
	request.set(str);
	camerasp::buffer_t data = response.get();
	console->info("{0} executed",str);
      }
      else if(started)
      {
	request.set(str);
	camerasp::buffer_t data = response.get();
	std::ofstream ofs("/home/chakra/data/img.jpg",std::ios::binary);
	ofs<<data;

	console->info("{0} executed",str);
      }
    }
    request.set("exit");
    camerasp::buffer_t data = response.get();
  }
  catch (std::exception& e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
