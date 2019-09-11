//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <jpeg/jpgconvert.hpp>
#include <string>
namespace camerasp
{
/**
* \brief Image archiving class
*
*  The images are saved to leveldb database
*  The client supplies a unique key and image data
**/
  class db_archive{
    public:
      ~db_archive();
      db_archive(std::string const& db_loc);
      void save_img(std::string const& date, img_info const& img);
      std::string get_image(std::string const& start);
    private:
      leveldb::DB* db;
  };
}

