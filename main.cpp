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



static const char ImageFile[] = "images/plant4.jpg";









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
  
  plantImage currentImage;
  

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
  /*for (int i = 0; i < (int) names.size(); ++i) {
    epf("%s\n", names[i].c_str());
    }*/
  chdir("..");
  closedir(dirp);
  return names;
}

void
saveThresholdedImage(void)
{
  //writePng("chunks/outputThing.png", g.currentImage.RGB_row_pointers, g.currentImage.width, g.currentImage.height, 8, 2);
  writeJpeg("chunks/outputThing.png", g.currentImage.RGB_row_pointers, g.currentImage.width, g.currentImage.height);
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
saveChunk(Chunk* chunk, int fileNumber){
  
  char filename[256];

  UInt8** pixels = make2DPointerArray<UInt8>((chunk->w)*3, chunk->h);
  for(int y = 0; y < chunk->h; y++){
    memset(pixels[y], 0xff, chunk->w*3*sizeof(UInt8));
  }
  epf("Saving region xy=[%d,%d] wh=[%d,%d] fv=%d, pixCount=%d\n", chunk->x, chunk->y, chunk->w, chunk->h, chunk->floodValue, chunk->pixelCount);
  for (int y = 0; y < chunk->h; ++y) {
    for (int x = 0; x < chunk->w; ++x) {
      if (g.currentImage.floodedMask[y+chunk->y][x+chunk->x] == chunk->floodValue) {
	//UInt8 *p = &pixels[(y*chunk->w + x) * 3];
	UInt8 *p = &pixels[y][x*3];
	UInt8 *t = &g.currentImage.rgbData[y+chunk->y][x+chunk->x][0];
	*p++ = *t++;
	*p++ = *t++;
	*p++ = *t++;
      }
    }
  }
  sprintf(filename, "chunks/img%05d.png", fileNumber);
  writePng(filename, pixels, chunk->w, chunk->h, 8, 2);

  bool saveDepthToo = g.flags.blockmatch;

  if(saveDepthToo){
    for(int y = 0; y < chunk->h; y++){
      memset(pixels[y], 0xff, chunk->w*3*sizeof(UInt8));
    }
    for (int y = 0; y < chunk->h; ++y) {
      for (int x = 0; x < chunk->w; ++x) {
	if (g.currentImage.floodedMask[y+chunk->y][x+chunk->x] == chunk->floodValue) {
	  UInt8 *p = &pixels[y][x*3];
	  UInt8 *t = &g.currentImage.blockMatchOutputData[y+chunk->y][x+chunk->x][0];
	  *p++ = *t++;
	  *p++ = *t++;
	  *p++ = *t++;
	}
      }
    }
    sprintf(filename, "chunks/img_depth%05d.png", fileNumber);
    writePng(filename, pixels, chunk->w, chunk->h, 8, 2);
  }
  free2DPointerArray<UInt8>(pixels, chunk->w*3, chunk->h);
}

void
saveFloodedRegions(void)
{
  
  int n = g.currentImage.chunkInfo.size();
  epf("Saving %d flooded regions\n", n);
  
  int startingNum = getLastNumber();
  for (int i = 0; i < n; ++i) {
    saveChunk(&g.currentImage.chunkInfo[i], startingNum+i);
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


  
  UInt8** row_pointers = readJpeg(ImageFile, &g.imageWidth, &g.imageHeight);

  g.currentImage = plantImage(row_pointers, g.imageWidth, g.imageHeight);
  g.currentImage.convertRgbImageToHsv();

  //writeJpeg("chunks/outputThing.png", g.currentImage.RGB_row_pointers, g.currentImage.width, g.currentImage.height);
  
  

  
	 
  
  if (g.flags.threshold) {
    if (g.flags.normalizeSat || g.flags.normalizeVal) {
      bool doSat = g.flags.normalizeSat;
      bool doVal = g.flags.normalizeVal;
      bool ignoreMask = true;
      g.currentImage.normalizeImage(doSat, doVal, ignoreMask);
    }
    g.currentImage.thresholdHSV();
  }
  if (g.flags.threshold) {
    if (g.flags.chopup) {
      epf("start flooding\n");
      g.currentImage.doFloodingUsingMask();
      epf("end flooding\n");
    }
  }
  if (g.flags.blockmatch) {
    g.currentImage.doBlockMatch();
  }
  
  if (g.flags.savefr) {
    g.currentImage.convertHsvImageToRgb();
    saveFloodedRegions();
  }
  if(g.flags.saveImage){
    g.currentImage.convertHsvImageToRgb();
    saveThresholdedImage();
  }
  
  epf("done and stuff\n");
  return 0;
  
}
