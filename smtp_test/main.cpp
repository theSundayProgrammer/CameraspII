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
#include <iostream>
#include <functional>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <camerasp/logger.hpp>
enum
{
  max_length = 1024
};

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
    std::cout << "Verifying " << subject_name << "\n";

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
      std::cout << "Connect failed: " << error.message() << "\n";
    }
  }
 void on_read (void (client::*write_handle)
                  (const asio::error_code &error,
                   size_t bytes_transferred),
               std::string const& request_)  
    {
      asio::async_write(socket_,
                        asio::buffer(request_),
                        std::bind(write_handle, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2));
    }
void on_write (void (client::*read_handle)
                  (const asio::error_code &error,
                   size_t bytes_transferred))
    {
      asio::async_read(socket_,
                  asio::buffer(reply_, max_length),
                  asio::transfer_at_least(4),
                  std::bind(this->read_handle, this,
                            std::placeholders::_1,
                            std::placeholders::_2));

    }
  void handle_handshake(const asio::error_code &error)
  {
    if (!error)
    {
        on_read(&client::handle_HELO, "HELO\r\n");
    }
    else
    {
      std::cout << "Handshake failed: " << error.message() << "\n";
    }
  }
  
  void handle_HELO(const asio::error_code &error,
                    size_t bytes_transferred)
  {
    if (!error)
      on_write(&client::handle_HELO_write)
    else
      std::cout << "Write failed: " << error.message() << "\n";
    
  }  
  void handle_HELO_write(const asio::error_code &error,
                   size_t bytes_transferred)
  {
    if (!error)
    {
      std::cout << "Reply: ";
      std::cout.write(reply_, bytes_transferred);
      on_read(handle_AUTH,"AUTH LOGIN\r\n");
    } else {
      std::cout << "Read failed: " << error.message() << "\n";
    }
  }
  void handle_AUTH(const asio::error_code &error,
                    size_t bytes_transferred)
  {
    if (!error)
    { }      //ToDo    
    else
      std::cout << "Write failed: " << error.message() << "\n";
  
  }

void handle_AUTH_write(const asio::error_code &error,
                   size_t bytes_transferred)
  {
    if (!error)
    {}
    else
      std::cout << "Read failed: " << error.message() << "\n";
  }


  asio::ssl::stream<asio::ip::tcp::socket> socket_;
  char request_[max_length];
  char reply_[max_length];
};
std::shared_ptr<spdlog::logger> console;
int main(int argc, char *argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }
    console = spdlog::stdout_color_mt("smtp_logger");
    asio::io_service io_service;

    asio::ip::tcp::resolver resolver(io_service);
    asio::ip::tcp::resolver::query query(argv[1], argv[2]);
    asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    asio::ssl::context ctx(asio::ssl::context::sslv23);
    ctx.load_verify_file("cacert.pem");

    client c(io_service, ctx, iterator);

    io_service.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
