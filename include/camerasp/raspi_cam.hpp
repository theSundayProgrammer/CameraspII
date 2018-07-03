//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// 
// http://www.boost.org/LICENSE_1_0.txt)
// This header was inspired by raspicam 
// http://github.com/cedric/raspicam
//////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef CAMERASP_CAM_STILL_HPP
#define CAMERASP_CAM_STILL_HPP
#include "cam_still_base.hpp"
#include <string>
#include <atomic>
#include <mmal/mmal.h>
#include <semaphore.h>
#include <boost/signals2/signal.hpp>
#include <jpeg/jpgconvert.hpp>
struct MMAL_COMPONENT_T;
struct MMAL_CONNECTION_T;
struct MMAL_POOL_T;
struct MMAL_PORT_T;
#define MMAL_CAMERA_CAPTURE_PORT 2
#define STILLS_FRAME_RATE_NUM 3
#define STILLS_FRAME_RATE_DEN 1
namespace asio 
{
 class io_context;
}
namespace camerasp {

  typedef void(*imageTakenCallback) (unsigned char * data, int error, unsigned int length);
/**
 * \brief Used in call back function
 *
 * The MMAL library does a call back on data ready
 * This structure is passed on as a void pointer
 * to the callback without
 */
struct RASPICAM_USERDATA
{
  MMAL_POOL_T *encoderPool; //< MMAL stuff
  sem_t* data_ready; //< semaphore that is raised when data is ready
  unsigned char *data; //< data buffer used to collect image
  unsigned int offset; //< pointer to first free location in data
  unsigned int length; //< total length of data
};
/**
 * \brief Uses MMAL to access camera
 *
 * This camera object uses Boost Signals/Slots library.
 * On launch a separate thread is created that waits for 
 * a semaphore and when it arrives all the connected slots
 * are signalled. When take_pictuure is called it triggers
 * a frame capture without waiting for completion.
 * Needs to be a singleton..
 */
class cam_still:public cam_still_base {

  private:
    sem_t data_ready;

    MMAL_COMPONENT_T * camera;	
    MMAL_COMPONENT_T * encoder;
    MMAL_CONNECTION_T * encoder_connection; 
    MMAL_POOL_T * encoder_pool;		
    MMAL_PORT_T * camera_still_port;
    MMAL_PORT_T * encoder_input_port;
    MMAL_PORT_T * encoder_output_port;

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

    bool is_initialized;
    bool stop_capture_flag;
    boost::signals2::signal<void(const img_info&)> on_image_capture;
    RASPICAM_USERDATA userdata;
    MMAL_FOURCC_T encoding;
    MMAL_PARAM_EXPOSUREMETERINGMODE_T metering;
    MMAL_PARAM_EXPOSUREMODE_T exposure;
    MMAL_PARAM_AWBMODE_T awb;
    MMAL_PARAM_IMAGEFX_T imageEffect;
    asio::io_context& io_service;

  public:
    ~cam_still();
    cam_still(asio::io_context& );
    cam_still(const cam_still&) = delete;
    cam_still(const cam_still&&) = delete;
    cam_still& operator=(const cam_still&) = delete;
    cam_still& operator=(const cam_still&&) = delete;
    int initialize();
    void release();
    bool open();
    int take_picture();
    void stop_capture();
    void commit_parameters();
    void await_data_ready();
    void stop_data_wait();


    void connect(std::function<void(const img_info&)> slot)
    {
      on_image_capture.connect(slot);
    }
    ///Returns an id of the camera. We assume the camera id is the one of the raspberry
    ///the id is obtained using raspberry serial number obtained in /proc/cpuinfo
    static std::string getId();

};

}
#endif // RASPICAM_H
