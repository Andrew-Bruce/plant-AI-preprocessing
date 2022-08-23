//
//  hsv.cpp
//

#include <stdio.h>

typedef unsigned int UInt32;
typedef unsigned char UInt8;

static UInt32 hsv2rgb(UInt32 hsv) __attribute__ ((unused));
static UInt32 rgb2hsv(UInt32 rgb) __attribute__ ((unused));

static inline UInt32 R3(UInt8 a, UInt8 b, UInt8 c)
{
  return (a << 16) | (b << 8) | c;
}

static UInt32
hsv2rgb(UInt8 hue, UInt8 sat, UInt8 v)
{
  if (sat == 0) {
    return R3(v, v, v);
  }
  UInt8 region = hue / 43;
  UInt8 remainder = (hue - (region * 43)) * 6;
  UInt8 p = (v * (255 - sat)) >> 8;
  UInt8 q = (v * (255 - ((sat * remainder) >> 8))) >> 8;
  UInt8 t = (v * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;
  switch (region) {
  case 0: return R3(v, t, p);
  case 1: return R3(q, v, p);
  case 2: return R3(p, v, t);
  case 3: return R3(p, q, v);
  case 4: return R3(t, p, v);
  default:return R3(v, p, q);
  }
}

static UInt32
hsv2rgb(UInt32 hsv)
{
  UInt8 hue = (hsv >> 16) & 0xff;
  UInt8 sat = (hsv >>  8) & 0xff;
  UInt8 v   = (hsv >>  0) & 0xff;
  return hsv2rgb(hue, sat, v);
}

static UInt32
rgb2hsv(UInt8 red, UInt8 grn, UInt8 blu)
{
  UInt8 rgbMin = (red < grn) ?
    (red < blu ? red : blu) :
    (grn < blu ? grn : blu);
  UInt8 rgbMax = (red > grn) ?
    (red > blu ? red : blu) :
    (grn > blu ? grn : blu);
  UInt8 hue = 0;
  UInt8 val = rgbMax;
  if (val == 0) {
    return 0;
  }
  UInt8 sat = 255 * long(rgbMax - rgbMin) / val;
  if (sat == 0) {
    return R3(hue, sat, val);
  }
  if (rgbMax == red) {
    hue = 0 + 43 * (grn - blu) / (rgbMax - rgbMin);
  } else if (rgbMax == grn) {
    hue = 85 + 43 * (blu - red) / (rgbMax - rgbMin);
  } else {
    hue = 171 + 43 * (red - grn) / (rgbMax - rgbMin);
  }
  return R3(hue, sat, val);
}

static UInt32
rgb2hsv(UInt32 rgb)
{
  UInt8 red = (rgb >> 16) & 0xff;
  UInt8 grn = (rgb >>  8) & 0xff;
  UInt8 blu = (rgb >>  0) & 0xff;
  return rgb2hsv(red, grn, blu);
}
