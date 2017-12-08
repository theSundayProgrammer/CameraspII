
/*
Copyright (c) 2017 Joseph Maraidassou (theSundayProgrammer.com)
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <camerasp/server_http.hpp>
#include <json/reader.h>
#include <json/writer.h>
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
#include <sstream>
#include <gsl/gsl_util>
std::shared_ptr<spdlog::logger> console;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
char const *home_page="/home/chakra/data/web";
char const *cmd= "./camerasp";
#else
const std::string config_path = "/home/pi/data";
char const *home_page="/home/pi/data/web";
char const *cmd= "/home/pi/bin/camerasp";
#endif

#include <gsl/gsl_util>
#include <iterator>
#include <raspi/RaspiCamControl.h>
#include <raspi/RaspiPreview.h>
#include <raspi/RaspiStill.h>
#include <raspi/RaspiCLI.h>
#include <semaphore.h>
#include <camerasp/ipc.hpp>

#define ASIO_ERROR_CODE asio::error_code


std::string create_file_name(   char * pattern, int frame)
{
  char finalName[256] ;
   if (snprintf(finalName,256, pattern, frame) > 0 )
   {
   return std::string(finalName);
   }
      return  std::string();
}

void dump_status(shared_state& state);
typedef SimpleWeb::Server<SimpleWeb::HTTP> http_server;
void default_resource_send(const http_server &server,
                           const std::shared_ptr<http_server::Response> &response,
                           const std::shared_ptr<std::ifstream> &ifs);
enum class process_state{  started, stop_pending, stopped};
class web_server
{
  public:
    web_server(Json::Value& root,shared_state& state_)
      :root_(root)
       ,state(state_)
  {


    // server config
    unsigned port_number=8088;
    auto server_config = root["Server"];
    if (!server_config.empty())
    {
      port_number = server_config["port"].asInt();
    }
    server.config.port=port_number;
    server.config.thread_pool_size=2;
  }
    auto send_failure (
        std::shared_ptr<http_server::Response> http_response,
        std::string const& message)
    {
      *http_response << "HTTP/1.1 400 Bad Request\r\n"
        <<  "Content-Length: " << message.size()<< "\r\n"
        <<  "Content-type: " << "application/text" <<"\r\n"
        << "\r\n"
        << message;
    }
    auto send_success (
        std::shared_ptr<http_server::Response> http_response,
        std::string const& message)
    {
      *http_response <<  "HTTP/1.1 200 OK\r\n" 
        <<  "Content-Length: " << message.size()<< "\r\n"
        <<  "Content-type: " << "application/text" <<"\r\n"
        << "\r\n"
        << message;
    }


    auto send_image (
        std::shared_ptr<http_server::Response> http_response,
        int n)

    {
      try
      {
        auto fName  = create_file_name(state.filename,n);
        std::ostringstream ostr;

        fprintf(stderr,"testing send%s\n", fName.c_str());
        ostr<< std::ifstream(fName.c_str()).rdbuf();
        *http_response <<  "HTTP/1.1 200 OK\r\n" 
          <<  "Content-Length: " << ostr.str().size()<< "\r\n"
          <<  "Content-type: " << "image/jpeg" <<"\r\n"
          << "Cache-Control: no-cache, must-revalidate" << "\r\n"
          << "\r\n"
          << ostr.str()
          ;
      }
      catch(std::runtime_error& e)
      {

        std::string success("Issue with reading imaghe file");
        send_failure(http_response,success);
      }
    }




    void do_fallback(
        std::shared_ptr<http_server::Response> http_response,
        std::shared_ptr<http_server::Request> http_request)
    {
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
    }
    void run(std::shared_ptr<asio::io_context> io_service, char *argv[], char* env[])
    {
      using namespace camerasp;

      // get previous image
      server.resource[R"(^/image\?prev=([0-9]+)$)"]["GET"]=[&](
          std::shared_ptr<http_server::Response> http_response,
          std::shared_ptr<http_server::Request> http_request)
      {
        std::string number=  http_request->path_match[1];
        int n = atoi(number.c_str());
        fprintf(stderr, "Str %s=%d\n",number.c_str(),n); 
        fprintf(stderr, "Count=%d, max_frames=%d",state.frame,state.max_frames);
        send_image(http_response,(state.frame-n)%state.max_frames);
      };

      // get current image
      server.resource["^/image"]["GET"]=[&](
          std::shared_ptr<http_server::Response> http_response,
          std::shared_ptr<http_server::Request> http_request)
      {
        send_image(http_response,1);
      };

      ;
      //default page server
      server.default_resource["GET"]=[&](
          std::shared_ptr<http_server::Response> http_response,
          std::shared_ptr<http_server::Request> http_request) 
      {
        do_fallback( http_response, http_request);
      } ;


      // 
      server.io_service = io_service;

      //start() returns on SIGTERM
      server.start();
    }
  private:

    process_state fg_state = process_state::stopped;
    pid_t child_pid;
    posix_spawn_file_actions_t child_fd_actions;
    Json::Value& root_;
    shared_state& state;
    http_server server;
};
int mmal_status_to_int(MMAL_STATUS_T status);

class Mmal_exception {
  public:
    Mmal_exception(MMAL_STATUS_T status_):status(status_){}
    MMAL_STATUS_T status;
}; 
class Error_handler
{
  public:
    Error_handler(const char* str_, bool throw_exception_):
      throw_exception(throw_exception_),
      str(str_){}
    Error_handler& operator=(MMAL_STATUS_T status)
    {
      if (status != MMAL_SUCCESS)
      {
	vcos_log_error(str);
	if(throw_exception)
	  throw Mmal_exception(status);
      }
      return *this;
    }
  private:
    bool throw_exception;
    const char* str;
};
Error_handler Error(const char* str,bool throw_exception=true)
{
  return Error_handler(str,throw_exception);
}


/**
 * Assign a default set of parameters to the state passed in
 *
 * @param state Pointer to state structure to assign defaults to
 */
static void default_status(shared_state& state)
{

   state.quality = 85;
   state.keep_running = 1;
   state.wantRAW = 0;
   state.frame= 0;
   state.verbose = 0;
   state.encoding = MMAL_ENCODING_JPEG;
   state.enableExifTags = 0;
   state.timelapse = 0;
   state.settings = 0;
   state.cameraNum = 0;
   state.sensor_mode = 0;
   state.restart_interval = 0;

   // Default to the OV5647 setup
   state.width = 2592;
   state.height = 1944;
    strncpy(state.camera_name, "OV5647", CAMERA_NAME_LENGTH);
   // Set up the camera_parameters to default
   RaspiCamControl raspicamcontrol;
   raspicamcontrol.set_defaults(&state.camera_parameters);



}

/**
 * Dump image state parameters to stderr. Used for debugging
 *
 * @param state Pointer to state structure to assign defaults to
 */
void dump_status(shared_state& state)
{


  fprintf(stderr, "Width %d, Height %d, quality %d, filename %s\n", state.width,
      state.height, state.quality, state.filename);
  fprintf(stderr, " Raw %s\n", 
      state.wantRAW ? "yes" : "no");
  fprintf(stderr, "Link to latest frame enabled ");
  fprintf(stderr, " no\n");


  //raspicamcontrol_dump_parameters(&state.camera_parameters);
}
pid_t spawn_reader(int argc, char *argv[],  char *env[])
{
  posix_spawn_file_actions_t child_fd_actions;
  int ret;
  char const *log_folder="./raspi_still.log";
  if ((ret = posix_spawn_file_actions_init (&child_fd_actions)))
    fprintf(stderr,"posix_spawn_file_actions_init"), exit(ret);
  if ((ret = posix_spawn_file_actions_addopen (
	  &child_fd_actions, 1,log_folder , 
	  O_WRONLY | O_CREAT | O_TRUNC, 0644)))
    fprintf(stderr,"posix_spawn_file_actions_addopen"), exit(ret);
  if ((ret = posix_spawn_file_actions_adddup2 (&child_fd_actions, 1, 2)))
    fprintf(stderr,"posix_spawn_file_actions_adddup2"), exit(ret);
  pid_t child_pid;
  if ((ret = posix_spawnp (&child_pid, "./raspicam.bin", &child_fd_actions, 
	  NULL, argv, env)))
    fprintf(stderr,"posix_spawn"), exit(ret);
//////////////////////////////////////////////////
  return child_pid;
}
const char* shared_mem_name = "RaspiStillSharedMem2";
int main(int argc, char *argv[],  char *env[])
{
  shared_mem_ptr<shared_state> mem(shared_mem_name,true);
  shared_state& state = *mem;
  auto io_service=std::make_shared<asio::io_context>();
  asio::signal_set signals_(*io_service);
  // The signal set is used to register termination notifications
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
  signals_.add(SIGCHLD);
  pid_t child_pid; 
  // signal handler for SIGCHLD must be set before the process is forked
  // In a service 'spawn' forks a child process
  std::function<void(ASIO_ERROR_CODE,int)> signal_handler = 
    [&] (ASIO_ERROR_CODE const& error, int signal_number)
    { 
      if(signal_number == SIGCHLD)
      {
        int ret;
        dump_status(state);
        waitpid(child_pid,&ret,0);
      } else {
        state.keep_running=0;
        signals_.async_wait(signal_handler);
      }
    };

  signals_.async_wait(signal_handler);

  Json::Value root = camerasp::get_DOM(config_path + "/options.json");
  {
    auto server_config = root["Server"];
    if (!server_config.empty())
    {
      std::string pwd = server_config["password"].asString();
      strcpy(state.password_smtp,pwd.c_str());
    }
  }
  child_pid =  spawn_reader(argc,argv,env);
  fprintf(stderr,"Child pid: %d\n", child_pid);
  web_server server(root,state);
  server.run(io_service,argv,env);
  fprintf(stderr,"Child pid: %d stopped\n", child_pid);
  dump_status(state);
  return 0; 
}





void default_resource_send(const http_server &server,
                           const std::shared_ptr<http_server::Response> &response,
                           const std::shared_ptr<std::ifstream> &ifs)
{
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
