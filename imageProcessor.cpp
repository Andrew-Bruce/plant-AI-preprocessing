#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <math.h>
#include <assert.h>

#include "chunk.hpp"

#include "common/utils.hpp"
#include "imageProcessor.hpp"
#include "common/hsv.hpp"

static const UInt BlockSize =  20;
static const UInt VerticalSearchWindow = 30;
static const UInt HorizontalSearchWindow = 30;
static const UInt RightShift = 550;
static const UInt DownShift =  48;

static const int MinPixelsWorthSaving = 1000;

plantImage::plantImage(void){
  
}

plantImage::plantImage(UInt8** RGB_row_pointers_, int width_, int height_)
{
  width = width_;
  height = height_;
  RGB_row_pointers = RGB_row_pointers_;

  const int numChannels = 3;
  
  HSV_row_pointers = (UInt8**)make2DPointerArray<UInt8>(width*numChannels, height);
  blockMatchOutput_row_pointers = (UInt8**)make2DPointerArray<UInt8>(width*numChannels, height);
  
  printf("making new image w h = %d, %d\n", width, height);
  
  

  
  rgbData = (UInt8***)make2DPointerArray<UInt8*>(width, height);
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      rgbData[y][x] = &(RGB_row_pointers_[y][x*numChannels]);
    }
  }
  
  hsvData = (UInt8***)make2DPointerArray<UInt8*>(width, height);
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      hsvData[y][x] = &(HSV_row_pointers[y][x*numChannels]);
    }
  }

  blockMatchOutputData = (UInt8***)make2DPointerArray<UInt8*>(width, height);
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      blockMatchOutputData[y][x] = &(blockMatchOutput_row_pointers[y][x*numChannels]);
    }
  }
  
  mask = (bool**)make2DPointerArray<bool>(width, height);
  floodedMask = (int**)make2DPointerArray<int>(width, height);
  

  depthMapOffset = (double**)make2DPointerArray<double>(width, height);
  diffFromMean = (double**)make2DPointerArray<double>(width, height);

  depthMap = (double**)make2DPointerArray<double>(width, height);

  matchesForPixel = (UInt8**)make2DPointerArray<UInt8>(width, height);
  
}

void
plantImage::convertRgbImageToHsv(void)
{
  for (UInt y = 0; y < height; ++y) {
    for (UInt x = 0; x < width; ++x) {
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
plantImage::convertHsvImageToRgb(void)
{
  for (UInt y = 0; y < height; ++y) {
    for (UInt x = 0; x < width; ++x) {
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



void
plantImage::getImageAverageSatVal(UInt *satP, UInt *valP, bool ignoreMask)
{
  UInt32 valSum = 0;
  UInt32 satSum = 0;
  int pixelCount = 0;
  for (UInt y = 0; y < height; ++y) {
    for (UInt x = 0; x < width; ++x) {
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
plantImage::getBlockAverageSatVal(int xx, int yy, UInt *satP, UInt *valP, UInt *pixelCountP, bool ignoreMask)
{
  UInt NormHorizontalBlocks = 16;
  UInt NormVerticalBlocks   = 16;
  UInt NormXPixelsPerBlock = width / NormHorizontalBlocks;
  UInt NormYPixelsPerBlock = height / NormVerticalBlocks;

  
  UInt valSum = 0;
  UInt satSum = 0;
  int pixelCount = 0;
  UInt xEnd = xx + NormXPixelsPerBlock;
  UInt yEnd = yy + NormYPixelsPerBlock;
  for (UInt y = yy; y < yEnd; ++y) {
    for (UInt x = 0; x < xEnd; ++x) {
      if((x<width)&&(y<height)){
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
plantImage::normalizeBlock(UInt xx, UInt yy, UInt imgAvgSat, UInt imgAvgVal, UInt blkAvgSat, UInt blkAvgVal, bool doSat, bool doVal, bool ignoreMask)
{
  UInt NormHorizontalBlocks = 16;
  UInt NormVerticalBlocks   = 16;
  UInt NormXPixelsPerBlock = width / NormHorizontalBlocks;
  UInt NormYPixelsPerBlock = height / NormVerticalBlocks;

  
  UInt xEnd = xx + NormXPixelsPerBlock;
  UInt yEnd = yy + NormYPixelsPerBlock;
  for (UInt y = yy; y < yEnd; y++) {
    for (UInt x = xx; x < xEnd; x++) {
      if((x<width)&&(y<height)){
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
plantImage::normalizeImage(bool doSat, bool doVal, bool ignoreMask)
{
  UInt NormHorizontalBlocks = 16;
  UInt NormVerticalBlocks   = 16;
  UInt NormXPixelsPerBlock = width / NormHorizontalBlocks;
  UInt NormYPixelsPerBlock = height / NormVerticalBlocks;

  
  
  UInt imgAvgSat;
  UInt imgAvgVal;

  getImageAverageSatVal(&imgAvgSat, &imgAvgVal, ignoreMask);
  Assert(imgAvgSat < 256);
  Assert(imgAvgVal < 256);
  
  epf("image avg sat:%d val:%d\n", imgAvgSat, imgAvgVal);
  
  UInt dy = height / NormVerticalBlocks;
  UInt dx = width  / NormHorizontalBlocks;
  
  for (UInt y = 0; y < height; y += dy) {
    for (UInt x = 0; x < width; x += dx) {
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
plantImage::setRgbPixel(UInt x, UInt y, UInt8 red, UInt8 green, UInt8 blue)
{
  Assert(x < width);
  Assert(y < height);
  Assert(x >= 0);
  Assert(y >= 0);
  rgbData[y][x][0] = red;
  rgbData[y][x][1] = green;
  rgbData[y][x][2] = blue;
      
}

void
plantImage::setHsvPixel(UInt x, UInt y, UInt8 hue, UInt8 sat, UInt8 val)
{
  hsvData[y][x][0] = hue;
  hsvData[y][x][1] = sat;
  hsvData[y][x][2] = val;
}


void
plantImage::dLine(UInt x0, UInt y0, UInt x1, UInt y1, UInt c)
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
plantImage::hLine(UInt x, UInt y, UInt len, UInt c)
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
plantImage::vLine(UInt32 x, UInt32 y, UInt32 len, UInt c)
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
plantImage::drawBox(UInt x, UInt y, UInt bsz, UInt c)
{
  hLine(x,     y,     bsz, c);
  hLine(x,     y+bsz, bsz, c);
  vLine(x,     y,     bsz, c);
  vLine(x+bsz, y,     bsz, c);
}

void
plantImage::hLineHSV(UInt x, UInt y, UInt len, UInt hue)
{
  for (UInt i = 0; i < len; ++i) {
    setHsvPixel(x+i, y+0, hue, 0xff, 0xff);
    setHsvPixel(x+i, y+1, hue, 0xff, 0xff);
  }
}

void
plantImage::vLineHSV(UInt32 x, UInt32 y, UInt32 len, UInt hue)
{
  for (UInt32 i = 0; i < len; ++i) {
    setHsvPixel(x,   y+i, hue, 0xff, 0xff);
    setHsvPixel(x+1, y+i, hue, 0xff, 0xff);
  }
}


void
plantImage::drawRectangleHSV(UInt x, UInt y, UInt dx, UInt dy, UInt hue)
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


double
plantImage::getMatchScore(UInt x0, UInt y0, UInt x1, UInt y1)
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
      
      if((testX0 < width)&&(testX1 < width)&&(testY0 < height)&&(testY1 < height)){
	
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
plantImage::matchBlockLeftToRight(UInt x0, UInt y0)
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
	if((y0+y < height)&&(x0+x<width)){
	  depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    
    dLine(x0, y0, x1, y1, 0xff0000);
    listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

void plantImage::matchBlockRightToLeft(UInt x0, UInt y0)
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
	if((y0+y < height)&&(x0+x<width)){
	  depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    //listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

void
plantImage::saveMapToFile(void){
  int poop;
  const char *fn = "depthThing.dat";
  poop = open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  epf("poop=%d\n", poop);
  if (poop < 0) {
    fatal("Open failed, %s: %s\n", fn, syserr());
  }
  int n = sizeof(diffFromMean);
  Assert(n == sizeof(double) * width * height);
  int w = write(poop, diffFromMean, n);
  if (w != n) {
    fatal("Write failed, %s: %s\n", fn, syserr());
  }
  close(poop);
}

void
plantImage::getMapFromFile(void){
  int poop;
  const char *fn = "depthThindat";
  poop = open(fn, O_RDONLY);
  epf("poop=%d\n", poop);
  if (poop < 0) {
    fatal("Open failed, %s: %s", fn, syserr());
  }
  int n = sizeof(depthMapOffset);
  Assert(n == sizeof(double) * width * height);
  int r = read(poop, depthMapOffset, n);
  if (r != n) {
    fatal("Read failed, %s: %s", fn, syserr());
  }
  close(poop);
}

void
plantImage::doBlockMatch(void)
{
  bool saveFile = false;
  bool readFile = false;
  int matchedBlocks = 0;
  
  for (UInt y = 0; y < height; y += 5) {
    for (UInt x = 0; x < width/2; x += 5) {
      matchBlockLeftToRight(x, y);
      matchedBlocks++;
    }
    epf("y = %d\n", y);
  }
  epf("other thing");
  for (UInt y = 0; y < height; y += 5) {
    for (UInt x = width/2; x < width; x += 5) {
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
  
  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	depthMap[y][x] /= matchesForPixel[y][x];
      }else{
	depthMap[y][x] = 0;
      }
    }
  }

  double meanDepthLen = 0;
  int totalPixlesAdded = 0;
  
  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	totalPixlesAdded++;
	meanDepthLen += depthMap[y][x];
      }
    }
  }
  meanDepthLen /= totalPixlesAdded;
  if(saveFile){
    for(int y = 0; y < (int)height; y++){
      for(int x = 0; x < (int)width; x++){
	diffFromMean[y][x] = depthMap[y][x]-meanDepthLen;
      }
    }
    saveMapToFile();
  }
  
  if(readFile){
    getMapFromFile();
    for(int y = 0; y < (int)height; y++){
      for(int x = 0; x < (int)width; x++){
	depthMap[y][x] -= depthMapOffset[y][x];
      }
    }
  }
  
  
  double maxValue = 0;
  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	if(depthMap[y][x] > maxValue){
	  maxValue = depthMap[y][x];
	}
      }
    }
  }

  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	blockMatchOutputData[y][x][0] = 0;
	blockMatchOutputData[y][x][1] = 0;
	blockMatchOutputData[y][x][2] = 0;
	double val = depthMap[y][x];
	
	val -= meanDepthLen;
	val *= -1;
	val *= 40;
	epf("val = %f", val);
	if(val > 0){
	  if(val > 255){
	    val = 255;
	  }
	  blockMatchOutputData[y][x][0] = (int)(val);
	}else{
	  val *= -1;
	  if(val > 255){
	    val = 255;
	  }
	  blockMatchOutputData[y][x][2] = (int)(val);
	}
	
      }else{
	blockMatchOutputData[y][x][0] = 0;
	blockMatchOutputData[y][x][1] = 255;
	blockMatchOutputData[y][x][2] = 0;
      }
    }
  }
  epf("depth map generated");
}







int
plantImage::hLineNumChunkPixles(int x, int y, int len, int chunkNum)
{
  int score = 0;
  Assert(x+len-1 < (int)width);
  for(int dx = 0; dx < len; dx++) {
    if(floodedMask[y][x+dx] == chunkNum) {
      score++;
    }
  }
  return score;
}

int
plantImage::vLineNumChunkPixels(int x, int y, int len, int chunkNum)
{
  int score = 0;
  Assert(y+len-1 < (int)height);
  for(int dy = 0; dy < len; dy++) {
    if(floodedMask[y+dy][x] == chunkNum) {
      score++;
    }
  }
  return score;
}

void
plantImage::splitChunkInHalf(int chunkIndex, bool horizontalSplit, int newFloodValue)
{
  
  int n = chunkInfo.size();
  assert(chunkIndex <= n);
  int floodValue = chunkInfo[chunkIndex].floodValue;
  int chunkX = chunkInfo[chunkIndex].x;
  int chunkY = chunkInfo[chunkIndex].y;
  int chunkW = chunkInfo[chunkIndex].w;
  int chunkH = chunkInfo[chunkIndex].h;
  
  int halfPixels = chunkInfo[chunkIndex].pixelCount/2;
  
  if (horizontalSplit) {
    int bestHLinePosY = 0;
    int totalPixels = 0;
    int topHalfPixels = 0;
    int bottomHalfPixels = 0;
    for(int y = chunkY; y < chunkY + chunkH; y++) {
      int x = chunkX;
      totalPixels += hLineNumChunkPixles(x, y, chunkW, floodValue);
      if (totalPixels >= halfPixels) {//keep moving line down untill more than half of pixels counted
	bestHLinePosY = y;
	break;
      }else{
	topHalfPixels = totalPixels;
      }
    }
    bottomHalfPixels = chunkInfo[chunkIndex].pixelCount - topHalfPixels;
    
    
    chunkInfo[chunkIndex].h = bestHLinePosY-chunkY+1;
    chunkInfo[chunkIndex].pixelCount = topHalfPixels;

    int newX = chunkX;
    int newY = bestHLinePosY+1;
    int newW = chunkW;
    int newH = (chunkY+chunkH)-(newY);
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++){
      for(int x = newX; x < newX+newW; x++){
	if (floodedMask[y][x] == floodValue){
	  floodedMask[y][x] = newFloodValue;
	}
      }
    }
  } else {
    int bestVLinePosX = 0;
    int totalPixels = 0;
    int topHalfPixels = 0;
    int bottomHalfPixels = 0;
    for(int x = chunkX; x < chunkX + chunkW; x++) {
      int y = chunkY;
      totalPixels += hLineNumChunkPixles(x, y, chunkH, floodValue);
      if (totalPixels >= halfPixels) {//keep moving line right untill more than half of pixels counted
	bestVLinePosX = x;
	break;
      }else{
	topHalfPixels = totalPixels;
      }
    }
    
    
    chunkInfo[chunkIndex].w = bestVLinePosX-chunkX;
    chunkInfo[chunkIndex].pixelCount = topHalfPixels;
    
    int newX = bestVLinePosX;
    int newY = chunkY;
    int newW = (chunkX+chunkW)-bestVLinePosX;
    int newH = chunkH;
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++) {
      for(int x = newX; x < newX+newW; x++) {
	if (floodedMask[y][x] == floodValue) {
	  floodedMask[y][x] = newFloodValue;
	}
      }
    }
  }
  
}

void
plantImage::floodFromThisPixel(int x, int y, int chunkNum) {
  Assert((x >= 0)&&(x < (int)width)&&(y >= 0)&&(y < (int)height));
  if (mask[y][x]) {
    floodedMask[y][x] = chunkNum;
  }
  
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= 0)&&(testX < (int)width)&&(testY >= 0)&&(testY < (int)height)) {
	  if (floodedMask[testY][testX] == 0) {
	    if (mask[testY][testX]) {
	      floodFromThisPixel(testX, testY, chunkNum);
	    }
	  }
	}
      }
    }
  }
}

void
plantImage::refloodFromThisPixelInRange(int x, int y, int x0, int y0, int x1, int y1, int oldFloodValue, int newFloodValue) {
  Assert((x >= x0)&&(x < x1)&&(y >= y0)&&(y < y1));
  if (floodedMask[y][x] == oldFloodValue) {
    floodedMask[y][x] = newFloodValue;
  }
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= x0)&&(testX < x1)&&(testY >= y0)&&(testY < y1)) {
	  if (floodedMask[testY][testX] == oldFloodValue) {
	    assert(mask[testY][testX]);
	    refloodFromThisPixelInRange(testX, testY, x0, y0, x1, y1, oldFloodValue, newFloodValue);
	  }
	}
      }
    }
  }
}

int
plantImage::refloodChunk(int chunkIndex, int startingNewFloodValue) {
  int x0 = chunkInfo[chunkIndex].x;
  int y0 = chunkInfo[chunkIndex].y;
  int x1 = x0 + chunkInfo[chunkIndex].w;
  int y1 = y0 + chunkInfo[chunkIndex].h;
  int oldFloodValue = chunkInfo[chunkIndex].floodValue;
  
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (floodedMask[y][x] == oldFloodValue) {
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, oldFloodValue, startingNewFloodValue);
	startingNewFloodValue++;
      }
    }
  }
  int highestValue = startingNewFloodValue-1;
  bool alreadyDoubleReflooded = false;
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (floodedMask[y][x] == highestValue) {
	assert(!alreadyDoubleReflooded);//should only go once
	alreadyDoubleReflooded = true;
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, highestValue, oldFloodValue);
      }
    }
  }
  return highestValue;
}

void
plantImage::calculateChunks(int numChunks) {
  chunkInfo.clear();
  badChunkInfo.clear();
  for (int chunk = 1; chunk < numChunks; chunk++) {
    int minX = 1000000;
    int maxX = -1000000;
    int minY = 1000000;
    int maxY = -1000000;
    int numPix = 0;
    for (int y = 0; y < (int)height; y++) {
      for (int x = 0; x < (int)width; x++) {
	if (floodedMask[y][x] == chunk) {
	  numPix++;
	  if (x < minX) {
	    minX = x;
	  }
	  if (x > maxX) {
	    maxX = x;
	  }
	  if (y < minY) {
	    minY = y;
	  }
	  if (y > maxY) {
	    maxY = y;
	  }
	}
      }
    }
    if(!(numPix > 0)){
      epf("chunk value %d has no pixels\n", chunk);
      assert(false);
    }
    int w = maxX-minX+1;
    int h = maxY-minY+1;
    Chunk ck(minX, minY, w, h, chunk, numPix);
    
    if (numPix >= MinPixelsWorthSaving) {
      chunkInfo.push_back(ck);
      if(w*h > ck.maxChunkSize) {
	ck.maxChunkSize = w*h;
      }
    }else{
      badChunkInfo.push_back(ck);
    }
  }
}

int
plantImage::splitChunk(int chunkIndexToSplit, int startingNewChunkValue, bool horizontalSplit) {
  splitChunkInHalf(chunkIndexToSplit, horizontalSplit, startingNewChunkValue);

  startingNewChunkValue++;
  
  startingNewChunkValue = refloodChunk(chunkIndexToSplit, startingNewChunkValue);
  
  startingNewChunkValue = refloodChunk(chunkInfo.size()-1, startingNewChunkValue);
  return startingNewChunkValue;
}

void
plantImage::drawChunkRectangles(void){
  int red     = 0;
  int green   = (0xff * 120) / 360;
  for(int i = 0; i < (int)chunkInfo.size(); i++) {
    int borderColor = green;
    drawRectangleHSV(chunkInfo[i].x, chunkInfo[i].y, chunkInfo[i].w, chunkInfo[i].h, borderColor);
  }
  
  for(int i = 0; i < (int)badChunkInfo.size(); i++) {
    int borderColor = red;
    drawRectangleHSV(badChunkInfo[i].x, badChunkInfo[i].y, badChunkInfo[i].w, badChunkInfo[i].h, borderColor);
  }
  
}

void
plantImage::destroyBadChunks(void){
  int magenta = (0xff * 300) / 360;
  for(int i = 0; i < (int) badChunkInfo.size(); i++){
    Chunk *theChunk= &badChunkInfo[i];
    int floodVal = theChunk->floodValue;
    for(int y = theChunk->y; y < theChunk->y+theChunk->h; y++){
      for(int x = theChunk->x; x < theChunk->x+theChunk->w; x++){
	if(floodedMask[y][x] == floodVal){
	  mask[y][x] = false;
	  floodedMask[y][x] = 0;
	  hsvData[y][x][0] = magenta;
	  hsvData[y][x][1] = 255;
	  hsvData[y][x][2] = 255;
	}
      }
    }
  }
}

void
plantImage::doFloodingUsingMask(){
  for(int y = 0; y < height; y++){
    memset(floodedMask[y], 0, width*sizeof(int));
  }
  
  int chunkID = 1;
  for (UInt y = 0; y < height; y++) {
    for (UInt x = 0; x < width; x++) {
      if ((floodedMask[y][x] == 0)&&(mask[y][x] == true)){
	floodFromThisPixel(x, y, chunkID);
	chunkID++;
      }
    }
  }
  
  int numChunks = chunkID;
  
  calculateChunks(numChunks);

  int maxSideLength = 400;
  for(int i = 0; i < (int)chunkInfo.size(); i++) {
    int w = chunkInfo[i].w;
    int h = chunkInfo[i].h;
    if (w>h) {
      if (w > maxSideLength) {
	numChunks = splitChunk(i, numChunks, false);
      }
    }else{
      if (h > maxSideLength) {
	numChunks = splitChunk(i, numChunks, true);
      }
    }
  }
  
  const bool colorChunks =false;

  if(colorChunks){
    int thing1   = (0xff * 0)   / 360;
    int thing2   = (0xff * 30)  / 360;
    int thing3   = (0xff * 60)  / 360;
    int thing4   = (0xff * 90)  / 360;
    int thing5   = (0xff * 120) / 360;
    int thing6   = (0xff * 150) / 360;
    int thing7   = (0xff * 180) / 360;
    int thing8   = (0xff * 210) / 360;
    int thing9   = (0xff * 240) / 360;
    int thing10  = (0xff * 270) / 360;
    int colorIndex[] = {thing1, thing2, thing3, thing4, thing5, thing6, thing7, thing8, thing9, thing10};
  
    for (UInt y = 0; y < height; y++) {
      for (UInt x = 0; x < width; x++) {
	if(floodedMask[y][x] != 0) {
	  hsvData[y][x][0] = colorIndex[floodedMask[y][x]%(sizeof(colorIndex)/sizeof(colorIndex[0]))];
	  hsvData[y][x][1] = 0xff;
	  hsvData[y][x][2] = 0xff;
	}
      }
    }
  }
  
  calculateChunks(numChunks);
  //destroyBadChunks();
  if(colorChunks){
    drawChunkRectangles();
  }
}




void
plantImage::thresholdHSV(void)
{
  // int red     = 0;
  int green   = (0xff * 120) / 360;
  int magenta = (0xff * 300) / 360;
  for(int y = 0; y < height; y++){
    memset(mask[y], true, width*sizeof(bool));
  }

  for (UInt y = 0; y < height; ++y) {
    for (UInt x = 0; x < width; ++x) {
      int hue = hsvData[y][x][0];
      int sat = hsvData[y][x][1];
      int val = hsvData[y][x][2];
      int dhue = abs(hue - magenta);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (dhue < 30) {
	if (val > 100) {
	  mask[y][x] = false;
	}
      }
      if (val<20) {
	mask[y][x] = false;
      } else {
	if (val < 35) {
	  if (sat < 200) {
	    mask[y][x] = false;
	  }
	} else if (val < 70) {
	  if (sat < 100) {
	    mask[y][x] = false;
	  }
	} else {
	  if (sat < 10) {
	    mask[y][x] = false;
	  }
	}
      }
    }
  }
  for (UInt y = 0; y < height; ++y) {
    for (UInt x = 0; x < width; ++x) {
      int hue = hsvData[y][x][0];
      // int sat = hsvData[y][x][1];
      int val = hsvData[y][x][2];
      int dhue = abs(hue - green);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (val > 20) {
	if (dhue > 50) {
	  mask[y][x] = false;
	}
      } else if (val < 150) {
	if (dhue > 60) {
	  mask[y][x] = false;
	}
      } else {
	if (dhue > 70) {
	  mask[y][x] = false;
	}
      }
    }
  }
  
  /*
  for (int i = 0; i < 300; i++) {
    bool tempMask[this->height][this->width];
    memcpy(tempMask, mask, sizeof(tempMask));
    for (UInt y = 0; y < height; y++) {
      for (UInt x = 0; x < width; x++) {
	if (!mask[y][x]) {
	  int adjacentCount = 0;
	  for (int dy = -1; dy <= 1; dy++) {
	    for (int dx = -1; dx <= 1; dx++) {
	      if (!((dy == 0)&&(dx == 0))) {
		if ((x+dx < width)&&(y+dy < height)){
		  if (mask[y+dy][x+dx]) {
		    adjacentCount++;
		  }
		}
	      }
	    }
	  }
	  int countThreshold = 5;
	  if (i < 2) {
	    countThreshold = 3;
	  } else if (i < 5) {
	    countThreshold = 4;
	  }
	  if (adjacentCount >= countThreshold) {
	    tempMask[y][x] = true;
	  }
	  //}
	}
      }
    }
    memcpy(mask, tempMask, sizeof(mask));
  }
  */


  int numPixlesMasked = 0;
  for (UInt y = 0; y < height; ++y) {
    for (UInt x = 0; x < width; ++x) {
      if (!mask[y][x]) {
	numPixlesMasked++;
	hsvData[y][x][0] = magenta;
	hsvData[y][x][1] = 0xff;
	hsvData[y][x][2] = 0xff;
      }
    }
  }
  printf("masked %d pixels\n", numPixlesMasked);
}
