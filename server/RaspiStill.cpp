/*
Copyright (c) 2017 Joseph Maraidassou (theSundayProgrammer.com)
The original copyright follows 
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

// We use some GNU extensions (asprintf, basename)
//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <gsl/gsl_util>
#include <iterator>
#include <string>
#include <fstream>
#define VERSION_STRING "v1.3.8"

#include "bcm_host.h"
#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/mmal/mmal_parameters_camera.h"


#include <raspi/RaspiCamControl.h>
#include <raspi/RaspiPreview.h>
#include <raspi/RaspiStill.h>
#include <raspi/RaspiCLI.h>
#include <semaphore.h>
#include <camerasp/ipc.hpp>
#include <camerasp/smtpClient.hpp>

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2
static const char *app_name="RaspiStill";

// Stills format information
// 0 implies variable
#define STILLS_FRAME_RATE_NUM 0
#define STILLS_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3


int mmal_status_to_int(MMAL_STATUS_T status);
void signal_handler(int signal_number);

class Mmal_exception {
public:
  Mmal_exception(MMAL_STATUS_T status_):status(status_){}
  MMAL_STATUS_T status;
}; 
class Error_handler
{
public:
  Error_handler(const char* str_, bool throw_exception_):
  throw_exception(throw_exception_),
  str(str_){}
  Error_handler& operator=(MMAL_STATUS_T status)
{
   if (status != MMAL_SUCCESS)
   {
      vcos_log_error(str);
      if(throw_exception)
        throw Mmal_exception(status);
   }
  return *this;
}
private:
  bool throw_exception;
  const char* str;
};
Error_handler Error(const char* str,bool throw_exception=true)
{
  return Error_handler(str,throw_exception);
}

/** Struct used to pass information in encoder port userdata to callback
 */
typedef struct
{
   FILE *file_handle;                   /// File handle to write buffer data to.
   VCOS_SEMAPHORE_T complete_semaphore; /// semaphore which is posted when we reach end of frame (indicates end of capture or fault)
   RASPISTILL_STATE *pstate;            /// pointer to our state in case required in callback
} PORT_USERDATA;

void display_valid_parameters(const char *app_name);

static void get_sensor_defaults(shared_state *state)
{
   MMAL_COMPONENT_T *camera_info;
   MMAL_STATUS_T status;


   // Try to get the camera name and maximum supported resolution
     Error("Failed to create camera_info component")= mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA_INFO, &camera_info);
     auto del_acm_info= gsl::finally ( [=] () {
                       mmal_component_destroy(camera_info);
                        });
      MMAL_PARAMETER_CAMERA_INFO_T param;
      param.hdr.id = MMAL_PARAMETER_CAMERA_INFO;
      param.hdr.size = sizeof(param)-4;  // Deliberately undersize to check firmware veresion
      status = mmal_port_parameter_get(camera_info->control, &param.hdr);

      if (status != MMAL_SUCCESS)
      {
         // Running on newer firmware
         param.hdr.size = sizeof(param);
         status = mmal_port_parameter_get(camera_info->control, &param.hdr);
         if (status == MMAL_SUCCESS && param.num_cameras > 0)
         {
            // Take the parameters from the first camera listed.
            state->max_width=state->width  = param.cameras[0].max_width;
            state->max_height =state->height = param.cameras[0].max_height;
            strncpy(state->camera_name,  param.cameras[0].camera_name, MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN);
            state->camera_name[MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN-1] = 0;
         }
         else
            vcos_log_error("Cannot read camera info, keeping the defaults for OV5647");
      }
      else
      {
         // Older firmware
         // Nothing to do here, keep the defaults for OV5647
      }

}

/**
 * Assign a default set of parameters to the state passed in
 *
 * @param state Pointer to state structure to assign defaults to
 */
static void default_status(shared_state& state)
{

   state.quality = 85;
   state.keep_running = 1;
   state.wantRAW = 0;
   state.frame= 0;
   state.verbose = 0;
   state.encoding = MMAL_ENCODING_JPEG;
   state.enableExifTags = 0;
   state.timelapse = 0;
   state.settings = 0;
   state.cameraNum = 0;
   state.sensor_mode = 0;
   state.restart_interval = 0;

   // Default to the OV5647 setup
   state.max_width=state.width  = 2592;
   state.max_height =state.height =1944;
   strncpy(state.camera_name, "OV5647", MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN);
   // Set up the camera_parameters to default
   RaspiCamControl raspicamcontrol;
   raspicamcontrol.set_defaults(&state.camera_parameters);



}

/**
 * Dump image state parameters to stderr. Used for debugging
 *
 * @param state Pointer to state structure to assign defaults to
 */
static void dump_status(RASPISTILL_STATE *state)
{

   if (!state)
   {
      vcos_assert(0);
      return;
   }

   fprintf(stderr, "Width %d, Height %d, quality %d, filename %s\n", state->state->width,
         state->state->height, state->state->quality, state->state->filename);
   fprintf(stderr, " Raw %s\n", 
         state->state->wantRAW ? "yes" : "no");
   fprintf(stderr, "Link to latest frame enabled ");
    fprintf(stderr, " no\n");


   raspicamcontrol_dump_parameters(&state->state->camera_parameters);
}


/**
 *  buffer header callback function for camera control
 *
 *  No actions taken in current version
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED)
   {
      MMAL_EVENT_PARAMETER_CHANGED_T *param = (MMAL_EVENT_PARAMETER_CHANGED_T *)buffer->data;
      switch (param->hdr.id) {
         case MMAL_PARAMETER_CAMERA_SETTINGS:
         {
            MMAL_PARAMETER_CAMERA_SETTINGS_T *settings = (MMAL_PARAMETER_CAMERA_SETTINGS_T*)param;
            vcos_log_error("Exposure now %u, analog gain %u/%u, digital gain %u/%u",
			settings->exposure,
                        settings->analog_gain.num, settings->analog_gain.den,
                        settings->digital_gain.num, settings->digital_gain.den);
            vcos_log_error("AWB R=%u/%u, B=%u/%u",
                        settings->awb_red_gain.num, settings->awb_red_gain.den,
                        settings->awb_blue_gain.num, settings->awb_blue_gain.den
                        );
         }
         break;
      }
   }
   else if (buffer->cmd == MMAL_EVENT_ERROR)
   {
      vcos_log_error("No data received from sensor. Check all connections, including the Sunny one on the camera board");
   }
   else
      vcos_log_error("Received unexpected camera control callback event, 0x%08x", buffer->cmd);

   mmal_buffer_header_release(buffer);
}

/**
 *  buffer header callback function for encoder
 *
 *  Callback will dump buffer data to the specific file
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void encoder_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   int complete = 0;

   // We pass our file handle and other stuff in via the userdata field.

   PORT_USERDATA *pData = (PORT_USERDATA *)port->userdata;

   if (pData)
   {
      unsigned int bytes_written = buffer->length;

      if (buffer->length && pData->file_handle)
      {
         mmal_buffer_header_mem_lock(buffer);

         bytes_written = fwrite(buffer->data, 1, buffer->length, pData->file_handle);

         mmal_buffer_header_mem_unlock(buffer);
      }

      // We need to check we wrote what we wanted - it's possible we have run out of storage.
      if (bytes_written != buffer->length)
      {
         vcos_log_error("Unable to write buffer to file - aborting");
         complete = 1;
      }

      // Now flag if we have completed
      if (buffer->flags & (MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED))
         complete = 1;
   }
   else
   {
      vcos_log_error("Received a encoder buffer callback with no state");
   }

   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
      MMAL_STATUS_T status = MMAL_SUCCESS;
      MMAL_BUFFER_HEADER_T *new_buffer;

      new_buffer = mmal_queue_get(pData->pstate->encoder_pool->queue);

      if (new_buffer)
      {
         status = mmal_port_send_buffer(port, new_buffer);
      }
      if (!new_buffer || status != MMAL_SUCCESS)
         vcos_log_error("Unable to return a buffer to the encoder port");
   }

   if (complete)
      vcos_semaphore_post(&(pData->complete_semaphore));
}

/**
 * Create the camera component, set up its ports
 *
 * @param state Pointer to state control struct. camera_component member set to the created camera_component if successful.
 *
 * @return MMAL_SUCCESS if all OK, something else otherwise
 *
 */
static MMAL_STATUS_T create_camera_component(RASPISTILL_STATE *state)
{
   MMAL_COMPONENT_T *camera = 0;
   MMAL_ES_FORMAT_T *format;
   MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
   int err_status=0;
   MMAL_STATUS_T status = MMAL_SUCCESS;
   MMAL_PARAMETER_INT32_T camera_num =
      {{MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, state->state->cameraNum};
   /* Create the component */
 try{
   Error("Failed to create camera component")= mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);


   RaspiCamControl raspicamcontrol;
   err_status = raspicamcontrol.set_stereo_mode(camera->output[0], &state->state->camera_parameters.stereo_mode);
   err_status += raspicamcontrol.set_stereo_mode(camera->output[1], &state->state->camera_parameters.stereo_mode);
   err_status += raspicamcontrol.set_stereo_mode(camera->output[2], &state->state->camera_parameters.stereo_mode);

   Error("Could not set stereo mode ")= static_cast<MMAL_STATUS_T>(err_status);

   Error("Could not select camera ")=  mmal_port_parameter_set(camera->control, &camera_num.hdr);

   Error("Camera doesn't have output ports") = camera->output_num == 0 ?  MMAL_ENOSYS : MMAL_SUCCESS;

   Error("Could not set sensor mode ")= mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, state->state->sensor_mode);

   preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
   video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
   still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

   if (state->state->settings)
   {
      MMAL_PARAMETER_CHANGE_EVENT_REQUEST_T change_event_request =
         {{MMAL_PARAMETER_CHANGE_EVENT_REQUEST, sizeof(MMAL_PARAMETER_CHANGE_EVENT_REQUEST_T)},
          MMAL_PARAMETER_CAMERA_SETTINGS, 1};

      Error("No camera settings events",false)= mmal_port_parameter_set(camera->control, &change_event_request.hdr);
   }

   // Enable the camera, and tell it its control callback function

      Error("Unable to enable control port")= mmal_port_enable(camera->control, camera_control_callback);

   //  set up the camera configuration
   {
      unsigned int h = 480;
      unsigned int w = 640;
      MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
      {
         { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
         .max_stills_w = state->state->width,
         .max_stills_h = state->state->height,
         .stills_yuv422 = 0,
         .one_shot_stills = 1,
         .max_preview_video_w = w,
         .max_preview_video_h = h,
         .num_preview_video_frames = 3,
         .stills_capture_circular_buffer_height = 0,
         .fast_preview_resume = 0,
         .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
      };


      mmal_port_parameter_set(camera->control, &cam_config.hdr);
   }

   raspicamcontrol.set_all_parameters(camera, &state->state->camera_parameters);

   // Now set up the port formats

   format = preview_port->format;
   format->encoding = MMAL_ENCODING_OPAQUE;
   format->encoding_variant = MMAL_ENCODING_I420;

   if(state->state->camera_parameters.shutter_speed > 6000*1000)
   {
        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
                                                     { 50, 1000 }, {166, 1000}};
        mmal_port_parameter_set(preview_port, &fps_range.hdr);
   }
   else if(state->state->camera_parameters.shutter_speed > 1000*1000)
   {
        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
                                                     { 166, 1000 }, {999, 1000}};
        mmal_port_parameter_set(preview_port, &fps_range.hdr);
   }

  Error("camera viewfinder format couldn't be set")= mmal_port_format_commit(preview_port);

   // Set the same format on the video  port (which we don't use here)
   mmal_format_full_copy(video_port->format, format);

      Error("camera video format couldn't be set")= mmal_port_format_commit(video_port);
   

   // Ensure there are enough buffers to avoid dropping frames
   if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

   format = still_port->format;

   if(state->state->camera_parameters.shutter_speed > 6000000)
   {
        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
                                                     { 50, 1000 }, {166, 1000}};
        mmal_port_parameter_set(still_port, &fps_range.hdr);
   }
   else if(state->state->camera_parameters.shutter_speed > 1000000)
   {
        MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
                                                     { 167, 1000 }, {999, 1000}};
        mmal_port_parameter_set(still_port, &fps_range.hdr);
   }
   // Set our stills format on the stills (for encoder) port
   format->encoding = MMAL_ENCODING_OPAQUE;
   format->es->video.width = VCOS_ALIGN_UP(state->state->width, 32);
   format->es->video.height = VCOS_ALIGN_UP(state->state->height, 16);
   format->es->video.crop.x = 0;
   format->es->video.crop.y = 0;
   format->es->video.crop.width = state->state->width;
   format->es->video.crop.height = state->state->height;
   format->es->video.frame_rate.num = STILLS_FRAME_RATE_NUM;
   format->es->video.frame_rate.den = STILLS_FRAME_RATE_DEN;


      Error("camera still format couldn't be set")= mmal_port_format_commit(still_port);

   /* Ensure there are enough buffers to avoid dropping frames */
   if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

   /* Enable component */

      Error("camera component couldn't be enabled")= mmal_component_enable(camera);



   state->camera_component = camera;

   if (state->state->verbose)
      fprintf(stderr, "Camera component done\n");

  status = MMAL_SUCCESS;
}
catch(Mmal_exception & e)
{
   if (camera)
      mmal_component_destroy(camera);
   status = e.status;
}
   return status;
}

/**
 * Destroy the camera component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_camera_component(RASPISTILL_STATE *state)
{
   if (state->camera_component)
   {
      mmal_component_destroy(state->camera_component);
      state->camera_component = NULL;
   }
}

/**
 * Create the encoder component, set up its ports
 *
 * @param state Pointer to state control struct. encoder_component member set to the created camera_component if successful.
 *
 * @return a MMAL_STATUS, MMAL_SUCCESS if all OK, something else otherwise
 */
static MMAL_STATUS_T create_encoder_component(RASPISTILL_STATE *state)
{
   MMAL_COMPONENT_T *encoder = 0;
   MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
   MMAL_POOL_T *pool;
 try {
      Error("Unable to create JPEG encoder component")= mmal_component_create(MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER, &encoder);

      Error("JPEG encoder doesn't have input/output ports") =
                                 (!encoder->input_num || !encoder->output_num)? 
                                 MMAL_ENOSYS:
                                 MMAL_SUCCESS;

   encoder_input = encoder->input[0];
   encoder_output = encoder->output[0];

   // We want same format on input and output
   mmal_format_copy(encoder_output->format, encoder_input->format);

   // Specify out output format
   encoder_output->format->encoding = state->state->encoding;

   encoder_output->buffer_size = encoder_output->buffer_size_recommended;

   if (encoder_output->buffer_size < encoder_output->buffer_size_min)
      encoder_output->buffer_size = encoder_output->buffer_size_min;

   encoder_output->buffer_num = encoder_output->buffer_num_recommended;

   if (encoder_output->buffer_num < encoder_output->buffer_num_min)
      encoder_output->buffer_num = encoder_output->buffer_num_min;

   // Commit the port changes to the output port

      Error("Unable to set format on video encoder output port")= mmal_port_format_commit(encoder_output);

   // Set the JPEG quality level

      Error("Unable to set JPEG quality")= mmal_port_parameter_set_uint32(encoder_output, MMAL_PARAMETER_JPEG_Q_FACTOR, state->state->quality);

   // Set the JPEG restart interval

      Error("Unable to set JPEG restart interval")= mmal_port_parameter_set_uint32(encoder_output, MMAL_PARAMETER_JPEG_RESTART_INTERVAL, state->state->restart_interval);

   // Set up any required thumbnail
   {
      MMAL_PARAMETER_THUMBNAIL_CONFIG_T param_thumb = {{MMAL_PARAMETER_THUMBNAIL_CONFIGURATION, sizeof(MMAL_PARAMETER_THUMBNAIL_CONFIG_T)}, 0, 0, 0, 0};

      mmal_port_parameter_set(encoder->control, &param_thumb.hdr);
   }

   //  Enable component

      Error("Unable to enable video encoder component")= mmal_component_enable(encoder);

   /* Create pool of buffer headers for the output port to consume */
   pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);

   if (!pool)
   {
      vcos_log_error("Failed to create buffer header pool for encoder output port %s", encoder_output->name);
   }

   state->encoder_pool = pool;
   state->encoder_component = encoder;

   if (state->state->verbose)
      fprintf(stderr, "Encoder component done\n");

   return MMAL_SUCCESS;
   }
   catch(Mmal_exception & e){
   if (encoder)
      mmal_component_destroy(encoder);

   return e.status;
  }
}

/**
 * Destroy the encoder component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_encoder_component(RASPISTILL_STATE *state)
{
   // Get rid of any port buffers first
   if (state->encoder_pool)
   {
      mmal_port_pool_destroy(state->encoder_component->output[0], state->encoder_pool);
   }

   if (state->encoder_component)
   {
      mmal_component_destroy(state->encoder_component);
      state->encoder_component = NULL;
   }
}


/**
 * Connect two specific ports together
 *
 * @param output_port Pointer the output port
 * @param input_port Pointer the input port
 * @param Pointer to a mmal connection pointer, reassigned if function successful
 * @return Returns a MMAL_STATUS_T giving result of operation
 *
 */
static MMAL_STATUS_T connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection)
{
   MMAL_STATUS_T status;

   status =  mmal_connection_create(connection, output_port, input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);

   if (status == MMAL_SUCCESS)
   {
      status =  mmal_connection_enable(*connection);
      if (status != MMAL_SUCCESS)
         mmal_connection_destroy(*connection);
   }

   return status;
}


/**
 * Allocates and generates a filename based on the
 * user-supplied pattern and the frame number.
 * On successful return, finalName and tempName point to malloc()ed strings
 * which must be freed externally.  (On failure, returns nulls that
 * don't need free()ing.)
 *
 * @param finalName pointer receives an
 * @param pattern sprintf pattern with %d to be replaced by frame
 * @param frame for timelapse, the frame number
 * @return Returns a MMAL_STATUS_T giving result of operation
*/

MMAL_STATUS_T create_filenames(char** finalName, char** tempName, char * pattern, int frame)
{
   *finalName = NULL;
   *tempName = NULL;
   if (0 > asprintf(finalName, pattern, frame%10000) ||
       0 > asprintf(tempName, "%s~", *finalName))
   {
      if (*finalName != NULL)
      {
         free(*finalName);
      }
      return MMAL_ENOMEM;    // It may be some other error, but it is not worth getting it right
   }
   return MMAL_SUCCESS;
}

/**
 * Checks if specified port is valid and enabled, then disables it
 *
 * @param port  Pointer the port
 *
 */
static void check_disable_port(MMAL_PORT_T *port)
{
   if (port && port->is_enabled)
      mmal_port_disable(port);
}
static shared_state* p_state=NULL;
/**
 * Handler for sigint signals
 *
 * @param signal_number ID of incoming signal.
 *
 */
void stop_handler(int signal_number)
{
   if (signal_number == SIGUSR1)
   {
       p_state->keep_running = 0;
   }
}
static void rename_file(const char *final_filename, const char *use_filename)
{

   vcos_assert(use_filename != NULL && final_filename != NULL);
   if (0 != rename(use_filename, final_filename))
   {
      vcos_log_error("Could not rename temp file to: %s; %s",
            final_filename,strerror(errno));
   }
}
void initialise_state( RASPISTILL_STATE& state)
{
  
   // Register our application with the logging system
   vcos_log_register("RaspiStill", VCOS_LOG_CATEGORY);

   signal(SIGINT, signal_handler);

   // Disable USR1 for the moment - may be reenabled if go in to signal capture mode
   signal(SIGUSR1, stop_handler);
   default_status(*state.state);

   state.camera_component = NULL;
   state.encoder_component = NULL;
   state.preview_connection = NULL;
   state.encoder_connection = NULL;
   state.encoder_pool = NULL;
   // Setup preview window defaults
   raspipreview_set_defaults(&state.preview_parameters);
   // Setup for sensor specific parameters
   get_sensor_defaults(state.state);

   // Parse the command line and put options in to our status structure
   if (parse_cmdline(state.state))
   {
      exit(EX_USAGE);
   }


}
int capture_pic(RASPISTILL_STATE& state,PORT_USERDATA& callback_data)
{
  MMAL_STATUS_T status = MMAL_SUCCESS;
  int num, q;

  MMAL_PORT_T *camera_still_port = state.camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
  MMAL_PORT_T *encoder_output_port = state.encoder_component->output[0];
  mmal_port_parameter_set_boolean(
      state.encoder_component->output[0], MMAL_PARAMETER_EXIF_DISABLE, 1);

  // Same with raw, apparently need to set it for each capture, whilst port
  // is not enabled
  if (state.state->wantRAW)
  {
    if (mmal_port_parameter_set_boolean(camera_still_port, MMAL_PARAMETER_ENABLE_RAW_CAPTURE, 1) != MMAL_SUCCESS)
    {
      vcos_log_error("RAW was requested, but failed to enable");
    }
  }

  // There is a possibility that shutter needs to be set each loop.
  status = mmal_port_parameter_set_uint32(state.camera_component->control, MMAL_PARAMETER_SHUTTER_SPEED, state.state->camera_parameters.shutter_speed); 
  if (mmal_status_to_int(status)!= MMAL_SUCCESS)
    vcos_log_error("Unable to set shutter speed");


  // Enable the encoder output port
  encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;

  if (state.state->verbose)
    fprintf(stderr, "Enabling encoder output port\n");

  // Enable the encoder output port and tell it its callback function
  status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);

  // Send all the buffers to the encoder output port
  num = mmal_queue_length(state.encoder_pool->queue);

  for (q=0;q<num;q++)
  {
    MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state.encoder_pool->queue);

    if (!buffer)
      vcos_log_error("Unable to get a required buffer %d from pool queue", q);

    if (mmal_port_send_buffer(encoder_output_port, buffer)!= MMAL_SUCCESS)
      vcos_log_error("Unable to send a buffer to encoder output port (%d)", q);
  }



  if (mmal_port_parameter_set_boolean(camera_still_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
  {
    vcos_log_error("%s: Failed to start capture", __func__);
  }
  else
  {
    // Wait for capture to complete
    // For some reason using vcos_semaphore_wait_timeout sometimes returns immediately with bad parameter error
    // even though it appears to be all correct, so reverting to untimed one until figure out why its erratic
    vcos_semaphore_wait(&callback_data.complete_semaphore);
  }

  // Ensure we don't die if get callback with no open file
  callback_data.file_handle = NULL;

  // Disable encoder output port
  status = mmal_port_disable(encoder_output_port);
  return status;
}
/**
 * main
 */
const char* shared_mem_name = "RaspiStillSharedMem2";
int main(int argc, const char **argv)
{
  namespace ipc = boost::interprocess;
  // Our main data storage vessel..
  RASPISTILL_STATE state;
  int exit_code = EX_OK;
   shared_mem_ptr<shared_state> mem(shared_mem_name,false);
  p_state = &*mem;
  state.state= p_state ;
  MMAL_STATUS_T status = MMAL_SUCCESS;
  MMAL_PORT_T *camera_preview_port = NULL;
  MMAL_PORT_T *camera_video_port = NULL;
  MMAL_PORT_T *preview_input_port = NULL;
  MMAL_PORT_T *encoder_input_port = NULL;
  bcm_host_init();
#define TO      "<theSundayProgrammer@gmail.com>"
#define CC      "<joseph.mariadassou@outlook.com>"
  smtp_client smtp;
  smtp.password = p_state->password_smtp;
  fprintf(stderr,"pwd=%s\n",smtp.password.c_str());
  if(smtp.password.length()==0)
    return -1;
  smtp.user= "joe.mariadassou@gmail.com";
  smtp.url="smtp://smtp.gmail.com:587";
  smtp.from = "<joe.mariadassou@gmail.com> Chakra";
  smtp.subject = "Test Message";
   smtp.recipient_ids.push_back(TO);
   smtp.recipient_ids.push_back(CC);
   do {
     initialise_state(state);

     if (state.state->verbose)
     {
       fprintf(stderr, "\n%s Camera App %s\n\n", app_name, VERSION_STRING);

       dump_status(&state);
     }
     // OK, we have a nice set of parameters. Now set up our components
     // We have three components. Camera, Preview and encoder.
     // Camera and encoder are different in stills/video, but preview
     // is the same so handed off to a separate module
     status = create_camera_component(&state);
     if (status!= MMAL_SUCCESS)
     {
       vcos_log_error("%s: Failed to create camera component", __func__);
       exit_code = EX_SOFTWARE;
       break;
     }
     if ( (status = raspipreview_create(&state.preview_parameters)) != MMAL_SUCCESS)
     {
       vcos_log_error("%s: Failed to create preview component", __func__);
       exit_code = EX_SOFTWARE;
       break;
     }
     if ((status = create_encoder_component(&state)) != MMAL_SUCCESS)
     {
       vcos_log_error("%s: Failed to create encode component", __func__);
       exit_code = EX_SOFTWARE;
       break;
     }

     if (state.state->verbose)
       fprintf(stderr, "Starting component connection stage\n");

     camera_preview_port = state.camera_component->output[MMAL_CAMERA_PREVIEW_PORT];
     camera_video_port   = state.camera_component->output[MMAL_CAMERA_VIDEO_PORT];
     MMAL_PORT_T *camera_still_port = state.camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
     encoder_input_port  = state.encoder_component->input[0];
     preview_input_port  = state.preview_parameters.preview_component->input[0];

     if (state.state->verbose)
       fprintf(stderr, "Connecting camera preview port to video render.\n");


     // Connect camera to preview (which might be a null_sink if no preview required)
     status = connect_ports(camera_preview_port, preview_input_port, &state.preview_connection);

     if (status != MMAL_SUCCESS)
     {
       mmal_status_to_int(status);
       vcos_log_error("%s: Failed to connect camera to preview", __func__);
       break;
     }

     if (state.state->verbose)
       fprintf(stderr, "Connecting camera stills port to encoder input port\n");

     // Now connect the camera to the encoder
     status = connect_ports(camera_still_port, encoder_input_port, &state.encoder_connection);

     if (status != MMAL_SUCCESS)
     {
       vcos_log_error("%s: Failed to connect camera video port to encoder input", __func__);
       break;
     }

     // Set up our userdata - this is passed though to the callback where we need the information.
     // Null until we open our filename
     PORT_USERDATA callback_data;
     callback_data.file_handle = NULL;
     callback_data.pstate = &state;
     VCOS_STATUS_T vcos_status;
     vcos_status = vcos_semaphore_create(&callback_data.complete_semaphore, "RaspiStill-sem", 0);

     vcos_assert(vcos_status == VCOS_SUCCESS);




     while (state.state->keep_running) 
     {
       FILE *output_file = NULL;
       char *use_filename = NULL;      // Temporary filename while image being written
       char *final_filename = NULL;    // Name that file gets once writing complete

       state.state->frame+=1;
       state.state->frame %= 10000;
       // Have a sleep so we don't hog the CPU.
       vcos_sleep(1000);

       // Open the file
       {
         vcos_assert(use_filename == NULL && final_filename == NULL);
         status = create_filenames(&final_filename, &use_filename, state.state->filename, state.state->frame);
         if (status  != MMAL_SUCCESS)
         {
           vcos_log_error("Unable to create filenames");
           break;
         }

         if (state.state->verbose)
           fprintf(stderr, "Opening output file %s\n", final_filename);
         // it is opening the temp~ filename which will be ranamed to the final filename

         output_file = fopen(use_filename, "wb");


       }

       // We only capture if a filename was specified and it opened
       if (output_file)
       {
         if (state.state->verbose)
           fprintf(stderr, "Starting capture %d\n", state.state->frame);
         callback_data.file_handle = output_file;
         capture_pic(state,callback_data);
         fclose(output_file);
         if (state.state->verbose)
           fprintf(stderr, "Finished capture %d\n", state.state->frame);
         rename_file(  final_filename, use_filename);
         fprintf(stderr,"file name = %s\n", final_filename);
         handle_motion(final_filename, smtp);
       }
       else
       {
         // Notify user, carry on but discarding encoded output buffers
         vcos_log_error("%s: Error opening output file: %s\nNo output file will be generated\n", __func__, use_filename);
       }
       if (use_filename)
       {
         free(use_filename);
       }
       if (final_filename)
       {
         free(final_filename);
       }
     } 

     vcos_semaphore_delete(&callback_data.complete_semaphore);

   }while(0);
  mmal_status_to_int(status);

  if (state.state->verbose)
    fprintf(stderr, "Closing down\n");

  // Disable all our ports that are not handled by connections
  check_disable_port(camera_video_port);

  if (state.preview_connection)
    mmal_connection_destroy(state.preview_connection);

  if (state.encoder_connection)
    mmal_connection_destroy(state.encoder_connection);


  /* Disable components */
  if (state.encoder_component)
    mmal_component_disable(state.encoder_component);

  if (state.preview_parameters.preview_component)
    mmal_component_disable(state.preview_parameters.preview_component);

  if (state.camera_component)
    mmal_component_disable(state.camera_component);

  destroy_encoder_component(&state);
  raspipreview_destroy(&state.preview_parameters);
  destroy_camera_component(&state);

  if (state.state->verbose)
    fprintf(stderr, "Close down completed, all components disconnected, disabled and destroyed\n\n");


  if (status != MMAL_SUCCESS)
    raspicamcontrol_check_configuration(128);

  return exit_code;
}

