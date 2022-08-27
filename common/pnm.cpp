
#include "utils.hpp"
#include "pnm.hpp"
#include <unistd.h>
#include <fcntl.h>

void
writePnmHeader(int fd, UInt width, UInt height)
{
  char hdr[64];
  sprintf(hdr, "P6\n%d %d\n255\n", width, height);
  write(fd, hdr, strlen(hdr));
}

void
writePnmToStdout(UInt8* rgbPixels, UInt width, UInt height)
{
  writePnmHeader(1, width, height);
  write(1, (char*)rgbPixels, width * height * 3);
}

void
writePpmToFile(UInt8 *rgbPixels, UInt width, UInt height, const char *filename)
{
  int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (fd < 0) {
    fatal("Cannot open %s: %s", filename, syserr());
  }
  writePnmHeader(fd, width, height);
  write(fd, rgbPixels, width * height * 3);
  close(fd);
}
