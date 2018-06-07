//////////////////////////////////////////////////////////////////////////////
// Copyright 2016 -2017 (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#ifdef __GNUC__
#include <stdio.h>
#include <camerasp/utils.hpp>
//fopen_s is not defined in gcc.
errno_t fopen_s(FILE** fp, const char* name, const char* mode)
{
  *fp = fopen(name, mode);
  if (*fp) return 0;
  else return -1;
}
#endif
