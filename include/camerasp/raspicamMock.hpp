#ifndef CAMERASP_RASPICAM_MOCK
#define CAMERASP_RASPICAM_MOCK
#include <camerasp/raspicamtypes.h>
#include <string>
namespace camerasp
{
  struct ImgInfo;
  class cam_still
  {
  public:
    cam_still(void);
    ~cam_still(void);
    void  setFormat(enum RASPICAM_FORMAT);
    void  setWidth(unsigned int w);
    void  setHeight(unsigned int h);
    void  setBrightness(unsigned int);
    void  setSharpness(int);
    void  setContrast(int);
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
    int takePicture(unsigned char * preallocated_data, unsigned int* length);
    unsigned int  getImageBufferSize(void)const;
    unsigned int  getWidth(void)const;
    unsigned int  getHeight(void)const;
    void setHorizontalFlip(bool hFlip);

    void setVerticalFlip(bool vFlip);

  private:
    bool changedSettings;
    bool verticalFlip;
    bool horizontalFlip;
    unsigned int width;
    unsigned int height;
  };
  std::string write_JPEG_dat(struct camerasp::ImgInfo const &dat);
}
#endif
