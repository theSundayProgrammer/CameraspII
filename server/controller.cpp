
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
std::shared_ptr<spdlog::logger> console;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
#else
const std::string config_path = "/srv/camerasp/";
#endif
//ToDo: set executable file name in json config
char const *cmd= "./camerasp";

#define ASIO_ERROR_CODE asio::error_code
typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;
void default_resource_send(const HttpServer &server,
                           const std::shared_ptr<HttpServer::Response> &response,
                           const std::shared_ptr<std::ifstream> &ifs) ;
int main(int argc, char *argv[], char* env[])
{
  namespace ipc=boost::interprocess;
  using namespace camerasp;
  try
  {
    HttpServer server;
    unsigned port_number=8088;
    auto root = camerasp::get_DOM(config_path + "options.json");
    auto server_config = root["Server"];
    if (!server_config.empty())
      port_number = server_config["port"].asInt();
    server.config.port=port_number;
    server.config.thread_pool_size=2;

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

    //Important: the working directory of the child process is the same as the child process.
    int ret;
    pid_t child_pid;
    posix_spawn_file_actions_t child_fd_actions;
    if (ret = posix_spawn_file_actions_init (&child_fd_actions))
      console->error("posix_spawn_file_actions_init"), exit(ret);
    if (ret = posix_spawn_file_actions_addopen (
	  &child_fd_actions, 1, "/tmp/foo-log", 
	  O_WRONLY | O_CREAT | O_TRUNC, 0644))
      console->error("posix_spawn_file_actions_addopen"), exit(ret);
    if (ret = posix_spawn_file_actions_adddup2 (&child_fd_actions, 1, 2))
      console->error("posix_spawn_file_actions_adddup2"), exit(ret);

    std::string str;

    if (ret = posix_spawnp (&child_pid, cmd, &child_fd_actions, 
	  NULL, argv, env))
      console->error("posix_spawn"), exit(ret);
    bool started =true;
    console->info("Child pid: {0}\n", child_pid);

    auto io_service=std::make_shared<asio::io_service>();
    server.io_service = io_service;
    server.resource["^/image\\?prev=([0-9]+)$"]["GET"]=[&](
	std::shared_ptr<HttpServer::Response> http_response,
	std::shared_ptr<HttpServer::Request> http_request)
    {
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
      if(!started )
      {
	started=true;
	if (ret = posix_spawnp (&child_pid, cmd, 
	      &child_fd_actions, NULL, argv, env))
	  console->error("posix_spawn"), exit(ret);
	console->info("Child pid: {0}\n", child_pid);
      }
      std::string success("Succeeded");
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
      //
      if(started){
	started=false;
	request.set("exit");
	camerasp::buffer_t data = response.get();
	console->info("{0} executed",str);
	std::string success("Succeeded");
	*http_response <<  "HTTP/1.1 200 OK\r\n" 
	  <<  "Content-Length: " << success.size()<< "\r\n"
	  <<  "Content-type: " << "application/text" <<"\r\n"
	  << "\r\n"
	  << success;
      }
    };
    server.default_resource["GET"]=[&](
	std::shared_ptr<HttpServer::Response> http_response,
	std::shared_ptr<HttpServer::Request> http_request) {
      try {
	auto web_root_path = 
	  boost::filesystem::canonical("/home/pi/data/web");
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
    signal(SIGCHLD,SIG_IGN);

    // The signal set is used to register termination notifications
    asio::signal_set signals_(*io_service);
    signals_.add(SIGINT);
    signals_.add(SIGTERM);

    // register the handle_stop callback
    signals_.async_wait([&]
	(ASIO_ERROR_CODE const& error, int signal_number) { 
	console->debug("SIGTERM received");
	server.stop();
	});

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


