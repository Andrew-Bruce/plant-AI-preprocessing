
#include <png.h>
#include "png.hpp"

void
pngWriteRgbPixelsToFile(const char *filename, UInt8 *buf, UInt imgWidth, UInt imgHeight)
{
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    fatal("Could not open file %s for writing: %s", filename, syserr());
  }
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fatal("Could not allocate write struct: %s", syserr());
  }
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fatal("Could not allocate info struct: %s", syserr());
  }
  // Setup Exception handling
  // if (setjmp(png_jmpbuf(png_ptr))) {
  // fatal("Error during png creation\n");
  // }
  png_init_io(png_ptr, fp);
  // Write header (8 bit colour depth)
  png_set_IHDR(png_ptr, info_ptr, imgWidth, imgHeight,
	       8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);
  for (UInt y = 0 ; y < imgHeight ; ++y) {
    png_write_row(png_ptr, (png_bytep) &buf[y*3*imgWidth]);
  }
  png_write_end(png_ptr, NULL);
  fclose(fp);
  png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
  return;
}
