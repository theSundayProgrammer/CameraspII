//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <sstream>
#include <string>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time
#include <functional>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <camerasp/logger.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
enum
{
  max_length = 1024
};
static std::string current_GMT_time()
{
  auto now = std::chrono::system_clock::now();                // system time
  auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&in_time_t), "%a, %b %d %Y %X GMT");
  return ss.str();
}

static std::string current_date_time()
{
  auto now = std::chrono::system_clock::now();                // system time
  auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%a, %b %d %Y %X %z");
  return ss.str();
}
class client
{
public:
  client(asio::io_service &io_service,
         asio::ssl::context &context,
         asio::ip::tcp::resolver::iterator endpoint_iterator)
      : socket_(io_service, context)
  {
    socket_.set_verify_mode(asio::ssl::verify_peer);
    socket_.set_verify_callback(
        std::bind(&client::verify_certificate, this, std::placeholders::_1, std::placeholders::_2));
    asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
                        std::bind(&client::handle_connect, this,
                                  std::placeholders::_1));
  }

private:
  bool verify_certificate(bool preverified,
                          asio::ssl::verify_context &ctx)
  {
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    console->debug("Verifying {0}", subject_name);
    return preverified;
  }

  void handle_connect(const asio::error_code &error)
  {
    if (!error)
    {
      socket_.async_handshake(asio::ssl::stream_base::client,
                              std::bind(&client::handle_handshake, this,
                                        std::placeholders::_1));
    }
    else
    {
      console->debug("Connect failed: {0}", error.message());
    }
  }

  void on_read_no_reply(void (client::*write_handle)(),
                        std::string const &request_)
  {
    asio::async_write(
        socket_,
        asio::buffer(request_),
        [this, write_handle](const asio::error_code &error,
                             size_t bytes_transferred) {
          if (!error)
            (this->*write_handle)();
          else
            console->debug("Write failed: {0}", error.message());

        });
  }
  void on_read(void (client::*write_handle)(),
               std::string const &request_)
  {
    asio::async_write(
        socket_,
        asio::buffer(request_),
        [this, write_handle](const asio::error_code &error,
                             size_t bytes_transferred) {
          if (!error)
            this->on_write(write_handle);
          else
            console->debug("Write failed: {0}", error.message());

        });
  }
  void on_write(void (client::*read_handle)())
  {
    asio::async_read(
        socket_,
        asio::buffer(reply_, max_length),
        asio::transfer_at_least(4),
        [this, read_handle](const asio::error_code &error,
                            size_t bytes_transferred) {
          if (!error)
          {
            console->debug("Reply: {0}", std::string(this->reply_, bytes_transferred));
            (this->*read_handle)();
          }
          else
          {
            console->debug("Read failed: {0}", error.message());
          }
        });
  }
  void handle_handshake(const asio::error_code &error)
  {
    if (!error)
    {
      on_read(&client::handle_HELO, std::string("HELO ") + server + "\r\n");
    }
    else
    {
      console->debug("Handshake failed: {0}", error.message());
    }
  }

  void handle_HELO()
  {

    on_read(&client::handle_AUTH, "AUTH LOGIN\r\n");
  }

  void handle_AUTH()
  {
    on_read(&client::handle_UID, EncodeBase64(uid) + "\r\n");
  }

  void handle_UID()
  {
    on_read(&client::handle_PWD, EncodeBase64(pwd) + "\r\n");
  }

  void handle_PWD()
  {
    on_read(&client::handle_from, std::string("MAIL FROM:<") + mail_from + ">" + "\r\n");
  }
  void handle_from()
  {
    on_read(&client::handle_recpt, std::string("RCPT TO:<") + mail_to + ">" + "\r\n");
  }

  void handle_recpt()
  {
    on_read(&client::handle_data, std::string("DATA") + "\r\n");
  }
  void handle_data()
  {
    on_read_no_reply(&client::handle_data_from, std::string("SUBJECT:") + mail_subject + "\r\n");
  }
  void handle_data_from()
  {
    on_read(&client::handle_data_to, std::string("From:") + mail_from + "" + "\r\n");
  }
  void handle_data_to()
  {
    on_read_no_reply(&client::handle_message, std::string("To:") + mail_to + "\r\n" + "\r\n");
  }
  void handle_message()
  {
    on_read(&client::handle_finish, std::string("Motion detetected on ") + current_date_time() + "\r\n" + ".\r\n");
  }
  void handle_finish()
  {
    console->debug("done");
  }
  
  static std::string EncodeBase64(const std::string &data)
  {
    using namespace boost::archive::iterators;
    std::stringstream os;
    typedef insert_linebreaks<
        base64_from_binary<  // convert binary values to base64 characters
            transform_width< // retrieve 6 bit integers from a sequence of 8 bit bytes
                const char *, 6, 8>>,
        72 // insert line breaks every 72 characters
        >
        base64_text; // compose all the above operations in to a new iterator

    std::copy(
        base64_text(data.c_str()),
        base64_text(data.c_str() + data.size()),
        ostream_iterator<char>(os));

    return os.str();
  }

public:
  std::string server;
  std::string uid, pwd;
  std::string mail_from, mail_to;
  std::string mail_subject;

private:
  asio::ssl::stream<asio::ip::tcp::socket> socket_;
  char request_[max_length];
  char reply_[max_length];
};
#include<iostream>
std::shared_ptr<spdlog::logger> console;
int main(int argc, char *argv[])
{
  try
  {
    if (argc != 5)
    {
      std::cerr << "Usage: client <host> <port> <uid> <pwd>\n";
      return 1;
    }
    console = spdlog::stdout_color_mt("smtp_logger");
    console->set_level(spdlog::level::debug);
    asio::io_service io_service;

    asio::ip::tcp::resolver resolver(io_service);
    asio::ip::tcp::resolver::query query(argv[1], argv[2]);
    asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    asio::ssl::context ctx(asio::ssl::context::sslv23);
    ctx.load_verify_file("cacert.pem");

    client c(io_service, ctx, iterator);
    c.uid = argv[3];
    c.pwd = argv[4];
    c.server = argv[1];
    c.mail_to = "joseph.mariadassou@outlook.com";
    c.mail_from = "theSundayProgrammer@gmail.com";
    c.mail_subject = "Motion Detected";
    io_service.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
