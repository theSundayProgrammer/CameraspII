#pragma once
#include <string>
#include <tuple>
 namespace leveldb{
  class DB;
}
namespace camerasp{
class Archiver
{
  public:
    Archiver();
    ~Archiver();
    bool write(std::string const& str);
    std::tuple<int,std::string> get_image(std::string const& start);
    std::tuple<int,std::string> get_key(std::string const& start);
  private:
    leveldb::DB *db;
};
}
