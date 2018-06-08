//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
// 
// detect motion and save in level db
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <camerasp/utils.hpp>
#include <camerasp/level_db.hpp>

namespace camerasp
{
  class save_ldb{
    public:
  save_ldb(std::string const& db_location);
  void operator()(img_info const& img);
    private:
  db_archive dB;
};
}
