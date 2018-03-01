#pragma once
<<<<<<< HEAD
=======
#include <camerasp/utils.hpp>
>>>>>>> 2aa781bcb33fb2912fa2890bf243152a6235ffd3
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

    camerasp::buffer_t try_get() const{
      unsigned int k=0;
      while(k <10 && !task.try_wait())
      {
	usleep(100*1000);
	++k;
      }
      if(k<10)
      {
<<<<<<< HEAD
=======
	console->debug("Response received");
>>>>>>> 2aa781bcb33fb2912fa2890bf243152a6235ffd3
	ipc::sharable_lock<upgradable_mutex_type> lock(mutex);
	return std::string(response,data_length);
      }
      else
      {
	throw std::runtime_error("ipc command timed out");
      }
    }
    camerasp::buffer_t get() const {
      task.wait();
<<<<<<< HEAD
=======
      console->debug("Response received");
>>>>>>> 2aa781bcb33fb2912fa2890bf243152a6235ffd3
      ipc::sharable_lock<upgradable_mutex_type> lock(mutex);
      return std::string(response,data_length);
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
<<<<<<< HEAD
=======
	console->debug("Response sent");
>>>>>>> 2aa781bcb33fb2912fa2890bf243152a6235ffd3
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
<<<<<<< HEAD
    shared_mem_ptr(const char* name,bool owner_):
      remover(owner_?name:nullptr)
      ,owner(owner_)
  {
    // Construct the shared_request_data.
    if(owner)
    {
      shm_mem = ipc::shared_memory_object(ipc::open_or_create , name, ipc::read_write);
      shm_mem.truncate(sizeof (T));
    region_ptr = std::make_unique<ipc::mapped_region> (shm_mem, ipc::read_write);

    ptr =static_cast<T*> (region_ptr->get_address());
    new (ptr) T;
    } 
    else
    {
      shm_mem =  ipc::shared_memory_object(ipc::open_only , name, ipc::read_write);
    region_ptr = std::make_unique<ipc::mapped_region> (shm_mem, ipc::read_write);

    ptr =static_cast<T*> (region_ptr->get_address());
    }
  }
    T& operator*()
    {
      return *ptr;
    }
=======
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
>>>>>>> 2aa781bcb33fb2912fa2890bf243152a6235ffd3
    T* operator->()
    {
      return ptr;
    }
    ~shared_mem_ptr()
    {
<<<<<<< HEAD
    if(owner)
=======
>>>>>>> 2aa781bcb33fb2912fa2890bf243152a6235ffd3
      ptr->~T();
    }
  private:
    shm_remove remover;
    ipc::shared_memory_object shm_mem;
    std::unique_ptr<ipc::mapped_region> region_ptr;
<<<<<<< HEAD
    bool owner;
=======
>>>>>>> 2aa781bcb33fb2912fa2890bf243152a6235ffd3
    T* ptr;
};

 typedef shared_data<1024*1024>    shared_response_data;
 typedef shared_data<64>    shared_request_data;
