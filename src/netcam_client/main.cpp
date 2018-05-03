#include <cstdlib>
#include <cstring>
#include <iostream>
#include "asio.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <camerasp/network.hpp>

using asio::ip::tcp;

enum { max_length = 1024*1024 };

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }

    asio::io_context io_context;

    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    asio::connect(s, resolver.resolve(argv[1], argv[2]));

    std::cout << "Enter message: ";
    char request[max_length];
    std::cin.getline(request, max_length);
    size_t request_length = std::strlen(request);
    request[request_length++]='\r';
    request[request_length++]='\n';
    asio::write(s, asio::buffer(request, request_length));
    char reply[max_length];
    response_t  response;
    std::string buf;
    std::function<void(std::error_code, std::size_t)> handle_input= [&](std::error_code ec, std::size_t length)
        {
           buf += std::string(reply,length);
          if (!ec)
          {
            if (buf.length() < response.length)
              s.async_read_some(asio::buffer(reply, max_length),handle_input);
            else{
              std::cout << "\n" ;
              std::ofstream ofs("./img.jpg", std::ios::binary);
              ofs.write(buf.data(), buf.length());
              std::cout << "Done" << std::endl;
            }
          } else 
            std::cout << "error " << ec << std::endl;
        };
    size_t reply_length = asio::read(s, asio::buffer((char*)&response,sizeof response));
    response.error = ntohl(response.error);
    if(response.error == 0) {
      response.length = ntohl(response.length);
      std::cout << "len:" << response.length << std::endl;
      response.length = response.length - sizeof response;
      s.async_read_some(asio::buffer(reply, max_length), handle_input);
      io_context.run();
    } else {
      std::cout << "Error : " << std::hex <<response.error << std::endl;
    }
    
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

