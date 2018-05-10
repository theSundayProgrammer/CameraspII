 

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
    nana::form& frm_,
    nana::picture& pic_
    ):s(io_context),
     frm(frm_),
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
          //frm.collocate();
          nana::API::refresh_window(frm);
         // probe_data();
        }
      } else  {
        fprintf(stdout,"Error in read %s\n", ec.message().c_str()); 
      }
    }
    void probe_data()
    {
          usleep(100*1000); //100 msec
      asio::write(s, asio::buffer(request, request_length));
        fprintf(stderr,"%s:%d\n",__FILE__, __LINE__);
      size_t reply_length = asio::read(s, asio::buffer((char*)&response,sizeof response));
      fprintf(stderr,"%s:%d\n",__FILE__, __LINE__);
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
      nana::form& frm;
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
    place p{fm};
    p.div("pic"); 
    p["pic"] << pic ;
    fprintf(stderr,"%s:%d\n",__FILE__, __LINE__);
    client cl(argv[1],argv[2],fm,pic);
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

