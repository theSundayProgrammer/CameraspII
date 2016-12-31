//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// adapted from IJG sample code
// Distributed under the Boost Software License, Version 1.0.
// 
//////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <jpeg/jpgconvert.h>
#include <setjmp.h>
#include <vector>
#define BLOCK_SIZE 16384
namespace 
{
    struct jpeg_dest: jpeg_destination_mgr
    {
        std::string buffer;
    };
    void my_init_destination(j_compress_ptr cinfo)
    {
        jpeg_dest *dest = static_cast<jpeg_dest*>(cinfo->dest);
        dest->buffer.resize(BLOCK_SIZE);
        dest->next_output_byte = (unsigned char*)&(dest->buffer[0]);
        dest->free_in_buffer = dest->buffer.size();
    }

    boolean my_empty_output_buffer(j_compress_ptr cinfo)
    {
        jpeg_dest *dest = static_cast<jpeg_dest*>(cinfo->dest);
        size_t oldsize = dest->buffer.size();
        dest->buffer.resize(oldsize + BLOCK_SIZE);
        dest->next_output_byte = (unsigned char*)&(dest->buffer[oldsize]);
        dest->free_in_buffer = dest->buffer.size() - oldsize;
        return true;
    }

    void my_term_destination(j_compress_ptr cinfo)
    {
        jpeg_dest *dest = static_cast<jpeg_dest*>(cinfo->dest);
        dest->buffer.resize(dest->buffer.size() - cinfo->dest->free_in_buffer);
    }
}
namespace camerasp
{
    std::string write_JPEG_dat (img_info const& img)
    {
      struct jpeg_compress_struct cinfo;
      struct jpeg_error_mgr jerr;
      JSAMPROW row_pointer[1];
      int row_stride;		/* physical row width in image buffer */
    
      /*  allocate and initialize JPEG compression object */
      cinfo.err = jpeg_std_error(&jerr);
      /* Now we can initialize the JPEG compression object. */
      jpeg_create_compress(&cinfo);
    
      /*  specify data destination (eg, a file) */
        jpeg_dest dest;
        cinfo.dest = &dest;
        dest.init_destination = &my_init_destination;
        dest.empty_output_buffer = &my_empty_output_buffer;
        dest.term_destination = &my_term_destination;
    
    
      cinfo.image_width = img.image_width; 	
      cinfo.image_height = img.image_height;
      cinfo.input_components = 3;		
      cinfo.in_color_space = JCS_RGB; 	
      jpeg_set_defaults(&cinfo);
      /* Now you can set any non-default parameters you wish to.
       * Here we just illustrate the use of quality (quantization table) scaling:
       */
      jpeg_set_quality(&cinfo, img.quality, TRUE /* limit to baseline-JPEG values */);
    
      /* Start compressor */
    
      /* TRUE ensures that we will write a complete interchange-JPEG file.
       * Pass TRUE unless you are very sure of what you're doing.
       */
      jpeg_start_compress(&cinfo, TRUE);
    
    
      row_stride = img.image_width * 3;	/* JSAMPLEs per row in image_buffer */
    
      while (cinfo.next_scanline < cinfo.image_height) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */
        row_pointer[0] = img.get_scan_line(cinfo.next_scanline, row_stride);
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
      }
    
    
      jpeg_finish_compress(&cinfo);
    
      jpeg_destroy_compress(&cinfo);
       return dest.buffer;
    }
}

