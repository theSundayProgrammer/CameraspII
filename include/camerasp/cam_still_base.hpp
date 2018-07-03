//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef CAMERASP_CAM_STILL_BASE_HPP 
#define CAMERASP_CAM_STILL_BASE_HPP 
#include <cstddef>
#include "raspicamtypes.h"
namespace camerasp {
///
/// \brief base class for still camera 
///
/** 
 * This is a simple struct wrapped around a class
 */
  class cam_still_base {


protected:
    unsigned int width; ///< can be 320 640 1280 or 1920
    unsigned int height; ///< can be 240 480 960 or 1440
    unsigned int rotation; ///< angle in degrees from 0 to 359 
    unsigned int brightness; ///< from 0 to 100
    unsigned int quality; ///< from 0 to 100
    int iso; ///< range to be specified
    int sharpness; ///< from -100 to 100
    int contrast; ///< from -100 to 100
    int saturation; ///< from -100 to 100
    bool horizontalFlip;///< left columns swap with right columns
    bool verticalFlip;///< top rows swap with bottom rows

    bool changed_settings; ///< flag that indicates need to commit changes

  public:
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
    void set_horizontal_flip(bool horizontalFlip);
    void set_vertical_flip(bool verticalFlip);

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
