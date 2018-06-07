
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
//
//////////////////////////////////////////////////////////////////////////////


#include <camerasp/leveldb.hpp>
#include <camerasp/logger.hpp>
#include <camerasp/utils.hpp>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <jpeg/jpgconvert.hpp>
#include <gsl/gsl_util>

namespace camerasp
{
  class db_archive{
    public:
      ~db_archive();
      db_archive();
      handle_motion(img_info const& img);
      std::string get_image(string const& start);
  };
}

