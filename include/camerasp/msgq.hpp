//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
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

/** \brief used in IPC */
  struct request_message_t{
    sequence_t sequence;//< Sequence number of request
    size_t length; //< length of message
    char content[4]; //< the first 4 bytes of the message
  };
/** \brief used in IPC */
  struct response_message_t{
    sequence_t sequence;//< Sequence number of request
    int  error;  //< Zero for no error
    size_t length; 
    char content[4];
  };
/** \brief client queue used in IPC */
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
      ipc::message_queue send_q; //< Request
      ipc::message_queue recv_q; //< Response
      sequence_t sequence; //< Sequence number of next request 
      size_t recv_size; //< maximum length of message
      request_message_t* request_message; //< current request 
  };
/** Message Queue used by server to wait for request */
  class server_msg_queues{
    public:
      typedef std::function<void(std::string const &, int error)> call_back_t;

      /// called by the controller to create the queues
      /// \param io_service used to post requests to main thread
      /// \param send_element_size the maximum size of response message
      /// \param send_queue_length maximum number of messages waiting to be sent
      /// \param recv_element_size maximum size of a request
      /// \param recv_queue_length length of the queue
      server_msg_queues(
        asio::io_service& io_service, 
        size_t send_element_size, 
        size_t send_queue_length,
        size_t recv_element_size, 
        size_t recv_queue_length);
      
      /// asynchronous call
      /// \param msg response
      /// \param fn function to call on completion
      void send_message(
        std::string const& msg, 
        call_back_t fn);

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
