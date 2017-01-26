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
#include <camerasp/types.hpp>
#define REQUEST_MEMORY_NAME "Web2FrameGrabber-Request"
#define RESPONSE_MEMORY_NAME "Web2FrameGrabber-Response"
#define FG2FS_REQUEST_NAME "FrameGrabber2FileSave-Request"
#define FG2FS_RESPONSE_NAME "FrameGrabber2FileSave-Response"
namespace ipc = boost::interprocess;
template<int MAX_REQUEST_LENGTH>
struct shared_data{
  private:
    typedef ipc::interprocess_upgradable_mutex upgradable_mutex_type;
    typedef ipc::interprocess_semaphore ipc_semaphore;
    mutable upgradable_mutex_type mutex;
    mutable ipc_semaphore task;
    char response[MAX_REQUEST_LENGTH];
    int  data_length;
  public:
    shared_data():
    task(0),
   data_length(0) 
    {
    }

    camerasp::buffer_t get() const {
      unsigned int k=0;
      while(k <10 && !task.try_wait())
      {
	usleep(100*1000);
	++k;
      }
      if(k<10)
      {
	console->debug("Response received");
	ipc::sharable_lock<upgradable_mutex_type> lock(mutex);
	return std::string(response,data_length);
      }
      else
      {
	throw std::runtime_error("ipc command timed out");
      }
    }
    void set(camerasp::buffer_t const & str) {
      ipc::scoped_lock<upgradable_mutex_type> lock(mutex);
      if(str.length() >= MAX_REQUEST_LENGTH)
	throw std::runtime_error("request length too long");
      else
      {
	std::copy(std::begin(str),std::end(str),std::begin(response));
	data_length=str.length();
	task.post();
	console->debug("Response sent");
      }
    }
  public:

};
class shm_remove
{
  public:
    shm_remove(char const* name_)
      :name(name_)
    {
      if(name)
	ipc::shared_memory_object::remove(name);
    }
    ~shm_remove()
    {
      if(name)
	ipc::shared_memory_object::remove(name);
    }

    shm_remove(shm_remove && other)
    {
      name = other.name;
      other.name=nullptr;
    }
    shm_remove& operator=(shm_remove && other)
    {
      name = other.name;
      other.name=nullptr;
      return *this;
    }
    shm_remove(shm_remove const &)=delete;
    shm_remove& operator=(shm_remove const &)=delete;
  private:
    char const* name;
};
template<class T>
class shared_mem_ptr
{
  public:
    shared_mem_ptr(const char* name):
      remover(name)
      ,shm_mem(ipc::open_or_create , name, ipc::read_write)
  {
    // Construct the shared_request_data.


    shm_mem.truncate(sizeof (T));

    region_ptr = std::make_unique<ipc::mapped_region> (shm_mem, ipc::read_write);

    ptr =static_cast<T*> (region_ptr->get_address());
    new (ptr) T;
    console->debug("Created response {0}", name);
  }
    T* operator->()
    {
      return ptr;
    }
    ~shared_mem_ptr()
    {
      ptr->~T();
    }
  private:
    shm_remove remover;
    ipc::shared_memory_object shm_mem;
    std::unique_ptr<ipc::mapped_region> region_ptr;
    T* ptr;
};

 typedef shared_data<1024*1024>    shared_response_data;
 typedef shared_data<64>    shared_request_data;
