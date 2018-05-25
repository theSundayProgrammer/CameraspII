#include <boost/interprocess/ipc/message_queue.hpp>
#include <iostream>
#include <vector>

using namespace boost::interprocess;
message_queue& get_send_queue()
{
  const int msgq_size = 10;
  static  message_queue mq
         (create_only               //only create
         ,"msgq_sender"           //name
         ,msgq_size                       //max message number
         ,sizeof(int)               //max message size
         );
  return mq;
}

message_queue& get_send_queue()
{
  const int msgq_size = 10;
  static  message_queue mq
         (create_only               //only create
         ,"msgq_sender"           //name
         ,msgq_size                       //max message number
         ,sizeof(int)               //max message size
         );
  return mq;
}
int main ()
{
   try{
      //Erase previous message queue
      message_queue::remove("message_queue");
      
      //Create a message_queue.
     message_queue& mq = get_send_queue();
      //Send 100 numbers
      for(int i = 0; i < 100; ++i){
        mq.send(&i, sizeof(i), 0);
      }
   }
   catch(interprocess_exception &ex){
      std::cout << ex.what() << std::endl;
      return 1;
   }

   return 0;
}
