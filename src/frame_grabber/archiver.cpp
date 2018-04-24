#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <camerasp/archiver.hpp>
#include <stdexcept>
#include <chrono>
#include <camerasp/utils.hpp>
using std::string;
#include <iomanip>
#include <sstream>
#include <gsl/gsl_util>
string get_gmt_time(){
  auto now = std::chrono::system_clock::now();                // system time
  auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&in_time_t), "%Y%m%d%H%M%S");
  console->debug("time={0}",ss.str());
  return ss.str();
}
namespace camerasp{
  Archiver::Archiver()
{
leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, "/home/pi/data/image.db", &db);
  if (!status.ok())
  {
    throw std::runtime_error("Cannot open database");
  }
}


Archiver::~Archiver(){
  delete db;
}
  bool Archiver::write(string const& buffer){
   leveldb::Status status = db->Put(leveldb::WriteOptions(),get_gmt_time(),buffer);
   return status.ok();
}
std::tuple<int,string> Archiver::get_key(string const& start)
{
  leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
  auto on_end = gsl::finally([it]() { delete it; });
  it->Seek(start);
  if (it->Valid())
  {
    auto str = it->key().ToString();
    console->debug("dat size = {0}", str.size());
    return std::make_tuple(0,str);
  }
  it->SeekToLast();
  if (it->Valid())
  {
    auto str = it->key().ToString();
    console->debug("dat size = {0}", str.size());
    return std::make_tuple(0,str);
  }
  return std::make_tuple(-1,"End of Database");
}
std::tuple<int,string> Archiver::get_image(string const& start)
{
  leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
  auto on_end = gsl::finally([it]() { delete it; });
  it->Seek(start);
  if (it->Valid())
  {
    return std::make_tuple(0,it->value().ToString());
  }
  it->SeekToLast();
  if (it->Valid())
  {
    return std::make_tuple(0,it->value().ToString());
  }
  return std::make_tuple(-1,"End of Database");
}
}

