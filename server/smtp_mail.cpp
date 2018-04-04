//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <camerasp/smtp_mail.hpp>
#include <cstdlib>
#include <sstream>
#include <string>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time
#include <functional>

#include <camerasp/logger.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>

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
smtp_client::smtp_client(asio::io_service &io_service,
         asio::ssl::context &context)
      : socket_(io_service, context)
  {
    socket_.set_verify_mode(asio::ssl::verify_peer);
    socket_.set_verify_callback(
        std::bind(&smtp_client::verify_certificate, this, std::placeholders::_1, std::placeholders::_2));
  }
  void smtp_client::send(asio::ip::tcp::resolver::iterator endpoint_iterator)
  {
    endpoint = endpoint_iterator;
    send();
  }
  void smtp_client::send()
  {
     
    asio::async_connect(
        socket_.lowest_layer(),
        endpoint,
        std::bind(&smtp_client::handle_connect, this, std::placeholders::_1));
  }


  bool smtp_client::verify_certificate(bool preverified,
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

  void smtp_client::handle_connect(const asio::error_code &error)
  {
    if (!error)
    {
      socket_.async_handshake(asio::ssl::stream_base::client,
                              std::bind(&smtp_client::handle_handshake, this,
                                        std::placeholders::_1));
    }
    else
    {
      console->debug("Connect failed: {0}", error.message());
    }
  }

  void smtp_client::on_read_no_reply(void (smtp_client::*write_handle)(),
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
  void smtp_client::on_read(void (smtp_client::*write_handle)(),
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
  void smtp_client::on_write(void (smtp_client::*read_handle)())
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
  void smtp_client::handle_handshake(const asio::error_code &error)
  {
    if (!error)
    {
      on_read(&smtp_client::handle_HELO, std::string("HELO ") + server + "\r\n");
    }
    else
    {
      console->debug("Handshake failed: {0}", error.message());
    }
  }

  void smtp_client::handle_HELO()
  {

    on_read(&smtp_client::handle_AUTH, "AUTH LOGIN\r\n");
  }

  void smtp_client::handle_AUTH()
  {
    on_read(&smtp_client::handle_UID, EncodeBase64(uid) + "\r\n");
  }

  void smtp_client::handle_UID()
  {
    on_read(&smtp_client::handle_PWD, EncodeBase64(pwd) + "\r\n");
  }

  void smtp_client::handle_PWD()
  {
    on_read(&smtp_client::handle_from, std::string("MAIL FROM:<") + mail_from + ">" + "\r\n");
  }
  void smtp_client::handle_from()
  {
    on_read(&smtp_client::handle_recpt, std::string("RCPT TO:<") + mail_to + ">" + "\r\n");
  }

  void smtp_client::handle_recpt()
  {
    on_read(&smtp_client::handle_data, std::string("DATA") + "\r\n");
  }
  void smtp_client::handle_data()
  {
    std::ostringstream ostr;
    ostr << "SUBJECT: Motion detection\r\n"
         << "MIME-Version: 1.0\r\n"
         << "Content-Type: multipart/mixed; boundary=" << boundary << "\r\n"
         << "\r\n"
         << "This is a message with multiple parts in MIME format."
         << "\r\n"
         << "--" << boundary << "\r\n"
         << "Content-Type: text/plain"
         << "\r\n"
         << "\r\n"
         << "This is the body of the message."
         << "\r\n"
         << "--" << boundary << "\r\n"
         << "Content-Type: application/octet-stream"
         << "\r\n"
         << "Content-Transfer-Encoding: base64"
         << "\r\n"
         << "Content-Disposition: attachment;\r\n filename=\"test.txt\""
         << "\r\n"
         << "\r\n"
         << "PGh0bWw+CiAgPGhlYWQ+CiAgPC9oZWFkPgogIDxib2R5PgogICAgPHA+VGhpcyBpcyB0aGUg"
         << "\r\n"
         << "Ym9keSBvZiB0aGUgbWVzc2FnZS48L3A+CiAgPC9ib2R5Pgo8L2h0bWw+Cg=="
         << "\r\n"
         << "--" << boundary << "--"
         << "\r\n";
    on_read_no_reply(&smtp_client::handle_quit0, ostr.str());
  }
  void smtp_client::handle_quit0()
  {
    on_read(&smtp_client::handle_quit1, ".\r\n");
  }
  void smtp_client::handle_quit1()
  {
    on_read(&smtp_client::handle_quit, "\r\n");
  }
  void smtp_client::handle_quit()
  {
    on_read(&smtp_client::handle_finish, std::string("QUIT") + "\r\n");
  }
  void smtp_client::handle_finish()
  {
    //if(k++ < 5)   send();
    console->debug("done");
  }

  