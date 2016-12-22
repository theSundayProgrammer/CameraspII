#include <camerasp/utils.hpp>
#include <camerasp/raspicamMock.hpp>
#include <vector>
#include <jpeg/jpgconvert.h>
namespace camerasp
{
  cam_still::cam_still(void) {
    changedSettings = false;
    console->debug("RaspiCam");
    width = 640;
    height = 480;
  }
  cam_still::~cam_still(void) { console->debug("~RaspiCam"); }
  void  cam_still::setFormat(enum RASPICAM_FORMAT) { console->debug("setFormat"); }
  void  cam_still::setWidth(unsigned int w) { console->debug("setWidth: " "{0}", w); width = w; }
  void  cam_still::setHeight(unsigned int h) { console->debug("setHeight: " "{0}", h); height = h; }
  void  cam_still::setBrightness(unsigned int) { console->debug("setBrightness"); }
  void  cam_still::setSharpness(int) { console->debug("setSharpness"); }
  void  cam_still::setContrast(int) { console->debug("setContrast"); }
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
  unsigned int  cam_still::getImageBufferSize(void)const {
    console->debug("getImageBufferSize");
    return 3 * this->height*this->width;
  }
  unsigned int  cam_still::getWidth(void)const { console->debug("getWidth"); return width; }
  unsigned int  cam_still::getHeight(void)const { console->debug("getHeight"); return height; }

  void cam_still::setHorizontalFlip(bool hFlip) {
    horizontalFlip = hFlip;
    changedSettings = true;
  }

  void cam_still::setVerticalFlip(bool vFlip) {
    verticalFlip = vFlip;
    changedSettings = true;
  }

  int cam_still::takePicture(unsigned char *dat, unsigned int* length) {
    *length = width*height * 3 + 54;
    return 0;
  }
  bool cam_still::open(bool) { console->debug("open"); return true; }

  buffer_t write_JPEG_dat(ImgInfo & img)
  {
    console->debug("retrieve");
    std::string buffer;
    FILE *fp = nullptr;
    fopen_s(&fp, "C:\\Users\\Public\\Pictures\\figure_3.jpg", "rb");
    if (fp) {
      for (int c = getc(fp); c != EOF; c = getc(fp)) {
        buffer.push_back(c);
      }
      fclose(fp);
    }
    return buffer;
  }
}
