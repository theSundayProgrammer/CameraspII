#ifndef JPGCONVERT_020615
#define JPGCONVERT_020615
#include <cstddef>
#include <cstdio>
#include <jpeglib.h>

#include <string>
#include <vector>
#include <iterator>

namespace camerasp
{
  typedef std::string buffer_t;

    struct ImgInfo
    {
      buffer_t  buffer ;
      unsigned int image_height;
      unsigned int image_width;
      unsigned int row_stride;
      unsigned int quality;
      JSAMPROW get_scan_line(int scan_line,  int row_stride) const;
      void put_scan_line(JSAMPLE *,  int row_stride) ;
      void xformbgr2rgb();
    };
    ImgInfo read_JPEG_file (std::string const& filename);
    void write_JPEG_file (std::string const& filename, ImgInfo const& img);
    std::string write_JPEG_dat (ImgInfo& img);
}
  
#endif
