#include <jpeg/jpgconvert.h>
#define BMP_HEADER_SIZE 54
void camerasp::ImgInfo::put_scan_line(JSAMPLE *data, int row_stride) 
{
    std::copy(data,data+row_stride, std::back_inserter(buffer));
}
void camerasp::ImgInfo::xformbgr2rgb()
{
  auto retval = &buffer[BMP_HEADER_SIZE];
  for (unsigned int scan_line = 0; scan_line < this->image_height; ++scan_line)
  {
    for (unsigned int i = 0; i < row_stride; i += 3)
    {
      auto& r = *retval;
      auto& g = *(retval + 1);
      auto& b = *(retval + 2);
      auto t = r;
      r = b;
      b = t;
      retval += 3;
    }
  }
}
JSAMPROW camerasp::ImgInfo::get_scan_line(int scan_line,  int stride) const
{
  return  reinterpret_cast<JSAMPROW>(const_cast<char *>(&buffer[BMP_HEADER_SIZE + scan_line*row_stride]));
}

