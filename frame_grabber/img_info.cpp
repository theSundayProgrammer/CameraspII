//////////////////////////////////////////////////////////////////////////////
// Copyright 2016-2017 Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#include <jpeg/jpgconvert.hpp>
#define BMP_HEADER_SIZE 54
#ifndef RASPICAM_MOCK
JSAMPROW camerasp::img_info::get_scan_line(int scan_line,  int stride) const
{
  return  reinterpret_cast<JSAMPROW>(const_cast<char *>(&buffer[BMP_HEADER_SIZE + scan_line*stride]));
}
#endif
