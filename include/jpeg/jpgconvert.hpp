//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#ifndef JPGCONVERT_020615
#define JPGCONVERT_020615
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
#include <iterator>
#include <camerasp/types.hpp>
#ifdef RASPICAM_MOCK
typedef unsigned char JSAMPLE;
#else
#include <jpeglib.h>
#endif
namespace camerasp
{

  struct img_info
  {
    buffer_t  buffer ;
    int error;
    unsigned int height;
    unsigned int width;
    unsigned int quality;

    public:
    img_info():error(0){}
    JSAMPROW get_scan_line(int scan_line,  int row_stride) const;
    void put_scan_line(JSAMPLE *,  int row_stride) ;
    void xformbgr2rgb();
    bool empty() { return buffer.empty(); }
  };
  img_info  read_JPEG_file (std::string const& filename);
  buffer_t write_JPEG_dat (img_info const& img);
}
  
#endif
