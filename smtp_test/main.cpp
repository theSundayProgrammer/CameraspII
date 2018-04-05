//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <string>

#include <camerasp/smtp_client.hpp>
#include <camerasp/logger.hpp>
#include <fstream>


#include <iostream>
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
    asio::ssl::context ctx(asio::ssl::context::sslv23);
    ctx.load_verify_file("cacert.pem");

       asio::io_service io_service;
      asio::ip::tcp::resolver resolver(io_service);
      asio::ip::tcp::resolver::query query(argv[1], argv[2]);
      asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

      smtp_client c(io_service, ctx);
      c.uid = argv[3];
      c.pwd = argv[4];
      c.server = "norwestcomputing.com.au";
      c.to = "joseph.mariadassou@outlook.com";
      c.from = "theSundayProgrammer@gmail.com";
      c.subject = "Motion Detected";
      c.message = "This is the body of the message.";
      c.filename = "mail.txt";
      std::ostringstream ostr;
      std::ifstream ifs;
      ifs.open("mail.txt", std::ios::binary);
      ostr << ifs.rdbuf();
      c.send(iterator);
      c.filecontent = ostr.str();
      ifs.close();
      io_service.run();
    }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

/*
MIME-Version: 1.0
Content-Type: multipart/mixed; boundary=frontier

This is a message with multiple parts in MIME format.
--frontier
Content-Type: text/plain

This is the body of the message.
--frontier
Content-Type: application/octet-stream
Content-Transfer-Encoding: base64
content-Descriptiom: test.bin

PGh0bWw+CiAgPGhlYWQ+CiAgPC9oZWFkPgogIDxib2R5PgogICAgPHA+VGhpcyBpcyB0aGUg
Ym9keSBvZiB0aGUgbWVzc2FnZS48L3A+CiAgPC9ib2R5Pgo8L2h0bWw+Cg==
--frontier--
*/
