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
      unsigned int height;
      unsigned int width;
      unsigned int row_stride;
      unsigned int quality;
#ifndef RASPICAM_MOCK
      JSAMPROW get_scan_line(int scan_line,  int row_stride) const;
#endif
      void put_scan_line(JSAMPLE *,  int row_stride) ;
      void xformbgr2rgb();
      bool empty() { return buffer.empty(); }
    };
    img_info  read_JPEG_file (std::string const& filename);
    buffer_t write_JPEG_dat (img_info const& img);
}
  
#endif
