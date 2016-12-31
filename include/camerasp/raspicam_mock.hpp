#ifndef CAMERASP_RASPICAM_MOCK
#define CAMERASP_RASPICAM_MOCK
#include <camerasp/raspicamtypes.h>
#include <camerasp/cam_still_base.hpp>
#include <string>
namespace camerasp
{
  struct ImgInfo;
  class cam_still : public cam_still_base
  {
  public:
    cam_still(void);
    ~cam_still(void);
    void  setFormat(enum RASPICAM_FORMAT);
    void  setISO(int);
    void  setSaturation(int);
    void  setVideoStabilization(bool);
    void  setExposureCompensation(int);
    void  setExposure(enum RASPICAM_EXPOSURE);
    void  setShutterSpeed(unsigned int);
    void  setAWB(enum RASPICAM_AWB);
    void  setAWB_RB(float, float);
    void stopCapture();
    void commitParameters();
    bool  open(bool val = true);
    int take_picture(unsigned char * preallocated_data, size_t* length);

  private:
  };
  std::string write_JPEG_dat(struct camerasp::ImgInfo const &dat);
}
#endif
