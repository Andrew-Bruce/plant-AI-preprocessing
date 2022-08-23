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
#include <jpeglib.h>
#include <png.h>
#include "utils.cpp"
#include "pnm.cpp"

#define printf dont_use_printf_use_epf

class Chunk {
public:
  static int maxChunkSize;
  int x;
  int y;
  int w;
  int h;
  int floodValue;
  int pixelCount;
  Chunk(void) { }
  Chunk(int _x, int _y, int _w, int _h, int _floodValue, int _pixelCount) {
    x = _x;
    y = _y;
    w = _w;
    h = _h;
    floodValue = _floodValue;
    pixelCount = _pixelCount;
  }
};

int Chunk::maxChunkSize = 0;

static const char ImageFile[] = "images/plant4.jpg";

static const UInt ImageWidth  = 1023;
static const UInt ImageHeight = 683;

static const UInt RightShift = 550;
static const UInt DownShift =  48;
static const UInt BlockSize =  20;
static const UInt VerticalSearchWindow = 30;
static const UInt HorizontalSearchWindow = 30;

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
  UInt8 hsvData[ImageHeight][ImageWidth][3];
  UInt8 rgbData[ImageHeight][ImageWidth][3];
  UInt8 tmpData[ImageHeight][ImageWidth][3];
  bool mask[ImageHeight][ImageWidth];
  int floodedMask[ImageHeight][ImageWidth];
  std::vector<Chunk> chunkInfo;
  std::vector<Chunk> badChunkInfo;


  double depthMapOffset[ImageHeight][ImageWidth];
  double diffFromMean[ImageHeight][ImageWidth];

  UInt8 blockMatchOutput[ImageHeight][ImageWidth][3];
  double depthMap[ImageHeight][ImageWidth];
  UInt8 matchesForPixel[ImageHeight][ImageWidth];

  Globals(void){

  }
};
static Globals g = Globals();

#include "do_args.cpp"
#include "readJpeg.cpp"
#include "png.cpp"
#include "hsv.cpp"
#include "normalize.cpp"
#include "draw.cpp"
#include "convert.cpp"
#include "blockMatch.cpp"
#include "chunkFlooding.cpp"

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

static void
saveThresholdedImage(void)
{
  pngWriteRgbPixelsToFile("chunks/outputThing.png", &g.rgbData[0][0][0], ImageWidth, ImageHeight);
  epf("image saved");
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

static void
saveFloodedRegions(void)
{
  int startingNum = getLastNumber();
  char filename[256];
  int n = g.chunkInfo.size();
  epf("Saving %d flooded regions", n);
  UInt8 *pixels = (UInt8 *) Malloc(Chunk::maxChunkSize * 3);
  for (int i = 0; i < n; ++i) {
    Chunk ck = g.chunkInfo[i];
    epf("Region xy=[%d,%d] wh=[%d,%d] fv=%d, pixCount=%d",
	ck.x, ck.y, ck.w, ck.h,	ck.floodValue, ck.pixelCount);
    memset(pixels, 0xff, ck.w*ck.h*3); // Set pixels to all white
    for (int y = 0; y < ck.h; ++y) {
      for (int x = 0; x < ck.w; ++x) {
	if (g.floodedMask[y+ck.y][x+ck.x] == ck.floodValue) {
	  UInt8 *p = &pixels[(y*ck.w + x) * 3];
	  UInt8 *t = &g.rgbData[y+ck.y][x+ck.x][0];
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
      Chunk ck = g.chunkInfo[i];
      memset(pixels, 0xff, ck.w*ck.h*3); // Set pixels to all white
      for (int y = 0; y < ck.h; ++y) {
	for (int x = 0; x < ck.w; ++x) {
	  if (g.floodedMask[y+ck.y][x+ck.x] == ck.floodValue) {
	    UInt8 *p = &pixels[(y*ck.w + x) * 3];
	    UInt8 *t = &g.blockMatchOutput[y+ck.y][x+ck.x][0];
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
  
  memset(g.mask, true, sizeof(g.mask));
  
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      int hue = g.hsvData[y][x][0];
      int sat = g.hsvData[y][x][1];
      int val = g.hsvData[y][x][2];
      int dhue = abs(hue - magenta);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (dhue < 30) {
	if (val > 100) {
	  g.mask[y][x] = false;
	}
      }
      if (val<20) {
	g.mask[y][x] = false;
      } else {
	if (val < 35) {
	  if (sat < 200) {
	    g.mask[y][x] = false;
	  }
	} else if (val < 70) {
	  if (sat < 100) {
	    g.mask[y][x] = false;
	  }
	} else {
	  if (sat < 10) {
	    g.mask[y][x] = false;
	  }
	}
      }
    }
  }
  
  
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      int hue = g.hsvData[y][x][0];
      // int sat = g.hsvData[y][x][1];
      int val = g.hsvData[y][x][2];
      int dhue = abs(hue - green);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (val > 20) {
	if (dhue > 50) {
	  g.mask[y][x] = false;
	}
      } else if (val < 150) {
	if (dhue > 60) {
	  g.mask[y][x] = false;
	}
      } else {
	if (dhue > 70) {
	  g.mask[y][x] = false;
	}
      }
    }
  }
  
  
  for (int i = 0; i < 300; i++) {
    bool tempMask[ImageHeight][ImageWidth];
    memcpy(tempMask, g.mask, sizeof(tempMask));
    for (UInt y = 0; y < ImageHeight; y++) {
      for (UInt x = 0; x < ImageWidth; x++) {
	if (!g.mask[y][x]) {
	  int adjacentCount = 0;
	  for (int dy = -1; dy <= 1; dy++) {
	    for (int dx = -1; dx <= 1; dx++) {
	      if (!((dy == 0)&&(dx == 0))) {
		if ((x+dx < ImageWidth)&&(y+dy < ImageHeight)){
		  if (g.mask[y+dy][x+dx]) {
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
    memcpy(g.mask, tempMask, sizeof(g.mask));
  }
  
  
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      if (!g.mask[y][x]) {
	g.hsvData[y][x][0] = magenta;
	g.hsvData[y][x][1] = 0xff;
	g.hsvData[y][x][2] = 0xff;
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
  epf("Starting up.");
  doCommandLineArgs(&argc, &argv, &arg_tab[0], 0, 0, UsageString);
  checkArgs();
  g.imageWidth = ImageWidth;
  g.imageHeight = ImageHeight;
  UInt8 *pp = &g.rgbData[0][0][0];
  readJpeg(ImageFile, pp, &g.imageWidth, &g.imageHeight);
  convertRgbImageToHsv();
  memset(g.mask, true, sizeof(g.mask));
  
  if (g.flags.threshold) {
    if (g.flags.normalizeSat || g.flags.normalizeVal) {
      bool doSat = g.flags.normalizeSat;
      bool doVal = g.flags.normalizeVal;
      bool ignoreMask = true;
      normalizeImage(doSat, doVal, ignoreMask);
    }
    thresholdHSV();
  }
  
  if (g.flags.threshold) {
    if (g.flags.chopup) {
      epf("start flooding");
      doFloodingUsingMask();
      epf("end flooding");
    }
  }
  if (g.flags.blockmatch) {
    doBlockMatch();
  }
  
  //writePnmToStdout((UInt8*)g.blockMatchOutput, ImageWidth, ImageHeight);
  
  if (g.flags.savefr) {
    convertHsvImageToRgb();
    saveFloodedRegions();
  }
  if(g.flags.saveImage){
    convertHsvImageToRgb();
    saveThresholdedImage();
  }
  
  //convertHsvImageToRgb();
  //writePnmToStdout(pp, ImageWidth, ImageHeight);
  
  epf("done and stuff\n");
  return 0;
}
