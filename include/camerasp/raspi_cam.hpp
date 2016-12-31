/**********************************************************
 Software developed by AVA ( Ava Group of the University of Cordoba, ava  at uco dot es)
 Main author Rafael Munoz Salinas (rmsalinas at uco dot es)
 This software is released under BSD license as expressed below
-------------------------------------------------------------------
Copyright (c) 2013, AVA ( Ava Group University of Cordoba, ava  at uco dot es)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:

   This product includes software developed by the Ava group of the University of Cordoba.

4. Neither the name of the University nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY AVA ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL AVA BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************/
#pragma once
#ifndef CAMERASP_CAM_STILL_HPP
#define CAMERASP_CAM_STILL_HPP
#include "cam_still_base.hpp"
#include <string>
#include <atomic>
#include <mmal/mmal.h>
struct MMAL_COMPONENT_T;
struct MMAL_CONNECTION_T;
struct MMAL_POOL_T;
struct MMAL_PORT_T;
#define MMAL_CAMERA_CAPTURE_PORT 2
#define STILLS_FRAME_RATE_NUM 3
#define STILLS_FRAME_RATE_DEN 1
namespace camerasp {
  typedef void(*imageTakenCallback) (unsigned char * data, int error, unsigned int length);

  class cam_still:public cam_still_base {

  private:

    MMAL_COMPONENT_T * camera;	 /// Pointer to the camera component
    MMAL_COMPONENT_T * encoder;	/// Pointer to the encoder component
    MMAL_CONNECTION_T * encoder_connection; // Connection from the camera to the encoder
    MMAL_POOL_T * encoder_pool;				  /// Pointer to the pool of buffers used by encoder output port
    MMAL_PORT_T * camera_still_port;
    MMAL_PORT_T * encoder_input_port;
    MMAL_PORT_T * encoder_output_port;
    RASPICAM_ENCODING encoding;
    RASPICAM_EXPOSURE exposure;
    RASPICAM_AWB awb;
    RASPICAM_IMAGE_EFFECT imageEffect;
    RASPICAM_METERING metering;

    MMAL_FOURCC_T convertEncoding(RASPICAM_ENCODING encoding);
    MMAL_PARAM_EXPOSUREMETERINGMODE_T convertMetering(RASPICAM_METERING metering);
    MMAL_PARAM_EXPOSUREMODE_T convertExposure(RASPICAM_EXPOSURE exposure);
    MMAL_PARAM_AWBMODE_T convertAWB(RASPICAM_AWB awb);
    MMAL_PARAM_IMAGEFX_T convertImageEffect(RASPICAM_IMAGE_EFFECT imageEffect);
    void commitBrightness();
    void commitQuality();
    void commitRotation();
    void commitISO();
    void commitSharpness();
    void commitContrast();
    void commitSaturation();
    void commitExposure();
    void commitAWB();
    void commitImageEffect();
    void commitMetering();
    void commitFlips();
    int start_capture();
    int create_camera();
    int create_encoder();
    void destroy_camera();
    void destroy_encoder();
    void set_defaults();
    MMAL_STATUS_T connectPorts(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection);

    bool _isInitialized;
    sem_t mutex;



  public:
    ~cam_still();
    cam_still();
    int initialize();
    void release();
    bool open();
    int take_picture(unsigned char * preallocated_data, size_t* length);
    void stop_capture();

    void commit_parameters();
    void setExposure(RASPICAM_EXPOSURE exposure);
    void setAWB(RASPICAM_AWB awb);
    void setImageEffect(RASPICAM_IMAGE_EFFECT imageEffect);
    void setMetering(RASPICAM_METERING metering);
    void setEncoding(RASPICAM_ENCODING encoding);

    RASPICAM_ENCODING get_encoding();
    RASPICAM_EXPOSURE get_exposure();
    RASPICAM_AWB get_AWB();
    RASPICAM_IMAGE_EFFECT get_image_effect();
    RASPICAM_METERING get_metering();

    //Returns an id of the camera. We assume the camera id is the one of the raspberry
    //the id is obtained using raspberry serial number obtained in /proc/cpuinfo
    static std::string getId();

  };

}
#endif // RASPICAM_H
