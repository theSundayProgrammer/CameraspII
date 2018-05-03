
//////////////////////////////////////////////////////////////////////////////
// Copyright 2016-2018 (c) Joseph Mariadassou
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
#include <cstdlib>
#include <iostream>
#include <memory>
#include <camerasp/timer.hpp>
#include <camerasp/msg_struct.hpp>
#include <arpa/inet.h>
#include <asio/signal_set.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <camerasp/utils.hpp>
#include <boost/filesystem.hpp>
std::shared_ptr<spdlog::logger> console;
#define ASIO_ERROR_CODE 
std::string home_path;
void configure_logger(Json::Value &root)
{
  namespace spd = spdlog;
  boost::filesystem::path root_path(home_path);
  auto log_config = root["Logging"];
  auto json_path = log_config["path"];
  auto logpath = root_path / (json_path.asString());
  auto size_mega_bytes = log_config["size"].asInt();
  auto count_files = log_config["count"].asInt();

  console = spd::rotating_logger_mt("fg", logpath.string(), 1048576 * size_mega_bytes, count_files);
  console->set_level(spd::level::debug);
}
using tcp=asio::ip::tcp;
const int no_error=0;

class session
: public std::enable_shared_from_this<session>
{
  public:
    session(tcp::socket socket,
    camerasp::periodic_frame_grabber& frame_grabber_)
      : socket_(std::move(socket))
      ,frame_grabber(frame_grabber_)
    {
    }

    void start()
    {
      do_read();
    }

  private:
    void do_read()
    {
      auto self(shared_from_this());
      socket_.async_read_some(
        asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length)
          {
            if (!ec)
            {

              in_buf += std::string(data_,length);
              if (in_buf.length() >=2 && *(in_buf.end()-1) == '\n' && *(in_buf.end()-2)=='\r')
		{
		  handle_request(in_buf);
               }
            else
              do_read();
          } else 
            std::cout << "error " << ec << std::endl;
          });
    }

     void handle_request(std::string const& uri)
      {
        std::smatch m;
        if (std::regex_search(uri, m, std::regex("image\\?prev=([0-9]+)$")))
        {
          int k = atoi(m[1].str().c_str());
          capture_frame(k);
        }
        else if (std::regex_search(uri, m, std::regex("image")))
        {
          int k = 0;
          capture_frame(k);
        }
      }
    void do_write (int sent)
    {
      auto self(shared_from_this());
      socket_.async_write_some(
          asio::buffer(out_buf.data()+ sent, out_buf.length()-sent),
          [this, self, sent](std::error_code ec, std::size_t length)
          {
            if (!ec)
            {
              int sent_ = sent + length;
              if (sent_ < out_buf.length())
                do_write(sent_);
            }
          });
    }
    void send_response(int err, std::string const& str)
{
  network_message_t response;
  response.error =htonl((uint32_t) err);
  response.length  =str.length() + sizeof(response.error) + sizeof(response.length);

  response.length  =htonl(response.length );
  out_buf = std::string(reinterpret_cast<char *>(&response), sizeof(response)) + str;
  do_write(0);
}
    void capture_frame (int k) {
      try
      {
  
        auto image = frame_grabber.get_image(k);
        send_response(no_error,image);
      }
      catch (std::runtime_error &er)
      {
        send_response(-1,"Failed");
        console->debug("Frame Capture Failed");
      }
    }
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    std::string in_buf;
    std::string out_buf;
    camerasp::periodic_frame_grabber& frame_grabber;
};

class server
{
  public:
    server(asio::io_context& io_context,camerasp::periodic_frame_grabber& frame_grabber_, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      socket_(io_context),
      frame_grabber(frame_grabber_)
  {
    do_accept();
  }

  private:
    void do_accept()
    {
      acceptor_.async_accept(socket_,
          [this](std::error_code ec)
          {
          if (!ec)
          {
          std::make_shared<session>(std::move(socket_),frame_grabber)->start();
          }

          do_accept();
          });
    }

    camerasp::periodic_frame_grabber& frame_grabber;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
      std::cerr << "Usage: netcam <path/to/options>\n";
      return 1;
    }
  auto &root = camerasp::get_root(argv[1]);
  //configure console
  home_path = root["home_path"].asString();
  const int no_error=0;
  try
  {
    configure_logger(root);
    asio::io_service frame_grabber_service;
    // The signal set is used to register termination notifications
    asio::signal_set signals_(frame_grabber_service);
    signals_.add(SIGINT);
    signals_.add(SIGTERM);

    // register the handle_stop callback
    signals_.async_wait([&](asio::error_code const &error, int signal_number) {
      frame_grabber_service.stop();
      console->debug("SIGTERM received");
    });

    //configure frame grabber
    camerasp::periodic_frame_grabber frame_grabber(frame_grabber_service, root);
    bool retval = frame_grabber.resume();
    if (!retval) {
      console->error("Unable to open Camera");
      return 1;
    }

    // Start worker threads
    auto port_no = root["Server"]["port"].asInt();
    server s(frame_grabber_service, frame_grabber, port_no);
    frame_grabber_service.run();
    console->info("frame_grabber_service.run complete, shutdown successful");
  }
  catch (Json::LogicError &err)
  {
    std::cerr << "Parse Error: {0}" << err.what() << std::endl;
    return -1;
  }
  catch (std::exception &e)
  {
    console->error("Exception: {0}", e.what());
  }

  return 0;
}
