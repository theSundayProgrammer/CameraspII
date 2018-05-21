 

#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/place.hpp>
#include <cstdlib>
#include <cstring>
#include <asio.hpp>
#include <functional>
#include <thread>
#include <arpa/inet.h>
#include <fstream>
struct response_t { uint32_t error; uint32_t length;};
using asio::ip::tcp;

enum { max_length = 1024*1024 };

class client{
  public:
     client(
    char const* host_, 
    char const* port_,
    nana::place& place_,
    nana::picture& pic_
    ):s(io_context),
     place(place_),
     pic(pic_),
     host(host_),
     port(port_)
    {
    fprintf(stderr,"%s:%d\n",__FILE__, __LINE__);
    }
   void stop()
   {
     io_context.stop();
   }
    void run()
    {
      tcp::resolver resolver(io_context);
      asio::connect(s, resolver.resolve(host,port));
      request_length = strlen(request);
      probe_data();
      fprintf(stderr,"%s:%d\n",__FILE__, __LINE__);
      io_context.run();
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
        fprintf(stdout,"Error in read %s\n", ec.message().c_str()); 
      }
    }
    void probe_data()
    {
      asio::write(s, asio::buffer(request, request_length));
      size_t reply_length = asio::read(s, asio::buffer((char*)&response,sizeof response));
      response.error = ntohl(response.error);
      if(response.error == 0) {
        response.length = ntohl(response.length);
        fprintf(stdout,"len: %d\n",response.length);
        response.length = response.length - sizeof response;
        s.async_read_some(asio::buffer(reply, max_length), [this](std::error_code ec, std::size_t length)
            {
            handle_input(ec,length);
        });
      } else {
        fprintf(stdout,"Error in read %x\n", response.error);
      }
    } 

    private:
      asio::io_context io_context;
      tcp::socket s;

    char const* host;
    char const* port;
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
    if (argc != 3)
    {
      fprintf(stderr,"Usage: client <host> <port>\n");
      return 1;
    }
    form fm;
    picture pic(fm);
    place place{fm};
    place.div("pic"); 
    place["pic"] << pic ;
    fprintf(stderr,"%s:%d\n",__FILE__, __LINE__);
    client cl(argv[1],argv[2],place,pic);
    fm.events().unload([&](){cl.stop();});
    std::thread thread([&]() {
      cl.run() ;
      });
    fm.show();
    exec();
    thread.join();
  }
  catch (std::exception& e)
  {
    fprintf(stderr,"Exception: %s \n" ,  e.what() );
  }
}

