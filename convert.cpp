//
// convert.cpp
//

static void
convertRgbImageToHsv(void)
{
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      UInt8 red = g.rgbData[y][x][0];
      UInt8 grn = g.rgbData[y][x][1];
      UInt8 blu = g.rgbData[y][x][2];
      UInt32 hsv = rgb2hsv(red, grn, blu);
      UInt8 hue = (hsv >> 16) & 0xff;
      UInt8 sat = (hsv >>  8) & 0xff;
      UInt8 val = (hsv >>  0) & 0xff;
      setHsvPixel(x, y, hue, sat, val);
    }
  }
}

static void
convertHsvImageToRgb(void)
{
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      UInt8 hue = g.hsvData[y][x][0];
      UInt8 sat = g.hsvData[y][x][1];
      UInt8 val = g.hsvData[y][x][2];
      UInt32 rgb = hsv2rgb(hue, sat, val);
      UInt8 red = (rgb >> 16) & 0xff;
      UInt8 grn = (rgb >>  8) & 0xff;
      UInt8 blu = (rgb >>  0) & 0xff;
      setRgbPixel(x, y, red, grn, blu);
    }
  }
}
