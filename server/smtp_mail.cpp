//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <camerasp/smtp_client.hpp>
#include <cstdlib>
#include <sstream>
#include <string>
#include <functional>
#include <camerasp/utils.hpp>
#include <camerasp/logger.hpp>
static int busy=0;
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
  if(busy) return;
  busy = 1;
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
        else{
          handle_finish();
          console->debug("Write failed: {0}", error.message());
          }
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
        else{
          handle_finish();
          console->debug("Write failed: {0}", error.message());
          }

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
          handle_finish();
        }
      });
}
void smtp_client::handle_handshake(const asio::error_code &error)
{
  if (!error)
  {
    on_read(&smtp_client::handle_HELO, std::string("EHLO ") + server + "\r\n");
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
  on_read(&smtp_client::handle_UID, camerasp::EncodeBase64(uid) + "\r\n");
}

void smtp_client::handle_UID()
{
  on_read(&smtp_client::handle_PWD, camerasp::EncodeBase64(pwd) + "\r\n");
}

void smtp_client::handle_PWD()
{
  on_read(&smtp_client::handle_from, std::string("MAIL FROM:<") + from + ">" + "\r\n");
}
void smtp_client::handle_from()
{
  on_read(&smtp_client::handle_recpt, std::string("RCPT TO:<") + to + ">" + "\r\n");
}

void smtp_client::handle_recpt()
{
  on_read(&smtp_client::handle_data, std::string("DATA") + "\r\n");
}
void smtp_client::handle_data()
{
  std::ostringstream ostr;
  ostr << "SUBJECT: " << subject << "\r\n"
       << "MIME-Version: 1.0\r\n"
       << "Content-Type: multipart/mixed; boundary=" << boundary << "\r\n"
       << "\r\n"
       << "This is a message with multiple parts in MIME format."
       << "\r\n"
       << "--" << boundary << "\r\n"
       << "Content-Type: text/plain"
       << "\r\n"
       << "\r\n"
       << message
       << "\r\n"
       << "--" << boundary << "\r\n";
  on_read_no_reply(&smtp_client::handle_file, ostr.str());
}
void smtp_client::handle_file()
{
  std::ostringstream ostr;
  ostr
  << "Content-Type: application/octet-stream"
  << "\r\n"
  << "Content-Transfer-Encoding: base64"
  << "\r\n"
  << "Content-Disposition: attachment;\r\n"
  << "  filename=\"" << filename << "\"; size=" << filecontent.size() * 4 / 3 << ";\r\n"
  << "  creation-date="
  << "\"" << camerasp::current_GMT_time() << "\"\r\n"
  << "  modification-date="
  << "\"" << camerasp::current_GMT_time() << "\"\r\n"
  << "\r\n";
  file_pos = 0;
  on_read_no_reply(&smtp_client::handle_file_open, ostr.str());
}
void smtp_client::handle_file_open()
{
  std::ostringstream ostr;
  if (size_t count = filecontent.size() - file_pos)
  {
    if (count > 54)
      count = 54;
    std::string base64 = camerasp::EncodeBase64(filecontent.substr(file_pos,count));
    if (count < 54)
      if (unsigned int writePaddChars = (3 - count % 3) % 3)
        base64.append(writePaddChars, '=');

    ostr << base64 << "\r\n";

    file_pos += count;
    on_read_no_reply(&smtp_client::handle_file_open, ostr.str());
  }
  else
  {
    ostr << "\r\n"
         << "--" << boundary << "--"
         << "\r\n";
    on_read_no_reply(&smtp_client::handle_quit0, ostr.str());
  }
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
  console->debug("done");
  busy = 0;
}
