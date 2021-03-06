 

#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/place.hpp>
#include <cstdlib>
#include <cstring>
#include <asio.hpp>
#include <functional>
#include <regex>
#include <thread>
#include <arpa/inet.h>
#include <iostream>
struct response_t { uint32_t error; uint32_t length;};
using high_resolution_timer = asio::basic_waitable_timer<std::chrono::steady_clock>;
using asio::ip::tcp;
using asio::ip::udp;
struct tcp_src { asio::ip::address address; std::string port;};
std::vector<tcp_src> get_endpoints()
{
  enum
  {
    max_length = 1024
  };
  std::vector<tcp_src> endpoints;
  const std::string send_message{"12068c99-18de-48e1-87b4-3e09bbbd8b15-Camerasp"};
  const std::string recv_message{"ee7f7fc7-9d54-480b-868d-fde1f5a67ab6-Camerasp"};

  asio::io_context io_context;

  udp::resolver resolver(io_context);

  udp::resolver::query query(udp::v4(), "255.255.255.255", "51253");
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
      std::regex rex(recv_message + ".(\\d+)" );
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_length);
        std::cout << std::endl;
        std::smatch sm;
        std::string response(reply, reply_length);
      if (std::regex_match(response,sm, rex))
      { 
        tcp_src src{sender_endpoint.address(), sm[1].str()};
        endpoints.push_back(src);
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


class client{
  public:
    client(
        asio::ip::address& host_,
        std::string port_,
        nana::place& place_,
        nana::picture& pic_
        ):s(io_context),
    err(0),
    place(place_),
    pic(pic_),
    host(host_),
    port(port_)
  {
  }
    void stop()
    {
      io_context.stop();
    }
    void run()
    {
      tcp::resolver resolver(io_context);
      asio::connect(s, resolver.resolve(host.to_string(),port));
      fprintf(stderr,"%s:%s\n",host.to_string().c_str(),port.c_str());
      request_length = strlen(request);
      probe_data();
      io_context.run();
    }
    int get_err() {
      return err;
    }

  private:
    void handle_input(std::error_code ec, std::size_t length)
    {
      buf += std::string(reply,length);
      if (!ec)
      {
        if (buf.length() < response.length)
          s.async_read_some(asio::buffer(reply, max_length), [this](std::error_code ec, std::size_t length)
              {
              handle_input(ec,length);
              });
        else
        {
          nana::paint::image img;
          img.open( buf.data(), buf.length());
          pic.load(img);
          place.collocate();
          //usleep(1000*1000); //mu-sec
          buf.clear();
          probe_data();
        }
      } else  {
        err=-1;
        fprintf(stderr,"Error in read %s\n", ec.message().c_str()); 
      }
    }
    void probe_data()
    {
      try {
      asio::write(s, asio::buffer(request, request_length));
      size_t reply_length = asio::read(s, asio::buffer((char*)&response,sizeof response));
      } catch (std::system_error& error) {
        fprintf(stderr, "Error in probe %s\n", error.what());
        err=-1;
        return ;
      }
      response.error = ntohl(response.error);
      if(response.error == 0) {
        response.length = ntohl(response.length);
        response.length = response.length - sizeof response;
        s.async_read_some(asio::buffer(reply, max_length), [this](std::error_code ec, std::size_t length)
            {
            handle_input(ec,length);
            });
      } else {
        err=-1;
        fprintf(stderr,"Error in read %x\n", response.error);
      }
    } 

  private:
    asio::io_context io_context;
    tcp::socket s;
    int err;
    asio::ip::address& host;
    std::string port;
    char const* request= "image?prev=0\r\n";
    int request_length ;
    response_t  response;
    std::string buf;
    enum {max_length =1024};
    char reply[max_length];
    nana::place& place;
    nana::picture& pic;
};



int main(int argc ,char* argv[])
{
    using namespace nana;
  try
  {
    auto endpoints = get_endpoints();
    if(!endpoints.empty())
    {  
      auto loc =endpoints.begin();
        form fm;
        picture pic(fm);
        place place{fm};
        place.div("pic"); 
        place["pic"] << pic ;
        client cl(loc->address,loc->port,place,pic);

        fm.events().unload([&](){cl.stop();});
        std::thread thread([&]() { 
            cl.run() ;
            if (cl.get_err() != 0)
              fm.close();
            });
        fm.show();
        exec();
        thread.join();
   }
   else
   {
     std::cerr << "No end points\n";
   }

  }
  catch (std::exception& e)
  {
    fprintf(stderr,"Exception: %s \n" ,  e.what() );
  }
}

