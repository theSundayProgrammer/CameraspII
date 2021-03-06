////////////////////////////////////////////////////////////////////////////// 
// Copyright 2016-2018 (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
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
#include <camerasp/msgq.hpp>
#include <boost/filesystem.hpp>
#include <sys/wait.h>
#include <sstream>
#include <gsl/gsl_util>
std::shared_ptr<spdlog::logger> console;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
char const *home_page = "/home/chakra/data/web";
char const *cmd = "./camerasp";
#else
const std::string config_path = "/srv/camerasp/";
char const *home_page = "/home/pi/data/web";
char const *cmd = "/home/pi/bin/camerasp";
#endif

using asio::ip::udp;
class address_broadcasting_server
{
public:
  address_broadcasting_server(asio::io_context &io_context, unsigned short port)
      : socket_(io_context, udp::endpoint(udp::v4(), port))
  {
  }

  void receive()
  {
    socket_.async_receive_from(
        asio::buffer(data_, max_length), sender_endpoint_,
        [this](std::error_code ec, std::size_t bytes_recvd) {
          if (!ec && bytes_recvd > 0 &&
              send_message == std::string(data_, bytes_recvd))
          {
            send(bytes_recvd);
          }
          else
          {
            receive();
          }
        });
  }

  void send(std::size_t length)
  {
    socket_.async_send_to(
        asio::buffer(recv_message), sender_endpoint_,
        [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {
          receive();
        });
  }

private:
  udp::socket socket_;
  udp::endpoint sender_endpoint_;
  enum
  {
    max_length = 1024
  };
  char data_[max_length];
  const std::string send_message{"12068c99-18de-48e1-87b4-3e09bbbd8b15-Camerasp"};
  const std::string recv_message{"ee7f7fc7-9d54-480b-868d-fde1f5a67ab6-Camerasp"};
};

//ToDo: set executable file name in json config
char const *log_folder = "/tmp/frame_grabber.log";
#define ASIO_ERROR_CODE asio::error_code
typedef SimpleWeb::Server<SimpleWeb::HTTP> http_server;
using sb = SimpleWeb::ServerBase<SimpleWeb::HTTP>;
void default_resource_send(const http_server &server,
                           const sb::response_ptr &response,
                           const std::shared_ptr<std::ifstream> &ifs);
enum class process_state
{
  started,
  stop_pending,
  stopped
};

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

namespace ipc = boost::interprocess;
using namespace camerasp;
using std::string;
auto send_success(
    sb::response_ptr http_response,
    std::string const &message)
{
  *http_response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Length: " << message.size() << "\r\n"
                 << "Content-type: "
                 << "application/text"
                 << "\r\n"
                 << "\r\n"
                 << message;
}

auto send_failure(
    sb::response_ptr http_response,
    std::string const &message)
{
  *http_response << "HTTP/1.1 400 Bad Request\r\n"
                 << "Content-Length: " << message.size() << "\r\n"
                 << "Content-type: "
                 << "application/text"
                 << "\r\n"
                 << "\r\n"
                 << message;
}
class web_server
{
public:
  web_server(Json::Value &root_,
             server_msg_queues &queue_)
      : queue(queue_),
        root(root_)
  {

    // server config
    unsigned port_number = 8088;
    auto server_config = root["Server"];
    if (!server_config.empty())
      port_number = server_config["port"].asInt();
    server.config.port = port_number;
    server.config.thread_pool_size = 2;
  }

  void exec_cmd(
      sb::response_ptr http_response,
      std::shared_ptr<http_server::Request> http_request)

  {
    if (fg_state == process_state::started)

    {
      std::string str = http_request->path_match[0];

      queue.send_message(str, [http_response](string const &data, int error) {
        *http_response
            << "HTTP/1.1 200 OK\r\n"
            << "Content-Length: " << data.size() << "\r\n"
            << "Content-type: "
            << "application/text"
            << "\r\n"
            << "Cache-Control: no-cache, must-revalidate"
            << "\r\n"
            << "\r\n"
            << data;
      });
    }
    else
    {
      std::string success("Frame Grabber not running. Issue start command");
      send_failure(http_response, success);
    }
  }
  auto send_image(
      sb::response_ptr http_response,
      std::shared_ptr<http_server::Request> http_request)

  {
    if (fg_state == process_state::started)
    {
      std::string str = http_request->path_match[0];
      queue.send_message(str, [http_response](std::string const &data,int err) {
        

        if (err ==0)
        {
          *http_response << "HTTP/1.1 200 OK\r\n"
                         << "Content-Length: " << data.size() << "\r\n"
                         << "Content-type: "
                         << "image/jpeg"
                         << "\r\n"
                         << "Cache-Control: no-cache, must-revalidate"
                         << "\r\n"
                         << "\r\n"
                         << std::string(data.begin() , data.end());

          console->info("sending image");
        }
        else
        {
          console->error("Not sending image");
          send_failure(http_response, "No image Available");
        }
      });
    }
    else
    {
      std::string success("Frame Grabber not running. Issue start command");
      send_failure(http_response, success);
    }
  }
  void set_props(
      sb::response_ptr http_response,
      std::shared_ptr<http_server::Request> http_request)
  {
    try
    {

      std::ostringstream ostr;
      ostr << http_request->content.rdbuf();
      std::string istr = ostr.str();
      auto camera = root["Camera"];
      std::regex pat("(\\w+)=(\\d+)");
      for (std::sregex_iterator p(std::begin(istr), std::end(istr), pat);
           p != std::sregex_iterator{};
           ++p)
      {
        camera[(*p)[1].str()] = atoi((*p)[2].str().c_str());
      }

      Json::StyledWriter writer;
      root["Camera"] = camera;
      auto update = writer.write(root);
      camerasp::write_file_content(config_path + "options.json", update);
      std::string data{"Config file updated. Restart camera"};
      *http_response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Length: " << data.size() << "\r\n"
                     << "Content-type: "
                     << "application/text"
                     << "\r\n"
                     << "Cache-Control: no-cache, must-revalidate"
                     << "\r\n"
                     << "\r\n"
                     << data;
    }
    catch (std::runtime_error &e)
    {
      std::string success("Frame Grabber not running. Issue start command");
      send_failure(http_response, success);
    }
  }
  void stop_camera(
      sb::response_ptr http_response,
      std::shared_ptr<http_server::Request> http_request)
  {
    std::string success("Succeeded");
    //
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    sigaction(SIGCHLD, nullptr, &act);
    console->debug("Mask= {0},{1},{2},{3}", act.sa_mask.__val[0], act.sa_mask.__val[1], act.sa_mask.__val[2], act.sa_mask.__val[3]);
    if (fg_state == process_state::started)
      try
      {
        queue.send_message("exit", [](string const& str, int err) {
        console->info("stop executed");
        });
        int status = 0;
        unsigned n = 0;
        const int max_wait_count=200;
        while (n < max_wait_count && 0 <= waitpid(child_pid, &status, WNOHANG))
        {
          usleep(100 * 1000);
          ++n;
        }
        if (n == max_wait_count)
          kill(child_pid, SIGKILL);
        fg_state = process_state::stopped;
        send_success(http_response, success);
      }
      catch (std::exception &e)
      {
        success = "stop failed - abortng";
        console->info(success);
        kill(child_pid, SIGKILL);
        fg_state = process_state::stop_pending;
        send_success(http_response, success);
      }
    else if (fg_state == process_state::stop_pending)
    {
      success = "Stop Pending. try again later";
      send_failure(http_response, success);
    }
    else
    {
      success = "Not running when command received";
      send_failure(http_response, success);
    }
  }

  void do_fallback(
      sb::response_ptr http_response,
      std::shared_ptr<http_server::Request> http_request)
  {
    try
    {
      auto web_root_path =
          boost::filesystem::canonical(home_page);
      auto path = boost::filesystem::canonical(web_root_path / http_request->path);
      //Check if path is within web_root_path
      if (std::distance(web_root_path.begin(), web_root_path.end()) >
              std::distance(path.begin(), path.end()) ||
          !std::equal(web_root_path.begin(), web_root_path.end(), path.begin()))
        throw std::invalid_argument("path must be within root path");
      if (boost::filesystem::is_directory(path))
        path /= "index.html";
      if (!(boost::filesystem::exists(path) &&
            boost::filesystem::is_regular_file(path)))
        throw std::invalid_argument("file does not exist");

      std::string cache_control, etag;

      // Uncomment the following line to enable Cache-Control
      // cache_control="Cache-Control: max-age=86400\r\n";

      auto ifs = std::make_shared<std::ifstream>();
      ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);

      if (*ifs)
      {
        auto length = ifs->tellg();
        ifs->seekg(0, std::ios::beg);

        *http_response << "HTTP/1.1 200 OK\r\n"
                       << cache_control << etag
                       << "Content-Length: " << length << "\r\n\r\n";
        default_resource_send(server, http_response, ifs);
      }
      else
        throw std::invalid_argument("could not read file");
    }
    catch (const std::exception &e)
    {
      std::string content = "Could not open path " + http_request->path + ": " + e.what();
      *http_response << "HTTP/1.1 400 Bad Request\r\n"
                     << "Content-Length: " << content.length() << "\r\n"
                     << "\r\n"
                     << content;
    }
  }
  void flip(sb::response_ptr http_response,
            std::shared_ptr<http_server::Request> http_request)
  {
    try
    {
      camerasp::buffer_t data("Failed");
      std::ostringstream ostr;
      ostr << http_request->content.rdbuf();
      std::string istr = ostr.str();
      auto camera = root["Camera"];
      std::regex pat("(\\w+)=(0|1)");
      for (std::sregex_iterator p(std::begin(istr), std::end(istr), pat);
           p != std::sregex_iterator{};
           ++p)
      {
        camera[(*p)[1].str()] = atoi((*p)[2].str().c_str());
        Json::StyledWriter writer;
        root["Camera"] = camera;
        auto update = writer.write(root);
        camerasp::write_file_content(config_path + "options.json", update);
        if (fg_state == process_state::started)
        {
          queue.send_message(std::string("/flip?") + (*p)[0].str().c_str(),
                             [this, http_response](std::string const &str, int error) {
                               *http_response << "HTTP/1.1 200 OK\r\n"
                                              << "Content-Length: " << str.size() << "\r\n"
                                              << "Content-type: "
                                              << "application/text"
                                              << "\r\n"
                                              << "Cache-Control: no-cache, must-revalidate"
                                              << "\r\n"
                                              << "\r\n"
                                              << str
                                              << "\r\n";
                             });
        }
      }
    }
    catch (std::runtime_error &e)
    {

      std::string success("Frame Grabber not running. Issue start command");
      send_failure(http_response, success);
    }
  }
  void run(std::shared_ptr<asio::io_context> io_service, char *argv[], char *env[]);

private:
  process_state fg_state = process_state::stopped;
  server_msg_queues &queue;
  pid_t child_pid;
  posix_spawn_file_actions_t child_fd_actions;
  Json::Value &root;
  http_server server;
};

void default_resource_send(const http_server &server,
                           const sb::response_ptr &response,
                           const std::shared_ptr<std::ifstream> &ifs)
{
  //read and send 128 KB at a time
  static std::vector<char> buffer(131072); // Safe when server is running on one thread
  std::streamsize read_length = ifs->read(&buffer[0], buffer.size()).gcount();
  if (read_length > 0)
  {
    response->write(&buffer[0], read_length);
    if (read_length == static_cast<std::streamsize>(buffer.size()))
    {
      server.send(
          response,
          [&server, response, ifs](const std::error_code &ec) {
            if (!ec)
              default_resource_send(server, response, ifs);
            else
              console->warn("Connection interrupted");
          });
    }
  }
}
void web_server::run(std::shared_ptr<asio::io_context> io_service, char *argv[], char *env[])
{
  using namespace camerasp;
  server.io_service = io_service;
  //Child process
  //Important: the working directory of the child process
  // is the same as that of the parent process.
  int ret;
  if (ret = posix_spawn_file_actions_init(&child_fd_actions))
    console->error("posix_spawn_file_actions_init"), exit(ret);
  if (ret = posix_spawn_file_actions_addopen(
          &child_fd_actions, 1, log_folder,
          O_WRONLY | O_CREAT | O_TRUNC, 0644))
    console->error("posix_spawn_file_actions_addopen"), exit(ret);
  if (ret = posix_spawn_file_actions_adddup2(&child_fd_actions, 1, 2))
    console->error("posix_spawn_file_actions_adddup2"), exit(ret);
  //

  // The signal set is used to register termination notifications
  asio::signal_set signals_(*io_service);
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
  signals_.add(SIGCHLD);

  // signal handler for SIGCHLD must be set before the process is forked
  // In a service 'spawn' forks a child process
  std::function<void(ASIO_ERROR_CODE, int)> signal_handler =
      [&](ASIO_ERROR_CODE const &error, int signal_number) {
        if (signal_number == SIGCHLD)
        {
          fg_state = process_state::stopped;
          console->debug("Process stopped");
          waitpid(child_pid, NULL, 0);
          signals_.async_wait(signal_handler);
        }
        else
        {
          console->debug("SIGTERM received");
          server.stop();
        }
      };

  if (int ret = posix_spawnp(&child_pid, cmd, &child_fd_actions,
                             NULL, argv, env))
    console->error("posix_spawn"), exit(ret);
  console->info("Child pid: {0}\n", child_pid);
  fg_state = process_state::started;

  // get previous image
  server.resource[R"(^/image\?prev=([0-9]+)$)"]["GET"] = [&](
                                                             sb::response_ptr http_response,
                                                             std::shared_ptr<http_server::Request> http_request) {
    send_image(http_response, http_request);
  };

  // replace camera properties
  server.resource["^/camera"]["PUT"] = [&](
                                           sb::response_ptr http_response,
                                           std::shared_ptr<http_server::Request> http_request) {
    set_props(http_response, http_request);
  };
  // flip horizontal vertical
  server.resource["^/flip"]["PUT"] = [&]( //\\?horizontal=(0|1)$
                                         sb::response_ptr http_response,
                                         std::shared_ptr<http_server::Request> http_request) {
    flip(http_response, http_request);

  };
  server.resource["^/resume$"]["GET"] = [&](
                                            sb::response_ptr http_response,
                                            std::shared_ptr<http_server::Request> http_request) {
    exec_cmd(http_response, http_request);
  };
  // pause/resume
  server.resource["^/pause$"]["GET"] = [&](
                                           sb::response_ptr http_response,
                                           std::shared_ptr<http_server::Request> http_request) {
    exec_cmd(http_response, http_request);
  };
  server.resource["^/resume$"]["GET"] = [&](
                                            sb::response_ptr http_response,
                                            std::shared_ptr<http_server::Request> http_request) {
    exec_cmd(http_response, http_request);
  };
  // get current image
  server.resource["^/image"]["GET"] = [&](
                                          sb::response_ptr http_response,
                                          std::shared_ptr<http_server::Request> http_request) {
    send_image(http_response, http_request);
  };

  //restart capture
  server.resource["^/config$"]["GET"] = [&](
                                            sb::response_ptr http_response,
                                            std::shared_ptr<http_server::Request> http_request) {
    Json::Value root = camerasp::get_DOM(config_path + "options.json");

    Json::FastWriter writer;
    auto camera_config = root["Camera"];
    auto response = writer.write(camera_config);
    send_success(http_response, response);
  };
  server.resource["^/start$"]["GET"] = [&](
                                           sb::response_ptr http_response,
                                           std::shared_ptr<http_server::Request> http_request) {
    std::string success("Succeeded");

    console->info("Resart Child command received");
    if (fg_state == process_state::started)
    {
      success = "Already Running";
      send_failure(http_response, success);
    }
    else if (fg_state == process_state::stop_pending)
    {
      success = "Stop Pending. try again later";
      send_failure(http_response, success);
    }
    else
    {
      if (int ret = posix_spawnp(&child_pid, cmd, &child_fd_actions, NULL, argv, env))
        console->error("posix_spawn"), exit(ret);
      fg_state = process_state::started;
      console->info("Child pid: {0}\n", child_pid);
      send_success(http_response, success);
    }
  };
  auto kill_child = [=]() {
    if (fg_state == process_state::started)
    {
      int status = 0;
      unsigned n = 0;
      kill(child_pid, SIGTERM);
      while (n < 20 && 0 <= waitpid(child_pid, &status, WNOHANG))
      {
        usleep(100 * 1000);
        ++n;
      }
      if (n == 20)
        kill(child_pid, SIGKILL);
      fg_state == process_state::stopped;
    }
  };
  auto _1 = gsl::finally([=]() { kill_child(); });
  //stop capture
  server.resource["^/abort$"]["GET"] = [&](
                                           sb::response_ptr http_response,
                                           std::shared_ptr<http_server::Request> http_request) {
    std::string success("Succeeded");
    //
    if (fg_state == process_state::started)

    {
      kill_child();
      send_success(http_response, success);
    }
    else if (fg_state == process_state::stop_pending)
    {
      success = "Stop Pending. try again later";
      send_failure(http_response, success);
    }
    else
    {
      success = "Not running when command received";
      send_failure(http_response, success);
    }
  };
  //stop capture
  server.resource["^/stop$"]["GET"] = [&](
                                          sb::response_ptr http_response,
                                          std::shared_ptr<http_server::Request> http_request) {
    stop_camera(http_response, http_request);
  };
  //default page server
  server.default_resource["GET"] = [&](
                                       sb::response_ptr http_response,
                                       std::shared_ptr<http_server::Request> http_request) {
    do_fallback(http_response, http_request);
  };

  signals_.async_wait(signal_handler);
  //start() returns on SIGTERM
  server.start();
  if (fg_state == process_state::started)
  {
    queue.send_message("exit", [](std::string const &str, int) {});
  }
}

int main(int argc, char *argv[], char *env[])
{
  try
  {

    Json::Value root = camerasp::get_DOM(config_path + "options.json");
    //configure console

    auto home_path = root["home_path"].asString();
    boost::filesystem::path root_path(home_path);
    auto log_config = root["Logging"];
    auto json_path = log_config["weblog"];
    auto logpath = root_path / (json_path.asString());
    auto size_mega_bytes = log_config["size"].asInt();
    auto count_files = log_config["count"].asInt();
    console = spdlog::rotating_logger_mt("console", logpath.string(), 1024 * 1024 * size_mega_bytes, count_files);
    console->set_level(spdlog::level::debug);
    console->debug("Starting");
    auto io_service = std::make_shared<asio::io_context>();

    address_broadcasting_server broadcaster(*io_service, 52153);
    broadcaster.receive();

    boost::interprocess::message_queue::remove(camerasp::sender_name);
    boost::interprocess::message_queue::remove(camerasp::recvr_name);
    server_msg_queues queue(*io_service, size_t(128), size_t(10), size_t(1024 * 1024), size_t(10));
    std::thread thread(
        [&queue]() {
          queue.recv_message();
        });
    //run web server

    web_server server(root, queue);
    server.run(io_service, argv, env);
    queue.stop_recv();
    thread.join();
  }
  catch (std::exception &e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
