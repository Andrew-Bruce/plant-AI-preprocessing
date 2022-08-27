#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <dirent.h>
#include <algorithm>
#include <cassert>
#include <string>
#include <png.h>

#include <cstdarg>
#include <cstdio>

#include "common/utils.hpp"
#include "common/pnm.hpp"
#include "common/do_args.hpp"


#include "common/jpeg.hpp"
#include "common/png.hpp"
#include "common/hsv.hpp"

#include "chunk.hpp"

#include "imageProcessor.hpp"









#define printf dont_use_printf_use_epf



int Chunk::maxChunkSize = 0;

static const char ImageFile[] = "images/plant4.jpg";





static const int MinPixelsWorthSaving = 1000;

class Globals {
public:
  UInt imageWidth;
  UInt imageHeight;
  struct {
    bool chopup;
    bool savefr;
    bool threshold;
    bool normalizeVal;
    bool normalizeSat;
    bool normalizeBoth;
    bool normalizeGreen;
    bool roundtrip;
    bool blockmatch;
    bool saveImage;
  } flags;
  
  image currentImage;
  

  Globals(void){

  }
};
static Globals g = Globals();





static const char UsageString[] = "hsv_tool <options>";
static const struct args arg_tab[] = {
  { "--help",	   ARG_USAGE,      0,                        "Print this help message"            },
  { "-chopup",     ARG_BOOL_SET,   &g.flags.chopup,          "segment into separate plants"       },
  { "-savefr",     ARG_BOOL_SET,   &g.flags.savefr,          "segmented plants as png"            },
  { "-threshold",  ARG_BOOL_SET,   &g.flags.threshold,       "Threshold green seedlings"          },
  { "-normval",    ARG_BOOL_SET,   &g.flags.normalizeVal,    "Normalize brightness"               },
  { "-normsat",    ARG_BOOL_SET,   &g.flags.normalizeSat,    "Normalize saturation"               },
  { "-normboth",   ARG_BOOL_SET,   &g.flags.normalizeBoth,   "Normalize sat & val"                },
  { "-blockmatch", ARG_BOOL_SET,   &g.flags.blockmatch,      "Block match"                        },
  { "-saveimage",  ARG_BOOL_SET,   &g.flags.saveImage,       "Save final image in hsv data as png"},
  { 0,             ARG_NULL,       0,                   0                                         },
};



int
hLineNumChunkPixles(int x, int y, int len, int chunkNum)
{
  int score = 0;
  Assert(x+len-1 < (int)ImageWidth);
  for(int dx = 0; dx < len; dx++) {
    if(g.currentImage.floodedMask[y][x+dx] == chunkNum) {
      score++;
    }
  }
  return score;
}

int
vLineNumChunkPixels(int x, int y, int len, int chunkNum)
{
  int score = 0;
  Assert(y+len-1 < (int)ImageHeight);
  for(int dy = 0; dy < len; dy++) {
    if(g.currentImage.floodedMask[y+dy][x] == chunkNum) {
      score++;
    }
  }
  return score;
}

static void
splitChunkInHalf(int chunkIndex, bool horizontalSplit, int newFloodValue)
{
  
  int n = g.currentImage.chunkInfo.size();
  assert(chunkIndex <= n);
  int floodValue = g.currentImage.chunkInfo[chunkIndex].floodValue;
  int chunkX = g.currentImage.chunkInfo[chunkIndex].x;
  int chunkY = g.currentImage.chunkInfo[chunkIndex].y;
  int chunkW = g.currentImage.chunkInfo[chunkIndex].w;
  int chunkH = g.currentImage.chunkInfo[chunkIndex].h;
  epf("splitting chunk %d new half %d\n", floodValue, newFloodValue);
  int halfPixels = g.currentImage.chunkInfo[chunkIndex].pixelCount/2;
  
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
    bottomHalfPixels = g.currentImage.chunkInfo[chunkIndex].pixelCount - topHalfPixels;
    
    
    g.currentImage.chunkInfo[chunkIndex].h = bestHLinePosY-chunkY+1;
    g.currentImage.chunkInfo[chunkIndex].pixelCount = topHalfPixels;

    int newX = chunkX;
    int newY = bestHLinePosY+1;
    int newW = chunkW;
    int newH = (chunkY+chunkH)-(newY);
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    g.currentImage.chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++){
      for(int x = newX; x < newX+newW; x++){
	if (g.currentImage.floodedMask[y][x] == floodValue){
	  g.currentImage.floodedMask[y][x] = newFloodValue;
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
    
    
    g.currentImage.chunkInfo[chunkIndex].w = bestVLinePosX-chunkX;
    g.currentImage.chunkInfo[chunkIndex].pixelCount = topHalfPixels;
    
    int newX = bestVLinePosX;
    int newY = chunkY;
    int newW = (chunkX+chunkW)-bestVLinePosX;
    int newH = chunkH;
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    g.currentImage.chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++) {
      for(int x = newX; x < newX+newW; x++) {
	if (g.currentImage.floodedMask[y][x] == floodValue) {
	  g.currentImage.floodedMask[y][x] = newFloodValue;
	}
      }
    }
  }
  
}

void
floodFromThisPixel(int x, int y, int chunkNum) {
  Assert((x >= 0)&&(x < (int)ImageWidth)&&(y >= 0)&&(y < (int)ImageHeight));
  if (g.currentImage.mask[y][x]) {
    g.currentImage.floodedMask[y][x] = chunkNum;
  }
  
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= 0)&&(testX < (int)ImageWidth)&&(testY >= 0)&&(testY < (int)ImageHeight)) {
	  if (g.currentImage.floodedMask[testY][testX] == 0) {
	    if (g.currentImage.mask[testY][testX]) {
	      floodFromThisPixel(testX, testY, chunkNum);
	    }
	  }
	}
      }
    }
  }
}

void
refloodFromThisPixelInRange(int x, int y, int x0, int y0, int x1, int y1, int oldFloodValue, int newFloodValue) {
  Assert((x >= x0)&&(x < x1)&&(y >= y0)&&(y < y1));
  if (g.currentImage.floodedMask[y][x] == oldFloodValue) {
    g.currentImage.floodedMask[y][x] = newFloodValue;
  }
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= x0)&&(testX < x1)&&(testY >= y0)&&(testY < y1)) {
	  if (g.currentImage.floodedMask[testY][testX] == oldFloodValue) {
	    assert(g.currentImage.mask[testY][testX]);
	    refloodFromThisPixelInRange(testX, testY, x0, y0, x1, y1, oldFloodValue, newFloodValue);
	  }
	}
      }
    }
  }
}

int
refloodChunk(int chunkIndex, int startingNewFloodValue) {
  int x0 = g.currentImage.chunkInfo[chunkIndex].x;
  int y0 = g.currentImage.chunkInfo[chunkIndex].y;
  int x1 = x0 + g.currentImage.chunkInfo[chunkIndex].w;
  int y1 = y0 + g.currentImage.chunkInfo[chunkIndex].h;
  int oldFloodValue = g.currentImage.chunkInfo[chunkIndex].floodValue;
  
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (g.currentImage.floodedMask[y][x] == oldFloodValue) {
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, oldFloodValue, startingNewFloodValue);
	epf("reflooded %d to new value %d\n", oldFloodValue, startingNewFloodValue);
	startingNewFloodValue++;
      }
    }
  }
  int highestValue = startingNewFloodValue-1;
  bool alreadyDoubleReflooded = false;
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (g.currentImage.floodedMask[y][x] == highestValue) {
	assert(!alreadyDoubleReflooded);//should only go once
	alreadyDoubleReflooded = true;
	epf("re-reflooded %d to old value %d\n", highestValue, oldFloodValue);
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, highestValue, oldFloodValue);
      }
    }
  }

  epf("highest value is %d\n", highestValue);
  return highestValue;
}

void
calculateChunks(int numChunks) {
  g.currentImage.chunkInfo.clear();
  g.currentImage.badChunkInfo.clear();
  for (int chunk = 1; chunk < numChunks; chunk++) {
    int minX = 1000000;
    int maxX = -1000000;
    int minY = 1000000;
    int maxY = -1000000;
    int numPix = 0;
    for (int y = 0; y < (int)ImageHeight; y++) {
      for (int x = 0; x < (int)ImageWidth; x++) {
	if (g.currentImage.floodedMask[y][x] == chunk) {
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
      g.currentImage.chunkInfo.push_back(ck);
      if(w*h > ck.maxChunkSize) {
	ck.maxChunkSize = w*h;
      }
    }else{
      g.currentImage.badChunkInfo.push_back(ck);
    }
  }
}

int
splitChunk(int chunkIndexToSplit, int startingNewChunkValue, bool horizontalSplit) {
  splitChunkInHalf(chunkIndexToSplit, horizontalSplit, startingNewChunkValue);
  startingNewChunkValue++;
  epf("reflooding chunk %d\n", g.currentImage.chunkInfo[chunkIndexToSplit].floodValue);
  startingNewChunkValue = refloodChunk(chunkIndexToSplit, startingNewChunkValue);
  epf("reflooding chunk %d\n", g.currentImage.chunkInfo[g.currentImage.chunkInfo.size()-1].floodValue);
  startingNewChunkValue = refloodChunk(g.currentImage.chunkInfo.size()-1, startingNewChunkValue);
  return startingNewChunkValue;
}

void
drawChunkRectangles() {
  int red     = 0;
  int green   = (0xff * 120) / 360;
  for(int i = 0; i < (int)g.currentImage.chunkInfo.size(); i++) {
    int borderColor = green;
    g.currentImage.drawRectangleHSV(g.currentImage.chunkInfo[i].x, g.currentImage.chunkInfo[i].y, g.currentImage.chunkInfo[i].w, g.currentImage.chunkInfo[i].h, borderColor);
  }
  
  for(int i = 0; i < (int)g.currentImage.badChunkInfo.size(); i++) {
    int borderColor = red;
    g.currentImage.drawRectangleHSV(g.currentImage.badChunkInfo[i].x, g.currentImage.badChunkInfo[i].y, g.currentImage.badChunkInfo[i].w, g.currentImage.badChunkInfo[i].h, borderColor);
  }
  
}

void
destroyBadChunks(){
  int magenta = (0xff * 300) / 360;
  for(int i = 0; i < (int) g.currentImage.badChunkInfo.size(); i++){
    Chunk *theChunk= &g.currentImage.badChunkInfo[i];
    int floodVal = theChunk->floodValue;
    for(int y = theChunk->y; y < theChunk->y+theChunk->h; y++){
      for(int x = theChunk->x; x < theChunk->x+theChunk->w; x++){
	if(g.currentImage.floodedMask[y][x] == floodVal){
	  g.currentImage.mask[y][x] = false;
	  g.currentImage.floodedMask[y][x] = 0;
	  g.currentImage.hsvData[y][x][0] = magenta;
	  g.currentImage.hsvData[y][x][1] = 255;
	  g.currentImage.hsvData[y][x][2] = 255;
	}
      }
    }
  }
}

void
doFloodingUsingMask(){
  memset(g.currentImage.floodedMask, 0, sizeof(g.currentImage.floodedMask));
  epf("flooding idk\n");
  
  //goes through g.mask which is the output from thresholding. when it finds a pixel in the mask that isn't part of any chunk (floodedMask == 0) it floods from that pixel. floodedmask has 0 for a pixel not part of any chunk, and any other value is the ID of the chunk
  int chunkID = 1;
  for (UInt y = 0; y < ImageHeight; y++) {
    for (UInt x = 0; x < ImageWidth; x++) {
      if ((g.currentImage.floodedMask[y][x] == 0)&&(g.currentImage.mask[y][x] == true)){
	floodFromThisPixel(x, y, chunkID);
	chunkID++;
      }
    }
  }
  
  int numChunks = chunkID;
  epf("calculating initial chunks num of %d\n", numChunks);
  calculateChunks(numChunks);

  int maxSideLength = 400;
  for(int i = 0; i < (int)g.currentImage.chunkInfo.size(); i++) {
    int w = g.currentImage.chunkInfo[i].w;
    int h = g.currentImage.chunkInfo[i].h;
    if (w>h) {
      if (w > maxSideLength) {
	epf("splitchunk\n");
	numChunks = splitChunk(i, numChunks, false);
	epf("num chunks is now %d\n", numChunks);
      }
    }else{
      if (h > maxSideLength) {
	epf("splitchunk\n");
	numChunks = splitChunk(i, numChunks, true);
	epf("num chunks is now %d\n", numChunks);
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
  
    for (UInt y = 0; y < ImageHeight; y++) {
      for (UInt x = 0; x < ImageWidth; x++) {
	if(g.currentImage.floodedMask[y][x] != 0) {
	  g.currentImage.hsvData[y][x][0] = colorIndex[g.currentImage.floodedMask[y][x]%(sizeof(colorIndex)/sizeof(colorIndex[0]))];
	  g.currentImage.hsvData[y][x][1] = 0xff;
	  g.currentImage.hsvData[y][x][2] = 0xff;
	}
      }
    }
  }
  epf("calculating post split chunks max of %d\n", numChunks);
  calculateChunks(numChunks);
  //destroyBadChunks();
  if(colorChunks){
    drawChunkRectangles();
  }
}


std::vector<std::string>
getListOfFiles(const char *s)
{
  std::vector<std::string> names;
  struct dirent *dp;
  DIR *dirp = opendir(s);
  if (dirp == NULL) {
    fatal("Can't open %s: %s", s, syserr());
  }
  chdir(s);
  while ((dp = readdir(dirp)) != NULL) {
    std::string nm = std::string(dp->d_name);
    if (nm[0] == '.') {
      continue;
    }
    Assert(dp->d_type == DT_REG);
    names.push_back(nm);
    
  }
  std::sort(names.begin(), names.end());
  for (int i = 0; i < (int) names.size(); ++i) {
    epf("%s\n", names[i].c_str());
  }
  chdir("..");
  closedir(dirp);
  return names;
}

void
saveThresholdedImage(void)
{
  pngWriteRgbPixelsToFile("chunks/outputThing.png", &g.currentImage.rgbData[0][0][0], ImageWidth, ImageHeight);
  epf("image saved\n");
}

int
getLastNumber(void){
  std::vector<std::string> names = getListOfFiles("chunks");
  int size = names.size();
  if(size == 0){
    return(0);
  }
  std::string last = names[size-1];
  
  int factor = 1;
  int output = 0;
  for(int i = last.length()-1; i >= 0; i--){
    if((last[i] >= '0')&&(last[i] <= '9')){
      int num = last[i] - '0';
      num *= factor;
      output += num;
      factor *= 10;
    }
  }
  return output+1;
}

void
saveFloodedRegions(void)
{
  int startingNum = getLastNumber();
  char filename[256];
  int n = g.currentImage.chunkInfo.size();
  epf("Saving %d flooded regions\n", n);
  UInt8 *pixels = (UInt8 *) Malloc(Chunk::maxChunkSize * 3);
  for (int i = 0; i < n; ++i) {
    Chunk ck = g.currentImage.chunkInfo[i];
    epf("Region xy=[%d,%d] wh=[%d,%d] fv=%d, pixCount=%d\n",
	ck.x, ck.y, ck.w, ck.h,	ck.floodValue, ck.pixelCount);
    memset(pixels, 0xff, ck.w*ck.h*3); // Set pixels to all white
    for (int y = 0; y < ck.h; ++y) {
      for (int x = 0; x < ck.w; ++x) {
	if (g.currentImage.floodedMask[y+ck.y][x+ck.x] == ck.floodValue) {
	  UInt8 *p = &pixels[(y*ck.w + x) * 3];
	  UInt8 *t = &g.currentImage.rgbData[y+ck.y][x+ck.x][0];
	  *p++ = *t++;
	  *p++ = *t++;
	  *p++ = *t++;
	}
      }
    }
    sprintf(filename, "chunks/img%05d.png", i+startingNum);
    pngWriteRgbPixelsToFile(filename, pixels, ck.w, ck.h);
  }
  
  bool saveDepthToo = g.flags.blockmatch;
  if(saveDepthToo){
    for (int i = 0; i < n; ++i) {
      Chunk ck = g.currentImage.chunkInfo[i];
      memset(pixels, 0xff, ck.w*ck.h*3); // Set pixels to all white
      for (int y = 0; y < ck.h; ++y) {
	for (int x = 0; x < ck.w; ++x) {
	  if (g.currentImage.floodedMask[y+ck.y][x+ck.x] == ck.floodValue) {
	    UInt8 *p = &pixels[(y*ck.w + x) * 3];
	    UInt8 *t = &g.currentImage.blockMatchOutput[y+ck.y][x+ck.x][0];
	    *p++ = *t++;
	    *p++ = *t++;
	    *p++ = *t++;
	  }
	}
      }
      sprintf(filename, "chunks/img_depth%05d.png", i+startingNum);
      pngWriteRgbPixelsToFile(filename, pixels, ck.w, ck.h);
    }
  }
  
  free(pixels);
}


static void
thresholdHSV(void)
{
  // int red     = 0;
  int green   = (0xff * 120) / 360;
  int magenta = (0xff * 300) / 360;
  
  memset(g.currentImage.mask, true, sizeof(g.currentImage.mask));
  
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      int hue = g.currentImage.hsvData[y][x][0];
      int sat = g.currentImage.hsvData[y][x][1];
      int val = g.currentImage.hsvData[y][x][2];
      int dhue = abs(hue - magenta);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (dhue < 30) {
	if (val > 100) {
	  g.currentImage.mask[y][x] = false;
	}
      }
      if (val<20) {
	g.currentImage.mask[y][x] = false;
      } else {
	if (val < 35) {
	  if (sat < 200) {
	    g.currentImage.mask[y][x] = false;
	  }
	} else if (val < 70) {
	  if (sat < 100) {
	    g.currentImage.mask[y][x] = false;
	  }
	} else {
	  if (sat < 10) {
	    g.currentImage.mask[y][x] = false;
	  }
	}
      }
    }
  }
  
  
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      int hue = g.currentImage.hsvData[y][x][0];
      // int sat = g.currentImage.hsvData[y][x][1];
      int val = g.currentImage.hsvData[y][x][2];
      int dhue = abs(hue - green);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (val > 20) {
	if (dhue > 50) {
	  g.currentImage.mask[y][x] = false;
	}
      } else if (val < 150) {
	if (dhue > 60) {
	  g.currentImage.mask[y][x] = false;
	}
      } else {
	if (dhue > 70) {
	  g.currentImage.mask[y][x] = false;
	}
      }
    }
  }
  
  
  for (int i = 0; i < 300; i++) {
    bool tempMask[ImageHeight][ImageWidth];
    memcpy(tempMask, g.currentImage.mask, sizeof(tempMask));
    for (UInt y = 0; y < ImageHeight; y++) {
      for (UInt x = 0; x < ImageWidth; x++) {
	if (!g.currentImage.mask[y][x]) {
	  int adjacentCount = 0;
	  for (int dy = -1; dy <= 1; dy++) {
	    for (int dx = -1; dx <= 1; dx++) {
	      if (!((dy == 0)&&(dx == 0))) {
		if ((x+dx < ImageWidth)&&(y+dy < ImageHeight)){
		  if (g.currentImage.mask[y+dy][x+dx]) {
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
    memcpy(g.currentImage.mask, tempMask, sizeof(g.currentImage.mask));
  }
  
  
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      if (!g.currentImage.mask[y][x]) {
	g.currentImage.hsvData[y][x][0] = magenta;
	g.currentImage.hsvData[y][x][1] = 0xff;
	g.currentImage.hsvData[y][x][2] = 0xff;
      }
    }
  }
}


static void
checkArgs(void)
{
  if (g.flags.normalizeBoth) {
    g.flags.normalizeVal = true;
    g.flags.normalizeSat = true;
  }
  if (! g.flags.threshold) {
    if (g.flags.chopup) {
      fatal("Can't chop-up without thresholding");
    }
  }
  if (g.flags.savefr) {
    if (!g.flags.chopup) {
      fatal("Can't save segments without chop-up");
    }
  }
}

int
main(int argc, char **argv)
{
  epf("Starting up.\n");
  doCommandLineArgs(&argc, &argv, &arg_tab[0], 0, 0, UsageString);
  checkArgs();
  g.imageWidth = ImageWidth;
  g.imageHeight = ImageHeight;
  UInt8 *pp = &g.currentImage.rgbData[0][0][0];
  readJpeg(ImageFile, pp, &g.imageWidth, &g.imageHeight);


  g.currentImage.convertRgbImageToHsv();


  memset(g.currentImage.mask, true, sizeof(g.currentImage.mask));
  
  if (g.flags.threshold) {
    if (g.flags.normalizeSat || g.flags.normalizeVal) {
      bool doSat = g.flags.normalizeSat;
      bool doVal = g.flags.normalizeVal;
      bool ignoreMask = true;
      g.currentImage.normalizeImage(doSat, doVal, ignoreMask);
    }
    thresholdHSV();
  }
  
  if (g.flags.threshold) {
    if (g.flags.chopup) {
      epf("start flooding\n");
      doFloodingUsingMask();
      epf("end flooding\n");
    }
  }
  if (g.flags.blockmatch) {
    g.currentImage.doBlockMatch();
  }
  
  //writePnmToStdout((UInt8*)g.blockMatchOutput, ImageWidth, ImageHeight);
  
  if (g.flags.savefr) {
    g.currentImage.convertHsvImageToRgb();
    saveFloodedRegions();
  }
  if(g.flags.saveImage){
    g.currentImage.convertHsvImageToRgb();
    saveThresholdedImage();
  }
  
  //convertHsvImageToRgb();
  //writePnmToStdout(pp, ImageWidth, ImageHeight);
  
  epf("done and stuff\n");
  return 0;
}
