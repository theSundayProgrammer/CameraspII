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

/**
 * \file RaspiCLI.c
 * Code for handling command line parameters
 *
 * \date 4th March 2013
 * \Author: James Hughes
 *
 * Description
 *
 * Some functions/structures for command line parameter parsing
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "interface/vcos/vcos.h"

#include <raspi/RaspiCLI.h>
#include <raspi/RaspiStill.h>


/// Comamnd ID's and Structure defining our command line options
#define CommandHelp         0
#define CommandWidth        1
#define CommandHeight       2
#define CommandQuality      3
#define CommandRaw          4
#define CommandOutput       5
#define CommandVerbose      6
#define CommandDemoMode     9
#define CommandEncoding     10
#define CommandExifTag      11
#define CommandTimelapse    12
#define CommandSettings     19
#define CommandCamSelect    20
#define CommandSensorMode   22
#define CommandDateTime     23
#define CommandTimeStamp    24
#define CommandFrameStart   25
#define CommandRestartInterval 26
#define CommandFrameMax   27

static COMMAND_LIST cmdline_commands[] =
{
   { CommandHelp,    "-help",       "?",  "This help information", 0 },
   { CommandWidth,   "-width",      "w",  "Set image width <size>", 1 },
   { CommandHeight,  "-height",     "h",  "Set image height <size>", 1 },
   { CommandQuality, "-quality",    "q",  "Set jpeg quality <0 to 100>", 1 },
   { CommandRaw,     "-raw",        "r",  "Add raw bayer data to jpeg metadata", 0 },
   { CommandOutput,  "-output",     "o",  "Output filename <filename> (to write to stdout, use '-o -'). If not specified, no file is saved", 1 },
   { CommandVerbose, "-verbose",    "v",  "Output verbose information during run", 0 },
   { CommandDemoMode,"-demo",       "d",  "Run a demo mode (cycle through range of camera options, no capture)", 0},
   { CommandEncoding,"-encoding",   "e",  "Encoding to use for output file (jpg, bmp, gif, png)", 1},
   { CommandTimelapse,"-timelapse", "tl", "Timelapse mode. Takes a picture every <t>ms. %d == frame number (Try: -o img_%04d.jpg)", 1},
   { CommandSettings, "-settings",  "set","Retrieve camera settings and write to stdout", 0},
   { CommandCamSelect, "-camselect","cs", "Select camera <number>. Default 0", 1 },
   { CommandSensorMode,"-mode",     "md", "Force sensor mode. 0=auto. See docs for other modes available", 1},
   { CommandFrameMax,"-framemax","mf",  "Maximum number of framesto save", 1},
   { CommandFrameStart,"-framestart","fs",  "Starting frame number in output pattern(%d)", 1},
   { CommandRestartInterval, "-restart","rs","JPEG Restart interval (default of 0 for none)", 1},
};

const char *app_name="RaspiStill";
static int cmdline_commands_size = sizeof(cmdline_commands) / sizeof(cmdline_commands[0]);
/**
 * Convert a string from command line to a comand_id from the list
 *
 * @param commands Array of command to check
 * @param num_command Number of commands in the array
 * @param arg String to search for in the list
 * @param num_parameters Returns the number of parameters used by the command
 *
 * @return command ID if found, -1 if not found
 *
 */
int raspicli_get_command_id(const COMMAND_LIST *commands, const int num_commands, const char *arg, int *num_parameters)
{
   int command_id = -1;
   int j;

   vcos_assert(commands);
   vcos_assert(num_parameters);
   vcos_assert(arg);

   if (!commands || !num_parameters || !arg)
      return -1;

   for (j = 0; j < num_commands; j++)
   {
      if (!strcmp(arg, commands[j].command) ||
          !strcmp(arg, commands[j].abbrev))
      {
         // match
         command_id = commands[j].id;
         *num_parameters = commands[j].num_parameters;
         break;
      }
   }

   return command_id;
}


/**
 * Display usage information for the application to stdout
 *
 * @param app_name String to display as the application name
 */
static void display_valid_parameters(const char *app_name)
{
   fprintf(stdout, "Runs camera for specific time, and take JPG capture at end if requested\n\n");
   fprintf(stdout, "usage: %s [options]\n\n", app_name);

   fprintf(stdout, "Image parameter commands\n\n");

   raspicli_display_help(cmdline_commands, cmdline_commands_size);

   // Now display any help information from the camcontrol code
   raspicamcontrol_display_help();

   fprintf(stdout, "\n");

   return;
}
/**
 * Display the list of commands in help format
 *
 * @param commands Array of command to check
 * @param num_command Number of commands in the arry
 *
 *
 */
void raspicli_display_help(const COMMAND_LIST *commands, const int num_commands)
{
   int i;

   vcos_assert(commands);

   if (!commands)
      return;

   for (i = 0; i < num_commands; i++)
   {
      fprintf(stdout, "-%s, -%s\t: %s\n", commands[i].abbrev,
         commands[i].command, commands[i].help);
   }
}


/**
 * Handler for sigint signals
 *
 * @param signal_number ID of incoming signal.
 *
 */
void signal_handler(int signal_number)
{
   if (signal_number == SIGUSR1)
   {
      // Handle but ignore - prevents us dropping out if started in none-signal mode
      // and someone sends us the USR1 signal anyway
   }
   else
   {
      // Going to abort on all other signals
      vcos_log_error("Aborting program\n");
      exit(130);
   }
}

/**
 * Parse the incoming command line and put resulting parameters in to the state
 *
 * @param state Pointer to state structure to assign any discovered parameters to
 * @return non-0 if failed for some reason, 0 otherwise
 */
int parse_cmdline(shared_state *state)
{
   // Parse the command line arguments.
   // We are looking for --<something> or -<abbreviation of something>

   int valid = 1;
   std::string cmd;
  std::fstream ifs("options.txt");
  std::istream_iterator<std::string>  ostr(ifs);
  std::istream_iterator<std::string>  eos;
  for(;ostr!=eos ;++ostr)
   {
      int command_id, num_parameters;

      if (ostr->empty())
         continue;
      cmd = *ostr;
      const char *argv = ostr->c_str();
      if (argv[0] != '-')
      {
        fprintf(stderr, "Invalid command line option (%s)\n", cmd.c_str());
         continue;
      }


      command_id = raspicli_get_command_id(cmdline_commands, cmdline_commands_size, ++argv, &num_parameters);
      //  We are now dealing with a command line option
      switch (command_id)
      {
      case CommandHelp:
         display_valid_parameters(app_name);
         // exit straight away if help requested
         return -1;

      case CommandWidth: // Width > 0
         if (sscanf((*++ostr).c_str(), "%u", &state->width) != 1)
            valid = 0;
         break;

      case CommandHeight: // Height > 0
         if (sscanf((*++ostr).c_str(), "%u", &state->height) != 1)
            valid = 0;
         break;

      case CommandQuality: // Quality = 1-100
         if (sscanf((*++ostr).c_str(), "%u", &state->quality) == 1)
         {
            if (state->quality > 100)
            {
               fprintf(stderr, "Setting max quality = 100\n");
               state->quality = 100;
            }
         }
         else
            valid = 0;

         break;

      case CommandRaw: // Add raw bayer data in metadata
         state->wantRAW = 1;
         break;

      case CommandOutput:  // output filename
      {
         int len = strlen((*++ostr).c_str());
         if (len)
         {
            //We use sprintf to append the frame number for timelapse mode
            //Ensure that any %<char> is either %% or %d.
            const char *percent = ostr->c_str();
            while(valid && *percent && (percent=strchr(percent, '%')) != NULL)
            {
               int digits=0;
               percent++;
               while(isdigit(*percent))
               {
                 percent++;
                 digits++;
               }
               if(!((*percent == '%' && !digits) || *percent == 'd'))
               {
                  valid = 0;
                  fprintf(stderr, "Filename contains %% characters, but not %%d or %%%% - sorry, will fail\n");
               }
               percent++;
            }
            strncpy(state->filename, ostr->c_str(), len+1);
         }
         else
            valid = 0;
         break;
      }


      case CommandFrameMax:  // use a staring value != 0
	{
	  if (sscanf((*++ostr).c_str(), "%d", &state->max_frames) == 1)
                  fprintf(stderr, "max frame = %d\n",state->max_frames);
	  else
	    valid = 0;
	  break;
	}
      case CommandFrameStart:  // use a staring value != 0
      {
         if (sscanf((*++ostr).c_str(), "%d", &state->frame) == 1)
                  fprintf(stderr, "start_frame = %d\n",state->frame);
         else
            valid = 0;
         break;
      }

      case CommandVerbose: // display lots of data during run
         state->verbose = 1;
         break;


      case CommandEncoding :
      {
         int len = strlen((*++ostr).c_str());
         valid = 0;

         if (len)
           state->encoding = img_format_from_string(ostr->c_str());
         break;
      }

      case CommandExifTag:
         if ( strcmp( (*++ostr).c_str(), "none" ) == 0 )
         {
            state->enableExifTags = 0;
         }
         break;

      case CommandTimelapse:
         if (sscanf((*++ostr).c_str(), "%u", &state->timelapse) != 1)
            valid = 0;
         break;


      case CommandSettings:
         state->settings = 1;
         break;

      case CommandCamSelect:  //Select camera input port
      {
         if (sscanf((*++ostr).c_str(), "%u", &state->cameraNum) == 1)
         {
         }
         else
            valid = 0;
         break;
      }


      case CommandSensorMode:
      {
         if (sscanf((*++ostr).c_str(), "%u", &state->sensor_mode) == 1)
         {
         }
         else
            valid = 0;
         break;
      }

      case CommandRestartInterval:
      {
         if (sscanf((*++ostr).c_str(), "%u", &state->restart_interval) == 1)
         {
         }
         else
            valid = 0;
         break;
      }

      default:
      {
         // Try parsing for any image specific parameters
         // result indicates how many parameters were used up, 0,1,2
         // but we adjust by -1 as we have used one already
         // If no parms were used, this must be a bad parameters
         if (!raspicamcontrol_parse_cmdline(&state->camera_parameters,ostr))
            valid = 0;

         break;
      }
      }
   if (!valid)
   {
      fprintf(stderr, "Invalid command line option (%s)\n", cmd.c_str());
   }
   }



   return 0;
}

