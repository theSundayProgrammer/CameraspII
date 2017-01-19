
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#include <camerasp/server_http.hpp>
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
#include <camerasp/ipc.hpp>
#include <boost/filesystem.hpp>
#include <sys/wait.h>
std::shared_ptr<spdlog::logger> console;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
#else
const std::string config_path = "/srv/camerasp/";
#endif
//ToDo: set executable file name in json config
char const *cmd= "/home/chakra/projects/camerasp2/frame_grabber/camerasp";
char const *home_page="/home/pi/data/web";
char const *log_folder="/tmp/foo-log";
#define ASIO_ERROR_CODE asio::error_code
typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;
void default_resource_send(const HttpServer &server,
                           const std::shared_ptr<HttpServer::Response> &response,
                           const std::shared_ptr<std::ifstream> &ifs) ;
enum process_state{  started, stop_pending, stopped};

/*****************************************
State Transition Table
----------------------------------------
State| Command| Result State
started| stop | stop_pending 
started| start| started
stopped| stop | stopped
stopped| start| started
stop_pending | stop | stop_pending
stop_pending | start| stop_pending
******************************************/


int main(int argc, char *argv[], char* env[])
{
  namespace ipc=boost::interprocess;
  using namespace camerasp;
  try
  {
    auto root = camerasp::get_DOM(config_path + "options.json");

    //configure console

    auto log_config = root["Logging"];
    auto json_path = log_config["path"];
    auto logpath = json_path.asString();
    auto size_mega_bytes = log_config["size"].asInt();
    auto count_files = log_config["count"].asInt();
    //console = spd::rotating_logger_mt("console", logpath, \
    1024 * 1024 * size_mega_bytes, count_files);
    console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);
    console->debug("Starting");

    // Construct the shared_request_data.
    ipc::shared_memory_object::remove(RESPONSE_MEMORY_NAME );
    ipc::shared_memory_object shm_response(ipc::open_or_create, 
	RESPONSE_MEMORY_NAME , ipc::read_write);


    shm_response.truncate(sizeof (shared_response_data));

    ipc::mapped_region region_response(shm_response, ipc::read_write);

    new (region_response.get_address()) shared_response_data;
    shared_response_data& response =*static_cast<shared_response_data*>
      (region_response.get_address());
    console->debug("CReated response");

    // Construct the :shared_request_data.
    ipc::shared_memory_object::remove(REQUEST_MEMORY_NAME);
    ipc::shared_memory_object shm_request(ipc::open_or_create, 
	REQUEST_MEMORY_NAME, ipc::read_write);


    shm_request.truncate(sizeof (shared_request_data));

    ipc::mapped_region region_request(shm_request, ipc::read_write);

    new (region_request.get_address()) shared_request_data;
    shared_request_data& request = *static_cast<shared_request_data *>      
      (region_request.get_address());

    console->debug("CReated request");
    BOOST_SCOPE_EXIT(argc) {
      ipc::shared_memory_object::remove(REQUEST_MEMORY_NAME);
      ipc::shared_memory_object::remove(RESPONSE_MEMORY_NAME);
    } BOOST_SCOPE_EXIT_END;

    //Child process
    //Important: the working directory of the child process
    // is the same as that of the parent process.
    int ret;
    pid_t child_pid;
    posix_spawn_file_actions_t child_fd_actions;
    if (ret = posix_spawn_file_actions_init (&child_fd_actions))
      console->error("posix_spawn_file_actions_init"), exit(ret);
    if (ret = posix_spawn_file_actions_addopen (
	  &child_fd_actions, 1,log_folder , 
	  O_WRONLY | O_CREAT | O_TRUNC, 0644))
      console->error("posix_spawn_file_actions_addopen"), exit(ret);
    if (ret = posix_spawn_file_actions_adddup2 (&child_fd_actions, 1, 2))
      console->error("posix_spawn_file_actions_adddup2"), exit(ret);
    if (ret = posix_spawnp (&child_pid, cmd, &child_fd_actions, 
	  NULL, argv, env))
      console->error("posix_spawn"), exit(ret);
    console->info("Child pid: {0}\n", child_pid);

     process_state fg_state = started;
    // HTTP Server
    HttpServer server;
    unsigned port_number=8088;
    auto server_config = root["Server"];
    if (!server_config.empty())
      port_number = server_config["port"].asInt();
    server.config.port=port_number;
    server.config.thread_pool_size=2;
    auto io_service=std::make_shared<asio::io_service>();
    server.io_service = io_service;
    // get previous image
    server.resource["^/image\\?prev=([0-9]+)$"]["GET"]=[&](
	std::shared_ptr<HttpServer::Response> http_response,
	std::shared_ptr<HttpServer::Request> http_request)
    {

    std::string str;

      str =http_request->path_match[0];
      request.set(str);
      camerasp::buffer_t data = response.get();
      console->debug("size of data sent= {0}", data.size());
      *http_response <<  "HTTP/1.1 200 OK\r\n" 
	<<  "Content-Length: " << data.size()<< "\r\n"
	<<  "Content-type: " << "image/jpeg" <<"\r\n"
	<< "\r\n"
	<< data;
    };

    // get current image
    server.resource["^/image$"]["GET"]=[&](
	std::shared_ptr<HttpServer::Response> http_response,
	std::shared_ptr<HttpServer::Request> http_request)
    {

    std::string str;

      str =http_request->path_match[0];
      request.set(str);
      camerasp::buffer_t data = response.get();
      *http_response <<  "HTTP/1.1 200 OK\r\n" 
	<<  "Content-Length: " << data.size()<< "\r\n"
	<<  "Content-type: " << "image/jpeg" <<"\r\n"
	<< "\r\n"
	<< data;
    };

    //restart capture
    server.resource["^/start$"]["GET"]=[&](
	std::shared_ptr<HttpServer::Response> http_response,
	std::shared_ptr<HttpServer::Request> http_request)
    {
      std::string success("Succeeded");
      if(fg_state == started )
	success="Already Running";
      else if (fg_state == stop_pending)
      {
	success= "Stop Pending. try again later";
      }
      else
      {
	if (ret = posix_spawnp (&child_pid, cmd, 
	      &child_fd_actions, NULL, argv, env))
	  console->error("posix_spawn"), exit(ret);
	fg_state = started;
	console->info("Child pid: {0}\n", child_pid);
      }
      *http_response <<  "HTTP/1.1 200 OK\r\n" 
	<<  "Content-Length: " << success.size()<< "\r\n"
	<<  "Content-type: " << "application/text" <<"\r\n"
	<< "\r\n"
	<< success;
    };

    //stop capture
    server.resource["^/stop$"]["GET"]=[&](
	std::shared_ptr<HttpServer::Response> http_response,
	std::shared_ptr<HttpServer::Request> http_request)
    {
      std::string success("Succeeded");
      //
      if(fg_state == started )
      {
	request.set("exit");
	camerasp::buffer_t data = response.get();
	console->info("stop executed");
	fg_state = stop_pending;
      }
      else if (fg_state == stop_pending)
      {
	success= "Stop Pending. try again later";
      }
      else
      {
	success="Not running when command received";
      }
      *http_response <<  "HTTP/1.1 200 OK\r\n" 
	<<  "Content-Length: " << success.size()<< "\r\n"
	<<  "Content-type: " << "application/text" <<"\r\n"
	<< "\r\n"
	<< success;
    };
    //default page server
    server.default_resource["GET"]=[&](
	std::shared_ptr<HttpServer::Response> http_response,
	std::shared_ptr<HttpServer::Request> http_request) {
      try {
	auto web_root_path = 
	  boost::filesystem::canonical(home_page);
	auto path=boost::filesystem::canonical(web_root_path/http_request->path);
	//Check if path is within web_root_path
	if(std::distance(web_root_path.begin(),web_root_path.end()) >
	    std::distance(path.begin(), path.end()) ||
	    !std::equal(web_root_path.begin(), web_root_path.end(), path.begin()))
	  throw std::invalid_argument("path must be within root path");
	if(boost::filesystem::is_directory(path))
	  path/="index.html";
	if(!(boost::filesystem::exists(path) &&
	      boost::filesystem::is_regular_file(path)))
	  throw std::invalid_argument("file does not exist");

	std::string cache_control, etag;

	// Uncomment the following line to enable Cache-Control
	// cache_control="Cache-Control: max-age=86400\r\n";

	auto ifs=std::make_shared<std::ifstream>();
	ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);

	if(*ifs) {
	  auto length=ifs->tellg();
	  ifs->seekg(0, std::ios::beg);

	  *http_response << "HTTP/1.1 200 OK\r\n" << cache_control << etag 
	    << "Content-Length: " << length << "\r\n\r\n";
	  default_resource_send(server, http_response, ifs);
	}
	else
	  throw std::invalid_argument("could not read file");
      }
      catch(const std::exception &e) {
	std::string content="Could not open path "+http_request->path+": "+e.what();
	*http_response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " 
	  << content.length() 
	  << "\r\n\r\n" 
	  << content;
      }
    };

    //ignore SIGCHLD notification - otherwise zombie processes linger
    //signal(SIGCHLD,SIG_IGN);

    // The signal set is used to register termination notifications
    asio::signal_set signals_(*io_service);
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    signals_.add(SIGCHLD);

    std::function<void(ASIO_ERROR_CODE,int)> signal_handler = [&] (ASIO_ERROR_CODE const& error, int signal_number)
    { 
      if(signal_number == SIGCHLD)
      {
	fg_state = stopped;
	console->debug("Process stopped");
        waitpid(child_pid,NULL,0);
	signals_.async_wait(signal_handler);
      } else {
	console->debug("SIGTERM received");
	server.stop();
      }
    };
    // register the handle_stop callback

    signals_.async_wait(signal_handler);
    server.start();
    if(started){
      request.set("exit");
      camerasp::buffer_t data = response.get();
    }
  }
  catch (std::exception& e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}

void default_resource_send(const HttpServer &server,
                           const std::shared_ptr<HttpServer::Response> &response,
                           const std::shared_ptr<std::ifstream> &ifs) {
    //read and send 128 KB at a time
    static std::vector<char> buffer(131072); // Safe when server is running on one thread
    std::streamsize read_length;
    if((read_length=ifs->read(&buffer[0], buffer.size()).gcount())>0) {
        response->write(&buffer[0], read_length);
        if(read_length==static_cast<std::streamsize>(buffer.size())) {
            server.send(
                response, 
                    [&server, response, ifs]
                (const std::error_code &ec) {
                if(!ec)
                    default_resource_send(server, response, ifs);
                else
                    console->warn("Connection interrupted");
            });
        }
    }
}


