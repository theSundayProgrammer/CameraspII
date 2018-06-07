#include <camerasp/msgq.hpp>
namespace camerasp
{

//  typedef std::function<void(std::string)> call_back;

//called by the controller to create the queues
server_msg_queues::server_msg_queues(
    asio::io_service &io_service_,
    size_t send_element_size,
    size_t send_queue_length,
    size_t recv_element_size,
    size_t recv_queue_length) : 
  io_service(io_service_),
  send_q(ipc::open_or_create, sender_name, send_queue_length, send_element_size),
  recv_q(ipc::open_or_create, recvr_name, recv_queue_length, recv_element_size),
  exit_flag(false),
  recv_size(recv_element_size),
  sequence(0)
{
}

void server_msg_queues::send_message(
    std::string const &str,
    call_back_t c)
{ 
  sequence_t k=0;
  {
    std::lock_guard<std::mutex> lock(q_mutex);
    k = ++sequence;
    this->response_handle.insert(std::make_pair(k, c));
  }
  unsigned int msg_size = sizeof(request_message_t) + str.length() - 4;
  request_message_t* msg = reinterpret_cast<request_message_t*>(malloc(msg_size));
  msg->sequence = k;
  msg->length = str.length();
  memcpy(&msg->content[0], str.data(), str.length());
  send_q.send(msg,msg_size,0);// Note: call is blocked if queue is full
  free(msg);
  
}

bool server_msg_queues::recv_message()
{
  //this function is called on a separate thread
  //and does not return until the exit flag is set
  //We'll do a busy wait now. To Do: wait on a semaphore
  size_t buf_size = sizeof(response_message_t) + recv_size - 4;
  response_message_t* msg = (response_message_t*)malloc(buf_size);
  while(!exit_flag)
  {
    size_t recv_count=0;
    unsigned int  priority = 0;
    if(recv_q.try_receive(msg, buf_size, recv_count,priority))
    {
      auto it = this->response_handle.find(msg->sequence);
      if (it != this->response_handle.end())
      {
        auto fn = it->second;
        this->response_handle.erase(it);
        std::string data(&msg->content[0], msg->length);
        io_service.post(std::bind(fn,data,msg->error));
      } else {
        //skip
      }
    } else {
      usleep(100*1000);
    }
  }
    size_t recv_count=0;
    unsigned int  priority = 0;
    usleep(100*1000);
    return    recv_q.try_receive(msg, buf_size, recv_count,priority);
}

void server_msg_queues::stop_recv()
{
  exit_flag = true;
}
const char *sender_name="Web2FrameGrabber-Request";
const char *recvr_name= "FrameGrabber2FileSave-Response";

}
