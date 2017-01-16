#pragma once
#include <camerasp/utils.hpp>
#include <boost/scope_exit.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>
#define REQUEST_MEMORY_NAME "SO12439099-Request"
#define RESPONSE_MEMORY_NAME "SO12439099-Response"
namespace ipc = boost::interprocess;
struct shared_request_data {
  private:
    typedef ipc::interprocess_upgradable_mutex upgradable_mutex_type;
    typedef ipc::interprocess_semaphore ipc_semaphore;
    enum local_constants { max_request_length = 64 } ;
    mutable upgradable_mutex_type mutex;
    mutable ipc_semaphore task;
    char request[max_request_length];
  public:
    shared_request_data():
    task(0)
    {
    }

    std::string get() const {
      task.wait();
	console->debug("Request received");
      ipc::sharable_lock<upgradable_mutex_type> lock(mutex);
      return std::string(&request[0]);
    }

    void set(std::string const & str) {
      ipc::scoped_lock<upgradable_mutex_type> lock(mutex);
      if(str.length() >= max_request_length)
	throw std::runtime_error("request length too long");
      else
      {
	console->debug("Response received");
	std::copy(std::begin(str),std::end(str),std::begin(request));
	request[str.length()]='\0';
	task.post();
      }
    }
  public:

};
struct shared_response_data{
  private:
    typedef ipc::interprocess_upgradable_mutex upgradable_mutex_type;
    typedef ipc::interprocess_semaphore ipc_semaphore;
    enum local_constants { max_request_length = 1024*1024 } ; // one MB of data
    mutable upgradable_mutex_type mutex;
    mutable ipc_semaphore task;
    char response[max_request_length];
    int  data_length;
  public:
    shared_response_data():
    task(0),
   data_length(0) 
    {
    }

    camerasp::buffer_t get() const {
      task.wait();
	console->debug("Request received");
      ipc::sharable_lock<upgradable_mutex_type> lock(mutex);
      return std::string(response,data_length);
    }

    void set(camerasp::buffer_t const & str) {
      ipc::scoped_lock<upgradable_mutex_type> lock(mutex);
      if(str.length() >= max_request_length)
	throw std::runtime_error("request length too long");
      else
      {
	console->debug("Response received");
	std::copy(std::begin(str),std::end(str),std::begin(response));
	data_length=str.length();
	task.post();
      }
    }
  public:

};

