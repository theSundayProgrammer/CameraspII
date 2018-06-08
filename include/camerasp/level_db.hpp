
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
//
//////////////////////////////////////////////////////////////////////////////


#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <string>
namespace camerasp
{
  class db_archive{
    public:
      ~db_archive();
      db_archive(std::string const& db_loc);
      void save_img(img_info const& img);
      std::string get_image(std::string const& start);
   private:
leveldb::DB* db;
  };
}

