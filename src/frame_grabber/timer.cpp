//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////

#include <camerasp/timer.hpp>
#include <jpeg/jpgconvert.hpp>
#include <fstream>
#include <sstream>
#include <camerasp/mot_detect.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
namespace 
{
void init_smtp(smtp_client &smtp)
{
  auto root = camerasp::get_root();
  auto email = root["email"];
  smtp.set_uid(email["uid"].asString());
  smtp.set_pwd(email["pwd"].asString());
  smtp.set_from(email["from"].asString());
  smtp.set_to(email["to"].asString());
  smtp.set_subject(email["subject"].asString());
  smtp.set_server(email["server"].asString());
  console->debug("Init_smtp {0}", __LINE__);
  //smtp.recipient_ids.push_back("joseph.mariadassou@outlook.com");
  //smtp.recipient_ids.push_back("parama_chakra@yahoo.com");
}
asio::ip::tcp::resolver::iterator resolve_socket_address(asio::io_context &io_service)
{
  auto root = camerasp::get_root();
  auto email = root["email"];
  auto url = email["host"].asString();
  auto port = email["port"].asString();
  asio::ip::tcp::resolver resolver(io_service);
  asio::ip::tcp::resolver::query query(url, port);
  asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
  console->debug("Resolved address at line {0}", __LINE__);
  return iterator;
}
}
namespace camerasp
{
  periodic_frame_grabber::periodic_frame_grabber(
      asio::io_context &io_service,
      Json::Value const &root) :
      ctx(asio::ssl::context::sslv23),
      smtp(io_service, ctx),
      socket_address(resolve_socket_address(io_service ))
  {
  ctx.load_verify_file("/home/pi/bin/cacert.pem");
  init_smtp(smtp);
  }
  void periodic_frame_grabber::operator()(img_info const& img)
  {
    static motion_detector detector;
    static int number_of_sequence=0;
    if(smtp.is_busy()) 
      return;
    if(detector.handle_motion(img))
    {
      if (number_of_sequence==0)
        smtp.set_message ( std::string("Date: ") + camerasp::current_GMT_time());
      auto buffer = write_JPEG_dat(img);
      smtp.add_attachment(std::string("image") + std::to_string(number_of_sequence) + ".jpg",buffer );
      ++number_of_sequence;
      if (number_of_sequence>4)
      {

        smtp.send(socket_address);
        number_of_sequence = 0;
      }
    }
    else if (number_of_sequence>0) 
    {

      smtp.send(socket_address);
      number_of_sequence = 0;
    }
  }
}
