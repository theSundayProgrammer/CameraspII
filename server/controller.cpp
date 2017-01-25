
////////////////////////////////////////////////////////////////////////////// // Copyright (c) Joseph Mariadassou
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
#include <fstream>
#include <camerasp/utils.hpp>
#include <camerasp/ipc.hpp>
#include <boost/filesystem.hpp>
#include <sys/wait.h>
std::shared_ptr<spdlog::logger> console;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
char const *home_page="/home/chakra/data/web";
#else
const std::string config_path = "/srv/camerasp/";
char const *home_page="/home/pi/data/web";
#endif
//ToDo: set executable file name in json config
char const *cmd= "/home/pi/bin/camerasp";
char const *log_folder="/tmp/frame_grabber.log";
#define ASIO_ERROR_CODE asio::error_code
typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;
void default_resource_send(const HttpServer &server,
                           const std::shared_ptr<HttpServer::Response> &response,
                           const std::shared_ptr<std::ifstream> &ifs) ;
enum class process_state{  started, stop_pending, stopped};

/*****************************************
State Transition Table
----------------------------------------
State| Command| Result State
----------------------------------------
started| stop | stop_pending 
started| start| started
stopped| stop | stopped
stopped| start| started
stop_pending | stop | stop_pending
stop_pending | start| stop_pending
******************************************/

namespace ipc=boost::interprocess;

class web_server
{
  public:
    web_server(Json::Value& root)
      :response( RESPONSE_MEMORY_NAME)
       ,request( REQUEST_MEMORY_NAME)
       ,root_(root)
  {

    //Child process
    //Important: the working directory of the child process
    // is the same as that of the parent process.
    int ret;
    if (ret = posix_spawn_file_actions_init (&child_fd_actions))
      console->error("posix_spawn_file_actions_init"), exit(ret);
    if (ret = posix_spawn_file_actions_addopen (
	  &child_fd_actions, 1,log_folder , 
	  O_WRONLY | O_CREAT | O_TRUNC, 0644))
      console->error("posix_spawn_file_actions_addopen"), exit(ret);
    if (ret = posix_spawn_file_actions_adddup2 (&child_fd_actions, 1, 2))
      console->error("posix_spawn_file_actions_adddup2"), exit(ret);

    // server config
    unsigned port_number=8088;
    auto server_config = root["Server"];
    if (!server_config.empty())
      port_number = server_config["port"].asInt();
    server.config.port=port_number;
    server.config.thread_pool_size=2;
  }
    void run(int argc, char *argv[], char* env[])
    {
      using namespace camerasp;

      if (int ret = posix_spawnp (&child_pid, cmd, &child_fd_actions, 
	    NULL, argv, env))
	console->error("posix_spawn"), exit(ret);
      console->info("Child pid: {0}\n", child_pid);
      process_state fg_state = process_state::started;
      // 
      auto io_service=std::make_shared<asio::io_service>();
      server.io_service = io_service;
      // send message 
      auto send_failure = [&] (
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::string const& message)
      {
	*http_response << "HTTP/1.1 400 Bad Request\r\n"
	  <<  "Content-Length: " << message.size()<< "\r\n"
	  <<  "Content-type: " << "application/text" <<"\r\n"
	  << "\r\n"
	  << message;
      };
      auto send_success = [&] (
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::string const& message)
      {
	*http_response <<  "HTTP/1.1 200 OK\r\n" 
	  <<  "Content-Length: " << message.size()<< "\r\n"
	  <<  "Content-type: " << "application/text" <<"\r\n"
	  << "\r\n"
	  << message;
      };
      // get image 
      auto exec_cmd =[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)

      {
	if(fg_state == process_state::started )
	{
	std::string str =http_request->path_match[0];
	request->set(str);
	camerasp::buffer_t data = response->get();
	*http_response <<  "HTTP/1.1 200 OK\r\n" 
	  <<  "Content-Length: " << data.size()<< "\r\n"
	  <<  "Content-type: " << "application/text" <<"\r\n"
	  << "\r\n"
	  << data;
	}
	else
	{
	  std::string success("Frame Grabber not running. Issue start command");
	  send_failure(http_response,success);
	}
      };
      auto get_image =[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)

      {
	if(fg_state == process_state::started )
	{
	  std::string str =http_request->path_match[0];
	  request->set(str);
	  camerasp::buffer_t data = response->get();
	  *http_response <<  "HTTP/1.1 200 OK\r\n" 
	    <<  "Content-Length: " << data.size()<< "\r\n"
	    <<  "Content-type: " << "image/jpeg" <<"\r\n"
	    << "\r\n"
	    << data;
	}
	else
	{
	  std::string success("Frame Grabber not running. Issue start command");
	  send_failure(http_response,success);
	}
      };

      // get previous image
      server.resource["^/image\\?prev=([0-9]+)$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	get_image(http_response,http_request);
      };

      // flip horizontal vertical
      server.resource["^/flip\\?horizontal=(0|1)$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	exec_cmd(http_response,http_request);
      };
      server.resource["^/resume$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	exec_cmd(http_response,http_request);
      };
      // pause/resume
      server.resource["^/pause$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	exec_cmd(http_response,http_request);
      };
      server.resource["^/resume$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	exec_cmd(http_response,http_request);
      };
      // get current image
      server.resource["^/image$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	get_image(http_response,http_request);
      };

      //restart capture
      server.resource["^/start$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	std::string success("Succeeded");
	if(fg_state == process_state::started )
	{
	  success="Already Running";
	  send_failure(http_response,success);
	}
	else if (fg_state == process_state::stop_pending)
	{
	  success= "Stop Pending. try again later";
	  send_failure(http_response,success);
	}
	else
	{
	  if (int ret = posix_spawnp (&child_pid, cmd, 
		&child_fd_actions, NULL, argv, env))
	    console->error("posix_spawn"), exit(ret);
	  fg_state = process_state::started;
	  console->info("Child pid: {0}\n", child_pid);
	  send_success(http_response,success);
	}
      };

      //stop capture
      server.resource["^/stop$"]["GET"]=[&](
	  std::shared_ptr<HttpServer::Response> http_response,
	  std::shared_ptr<HttpServer::Request> http_request)
      {
	std::string success("Succeeded");
	//
	if(fg_state == process_state::started )
	{
	  request->set("exit");
	  camerasp::buffer_t data = response->get();
	  console->info("stop executed");
	  fg_state = process_state::stop_pending;
	  send_success(http_response,success);
	}
	else if (fg_state == process_state::stop_pending)
	{
	  success= "Stop Pending. try again later";
	  send_failure(http_response,success);
	}
	else
	{
	  success="Not running when command received";
	  send_failure(http_response,success);
	}
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
	  *http_response << "HTTP/1.1 400 Bad Request\r\n"
	    << "Content-Length: " << content.length() << "\r\n"
	    << "\r\n" 
	    << content;
	}
      };


      // The signal set is used to register termination notifications
      asio::signal_set signals_(*io_service);
      signals_.add(SIGINT);
      signals_.add(SIGTERM);
      signals_.add(SIGCHLD);

      std::function<void(ASIO_ERROR_CODE,int)> signal_handler = [&] (ASIO_ERROR_CODE const& error, int signal_number)
      { 
	if(signal_number == SIGCHLD)
	{
	  fg_state = process_state::stopped;
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


      //start() returns on SIGTERM
      server.start();
      if(fg_state == process_state::started )
      {
	request->set("exit");
	camerasp::buffer_t data = response->get();
      }
    }
  private:

    shared_mem_ptr<shared_response_data> response;
    shared_mem_ptr<shared_request_data> request;
    pid_t child_pid;
    posix_spawn_file_actions_t child_fd_actions;
    Json::Value& root_;
    HttpServer server;
};

int main(int argc, char *argv[], char* env[])
{
  try
  {

    Json::Value root = camerasp::get_DOM(config_path + "options.json");
    //configure console

    auto log_config = root["Logging"];
    auto json_path = log_config["path"];
    auto logpath = json_path.asString();
    auto size_mega_bytes = log_config["size"].asInt();
    auto count_files = log_config["count"].asInt();
    //console = spd::rotating_logger_mt("console", logpath, 1024 * 1024 * size_mega_bytes, count_files);
    console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);
    console->debug("Starting");

    //run web server
    web_server server(root);
    server.run(argc,argv,env);

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
    std::streamsize read_length=ifs->read(&buffer[0], buffer.size()).gcount();
    if(read_length>0) {
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


