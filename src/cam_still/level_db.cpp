
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
//
//////////////////////////////////////////////////////////////////////////////

#include <camerasp/utils.hpp>
#include <camerasp/level_db.hpp>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <jpeg/jpgconvert.hpp>
#include <gsl/gsl_util>
namespace camerasp
{

  db_archive::~db_archive(){
    delete db;
  }
  db_archive::db_archive(std::string const& db_loc)
    //"/home/pi/data/image.db"
  {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, db_loc , &db);
    auto db_ok = (bool)status.ok();
    console->debug("Level Db opn db status = {0}", db_ok);
  }
  void db_archive::save_img(std::string const& date,img_info const& img)
  {

    auto buffer = write_JPEG_dat(img);
    auto status = db->Put(leveldb::WriteOptions(),date,buffer);
    auto db_ok = status.ok();
    return;
  }
  std::string db_archive::get_image(std::string const& start)
  {
    leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
    auto on_end = gsl::finally([it]() { delete it; });
    it->Seek(start);
    if (it->Valid())
    {
      return it->value().ToString();
    }
    return "Database is Empty";
  }
}

