#include "utils.hpp"
#include <jpeglib.h>

UInt8 *
readJpeg(const char *fn, UInt8 *rgbData, UInt32 *widthP, UInt32 *heightP)
{
  Assert(widthP != NULL);
  Assert(heightP != NULL);
  
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  UInt32 sz;
  const UInt8 *p = Readfile(fn, &sz);
  cinfo.err = jpeg_std_error(&jerr);	
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, p, sz);
  int rc = jpeg_read_header(&cinfo, TRUE);
  if (rc != 1) {
    fatal("%s is not a normal JPEG", fn);
  }
  jpeg_start_decompress(&cinfo);
  UInt32 width = cinfo.output_width;
  UInt32 height = cinfo.output_height;
  UInt32 pixelSize = cinfo.output_components;
  Assert(pixelSize == 3);
  int rgbSize = width * height * pixelSize;
  /*
    epf("width=%d, height=%d, pixelSize=%d",
      cinfo.output_width,
      cinfo.output_height,
      cinfo.output_components);
    Assert(cinfo.output_width == ImageWidth);
    Assert(cinfo.output_height == ImageHeight);
    Assert(rgbSize == sizeof(pixelData));
  */
  if (rgbData == NULL) {
    rgbData = (UInt8 *) Malloc(rgbSize);
    *widthP = width;
    *heightP = height;
  } else {
    Assert(width == *widthP);//if messes up main.cpp has the wrong size
    Assert(height == *heightP);
  }
  int rowStride = width * pixelSize;
  while (cinfo.output_scanline < cinfo.output_height) {
    unsigned char *buffer_array[1];
    buffer_array[0] = rgbData + (cinfo.output_scanline) * rowStride;
    jpeg_read_scanlines(&cinfo, buffer_array, 1);
  }
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free((void *) p);
  return rgbData;
}
