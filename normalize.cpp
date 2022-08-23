static const UInt NormHorizontalBlocks = 16;
static const UInt NormVerticalBlocks   = 16;
static const UInt NormXPixelsPerBlock = ImageWidth / NormHorizontalBlocks;
static const UInt NormYPixelsPerBlock = ImageHeight / NormVerticalBlocks;

static void
getImageAverageSatVal(UInt *satP, UInt *valP, bool ignoreMask)
{
  UInt32 valSum = 0;
  UInt32 satSum = 0;
  int pixelCount = 0;
  for (UInt y = 0; y < ImageHeight; ++y) {
    for (UInt x = 0; x < ImageWidth; ++x) {
      if (g.mask[y][x] || ignoreMask){
	pixelCount++;
	satSum += g.hsvData[y][x][1];
	valSum += g.hsvData[y][x][2];
      }
    }
  }
  Assert(pixelCount > 0);
  *satP = satSum/pixelCount;
  *valP = valSum/pixelCount;
}

static void
getBlockAverageSatVal(int xx, int yy, UInt *satP, UInt *valP, UInt *pixelCountP, bool ignoreMask)
{
  UInt valSum = 0;
  UInt satSum = 0;
  int pixelCount = 0;
  UInt xEnd = xx + NormXPixelsPerBlock;
  UInt yEnd = yy + NormYPixelsPerBlock;
  for (UInt y = yy; y < yEnd; ++y) {
    for (UInt x = 0; x < xEnd; ++x) {
      if((x<ImageWidth)&&(y<ImageHeight)){
	if (g.mask[y][x] || ignoreMask){
	  pixelCount++;
	  satSum += g.hsvData[y][x][1];
	  valSum += g.hsvData[y][x][2];
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

static void
normalizeBlock(UInt xx, UInt yy, UInt imgAvgSat, UInt imgAvgVal, UInt blkAvgSat, UInt blkAvgVal, bool doSat, bool doVal, bool ignoreMask)
{
  UInt xEnd = xx + NormXPixelsPerBlock;
  UInt yEnd = yy + NormYPixelsPerBlock;
  for (UInt y = yy; y < yEnd; y++) {
    for (UInt x = xx; x < xEnd; x++) {
      if((x<ImageWidth)&&(y<ImageHeight)){
	if (g.mask[y][x] || ignoreMask){
	  if(doSat){
	    UInt s = (g.hsvData[y][x][1] * imgAvgSat) / blkAvgSat;
	    g.hsvData[y][x][1] = s;
	    s = (s > 255) ? 255 : s;
	  }
	  if(doVal){
	    UInt v = (g.hsvData[y][x][2] * imgAvgVal) / blkAvgVal;
	    v = (v > 255) ? 255 : v;
	    g.hsvData[y][x][2] = v;
	  }
	}
      }
    }
  }
}

static void
normalizeImage(bool doSat, bool doVal, bool ignoreMask)
{
  UInt imgAvgSat;
  UInt imgAvgVal;

  getImageAverageSatVal(&imgAvgSat, &imgAvgVal, ignoreMask);
  Assert(imgAvgSat < 256);
  Assert(imgAvgVal < 256);
  
  epf("image avg sat:%d val:%d", imgAvgSat, imgAvgVal);
  
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
