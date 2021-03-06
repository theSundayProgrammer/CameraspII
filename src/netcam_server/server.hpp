using tcp=asio::ip::tcp;
const int no_error=0;

typedef asio::basic_waitable_timer<std::chrono::steady_clock>   high_resolution_timer;
class session
: public std::enable_shared_from_this<session>
{
  public:
    session(tcp::socket socket,
    camerasp::frame_grabber& frame_grabber_)
      : socket_(std::move(socket))
      ,frame_grabber(frame_grabber_)
      , input_deadline(socket_.get_io_service())
    {
    }
    ~session()
    {
      console->info("Session ended");
    }
    void start()
    {
      auto self(shared_from_this());
      stop_on_=true;
      input_deadline.expires_from_now(std::chrono::seconds(10));
      input_deadline.async_wait([this,self] (const asio::error_code& ec) { stop(); });
      do_read();
    }

  private:
    std::atomic<bool> stop_on_;
    void stop()
    {
      if(stop_on_)
      {
        socket_.close();
        input_deadline.cancel();
      }else {
        stop_on_=true;
        auto self(shared_from_this());
        input_deadline.expires_from_now(std::chrono::seconds(10));
        input_deadline.async_wait([this,self] (const asio::error_code& ec){ stop(); });
      }

    }
    void do_read()
    {
      auto self(shared_from_this());
      socket_.async_read_some(
        asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            stop_on_=false;
            in_buf += std::string(data_,length);
            if (in_buf.length() >=2 && *(in_buf.end()-1) == '\n' && *(in_buf.end()-2)=='\r')
              handle_request(in_buf);
            else
              do_read();
          } else 
            std::cout << "error " << ec << std::endl;
        });
    }

     void handle_request(std::string const& uri)
      {
        std::smatch m;
        if (std::regex_search(uri, m, std::regex("image\\?prev=([0-9]+)$")))
        {
          int k = atoi(m[1].str().c_str());
          capture_frame(k);
        }
        else if (std::regex_search(uri, m, std::regex("flip\\?vertical=(0|1)$")))
        {
          int k = atoi(m[1].str().c_str());
          if (0 == frame_grabber.set_vertical_flip(k != 0))
            send_response(0,"Success");
          else
            send_response(-1,"Failed");
        }
        else if (std::regex_search(uri, m, std::regex("flip\\?horizontal=(0|1)$")))
        {
          int k = atoi(m[1].str().c_str());
          if (0 == frame_grabber.set_horizontal_flip(k != 0))
            send_response(0,"Success");
          else
            send_response(-1,"Failed");
        }
        else if (std::regex_search(uri, m, std::regex("image")))
        {
          int k = 0;
          capture_frame(k);
        }
        else if (std::regex_search(uri, m, std::regex("exit")))
        {
          frame_grabber.pause();
          socket_.get_io_service().stop();
          send_response(no_error,"Stopping");
          console->debug("SIGTERM handled");
        }
      }
    void do_write (int sent)
    {
      auto self(shared_from_this());
      socket_.async_write_some(
          asio::buffer(out_buf.data()+ sent, out_buf.length()-sent),
          [this, self, sent](std::error_code ec, std::size_t length)
          {
            if (!ec)
            {
              int sent_ = sent + length;
              if (sent_ < out_buf.length())
                do_write(sent_);
              else
              {
                in_buf.clear();
                do_read();
              }
            }
          });
    }
    void send_response(int err, std::string const& str)
    {
      response_t response;
      response.error =htonl((uint32_t) err);
      response.length  =str.length() + sizeof(response.error) + sizeof(response.length);

      response.length  =htonl(response.length );
      out_buf = std::string(reinterpret_cast<char *>(&response), sizeof(response)) + str;
      do_write(0);
    }
    void capture_frame (int k)
    {
      try
      {
        auto image = frame_grabber.get_image(k);
        send_response(no_error,image);
      }
      catch (std::runtime_error &er)
      {
        send_response(-1,"Failed");
        console->debug("Frame Capture Failed");
      }
    }
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    std::string in_buf;
    std::string out_buf;
    camerasp::frame_grabber& frame_grabber;
    high_resolution_timer input_deadline;
};

class server
{
  public:
    server(asio::io_context& io_context,camerasp::frame_grabber& frame_grabber_, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      socket_(io_context),
      frame_grabber(frame_grabber_)
  {
    do_accept();
  }

  private:
    void do_accept()
    {
      acceptor_.async_accept(socket_,
          [this](std::error_code ec)
          {
            if (!ec)
            {
              std::make_shared<session>(std::move(socket_),frame_grabber)->start();
            }

            do_accept();
          });
    }

    camerasp::frame_grabber& frame_grabber;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

