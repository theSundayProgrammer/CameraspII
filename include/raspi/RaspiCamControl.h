/*
Copyright (c) 2013, Broadcom Europe Ltd
Copyright (c) 2013, James Hughes
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef RASPICAMCONTROL_H_
#define RASPICAMCONTROL_H_

#include "RaspiCamControlShared.hpp"

void raspicamcontrol_check_configuration(int min_gpu_mem);

bool raspicamcontrol_parse_cmdline(RASPICAM_CAMERA_PARAMETERS *params, std::istream_iterator<std::string>&);
void raspicamcontrol_display_help();
int raspicamcontrol_cycle_test(MMAL_COMPONENT_T *camera);

void raspicamcontrol_dump_parameters(const RASPICAM_CAMERA_PARAMETERS *params);


void raspicamcontrol_check_configuration(int min_gpu_mem);
class RaspiCamControl
{
  public:

    void set_defaults(RASPICAM_CAMERA_PARAMETERS *params);
    int set_all_parameters(MMAL_COMPONENT_T *camera, const RASPICAM_CAMERA_PARAMETERS *params);
    int get_all_parameters(MMAL_COMPONENT_T *camera, RASPICAM_CAMERA_PARAMETERS *params);
    // Individual setting functions
    int set_saturation(MMAL_COMPONENT_T *camera, int saturation);
    int set_sharpness(MMAL_COMPONENT_T *camera, int sharpness);
    int set_contrast(MMAL_COMPONENT_T *camera, int contrast);
    int set_brightness(MMAL_COMPONENT_T *camera, int brightness);
    int set_ISO(MMAL_COMPONENT_T *camera, int ISO);
    int set_metering_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_EXPOSUREMETERINGMODE_T mode);
    int set_video_stabilisation(MMAL_COMPONENT_T *camera, int vstabilisation);
    int set_exposure_compensation(MMAL_COMPONENT_T *camera, int exp_comp);
    int set_exposure_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_EXPOSUREMODE_T mode);
    int set_awb_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_AWBMODE_T awb_mode);
    int set_awb_gains(MMAL_COMPONENT_T *camera, float r_gain, float b_gain);
    int set_imageFX(MMAL_COMPONENT_T *camera, MMAL_PARAM_IMAGEFX_T imageFX);
    int set_colourFX(MMAL_COMPONENT_T *camera, const MMAL_PARAM_COLOURFX_T *colourFX);
    int set_rotation(MMAL_COMPONENT_T *camera, int rotation);
    int set_flips(MMAL_COMPONENT_T *camera, int hflip, int vflip);
    int set_ROI(MMAL_COMPONENT_T *camera, PARAM_FLOAT_RECT_T rect);
    int set_shutter_speed(MMAL_COMPONENT_T *camera, int speed_ms);
    int set_DRC(MMAL_COMPONENT_T *camera, MMAL_PARAMETER_DRC_STRENGTH_T strength);
    int set_stats_pass(MMAL_COMPONENT_T *camera, int stats_pass);
    int set_annotate(MMAL_COMPONENT_T *camera, const int bitmask, const char *string,
                                     const int text_size, const int text_colour, const int bg_colour);
    int set_stereo_mode(MMAL_PORT_T *port, MMAL_PARAMETER_STEREOSCOPIC_MODE_T *stereo_mode);
    
    //Individual getting functions
    int get_saturation(MMAL_COMPONENT_T *camera);
    int get_sharpness(MMAL_COMPONENT_T *camera);
    int get_contrast(MMAL_COMPONENT_T *camera);
    int get_brightness(MMAL_COMPONENT_T *camera);
    int get_ISO(MMAL_COMPONENT_T *camera);
    MMAL_PARAM_EXPOSUREMETERINGMODE_T raspicamcontrol_get_metering_mode(MMAL_COMPONENT_T *camera);
    int get_video_stabilisation(MMAL_COMPONENT_T *camera);
    int get_exposure_compensation(MMAL_COMPONENT_T *camera);
    MMAL_PARAM_THUMBNAIL_CONFIG_T get_thumbnail_parameters(MMAL_COMPONENT_T *camera);
    MMAL_PARAM_EXPOSUREMODE_T get_exposure_mode(MMAL_COMPONENT_T *camera);
    MMAL_PARAM_AWBMODE_T get_awb_mode(MMAL_COMPONENT_T *camera);
    MMAL_PARAM_IMAGEFX_T get_imageFX(MMAL_COMPONENT_T *camera);
    MMAL_PARAM_COLOURFX_T get_colourFX(MMAL_COMPONENT_T *camera);
};

const char *string_from_exposure_mode(MMAL_PARAM_EXPOSUREMODE_T em);
MMAL_PARAM_EXPOSUREMODE_T exposure_mode_from_string(const char *str);

const char *string_from_awb_mode(MMAL_PARAM_AWBMODE_T em);
MMAL_PARAM_AWBMODE_T awb_mode_from_string(const char *str);

const char *string_mode_from_imagefx(MMAL_PARAM_IMAGEFX_T);
MMAL_PARAM_IMAGEFX_T imagefx_mode_from_string(const char *str);

const char *string_from_metering_mode(MMAL_PARAM_EXPOSUREMETERINGMODE_T em);
MMAL_PARAM_EXPOSUREMETERINGMODE_T metering_mode_from_string(const char *str);

MMAL_PARAMETER_DRC_STRENGTH_T drc_mode_from_string(const char *str);
const char *string_from_drc_mode(MMAL_PARAMETER_DRC_STRENGTH_T em);

MMAL_STEREOSCOPIC_MODE_T stereo_mode_from_string(const char *str);
const char *string_from_stereo_mode(MMAL_STEREOSCOPIC_MODE_T em);

const char *string_from_img_format(MMAL_FOURCC_T em);
MMAL_FOURCC_T img_format_from_string(const char *str);

#endif /* RASPICAMCONTROL_H_ */

