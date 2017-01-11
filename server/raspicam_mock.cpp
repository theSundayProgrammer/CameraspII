#ifdef RASPICAM_MOCK
#include <camerasp/utils.hpp>
#include <camerasp/cam_still.hpp>
#include <vector>
//#include <jpeg/jpgconvert.h>
char const* img_path = "/home/chakra/data/test.jpg";
//char const* img_path = "C:\\Users\\Public\\Pictures\\fig3.jpg";
namespace camerasp
{
  cam_still::cam_still(void) {
    console->debug("RaspiCam");
  }
  cam_still::~cam_still(void) { console->debug("~RaspiCam"); }
  void  cam_still::setFormat(enum RASPICAM_FORMAT) { console->debug("setFormat"); }
  void  cam_still::setISO(int) { console->debug("setISO"); }
  void  cam_still::setSaturation(int) { console->debug("setSaturation"); }
  void  cam_still::setVideoStabilization(bool) { console->debug("setVideoStabilization"); }
  void  cam_still::setExposureCompensation(int) { console->debug("setExposureCompensation"); }
  void  cam_still::setExposure(enum RASPICAM_EXPOSURE) { console->debug("setExposure"); }
  void  cam_still::setShutterSpeed(unsigned int) { console->debug("setShutterSpeed"); }
  void  cam_still::setAWB(enum RASPICAM_AWB) { console->debug("setAWB"); }
  void  cam_still::setAWB_RB(float, float) { console->debug("setAWB_RB"); }
  void  cam_still::commitParameters() { console->debug("commitParameters"); }


  void  cam_still::stopCapture() {
    console->debug("stopCapture");
  }

  bool cam_still::open(bool) { console->debug("open"); return true; }
string get_image()
{
  stringstream content_stream;
  ifstream ifs("/home/chakra/data/test.jpg");
         
  content_stream << ifs.rdbuf();
  return content_stream.str();
}

  int cam_still::take_picture(unsigned char* data, size_t *length)
  {
    console->debug("retrieve");
    std::string buffer=get_image();
    for (auto c: buffer){ *data++=c; }
    *length = buffer.size();
    return 0;
  }
}
#endif

