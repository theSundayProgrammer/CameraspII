/**********************************************************
Copyright i2016-2017 Joseph Mariadassou (theSundayProgrammer@gmail.com). 
This file is inspired by Raspicam
http://github.com/cedricve/raspicam. The original copyright folows:

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

#ifndef RASPICAM_MOCK
#include <fstream>
#include <camerasp/cam_still.hpp>
#include <mmal/mmal.h>
#include <mmal/mmal_buffer.h>
#include <mmal/util/mmal_connection.h>
#include <mmal/util/mmal_default_components.h>
#include <mmal/util/mmal_util.h>
#include <mmal/util/mmal_util_params.h>
#include <camerasp/utils.hpp>
#include <gsl/gsl_util>
#include <stdexcept>
using namespace std;
#define API_NAME "raspicam_still"
namespace camerasp
{
/** \brief type used in call back of frame grabber
   *
   */

static void control_callback(
    MMAL_PORT_T *port,
    MMAL_BUFFER_HEADER_T *buffer)
{
  (void)port;
  if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED) { 
      mmal_buffer_header_release(buffer);
   }  else  {
    // Unexpected control callback event!
    console->error("control call back unexpected command {0}", buffer->cmd);
    mmal_buffer_header_release(buffer);
    throw std::runtime_error("Unepected contol call back");
  }
  
}

static void buffer_callback(
  MMAL_PORT_T *port,
  MMAL_BUFFER_HEADER_T *buffer
  ) {
  const unsigned int END_FLAG =
      MMAL_BUFFER_HEADER_FLAG_FRAME_END |
      MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED;

  RASPICAM_USERDATA *userdata = (RASPICAM_USERDATA *)port->userdata;
  unsigned int flags = buffer->flags;
  {
    mmal_buffer_header_mem_lock(buffer);
    auto _ = gsl::finally([buffer]() {
      mmal_buffer_header_mem_unlock(buffer);
      mmal_buffer_header_release(buffer);
    });
    if (userdata == NULL) {
      return;
    } else if (buffer->length + userdata->offset > userdata->length) {
      console->error(API_NAME
                     ": Buffer provided {0} was too small offset={1}!"
                     " Failed to copy data into buffer.",
                     userdata->offset,
                     userdata->length);
      return;
    } else {
      for (unsigned int i = 0; i < buffer->length; ++i)
        userdata->data[userdata->offset++] = buffer->data[i];
    }
  }
  if (END_FLAG & flags) {
    sem_post(userdata->data_ready);
  } else if (port->is_enabled) {
    MMAL_BUFFER_HEADER_T *new_buffer =
        mmal_queue_get(userdata->encoderPool->queue);
    if (new_buffer)
      mmal_port_send_buffer(port, new_buffer);
  }
}

cam_still::~cam_still() {
  sem_destroy(&data_ready);
  release();
}
void cam_still::stop_data_wait()
{
  stop_capture_flag = true;
  sem_post(&data_ready);
}
void cam_still::await_data_ready()
{
  for(;;)
  {
    if (stop_capture_flag)
      break;
    int ret = sem_wait(&data_ready);
    if(ret == -1 && errno == EINTR)
      continue;
    stop_capture();

    img_info info;
    info.buffer = std::string((char*)userdata.data,userdata.length);
    info.width = get_width();
    info.height = get_height();
    info.quality = 100;
    asio::post(io_service, [this,info]() {
    on_image_capture(info);});
  }
}
cam_still::cam_still(asio::io_context& io_service_) :
      stop_capture_flag(false),
      io_service(io_service_),
      encoding(MMAL_ENCODING_BMP),
      metering(MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE),
      exposure(MMAL_PARAM_EXPOSUREMODE_AUTO),
      awb(MMAL_PARAM_AWBMODE_AUTO),
      imageEffect(MMAL_PARAM_IMAGEFX_NONE) {
        camera = NULL;
        encoder = NULL;
        encoder_connection = NULL;
        encoder_pool = NULL;
        camera_still_port = NULL;
        encoder_input_port = NULL;
        encoder_output_port = NULL;
        is_initialized = false;
        sem_init(&data_ready,0,0);
        set_defaults();
      }
void cam_still::set_defaults() {
  width = 640;
  height = 480;
  encoder = NULL;
  encoder_connection = NULL;
  sharpness = 0;
  contrast = 0;
  brightness = 50;
  quality = 85;
  saturation = 0;
  iso = 400;
  //videoStabilisation = 0;
  //exposureCompensation = 0;
  //colourEffects.enable = 0;
  //colourEffects.u = 128;
  //colourEffects.v = 128;
  rotation = 0;
  changed_settings = true;
  horizontalFlip = false;
  verticalFlip = false;
  encoding = MMAL_ENCODING_BMP;
  metering = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;
  exposure = MMAL_PARAM_EXPOSUREMODE_AUTO;
  awb = MMAL_PARAM_AWBMODE_AUTO;
  imageEffect = MMAL_PARAM_IMAGEFX_NONE;
}

void cam_still::commit_parameters() {
  if (!changed_settings)
    return;
  commitSharpness();
  commitContrast();
  commitBrightness();
  commitQuality();
  commitSaturation();
  commitISO();
  commitExposure();
  commitMetering();
  commitAWB();
  commitImageEffect();
  commitRotation();
  commitFlips();
  // Set Video Stabilization
  if (mmal_port_parameter_set_boolean(camera->control, MMAL_PARAMETER_VIDEO_STABILISATION,
                                      0) != MMAL_SUCCESS)
    console->error(API_NAME
                   ": Failed to set video stabilization parameter.");
  // Set Exposure Compensation
  if (mmal_port_parameter_set_int32(camera->control, MMAL_PARAMETER_EXPOSURE_COMP, 0) != MMAL_SUCCESS)
    console->error(API_NAME
                   ": Failed to set exposure compensation parameter.");
  // Set Color Efects
  MMAL_PARAMETER_COLOURFX_T colfx = {
     { MMAL_PARAMETER_COLOUR_EFFECT, sizeof(colfx) },
     0, 0, 0};
  colfx.enable = 0;
  colfx.u = 128;
  colfx.v = 128;
  if (mmal_port_parameter_set(camera->control, &colfx.hdr) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set color effects parameter.");
  // Set ROI
  MMAL_PARAMETER_INPUT_CROP_T crop =
      {
        {MMAL_PARAMETER_INPUT_CROP, sizeof(MMAL_PARAMETER_INPUT_CROP_T)}, 
        {0, 0, 0, 0}
      };
  crop.rect.x = (65536 * 0);
  crop.rect.y = (65536 * 0);
  crop.rect.width = (65536 * 1);
  crop.rect.height = (65536 * 1);
  if (mmal_port_parameter_set(camera->control, &crop.hdr) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set ROI parameter.");
  // Set encoder encoding
  if (encoder_output_port != NULL) {
    encoder_output_port->format->encoding = (encoding);
    mmal_port_format_commit(encoder_output_port);
  }
  changed_settings = false;
}

MMAL_STATUS_T
cam_still::connectPorts(
    MMAL_PORT_T *output_port,
    MMAL_PORT_T *input_port,
    MMAL_CONNECTION_T **connection) {
  MMAL_STATUS_T status =
      mmal_connection_create(
          connection,
          output_port,
          input_port,
          MMAL_CONNECTION_FLAG_TUNNELLING |
              MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT
      );
  if (status == MMAL_SUCCESS) {
    status = mmal_connection_enable(*connection);
    if (status != MMAL_SUCCESS)
      mmal_connection_destroy(*connection);
  }

  return status;
}

int cam_still::create_camera() {
  if (mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera)) {
    console->error(API_NAME ": Failed to create camera component.");
    return -1;
  }

  if (!camera->output_num) {
    console->error(API_NAME ": Camera does not have output ports!");
    destroy_camera();
    return -1;
  }

  camera_still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

  // Enable the camera, and tell it its control callback function
  if (mmal_port_enable(camera->control, control_callback)) {
    console->error(API_NAME ": Could not enable control port.");
    destroy_camera();
    return -1;
  }

  MMAL_PARAMETER_CAMERA_CONFIG_T camConfig = {
      {MMAL_PARAMETER_CAMERA_CONFIG, sizeof(camConfig)},
      width,                              // max_stills_w
      height,                             // max_stills_h
      0,                                  // stills_yuv422
      1,                                  // one_shot_stills
      width,                              // max_preview_video_w
      height,                             // max_preview_video_h
      3,                                  // num_preview_video_frames
      0,                                  // stills_capture_circular_buffer_height
      0,                                  // fast_preview_resume
      MMAL_PARAM_TIMESTAMP_MODE_RESET_STC // use_stc_timestamp
  };
  if (mmal_port_parameter_set(camera->control, &camConfig.hdr) !=
      MMAL_SUCCESS)
    console->error(API_NAME ": Could not set port parameters.");

  commit_parameters();

  MMAL_ES_FORMAT_T *format = camera_still_port->format;
  format->encoding = MMAL_ENCODING_OPAQUE;
  format->es->video.width = width;
  format->es->video.height = height;
  format->es->video.crop.x = 0;
  format->es->video.crop.y = 0;
  format->es->video.crop.width = width;
  format->es->video.crop.height = height;
  format->es->video.frame_rate.num = STILLS_FRAME_RATE_NUM;
  format->es->video.frame_rate.den = STILLS_FRAME_RATE_DEN;

  if (camera_still_port->buffer_size < camera_still_port->buffer_size_min)
    camera_still_port->buffer_size = camera_still_port->buffer_size_min;

  camera_still_port->buffer_num = camera_still_port->buffer_num_recommended;

  if (mmal_port_format_commit(camera_still_port)) {
    console->error(API_NAME ": Camera still format could not be set.");
    destroy_camera();
    return -1;
  }

  if (mmal_component_enable(camera)) {
    console->error(API_NAME ": Camera component could not be enabled.");
    destroy_camera();
    return -1;
  }

  if (!(encoder_pool = mmal_port_pool_create(
      camera_still_port,
      camera_still_port->buffer_num,
      camera_still_port->buffer_size))) {
    console->error(API_NAME
      ": Failed to create buffer header pool for camera.");
    destroy_camera();
    return -1;
  }

  return 0;
}

int cam_still::create_encoder() {
  if (mmal_component_create(MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER, &encoder)) {
    console->error(API_NAME ": Could not create encoder component.");
    destroy_encoder();
    return -1;
  }
  if (0==encoder->input_num || 0==encoder->output_num) {
    console->error(API_NAME ": Encoder does not have input/output ports.");
    destroy_encoder();
    return -1;
  }

  encoder_input_port = encoder->input[0];
  encoder_output_port = encoder->output[0];

  mmal_format_copy(encoder_output_port->format,
                   encoder_input_port->format);
  encoder_output_port->format->encoding = (encoding); // Set output encoding
  encoder_output_port->buffer_size =
      encoder_output_port->buffer_size_recommended;
  if (encoder_output_port->buffer_size <
      encoder_output_port->buffer_size_min)
    encoder_output_port->buffer_size = encoder_output_port->buffer_size_min;
  encoder_output_port->buffer_num =
      encoder_output_port->buffer_num_recommended;
  if (encoder_output_port->buffer_num < encoder_output_port->buffer_num_min)
    encoder_output_port->buffer_num = encoder_output_port->buffer_num_min;

  if (mmal_port_format_commit(encoder_output_port)) {
    console->error(API_NAME
                   ": Could not set format on encoder output port.");
    destroy_encoder();
    return -1;
  }
  if (mmal_component_enable(encoder)) {
    console->error(API_NAME ": Could not enable encoder component.");
    destroy_encoder();
    return -1;
  }
  if (!(encoder_pool =
          mmal_port_pool_create(
            encoder_output_port,
            encoder_output_port->buffer_num,
            encoder_output_port->buffer_size))) {
    console->error(
      API_NAME
      ": Failed to create buffer header pool for encoder output port.");
    destroy_encoder();
    return -1;
  }
  return 0;
}

void cam_still::destroy_camera() {
  if (camera) {
    mmal_component_destroy(camera);
    camera = NULL;
  }
}

void cam_still::destroy_encoder() {
  if (encoder_pool) {
    mmal_port_pool_destroy(encoder->output[0], encoder_pool);
    encoder_pool = NULL;
  }
  if (encoder) {
    mmal_component_destroy(encoder);
    encoder = NULL;
  }
}

void cam_still::release() {
  if (!is_initialized)
    return;
  mmal_connection_destroy(encoder_connection);
  destroy_encoder();
  destroy_camera();
  console->info(API_NAME ": release called");
   delete [] userdata.data;
  is_initialized = false;
}

int cam_still::initialize() {
  if (is_initialized)
    return 0;
  if (create_camera()) {
    console->error(API_NAME ": Failed to create camera component.");
    destroy_camera();
    return -1;
  } else if (create_encoder()) {
    console->error(API_NAME ": Failed to create encoder component.");
    destroy_camera();
    return -1;
  } else {
    camera_still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];
    encoder_input_port = encoder->input[0];
    encoder_output_port = encoder->output[0];
    if (connectPorts(
            camera_still_port,
            encoder_input_port,
            &encoder_connection) != MMAL_SUCCESS) {
      console->error("ERROR: Could not connect encoder ports!");
      return -1;
    }
  }
  userdata.encoderPool = encoder_pool;
  userdata.length = image_buffer_size();
  userdata.offset = 0;
  userdata.data = new unsigned char[userdata.length];
  is_initialized = true;
  return 0;
}
int cam_still::take_picture() {
  initialize();
  int ret = 0;
  timespec ts = {0, 0};
  encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)userdata;

  return start_capture();
}
int cam_still::start_capture()
{
  // If the parameters were changed and this function wasn't called, it will be called here
  // However if the parameters weren't changed, the function won't do anything - it will return right away
  commit_parameters();

  if (encoder_output_port->is_enabled)
  {
    console->error(API_NAME
                   ": Could not enable encoder output port."
                   " Try waiting longer before attempting to take another picture.");
    return -1;
  }
  if (mmal_port_enable(encoder_output_port, buffer_callback) != MMAL_SUCCESS)
  {
    console->error(API_NAME ": Could not enable encoder output port.");
    return -1;
  }
  int num = mmal_queue_length(encoder_pool->queue);
  for (int b = 0; b < num; b++)
  {
    MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(encoder_pool->queue);

    if (!buffer)
      console->error(
          API_NAME ": Could not get buffer (# {0}) from pool queue.",
          b);

    if (mmal_port_send_buffer(encoder_output_port, buffer) != MMAL_SUCCESS)
      console->error(API_NAME ": Could not send a buffer (# {0}) to encoder output port.",
                     b);
  }
  if (mmal_port_parameter_set_boolean(
          camera_still_port,
          MMAL_PARAMETER_CAPTURE,
          1) != MMAL_SUCCESS) {
    console->error(API_NAME ": Failed to start capture.");
    return -1;
  }
  return 0;
}

void cam_still::stop_capture()
{
  if (!encoder_output_port->is_enabled)
    return;
  mmal_port_disable(encoder_output_port);
}

void cam_still::commitBrightness()
{
  mmal_port_parameter_set_rational(
      camera->control,
      MMAL_PARAMETER_BRIGHTNESS,
      (MMAL_RATIONAL_T){(int32_t)brightness, 100});
}

void cam_still::commitQuality()
{
  if (encoder_output_port != NULL)
    mmal_port_parameter_set_uint32(
        encoder_output_port,
        MMAL_PARAMETER_JPEG_Q_FACTOR,
        quality);
}

void cam_still::commitRotation()
{
  int rotation = int(this->rotation / 90) * 90;
  mmal_port_parameter_set_int32(camera->output[0], MMAL_PARAMETER_ROTATION, rotation);
  mmal_port_parameter_set_int32(camera->output[1], MMAL_PARAMETER_ROTATION, rotation);
  mmal_port_parameter_set_int32(camera->output[2], MMAL_PARAMETER_ROTATION, rotation);
}

void cam_still::commitISO()
{
  if (mmal_port_parameter_set_uint32(
          camera->control,
          MMAL_PARAMETER_ISO,
          iso) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set ISO parameter.");
}

void cam_still::commitSharpness()
{
  if (mmal_port_parameter_set_rational(
          camera->control,
          MMAL_PARAMETER_SHARPNESS,
          (MMAL_RATIONAL_T){sharpness, 100}) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set sharpness parameter.");
}

void cam_still::commitContrast() {
  if (mmal_port_parameter_set_rational(
          camera->control,
          MMAL_PARAMETER_CONTRAST,
          (MMAL_RATIONAL_T){contrast, 100}) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set contrast parameter.");
}

void cam_still::commitSaturation() {
  if (mmal_port_parameter_set_rational(
          camera->control,
          MMAL_PARAMETER_SATURATION,
          (MMAL_RATIONAL_T){saturation, 100}) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set saturation parameter.");
}

void cam_still::commitExposure() {
  MMAL_PARAMETER_EXPOSUREMODE_T exp_mode = {
      {MMAL_PARAMETER_EXPOSURE_MODE, sizeof(exp_mode)},
      (exposure)};
  if (mmal_port_parameter_set(camera->control, &exp_mode.hdr) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set exposure parameter.");
}

void cam_still::commitAWB() {
  MMAL_PARAMETER_AWBMODE_T param = {
      {MMAL_PARAMETER_AWB_MODE, sizeof(param)},
      (awb)};
  if (mmal_port_parameter_set(camera->control, &param.hdr) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set AWB parameter.");
}

void cam_still::commitImageEffect() {
  MMAL_PARAMETER_IMAGEFX_T imgFX = {
      {MMAL_PARAMETER_IMAGE_EFFECT, sizeof(imgFX)},
      (imageEffect)};
  if (mmal_port_parameter_set(camera->control, &imgFX.hdr) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set image effect parameter.");
}

void cam_still::commitMetering() {
  MMAL_PARAMETER_EXPOSUREMETERINGMODE_T meter_mode = {
      {MMAL_PARAMETER_EXP_METERING_MODE, sizeof(meter_mode)},
      (metering)};
  if (mmal_port_parameter_set(camera->control, &meter_mode.hdr) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set metering parameter.");
}

void cam_still::commitFlips() {
  MMAL_PARAMETER_MIRROR_T mirror = {
      {MMAL_PARAMETER_MIRROR, sizeof(MMAL_PARAMETER_MIRROR_T)},
      MMAL_PARAM_MIRROR_NONE};
  if (horizontalFlip && verticalFlip)
    mirror.value = MMAL_PARAM_MIRROR_BOTH;
  else if (horizontalFlip)
    mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
  else if (verticalFlip)
    mirror.value = MMAL_PARAM_MIRROR_VERTICAL;
  if (mmal_port_parameter_set(camera->output[0], &mirror.hdr) != MMAL_SUCCESS ||
      mmal_port_parameter_set(camera->output[1], &mirror.hdr) != MMAL_SUCCESS ||
      mmal_port_parameter_set(camera->output[2], &mirror.hdr) != MMAL_SUCCESS)
    console->error(API_NAME ": Failed to set horizontal/vertical flip parameter.");
}

//Returns an id of the camera. We assume the camera
// id is obtained using raspberry serial number obtained in /proc/cpuinfo
string cam_still::getId() {
  char serial[64];
  serial[0] = '\0';
  ifstream file("/proc/cpuinfo");
  if (file) {
    //read lines until find serial
    bool found = false;
    while (!file.eof() && !found) {
      string str;
      std::getline(file, str);

      if (str.find("Serial") != string::npos) {
        char aux[100];
        if (sscanf(str.c_str(), "%s : %s", aux, serial) != 2){
          console->error("Error parsing /proc/cpuinfo");
        }else{
          found = true;
        }
      }
    }
  } else {
    console->error("Could not read /proc/cpuinfo");
  }

  return serial;
}

bool cam_still::open()
{
  return initialize() == 0;
}
}
#endif
