/**
 * Parse a possible command pair - command and parameter
 * @param istr input stream
 * @return true if success, false otherwise
 */
bool raspicamcontrol_parse_cmdline(RASPICAM_CAMERA_PARAMETERS *params, std::istream_iterator<std::string>& istr)
{
   int command_id,  num_parameters;
   using string = std::string;
   std::istream_iterator<string> eos;
   if (istr == eos)
       return false;

   command_id = raspicli_get_command_id(cmdline_commands, cmdline_commands_size, istr->c_str(), &num_parameters);

   // If invalid command, or we are missing a parameter, drop out
   if (command_id==-1 || ( num_parameters > 0 && ++istr==eos))
      return false;

   switch (command_id)
   {
   case CommandSharpness : // sharpness - needs single number parameter
      sscanf(istr->c_str(), "%d", &params->sharpness);
      
      break;

   case CommandContrast : // contrast - needs single number parameter
      sscanf(istr->c_str(), "%d", &params->contrast);
      
      break;

   case CommandBrightness : // brightness - needs single number parameter
      sscanf(istr->c_str(), "%d", &params->brightness);
      
      break;

   case CommandSaturation : // saturation - needs single number parameter
      sscanf(istr->c_str(), "%d", &params->saturation);
      
      break;

   case CommandISO : // ISO - needs single number parameter
      sscanf(istr->c_str(), "%d", &params->ISO);
      
      break;

   case CommandVideoStab : // video stabilisation - if here, its on
      params->videoStabilisation = 1;
      
      break;

   case CommandEVComp : // EV - needs single number parameter
      sscanf(istr->c_str(), "%d", &params->exposureCompensation);
      
      break;

   case CommandExposure : // exposure mode - needs string
      params->exposureMode = exposure_mode_from_string(istr->c_str());
      
      break;

   case CommandAWB : // AWB mode - needs single number parameter
      params->awbMode = awb_mode_from_string(istr->c_str());
      
      break;

   case CommandImageFX : // Image FX - needs string
      params->imageEffect = imagefx_mode_from_string(istr->c_str());
      
      break;

   case CommandColourFX : // Colour FX - needs string "u:v"
      sscanf(istr->c_str(), "%d:%d", &params->colourEffects.u, &params->colourEffects.v);
      params->colourEffects.enable = 1;
      
      break;

   case CommandMeterMode:
      params->exposureMeterMode = metering_mode_from_string(istr->c_str());
      
      break;

   case CommandRotation : // Rotation - degree
      sscanf(istr->c_str(), "%d", &params->rotation);
      
      break;

   case CommandHFlip :
      params->hflip  = 1;
      
      break;

   case CommandVFlip :
      params->vflip = 1;
      
      break;

   case CommandROI :
   {
      double x,y,w,h;
      int args;

      args = sscanf(istr->c_str(), "%lf,%lf,%lf,%lf", &x,&y,&w,&h);

      if (args != 4 || x > 1.0 || y > 1.0 || w > 1.0 || h > 1.0)
      {
         return false;
      }

      // Make sure we stay within bounds
      if (x + w > 1.0)
         w = 1 - x;

      if (y + h > 1.0)
         h = 1 - y;

      params->roi.x = x;
      params->roi.y = y;
      params->roi.w = w;
      params->roi.h = h;

      
      break;
   }

   case CommandShutterSpeed : // Shutter speed needs single number parameter
   {
      sscanf(istr->c_str(), "%d", &params->shutter_speed);
      
      break;
   }

   case CommandAwbGains :
      {
      double r,b;
      int args;

      args = sscanf(istr->c_str(), "%lf,%lf", &r,&b);

      if (args != 2 || r > 8.0 || b > 8.0)
      {
         return false;
      }

      params->awb_gains_r = r;
      params->awb_gains_b = b;

      
      break;
      }

   case CommandDRCLevel:
   {
      params->drc_level = drc_mode_from_string(istr->c_str());
      
      break;
   }

   case CommandStatsPass:
   {
      params->stats_pass = MMAL_TRUE;
      
      break;
   }

   case CommandStereoMode:
   {
      params->stereo_mode.mode = stereo_mode_from_string(istr->c_str());
      
      break;
   }

   case CommandStereoDecimate:
   {
      params->stereo_mode.decimate = MMAL_TRUE;
      
      break;
   }

   case CommandStereoSwap:
   {
      params->stereo_mode.swap_eyes = MMAL_TRUE;
      
      break;
   }

   }
   return true;
}
