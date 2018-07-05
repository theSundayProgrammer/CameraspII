/**********************************************************
Copyright (c) 2015-2018 Joseph Mariadassou (theSundayProgrammer@gmail.com). 

****************************************************************/
#include <camerasp/cam_still_base.hpp>
namespace camerasp
{

  cam_still_base::cam_still_base() {
    width = 640;
    height = 480;
    sharpness = 0;
    contrast = 0;
    brightness = 50;
    quality = 85;
    saturation = 0;
    iso = 400;
    rotation = 0;
    changed_settings = true;
    horizontalFlip = false;
    verticalFlip = false;
  }




  size_t cam_still_base::image_buffer_size() const {
    return width * height * 3 + 54;	//oversize the buffer so to fit BMP images
  }


  void cam_still_base::set_width(unsigned int width)  {
    this->width = width;
    changed_settings = true;
  }

  void cam_still_base::set_height(unsigned int height)  {
    this->height = height;
    changed_settings = true;
  }

  void cam_still_base::set_capture_size(unsigned int width, unsigned int height)  {
    set_width(width);
    set_height(height);
  }

  void cam_still_base::set_brightness(unsigned int brightness)  {
    if (brightness > 100)
      brightness = brightness % 100;
    this->brightness = brightness;
    changed_settings = true;
  }

  void cam_still_base::set_quality(unsigned int quality)  {
    if (quality > 100)
      quality = 100;
    this->quality = quality;
    changed_settings = true;
  }

  void cam_still_base::set_rotation(int rotation)  {
    while (rotation < 0)
      rotation += 360;
    if (rotation >= 360)
      rotation = rotation % 360;
    this->rotation = rotation;
    changed_settings = true;
  }

  void cam_still_base::set_ISO(int iso)  {
    this->iso = iso;
    changed_settings = true;
  }

  void cam_still_base::set_sharpness(int sharpness)  {
    if (sharpness < -100)
      sharpness = -100;
    if (sharpness > 100)
      sharpness = 100;
    this->sharpness = sharpness;
    changed_settings = true;
  }

  void cam_still_base::set_contrast(int contrast)  {
    if (contrast < -100)
      contrast = -100;
    if (contrast > 100)
      contrast = 100;
    this->contrast = contrast;
    changed_settings = true;
  }

  void cam_still_base::set_saturation(int saturation)  {
    if (saturation < -100)
      saturation = -100;
    if (saturation > 100)
      saturation = 100;
    this->saturation = saturation;
    changed_settings = true;
  }

  void cam_still_base::set_horizontal_flip(bool hFlip)  {
    horizontalFlip = hFlip;
    changed_settings = true;
  }

  void cam_still_base::set_vertical_flip(bool vFlip)  {
    verticalFlip = vFlip;
    changed_settings = true;
  }

  unsigned int cam_still_base::get_width()  const{
    return width;
  }

  unsigned int cam_still_base::get_height()  const{
    return height;
  }

  unsigned int cam_still_base::get_brightness()const
  {
    return brightness;
  }

  unsigned int cam_still_base::get_rotation()  const{
    return rotation;
  }

  unsigned int cam_still_base::get_quality()  const{
    return quality;
  }

  int cam_still_base::get_ISO()  const{
    return iso;
  }

  int cam_still_base::get_sharpness() const {
    return sharpness;
  }

  int cam_still_base::get_contrast()  const{
    return contrast;
  }

  int cam_still_base::get_saturation()  const{
    return saturation;
  }

  bool cam_still_base::is_horizontally_flipped()  const{
    return horizontalFlip;
  }

  bool cam_still_base::is_vertically_flipped()  const{
    return verticalFlip;
  }

}
