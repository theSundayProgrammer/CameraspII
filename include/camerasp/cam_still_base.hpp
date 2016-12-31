#pragma once
#ifndef CAMERASP_CAM_STILL_BASE_HPP 
#define CAMERASP_CAM_STILL_BASE_HPP 
#include <cstddef>
#include "raspicamtypes.h"
namespace camerasp {

  class cam_still_base {


protected:
    unsigned int width;
    unsigned int height;
    unsigned int rotation; // 0 to 359
    unsigned int brightness; // 0 to 100
    unsigned int quality; // 0 to 100
    int iso;
    int sharpness; // -100 to 100
    int contrast; // -100 to 100
    int saturation; // -100 to 100
    bool horizontalFlip;
    bool verticalFlip;

    bool changed_settings;

  public:
    ~cam_still_base();
    cam_still_base();
    int initialize();

    void set_width(unsigned int width);
    void set_height(unsigned int height);
    void setCaptureSize(unsigned int width, unsigned int height);
    void setBrightness(unsigned int brightness);
    void setQuality(unsigned int quality);
    void setRotation(int rotation);
    void setISO(int iso);
    void setSharpness(int sharpness);
    void setContrast(int contrast);
    void setSaturation(int saturation);
    void set_horizontal_flip(bool hFlip);
    void set_vertical_flip(bool vFlip);

    unsigned int get_width()const;
    unsigned int get_height()const;
    size_t image_buffer_size() const;
    unsigned int get_brightness()const;
    unsigned int get_rotation()const;
    unsigned int get_quality()const;
    int get_ISO()const;
    int get_sharpness()const;
    int get_contrast()const;
    int get_saturation()const;
    bool is_horizontally_flipped()const;
    bool is_vertically_flipped()const;


  };

}
#endif 
