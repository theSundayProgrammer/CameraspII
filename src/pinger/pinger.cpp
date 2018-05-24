
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <chrono>
#include <asio.hpp>
using asio::ip::udp;

enum
{
  max_length = 1024
};
using high_resolution_timer = asio::basic_waitable_timer<std::chrono::steady_clock>;

std::vector<udp::endpoint> get_endpoints()
{
  std::vector<udp::endpoint> endpoints;
const std::string send_message{"12068c99-18de-48e1-87b4-3e09bbbd8b15-Camerasp"};
const std::string recv_message{"ee7f7fc7-9d54-480b-868d-fde1f5a67ab6-Camerasp"};

    asio::io_context io_context;

    udp::resolver resolver(io_context);

    udp::resolver::query query(udp::v4(), "255.255.255.255", "52153");
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    udp::socket s(io_context);
    s.open(udp::v4());

    s.set_option(asio::socket_base::broadcast(true));

    s.send_to(asio::buffer(send_message), receiver_endpoint);
    bool have_reply = false;
    udp::endpoint sender_endpoint;
    char reply[max_length];
    std::function<void(std::error_code, std::size_t)> do_receive =
        [&](std::error_code ec, std::size_t reply_length) {

          if (recv_message == std::string(reply, reply_length))
          {
            std::cout << "Reply is: ";
            std::cout.write(reply, reply_length);
            endpoints.push_back(sender_endpoint);
          }
          have_reply = true;
          s.async_receive_from(asio::buffer(reply, max_length), sender_endpoint, do_receive);
        };
    s.async_receive_from(asio::buffer(reply, max_length), sender_endpoint, do_receive);

    high_resolution_timer timer_(io_context);
    int count = 0;
    auto prev = high_resolution_timer::clock_type::now();
    std::chrono::seconds sampling_period(2);
    std::function<void(asio::error_code)> time_out =
        [&](asio::error_code ec) {
          if (have_reply)
          {
            have_reply = false;
            prev = high_resolution_timer::clock_type::now();
            timer_.expires_at(prev + sampling_period);
            timer_.async_wait(time_out);
          }
          else
          {
            io_context.stop();
          }
        };

    timer_.expires_at(prev + sampling_period);
    timer_.async_wait(time_out);
    io_context.run();
    return endpoints;
}

int main(int argc, char *argv[])
{
  try
  {
    for( auto& sender_endpoint: get_endpoints())
      std::cout << "\nfrom " << sender_endpoint.address()<< "\n";
    std::cout<< "----------------\n"<<std::endl;
    return 0;
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}
