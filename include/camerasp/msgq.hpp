#pragma once
#include <camerasp/utils.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <camerasp/types.hpp>
#include <map>
#include <functional>
#include <mutex>
#include <string>
namespace camerasp{
namespace ipc = boost::interprocess;
extern const char *sender_name;
extern const char *recvr_name;
typedef size_t sequence_t;
  struct request_message_t{
    sequence_t sequence;
    size_t length;
    char content[4];
  };
  struct response_message_t{
    sequence_t sequence;
    int  error;
    size_t length;
    char content[4];
  };
  class client_msg_queues{
    public:
      client_msg_queues(
        size_t recv_element_size);
    ~client_msg_queues();

    /// waits for a message indefintely inside an infinite loop.
    /// It runs on a separate thread. The response is returned in the same thread synchronously.
    /// To do: consider asynchronous
    std::string get_request();

    /// send_response is called on the same thread as get request.
    void send_response(unsigned int error, std::string const& str);

    private:
      ipc::message_queue send_q;
      ipc::message_queue recv_q;
      sequence_t sequence;
      size_t recv_size;
      request_message_t* request_message;
  };

  class server_msg_queues{
    public:
      typedef std::function<void(std::string const &, int error)> call_back_t;

      //called by the controller to create the queues
      server_msg_queues(
        asio::io_service& io_service, 
        size_t send_element_size, 
        size_t send_queue_length,
        size_t recv_element_size, 
        size_t recv_queue_length);
      
      /// asynchronous call
      void send_message(
        std::string const& , 
        call_back_t);

      /// recv_ message polls the recv queue every 100 ms. It reads the header
      /// and posts the message to io_service. It runs on a separate thread
      bool recv_message();
      
      /// stop_recv is called by the main thread to exit the receive loop
      void stop_recv();
 
      

    private:
      asio::io_service& io_service;
      bool exit_flag;
      ipc::message_queue send_q;
      ipc::message_queue recv_q;
      std::map<sequence_t, call_back_t> response_handle;
      /// the mutex is for sequential access to response_handle
      std::mutex q_mutex;
      size_t recv_size;
      sequence_t sequence;
  };
}
