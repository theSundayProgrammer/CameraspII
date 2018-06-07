
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
#include <jpeg/jpgconvert.hpp>
#include <gsl/gsl_util>
#include <string>
namespace camerasp
{
  class db_archive{
    public:
      ~db_archive();
      db_archive();
      void handle_motion(img_info const& img);
      std::string get_image(std::string const& start);
   private:
leveldb::DB* db;
  };
}

