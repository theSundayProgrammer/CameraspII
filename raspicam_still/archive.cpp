#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <interface/mmal/mmal_encodings.h>
#include <interface/mmal/mmal_types.h>
#include <camerasp/utils.hpp>
#include <json/json.h>
#define VERSION_STRING "v1.3.8"
#include <string>
using std::string;

#include <semaphore.h>

#include "RaspiCLI.h"
#include "RaspiCamControl.h"
// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2


// Stills format information
// 0 implies variable
#define STILLS_FRAME_RATE_NUM 0
#define STILLS_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

#define MAX_USER_EXIF_TAGS      32
#define MAX_EXIF_PAYLOAD_LENGTH 128

int mmal_status_to_int(MMAL_STATUS_T status);
static void signal_handler(int signal_number);


/** Structure containing all state information for the current run
 */
typedef struct
{
   int timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
   unsigned int width;                          /// Requested width of image
   unsigned int height;                         /// requested height of image
   char camera_name[16]; // Name of the camera sensor
   int quality;                        /// JPEG quality setting (1-100)
   int wantRAW;                        /// Flag for whether the JPEG metadata also contains the RAW bayer image
   string filename;                     /// filename of output file
   string linkname;                     /// filename of output file
   int frameStart;                     /// First number of frame output counter
   int verbose;                        /// !0 if want detailed run information
   MMAL_FOURCC_T encoding;             /// Encoding to use for the output file.
   const char *exifTags[MAX_USER_EXIF_TAGS]; /// Array of pointers to tags supplied from the command line
   int enableExifTags;                 /// Enable/Disable EXIF tags in output
   int timelapse;                      /// Delay between each picture in timelapse mode. If 0, disable timelapse
   FRAME_NEXT frame_next_method;                /// Which method to use to advance to next frame
   int settings;                       /// Request settings from the camera
   int cameraNum;                      /// Camera number
   int burstCaptureMode;               /// Enable burst mode
   int sensor_mode;                     /// Sensor mode. 0=auto. Check docs/forum for modes selected by other values.
   int datetime;                       /// Use DateTime instead of frame#
   int timestamp;                      /// Use timestamp instead of frame#
   int restart_interval;               /// JPEG restart interval. 0 for none.

   //RASPIPREVIEW_PARAMETERS preview_parameters;    /// Preview setup parameters
   RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

} RASPISTILL_STATE;

/** Struct used to pass information in encoder port userdata to callback
 */
typedef struct
{
   FILE *file_handle;                   /// File handle to write buffer data to.
   VCOS_SEMAPHORE_T complete_semaphore; /// semaphore which is posted when we reach end of frame (indicates end of capture or fault)
   RASPISTILL_STATE *pstate;            /// pointer to our state in case required in callback
} PORT_USERDATA;

static void display_valid_parameters(const char *app_name);
static void store_exif_tag(RASPISTILL_STATE *state, const char *exif_tag);



static void set_sensor_defaults(RASPISTILL_STATE *state)
{

   // Default to the OV5647 setup
   state->width = 2592;
   state->height = 1944;
   strncpy(state->camera_name, "OV5647", 16);

}
Json::Value make_json(MMAL_PARAM_COLOURFX_T colour_effect)
{
  Json::Value colourfx;
  colourfx["enable"] = colour_effect.enable;
  colourfx["u"] = colour_effect.u;
  colourfx["v"] = colour_effect.v;
  return colourfx;
}
void read_json(MMAL_PARAM_COLOURFX_T& colour_effect,Json::Value& colourfx)
{
   colour_effect.enable= colourfx["enable"].asInt();
   colour_effect.u=colourfx["u"].asInt();
  colour_effect.v= colourfx["v"].asInt();
}
Json::Value make_json(PARAM_FLOAT_RECT_T rect)
{
  Json::Value node;
  node["x"]=rect.x;
  node["y"]=rect.y;
  node["h"]=rect.h;
  node["w"]=rect.w;
  return node;
}
void read_json(PARAM_FLOAT_RECT_T& rect,Json::Value& node)
{
  rect.x=node["x"].asFloat();
  rect.y=node["y"].asFloat();
  rect.h=node["h"].asFloat();
  rect.w=node["w"].asFloat();
}
static void dump_status(RASPISTILL_STATE *state)
{
   int i;

   if (!state)
   {
      return;
   }

   fprintf(stderr, "Width %d, Height %d, quality %d, filename %s\n", state->width,
         state->height, state->quality, state->filename.c_str());
   fprintf(stderr, "Time delay %d, Raw %s\n", state->timeout,
         state->wantRAW ? "yes" : "no");
   fprintf(stderr, "Link to latest frame enabled ");
   if (!state->linkname.empty())
   {
      fprintf(stderr, " yes, -> %s\n", state->linkname.c_str());
   }
   else
   {
      fprintf(stderr, " no\n");
   }

   fprintf(stderr, "Capture method : ");
   fprintf(stderr, "%s", string_mode_from_frame_next(state->frame_next_method));
   fprintf(stderr, "\n\n");

      fprintf(stderr, "EXIF tags disabled\n");

   raspicamcontrol_dump_parameters(&state->camera_parameters);
}
/**
 * Assign a default set of parameters to the state passed in
 *
 * @param state Pointer to state structure to assign defaults to
 */
static void default_status(RASPISTILL_STATE *state)
{
   if (!state)
   {
      return;
   }

   state->timeout = 5000; // 5s delay before take image
   state->quality = 85;
   state->wantRAW = 0;
   state->frameStart = 0;
   state->verbose = 0;
   state->encoding = MMAL_ENCODING_JPEG;
   state->enableExifTags=0;
   state->timelapse = 0;
   state->frame_next_method = FRAME_NEXT_SINGLE;
   state->settings = 0;
   state->cameraNum = 0;
   state->burstCaptureMode=0;
   state->sensor_mode = 0;
   state->datetime = 0;
   state->timestamp = 0;
   state->restart_interval = 0;

   raspicamcontrol_set_defaults(&state->camera_parameters);
}

/*


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

*/

/**
 * Give the supplied parameter block a set of default values
 * @params Pointer to parameter block
 */
void save_camera_control(RASPICAM_CAMERA_PARAMETERS *params, Json::Value& camera_control)
{
   vcos_assert(params);

   camera_control["sharpness"]=params->sharpness ;
   camera_control["contrast"]=params->contrast ;
   camera_control["brightness"]=params->brightness ;
   camera_control["saturation"]=params->saturation ;
   camera_control["ISO"]=params->ISO ;                    // 0 = auto
   camera_control["videoStabilisation"]=params->videoStabilisation ;
   camera_control["exposureCompensation"]=params->exposureCompensation ;
   camera_control["exposureMode"]=string_from_exposure_mode(params->exposureMode) ;
   camera_control["exposureMeterMode"]=string_from_metering_mode(params->exposureMeterMode);
   camera_control["awbMode"]=string_from_awb_mode(params->awbMode);
   camera_control["imageEffect"]=string_mode_from_imagefx(params->imageEffect);
   camera_control["colourEffect"]= make_json(params->colourEffects);
   camera_control["rotation"]=params->rotation ;
   camera_control["hflip"]=params->hflip ;
   camera_control["roi"]=make_json(params->roi);
   camera_control["shutter_speed"]=params->shutter_speed ;          // 0 = auto
   camera_control["awb_gains_r"]=params->awb_gains_r ;      // Only have any function if AWB OFF is used.
   camera_control["awb_gains_b"]=params->awb_gains_b ;
   camera_control["drc_level"]=string_from_drc_mode(params->drc_level);
   camera_control["stats_pass"]=params->stats_pass ;
   camera_control["enable_annotate"]=params->enable_annotate ;
   //camera_control["annotate_string"]=params->annotate_string; //todo
   camera_control["annotate_text_size"]=params->annotate_text_size ;	//Use firmware default
   camera_control["annotate_text_colour"]=params->annotate_text_colour ;   //Use firmware default
   camera_control["annotate_bg_colour"]=params->annotate_bg_colour ;     //Use firmware default
   camera_control["stereo_mode_mode"] = string_from_stereo_mode(params->stereo_mode.mode);
   camera_control["stereo_mode_decimate"] = params->stereo_mode.decimate ;
   camera_control["stereo_mode_swap_eyes"] = params->stereo_mode.swap_eyes ;
}
void read_camera_control(RASPICAM_CAMERA_PARAMETERS *params, Json::Value& camera_control)
{
   vcos_assert(params);

   params->sharpness =camera_control["sharpness"].asInt();
   params->contrast =camera_control["contrast"].asInt();
   params->brightness =camera_control["brightness"].asInt();
   params->saturation =camera_control["saturation"].asInt();
   params->ISO =camera_control["ISO"].asInt();                    // 0 = auto
   params->videoStabilisation =camera_control["videoStabilisation"].asInt();
   params->exposureCompensation =camera_control["exposureCompensation"].asInt();
   params->exposureMode =exposure_mode_from_string(camera_control["exposureMode"].asString().c_str());
   params->exposureMeterMode =metering_mode_from_string(camera_control["exposureMeterMode"].asString().c_str());
   params->awbMode =awb_mode_from_string(camera_control["awbMode"].asString().c_str());
   params->imageEffect =imagefx_mode_from_string(camera_control["imageEffect"].asString().c_str());
   read_json(params->colourEffects,camera_control["colourEffect"]);
   params->rotation =camera_control["rotation"].asInt();
   params->hflip =camera_control["hflip"].asInt();
   read_json(params->roi,camera_control["roi"]);
   params->shutter_speed =camera_control["shutter_speed"].asInt();          // 0 = auto
   params->awb_gains_r =camera_control["awb_gains_r"].asInt();      // Only have any function if AWB OFF is used.
   params->awb_gains_b =camera_control["awb_gains_b"].asInt();
   params->drc_level=drc_mode_from_string(camera_control["drc_level"].asString().c_str());
   params->stats_pass =camera_control["stats_pass"].asInt();
   params->enable_annotate =camera_control["enable_annotate"].asInt();
   //camera_control["annotate_string"]=params->annotate_string; //todo
   params->annotate_text_size =camera_control["annotate_text_size"].asInt();	//Use firmware default
   params->annotate_text_colour =camera_control["annotate_text_colour"].asInt();   //Use firmware default
   params->annotate_bg_colour =camera_control["annotate_bg_colour"].asInt();     //Use firmware default
    params->stereo_mode.mode =stereo_mode_from_string(camera_control["stereo_mode_mode"].asString().c_str());
    params->stereo_mode.decimate =camera_control["stereo_mode_decimate"].asInt() ;
    params->stereo_mode.swap_eyes =camera_control["stereo_mode_swap_eyes"].asInt() ;
}
void save_config(  RASPISTILL_STATE& state,Json::Value& camera_config)
{
  camera_config["timeout"]=state.timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
  camera_config["width"]=state.width;                          /// Requested width of image
  camera_config["height"]=state.height;                         /// requested height of image
  camera_config["quality"]=state.quality;                        /// JPEG quality setting (1-100)
  camera_config["wantRAW"]=state.wantRAW;                        /// Flag for whether the JPEG metadata also contains the RAW bayer image
  if(!state.filename.empty())
    camera_config["filename"]=state.filename;                     /// filename of output file
  if(!state.linkname.empty())
    camera_config["linkname"]=state.linkname;                     /// filename of output file
  camera_config["frameStart"]=state.frameStart;                     /// First number of frame output counter
  camera_config["verbose"]=state.verbose;                        /// !0 if want detailed run information
  camera_config["encoding"]=string_from_img_format(state.encoding);             /// Encoding to use for the output file.
  camera_config["enableExifTags"]=state.enableExifTags;                 /// Enable/Disable EXIF tags in output
  camera_config["timelapse"]=state.timelapse;                      /// Delay between each picture in timelapse mode. If 0, disable timelapse
  camera_config["frame_next_method"]=string_mode_from_frame_next(state.frame_next_method);                /// Which method to use to advance to next frame
  camera_config["settings"]=state.settings;                       /// Request settings from the camera
  camera_config["cameraNum"]=state.cameraNum;                      /// Camera number
  camera_config["burstCaptureMode"]=state.burstCaptureMode;               /// Enable burst mode
  camera_config["sensor_mode"]=state.sensor_mode;                     /// Sensor mode. 0=auto. Check docs/forum for modes selected by other values.
  camera_config["datetime"]=state.datetime;                       /// Use DateTime instead of frame#
  camera_config["timestamp"]=state.timestamp;                      /// Use timestamp instead of frame#
  camera_config["restart_interval"]=state.restart_interval;               /// JPEG restart interval. 0 for none.
}
void read_config(RASPISTILL_STATE& state,  Json::Value& camera_config )
{
  state.timeout=  camera_config["timeout"].asInt();                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
  state.width=  camera_config["width"].asInt();                          /// Requested width of image
  state.height=  camera_config["height"].asInt();                         /// requested height of image
  state.quality=  camera_config["quality"].asInt();                        /// JPEG quality setting (1-100)
  state.wantRAW=  camera_config["wantRAW"].asInt();                        /// Flag for whether the JPEG metadata also contains the RAW bayer image
  state.frameStart=  camera_config["frameStart"].asInt();                     /// First number of frame output counter
  if(camera_config.isMember("linkname"))
    state.linkname=camera_config["linkname"].asString();
  if(camera_config.isMember("filename"))
    state.filename=camera_config["filename"].asString();
  state.verbose=  camera_config["verbose"].asInt();                        /// !0 if want detailed run information
  state.encoding=  img_format_from_string(camera_config["encoding"].asString().c_str());             /// Encoding to use for the output file.
  state.timelapse=  camera_config["timelapse"].asInt();                      /// Delay between each picture in timelapse mode. If 0, disable timelapse
  //state.frame_next_method=  next_frame_description[camera_config["frame_next_method"].asInt()].nextFrameMethod;                /// Which method to use to advance to next frame
  state.settings=  camera_config["settings"].asInt();                       /// Request settings from the camera
  state.cameraNum=  camera_config["cameraNum"].asInt();                      /// Camera number
  state.burstCaptureMode=  camera_config["burstCaptureMode"].asInt();               /// Enable burst mode
  state.sensor_mode=  camera_config["sensor_mode"].asInt();                     /// Sensor mode. 0=auto. Check docs/forum for modes selected by other values.
  state.datetime=  camera_config["datetime"].asInt();                       /// Use DateTime instead of frame#
  state.timestamp=  camera_config["timestamp"].asInt();                      /// Use timestamp instead of frame#
  state.restart_interval=  camera_config["restart_interval"].asInt();               /// JPEG restart interval. 0 for none.
}
/**
 * main
 */
int main(int argc, const char **argv)
{
  // Our main data storage vessel..
  RASPISTILL_STATE state;
  int exit_code = EX_OK;

  MMAL_STATUS_T status = MMAL_SUCCESS;

  default_status(&state);
  Json::Value root = camerasp::get_DOM("options.json");
  auto camera_config = root["Camera"];
  auto camera_control = root["CamControl"];
  read_config(state,camera_config);
  read_camera_control(&state.camera_parameters,camera_control);
  if (state.verbose)
  {
    fprintf(stderr, "\n%s Camera App %s\n\n", basename(argv[0]), VERSION_STRING);

    dump_status(&state);
  }
   save_camera_control(&state.camera_parameters,camera_control);
	Json::StyledWriter writer; 
	root["CamControl"] = camera_control;
	auto update = writer.write(root);
	camerasp::write_file_content("options.json",update);
  return exit_code;
}


// raspicam.bin -v -tl 1000 -o img_%04d.jpg -h 480 -w 640 -t 36000000 -x none > err.txt
