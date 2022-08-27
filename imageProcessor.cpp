#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include "chunk.hpp"

#include "common/utils.hpp"
#include "imageProcessor.hpp"
#include "common/hsv.hpp"

static const UInt BlockSize =  20;
static const UInt VerticalSearchWindow = 30;
static const UInt HorizontalSearchWindow = 30;
static const UInt RightShift = 550;
static const UInt DownShift =  48;



image::image(void){
  
}

image::image(const char* filename, int* widthOutput, int* heightOutput)
{
  //readJpeg(filename, rgbData, widthOutput, heightOutput);
}

void
image::convertRgbImageToHsv(void)
{
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      UInt8 red = rgbData[y][x][0];
      UInt8 grn = rgbData[y][x][1];
      UInt8 blu = rgbData[y][x][2];
      UInt32 hsv = rgb2hsv(red, grn, blu);
      UInt8 hue = (hsv >> 16) & 0xff;
      UInt8 sat = (hsv >>  8) & 0xff;
      UInt8 val = (hsv >>  0) & 0xff;
      setHsvPixel(x, y, hue, sat, val);
    }
  }
}

void
image::convertHsvImageToRgb(void)
{
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      UInt8 hue = hsvData[y][x][0];
      UInt8 sat = hsvData[y][x][1];
      UInt8 val = hsvData[y][x][2];
      UInt32 rgb = hsv2rgb(hue, sat, val);
      UInt8 red = (rgb >> 16) & 0xff;
      UInt8 grn = (rgb >>  8) & 0xff;
      UInt8 blu = (rgb >>  0) & 0xff;
      setRgbPixel(x, y, red, grn, blu);
    }
  }
}


const UInt NormHorizontalBlocks = 16;
const UInt NormVerticalBlocks   = 16;
const UInt NormXPixelsPerBlock = ImageWidth / NormHorizontalBlocks;
const UInt NormYPixelsPerBlock = ImageHeight / NormVerticalBlocks;

void
image::getImageAverageSatVal(UInt *satP, UInt *valP, bool ignoreMask)
{
  UInt32 valSum = 0;
  UInt32 satSum = 0;
  int pixelCount = 0;
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      if (mask[y][x] || ignoreMask){
	pixelCount++;
	satSum += hsvData[y][x][1];
	valSum += hsvData[y][x][2];
      }
    }
  }
  Assert(pixelCount > 0);
  *satP = satSum/pixelCount;
  *valP = valSum/pixelCount;
}

void
image::getBlockAverageSatVal(int xx, int yy, UInt *satP, UInt *valP, UInt *pixelCountP, bool ignoreMask)
{
  UInt valSum = 0;
  UInt satSum = 0;
  int pixelCount = 0;
  UInt xEnd = xx + NormXPixelsPerBlock;
  UInt yEnd = yy + NormYPixelsPerBlock;
  for (UInt y = yy; y < yEnd; ++y) {
    for (UInt x = 0; x < xEnd; ++x) {
      if((x<ImageWidth)&&(y<ImageHeight)){
	if (mask[y][x] || ignoreMask){
	  pixelCount++;
	  satSum += hsvData[y][x][1];
	  valSum += hsvData[y][x][2];
	}
      }
    }
  }
  *pixelCountP = pixelCount;
  if (pixelCount == 0) {
    *satP = 0;
    *valP = 0;
  } else {
    *satP = satSum/pixelCount;
    *valP = valSum/pixelCount;
  }
}

void
image::normalizeBlock(UInt xx, UInt yy, UInt imgAvgSat, UInt imgAvgVal, UInt blkAvgSat, UInt blkAvgVal, bool doSat, bool doVal, bool ignoreMask)
{
  UInt xEnd = xx + NormXPixelsPerBlock;
  UInt yEnd = yy + NormYPixelsPerBlock;
  for (UInt y = yy; y < yEnd; y++) {
    for (UInt x = xx; x < xEnd; x++) {
      if((x<ImageWidth)&&(y<ImageHeight)){
	if (mask[y][x] || ignoreMask){
	  if(doSat){
	    UInt s = (hsvData[y][x][1] * imgAvgSat) / blkAvgSat;
	    hsvData[y][x][1] = s;
	    s = (s > 255) ? 255 : s;
	  }
	  if(doVal){
	    UInt v = (hsvData[y][x][2] * imgAvgVal) / blkAvgVal;
	    v = (v > 255) ? 255 : v;
	    hsvData[y][x][2] = v;
	  }
	}
      }
    }
  }
}

void
image::normalizeImage(bool doSat, bool doVal, bool ignoreMask)
{
  UInt imgAvgSat;
  UInt imgAvgVal;

  getImageAverageSatVal(&imgAvgSat, &imgAvgVal, ignoreMask);
  Assert(imgAvgSat < 256);
  Assert(imgAvgVal < 256);
  
  epf("image avg sat:%d val:%d\n", imgAvgSat, imgAvgVal);
  
  UInt dy = ImageHeight / NormVerticalBlocks;
  UInt dx = ImageWidth  / NormHorizontalBlocks;
  
  for (UInt y = 0; y < ImageHeight; y += dy) {
    for (UInt x = 0; x < ImageWidth; x += dx) {
      UInt blkAvgVal;
      UInt blkAvgSat;
      UInt pixelCount;
      getBlockAverageSatVal(x, y, &blkAvgSat, &blkAvgVal, &pixelCount, ignoreMask);
      if(pixelCount > 0){
	normalizeBlock(x, y, imgAvgSat, imgAvgVal, blkAvgSat, blkAvgVal, doSat, doVal, ignoreMask);
      }
    }
  }
}







void
image::setRgbPixel(UInt x, UInt y, UInt8 red, UInt8 green, UInt8 blue)
{
  if (y < ImageHeight) {
    if (x < ImageWidth) {
      //      Assert(x < ImageWidth);
      //      Assert(y < ImageHeight);
      //      Assert(x >= 0);
      //      Assert(y >= 0);
      rgbData[y][x][0] = red;
      rgbData[y][x][1] = green;
      rgbData[y][x][2] = blue;
    }
  }
}

void
image::setHsvPixel(UInt x, UInt y, UInt8 hue, UInt8 sat, UInt8 val)
{
  hsvData[y][x][0] = hue;
  hsvData[y][x][1] = sat;
  hsvData[y][x][2] = val;
}


void
image::dLine(UInt x0, UInt y0, UInt x1, UInt y1, UInt c)
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

void
image::hLine(UInt x, UInt y, UInt len, UInt c)
{
  UInt8 red = (c >> 16) & 0xff;
  UInt8 grn = (c >>  8) & 0xff;
  UInt8 blu = (c >>  0) & 0xff;
  for (UInt i = 0; i < len; ++i) {
    setRgbPixel(x+i, y+0, red, grn, blu);
    setRgbPixel(x+i, y+1, red, grn, blu);
  }
}

void
image::vLine(UInt32 x, UInt32 y, UInt32 len, UInt c)
{
  UInt8 red = (c >> 16) & 0xff;
  UInt8 grn = (c >>  8) & 0xff;
  UInt8 blu = (c >>  0) & 0xff;
  for (UInt32 i = 0; i < len; ++i) {
    setRgbPixel(x,   y+i, red, grn, blu);
    setRgbPixel(x+1, y+i, red, grn, blu);
  }
}

void
image::drawBox(UInt x, UInt y, UInt bsz, UInt c)
{
  hLine(x,     y,     bsz, c);
  hLine(x,     y+bsz, bsz, c);
  vLine(x,     y,     bsz, c);
  vLine(x+bsz, y,     bsz, c);
}

void
image::hLineHSV(UInt x, UInt y, UInt len, UInt hue)
{
  for (UInt i = 0; i < len; ++i) {
    setHsvPixel(x+i, y+0, hue, 0xff, 0xff);
    setHsvPixel(x+i, y+1, hue, 0xff, 0xff);
  }
}

void
image::vLineHSV(UInt32 x, UInt32 y, UInt32 len, UInt hue)
{
  for (UInt32 i = 0; i < len; ++i) {
    setHsvPixel(x,   y+i, hue, 0xff, 0xff);
    setHsvPixel(x+1, y+i, hue, 0xff, 0xff);
  }
}


void
image::drawRectangleHSV(UInt x, UInt y, UInt dx, UInt dy, UInt hue)
{
  hLineHSV(x,     y,     dx, hue);
  hLineHSV(x,     y+dy, dx, hue);
  vLineHSV(x,     y,     dy, hue);
  vLineHSV(x+dx,  y,     dy, hue);
}



class LineThing {
public:
  int x0;
  int y0;
  int x1;
  int y1;
  int c;

  LineThing(int _x0, int _y0, int _x1, int _y1, int _c){
    x0 = _x0;
    y0 = _y0;
    x1 = _x1;
    y1 = _y1;
    c = _c;
  }
};

std::vector<LineThing> listOfLines;

#include <math.h>
double
image::getMatchScore(UInt x0, UInt y0, UInt x1, UInt y1)
{
  
  UInt32 score = 0;
  
  UInt testX0;
  UInt testY0;
  UInt testX1;
  UInt testY1;
  
  int magenta = (0xff * 300) / 360;  
  
  int pixelsChecked = 0;
  for (UInt dy = 0; dy < BlockSize; ++dy) {
    for (UInt dx = 0; dx < BlockSize; ++dx) {
      testX0 = x0 + dx;
      testY0 = y0 + dy;
      testX1 = x1 + dx;
      testY1 = y1 + dy;
      
      if((testX0 < ImageWidth)&&(testX1 < ImageWidth)&&(testY0 < ImageHeight)&&(testY1 < ImageHeight)){
	
	if((abs(hsvData[testY0][testX0][0]-magenta) > 5)&&(abs(hsvData[testY0][testX0][0]-magenta) > 5)){
	
	  score += abs(hsvData[testY0][testX0][0] - hsvData[testY1][testX1][0]);
	  score += abs(hsvData[testY0][testX0][1] - hsvData[testY1][testX1][1]);
	  score += abs(hsvData[testY0][testX0][2] - hsvData[testY1][testX1][2]);
	  pixelsChecked++;
	}
      }
    }
  }
  
  
  if(pixelsChecked > 30){
    return score/pixelsChecked;
  } else {
    return 10000000;
  }
}

void
image::matchBlockLeftToRight(UInt x0, UInt y0)
{
  double lowestScore = 10000001.0;
  UInt32 x1 = x0;
  UInt32 y1 = y0;
  int triedBlocks = 0;
  //Assert(getMatchScore(x0, y0, x0, y0) == 0);
  for (UInt j = 0; j < VerticalSearchWindow; ++j) {
    UInt32 y = y0 + j + DownShift;
    for (UInt i = 0; i < HorizontalSearchWindow; ++i) {
      UInt32 x = x0 + i + RightShift;
      UInt32 score = getMatchScore(x0, y0, x, y);
      triedBlocks++;
      if (score < lowestScore) {
	lowestScore = score;
	x1 = x;
	y1 = y;
      }
    }
  }
  
  if(lowestScore != 10000000){

    int dx = x0-x1;
    int dy = y0-y1;
    for(int y = 0; y < (int)BlockSize; y++){
      for(int x = 0; x < (int)BlockSize; x++){
	if((y0+y < ImageHeight)&&(x0+x<ImageWidth)){
	  depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    
    dLine(x0, y0, x1, y1, 0xff0000);
    listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

void image::matchBlockRightToLeft(UInt x0, UInt y0)
{
  double lowestScore = 10000001.0;
  UInt32 x1 = x0;
  UInt32 y1 = y0;
  int triedBlocks = 0;
  //Assert(getMatchScore(x0, y0, x0, y0) == 0);
  for (UInt j = 0; j < VerticalSearchWindow; ++j) {
    UInt32 y = y0 - j - DownShift;
    for (UInt i = 0; i < HorizontalSearchWindow; ++i) {
      UInt32 x = x0 - i - RightShift;
      UInt32 score = getMatchScore(x0, y0, x, y);
      triedBlocks++;
      if (score < lowestScore) {
	lowestScore = score;
	x1 = x;
	y1 = y;
      }
    }
  }
  
  if(lowestScore != 10000000){

    int dx = x0-x1;
    int dy = y0-y1;
    for(int y = 0; y < (int)BlockSize; y++){
      for(int x = 0; x < (int)BlockSize; x++){
	if((y0+y < ImageHeight)&&(x0+x<ImageWidth)){
	  depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    //listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

void
image::saveMapToFile(void){
  int poop;
  const char *fn = "depthThing.dat";
  poop = open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  epf("poop=%d\n", poop);
  if (poop < 0) {
    fatal("Open failed, %s: %s\n", fn, syserr());
  }
  int n = sizeof(diffFromMean);
  Assert(n == sizeof(double) * ImageWidth * ImageHeight);
  int w = write(poop, diffFromMean, n);
  if (w != n) {
    fatal("Write failed, %s: %s\n", fn, syserr());
  }
  close(poop);
}

void
image::getMapFromFile(void){
  int poop;
  const char *fn = "depthThindat";
  poop = open(fn, O_RDONLY);
  epf("poop=%d\n", poop);
  if (poop < 0) {
    fatal("Open failed, %s: %s", fn, syserr());
  }
  int n = sizeof(depthMapOffset);
  Assert(n == sizeof(double) * ImageWidth * ImageHeight);
  int r = read(poop, depthMapOffset, n);
  if (r != n) {
    fatal("Read failed, %s: %s", fn, syserr());
  }
  close(poop);
}

void
image::doBlockMatch(void)
{
  bool saveFile = false;
  bool readFile = false;
  int matchedBlocks = 0;
  
  for (UInt y = 0; y < ImageHeight; y += 5) {
    for (UInt x = 0; x < ImageWidth/2; x += 5) {
      matchBlockLeftToRight(x, y);
      matchedBlocks++;
    }
    epf("y = %d\n", y);
  }
  epf("other thing");
  for (UInt y = 0; y < ImageHeight; y += 5) {
    for (UInt x = ImageWidth/2; x < ImageWidth; x += 5) {
      matchBlockRightToLeft(x, y);
      matchedBlocks++;
    }
    epf("y = %d\n", y);
  }
  
  epf("mathed %d blocks\n", matchedBlocks);

  
  /*
  for(int i = 0; i < (int)listOfLines.size(); i+=10){
    LineThing *poop = &listOfLines[i];
    //dLine(poop->x0, poop->y0, poop->x1, poop->y1, poop->c);
    //dLine(poop->x0, poop->y0, poop->x1, poop->y1, 0x00f00f*i);
  }
  */
  
  for(int y = 0; y < (int)ImageHeight; y++){
    for(int x = 0; x < (int)ImageWidth; x++){
      if(matchesForPixel[y][x] != 0){
	depthMap[y][x] /= matchesForPixel[y][x];
      }else{
	depthMap[y][x] = 0;
      }
    }
  }

  double meanDepthLen = 0;
  int totalPixlesAdded = 0;
  
  for(int y = 0; y < (int)ImageHeight; y++){
    for(int x = 0; x < (int)ImageWidth; x++){
      if(matchesForPixel[y][x] != 0){
	totalPixlesAdded++;
	meanDepthLen += depthMap[y][x];
      }
    }
  }
  meanDepthLen /= totalPixlesAdded;
  if(saveFile){
    for(int y = 0; y < (int)ImageHeight; y++){
      for(int x = 0; x < (int)ImageWidth; x++){
	diffFromMean[y][x] = depthMap[y][x]-meanDepthLen;
      }
    }
    saveMapToFile();
  }
  
  if(readFile){
    getMapFromFile();
    for(int y = 0; y < (int)ImageHeight; y++){
      for(int x = 0; x < (int)ImageWidth; x++){
	depthMap[y][x] -= depthMapOffset[y][x];
      }
    }
  }
  
  
  double maxValue = 0;
  for(int y = 0; y < (int)ImageHeight; y++){
    for(int x = 0; x < (int)ImageWidth; x++){
      if(matchesForPixel[y][x] != 0){
	if(depthMap[y][x] > maxValue){
	  maxValue = depthMap[y][x];
	}
      }
    }
  }

  for(int y = 0; y < (int)ImageHeight; y++){
    for(int x = 0; x < (int)ImageWidth; x++){
      if(matchesForPixel[y][x] != 0){
	blockMatchOutput[y][x][0] = 0;
	blockMatchOutput[y][x][1] = 0;
	blockMatchOutput[y][x][2] = 0;
	double val = depthMap[y][x];
	
	val -= meanDepthLen;
	val *= -1;
	val *= 40;
	epf("val = %f", val);
	if(val > 0){
	  if(val > 255){
	    val = 255;
	  }
	  blockMatchOutput[y][x][0] = (int)(val);
	}else{
	  val *= -1;
	  if(val > 255){
	    val = 255;
	  }
	  blockMatchOutput[y][x][2] = (int)(val);
	}
	
      }else{
	blockMatchOutput[y][x][0] = 0;
	blockMatchOutput[y][x][1] = 255;
	blockMatchOutput[y][x][2] = 0;
      }
    }
  }
  epf("depth map generated");
}
