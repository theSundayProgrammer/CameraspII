////////////////////////////////////////////////////////////////////////////// 
// Copyright 2016-2018 (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
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
#include <boost/filesystem.hpp>
#include <sys/wait.h>
#include <iostream>
#include <gsl/gsl_util>
std::shared_ptr<spdlog::logger> console;
char const *cmd = "./server";

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

#define ASIO_ERROR_CODE asio::error_code
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

using std::string;

void run(std::shared_ptr<asio::io_context> io_service, char *argv[], char *env[])
{
  //Child process
  //Important: the working directory of the child process
  // is the same as that of the parent process.
  int ret;
  pid_t child_pid;
  auto fg_state = process_state::stopped;
  posix_spawn_file_actions_t child_fd_actions;
  if (ret = posix_spawn_file_actions_init(&child_fd_actions))
    console->error("posix_spawn_file_actions_init"), exit(ret);
  if (ret = posix_spawn_file_actions_adddup2(&child_fd_actions, 1, 2))
    console->error("posix_spawn_file_actions_adddup2"), exit(ret);
  //
  // The signal set is used to register termination notifications
  asio::signal_set signals_(*io_service);
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
  signals_.add(SIGCHLD);

  char cur_path[260];
  int p_len = readlink("/proc/self/exe", cur_path,260);
  boost::filesystem::path p(std::string(cur_path,p_len));
  auto child_path= p.parent_path()/"server";
  std::string path_name=child_path.string();
  const char* cmd= path_name.c_str(); 
  auto kill_child = [&]() {
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
    }
  };
  auto launch = [&]() { 
    if (int ret = posix_spawnp(&child_pid, cmd, &child_fd_actions,
                             NULL, argv, env))
    console->error("posix_spawn"), exit(ret);
  console->info("Child pid: {0}\n", child_pid);
  fg_state = process_state::started;
  };
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
          launch();
        }
        else
        {
          console->debug("SIGTERM received");
          io_service->stop();
        }
      };


  auto _1 = gsl::finally([=]() { kill_child(); });

  signals_.async_wait(signal_handler);
 launch();
  io_service->run();
}

int main(int argc, char *argv[], char *env[])
{
  try
  {

    //configure console

    if (argc != 2)
    {
      std::cerr << "Usage: netcam <path/to/options>\n";
      return 1;
    }
  auto &root = camerasp::get_root(argv[1]);
    auto home_path = root["home_path"].asString();
    boost::filesystem::path root_path(home_path);
    auto log_config = root["Logging"];
    auto json_path = log_config["weblog"];
    auto logpath = root_path / (json_path.asString());
      console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);
    console->debug("Starting");
    auto io_service = std::make_shared<asio::io_context>();

    address_broadcasting_server broadcaster(*io_service, 52153);
    broadcaster.receive();
    run(io_service,argv,env);
  }
  catch (std::exception &e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
