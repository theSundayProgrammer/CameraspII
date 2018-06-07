
//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
//
//////////////////////////////////////////////////////////////////////////////

#include <camerasp/level_db.hpp>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <jpeg/jpgconvert.hpp>
#include <gsl/gsl_util>
namespace camerasp
{

  level_db::~level_db(){
    delete db;
  }
  level_db::level_db()
  {
    // Erode kernel -- used in motion detection
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "/home/pi/data/image.db", &db);
    db_ok = (bool)status.ok();
    //console->debug("Level Db opn db status = {0}", db_ok);
  }
  void level_db::handle_motion(img_info const& img)
  {

    auto buffer = write_JPEG_dat(img);
    auto status = db->Put(leveldb::WriteOptions(),get_gmt_time(),buffer);
    db_ok = status.ok();
    return;
  }
  std::string level_db::get_image(string const& start)
  {
    leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
    auto on_end = gsl::finally([it]() { delete it; });
    it->Seek(start);
    if (it->Valid())
    {
      return it->value().ToString();
    }
    //  it->SeekToLast();
    //  if (it->Valid())
    //  {
    //    return it->value().ToString();
    //  }
    return "Database is Empty"
  }
}

