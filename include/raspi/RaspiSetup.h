#pragma once
#ifndef RASPISTILL_RASPISETUP_H
#define RASPISTILL_RASPISETUP_H
#include "RaspiCamControlShared.hpp"
enum some_constants
{
   CAMERA_NAME_LENGTH= 128
};
struct shared_state
{
   unsigned int max_width;                          /// Max width of image
   unsigned int max_height;                         /// Max height of image
   unsigned int width;                          /// Requested width of image
   unsigned int height;                         /// requested height of image
   char camera_name[CAMERA_NAME_LENGTH]; // Name of the camera sensor
   int quality;                        /// JPEG quality setting (1-100)
   int wantRAW;                        /// Flag for whether the JPEG metadata also contains the RAW bayer image
   char filename[128];                     /// filename of output file
   int frame;                     /// First number of frame output counter
   int verbose;                        /// !0 if want detailed run information
   MMAL_FOURCC_T encoding;             /// Encoding to use for the output file.
   int enableExifTags;                 /// Enable/Disable EXIF tags in output
   int timelapse;                      /// Delay between each picture in timelapse mode. If 0, disable timelapse
   int settings;                       /// Request settings from the camera
   int cameraNum;                      /// Camera number
   int sensor_mode;                     /// Sensor mode. 0=auto. Check docs/forum for modes selected by other values.
   int restart_interval;               /// JPEG restart interval. 0 for none.
   int keep_running;
   int max_frames;
   char password_smtp[24];
   RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters
} ;

int parse_cmdline(shared_state *state);
#endif
