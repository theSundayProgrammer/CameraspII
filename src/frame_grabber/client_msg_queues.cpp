
#include <camerasp/msgq.hpp>
namespace camerasp
{

//  typedef std::function<void(std::string)> call_back;

//called by the controller to create the queues
client_msg_queues::client_msg_queues(size_t recv_element_size) : 
  send_q(ipc::open_only, recvr_name),
  recv_q(ipc::open_only, sender_name),
  recv_size(recv_element_size)
{
  size_t buf_size = sizeof(request_message_t) + recv_size - 4;
  request_message = (request_message_t *)malloc(buf_size);
}

client_msg_queues::~client_msg_queues()
{
  free(request_message);
}

/// assumes single threaded
std::string client_msg_queues::get_request()
{

  size_t recv_count = 0;
  unsigned int priority = 0;
  size_t buf_size = sizeof(request_message_t) + recv_size - 4;
  for (;;)
  {
    if(recv_q.try_receive(request_message, buf_size, recv_count,priority))
    {
      this->sequence = request_message->sequence;
      return std::string(&request_message->content[0], request_message->length);
    } else {
      /// wait 100 ms
      usleep(100 * 1000);
    }
  }

}

///assumes single threaded
void client_msg_queues::send_response(unsigned int error, std::string const& str)
{
  unsigned long buf_size=sizeof(response_message_t) + str.length() -4;
  response_message_t* response = (response_message_t*)malloc(buf_size);
  response->error = error;
  response->sequence = sequence;
  response->length = str.length();
  memcpy(&response->content[0], str.data(), str.length() );
  send_q.send(response,buf_size, 0);
  free(response);
}


const char *sender_name = "Web2FrameGrabber-Request";
const char *recvr_name = "FrameGrabber2FileSave-Response";
}