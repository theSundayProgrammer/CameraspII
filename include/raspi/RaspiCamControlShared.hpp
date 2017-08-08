#ifndef RASPICAMCONTROL_SHARED_MEM
#define RASPICAMCONTROL_SHARED_MEM
#include <iterator>
#include <string>
#include <fstream>
#include <interface/mmal/mmal_component.h>
/// Annotate bitmask options
/// Supplied by user on command line
#define ANNOTATE_USER_TEXT          1
/// Supplied by app using this module
#define ANNOTATE_APP_TEXT           2
/// Insert current date
#define ANNOTATE_DATE_TEXT          4
// Insert current time
#define ANNOTATE_TIME_TEXT          8

#define ANNOTATE_SHUTTER_SETTINGS   16
#define ANNOTATE_CAF_SETTINGS       32
#define ANNOTATE_GAIN_SETTINGS      64
#define ANNOTATE_LENS_SETTINGS      128
#define ANNOTATE_MOTION_SETTINGS    256
#define ANNOTATE_FRAME_NUMBER       512
#define ANNOTATE_BLACK_BACKGROUND   1024


/// Frame advance method
// There isn't actually a MMAL structure for the following, so make one
typedef struct mmal_param_colourfx_s
{
   int enable;       /// Turn colourFX on or off
   int u,v;          /// U and V to use
} MMAL_PARAM_COLOURFX_T;

typedef struct mmal_param_thumbnail_config_s
{
   int enable;
} MMAL_PARAM_THUMBNAIL_CONFIG_T;

typedef struct param_float_rect_s
{
   double x;
   double y;
   double w;
   double h;
} PARAM_FLOAT_RECT_T;

/// struct contain camera settings
struct RASPICAM_CAMERA_PARAMETERS
{
   int sharpness;             /// -100 to 100
   int contrast;              /// -100 to 100
   int brightness;            ///  0 to 100
   int saturation;            ///  -100 to 100
   int ISO;                   ///  TODO : what range?
   int videoStabilisation;    /// 0 or 1 (false or true)
   int exposureCompensation;  /// -10 to +10 ?
   MMAL_PARAM_EXPOSUREMODE_T exposureMode;
   MMAL_PARAM_EXPOSUREMETERINGMODE_T exposureMeterMode;
   MMAL_PARAM_AWBMODE_T awbMode;
   MMAL_PARAM_IMAGEFX_T imageEffect;
   MMAL_PARAMETER_IMAGEFX_PARAMETERS_T imageEffectsParameters;
   MMAL_PARAM_COLOURFX_T colourEffects;
   int rotation;              /// 0-359
   int hflip;                 /// 0 or 1
   int vflip;                 /// 0 or 1
   PARAM_FLOAT_RECT_T  roi;   /// region of interest to use on the sensor. Normalised [0,1] values in the rect
   int shutter_speed;         /// 0 = auto, otherwise the shutter speed in ms
   float awb_gains_r;         /// AWB red gain
   float awb_gains_b;         /// AWB blue gain
   MMAL_PARAMETER_DRC_STRENGTH_T drc_level;  // Strength of Dynamic Range compression to apply
   MMAL_BOOL_T stats_pass;    /// Stills capture statistics pass on/off
   int enable_annotate;       /// Flag to enable the annotate, 0 = disabled, otherwise a bitmask of what needs to be displayed
   MMAL_PARAMETER_STEREOSCOPIC_MODE_T stereo_mode;
} ;
#endif /* RASPICAMCONTROL_H_ */

