//
//  draw.cpp
//

static void
setRgbPixel(UInt x, UInt y, UInt8 red, UInt8 green, UInt8 blue)
{
  if (y < ImageHeight) {
    if (x < ImageWidth) {
      //      Assert(x < ImageWidth);
      //      Assert(y < ImageHeight);
      //      Assert(x >= 0);
      //      Assert(y >= 0);
      g.rgbData[y][x][0] = red;
      g.rgbData[y][x][1] = green;
      g.rgbData[y][x][2] = blue;
    }
  }
}

static void
setHsvPixel(UInt x, UInt y, UInt8 hue, UInt8 sat, UInt8 val)
{
  g.hsvData[y][x][0] = hue;
  g.hsvData[y][x][1] = sat;
  g.hsvData[y][x][2] = val;
}


static void
dLine(UInt x0, UInt y0, UInt x1, UInt y1, UInt c)
{
  UInt8 red = (c >> 16) & 0xff;
  UInt8 grn = (c >>  8) & 0xff;
  UInt8 blu = (c >>  0) & 0xff;

  if (x0 > x1) {
    int tmp;
    tmp = x1;
    x1 = x0;
    x0 = tmp;
    tmp = y1;
    y1 = y0;
    y0 = tmp;
  }
  double slope;
  slope = ((double) y0 - y1);
  slope /= ((double) x0 - x1);
  for(UInt i = 0; i < x1-x0; i++){
    UInt xPix = x0 + i;
    UInt yPix = (int)(y0+(slope*i));
    setRgbPixel(xPix, yPix, red, grn, blu);
    setRgbPixel(xPix, yPix+1, red, grn, blu);
  }
  
}

static void
hLine(UInt x, UInt y, UInt len, UInt c)
{
  UInt8 red = (c >> 16) & 0xff;
  UInt8 grn = (c >>  8) & 0xff;
  UInt8 blu = (c >>  0) & 0xff;
  for (UInt i = 0; i < len; ++i) {
    setRgbPixel(x+i, y+0, red, grn, blu);
    setRgbPixel(x+i, y+1, red, grn, blu);
  }
}

static void
vLine(UInt32 x, UInt32 y, UInt32 len, UInt c)
{
  UInt8 red = (c >> 16) & 0xff;
  UInt8 grn = (c >>  8) & 0xff;
  UInt8 blu = (c >>  0) & 0xff;
  for (UInt32 i = 0; i < len; ++i) {
    setRgbPixel(x,   y+i, red, grn, blu);
    setRgbPixel(x+1, y+i, red, grn, blu);
  }
}

static void drawBox(UInt x, UInt y, UInt bsz, UInt c) __attribute__((unused));

static void
drawBox(UInt x, UInt y, UInt bsz, UInt c)
{
  hLine(x,     y,     bsz, c);
  hLine(x,     y+bsz, bsz, c);
  vLine(x,     y,     bsz, c);
  vLine(x+bsz, y,     bsz, c);
}

static void
hLineHSV(UInt x, UInt y, UInt len, UInt hue)
{
  for (UInt i = 0; i < len; ++i) {
    setHsvPixel(x+i, y+0, hue, 0xff, 0xff);
    setHsvPixel(x+i, y+1, hue, 0xff, 0xff);
  }
}

static void
vLineHSV(UInt32 x, UInt32 y, UInt32 len, UInt hue)
{
  for (UInt32 i = 0; i < len; ++i) {
    setHsvPixel(x,   y+i, hue, 0xff, 0xff);
    setHsvPixel(x+1, y+i, hue, 0xff, 0xff);
  }
}

static void drawRectangleHSV(UInt x, UInt y, UInt dx, UInt dy, UInt c) __attribute__((unused));

static void
drawRectangleHSV(UInt x, UInt y, UInt dx, UInt dy, UInt hue)
{
  hLineHSV(x,     y,     dx, hue);
  hLineHSV(x,     y+dy, dx, hue);
  vLineHSV(x,     y,     dy, hue);
  vLineHSV(x+dx, y,     dy, hue);
}
