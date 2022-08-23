//
//  blockMatch.cpp
//

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
static double
getMatchScore(UInt x0, UInt y0, UInt x1, UInt y1)
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
	
	if((abs(g.hsvData[testY0][testX0][0]-magenta) > 5)&&(abs(g.hsvData[testY0][testX0][0]-magenta) > 5)){
	
	  score += abs(g.hsvData[testY0][testX0][0] - g.hsvData[testY1][testX1][0]);
	  score += abs(g.hsvData[testY0][testX0][1] - g.hsvData[testY1][testX1][1]);
	  score += abs(g.hsvData[testY0][testX0][2] - g.hsvData[testY1][testX1][2]);
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

static void
matchBlockLeftToRight(UInt x0, UInt y0)
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
	  g.depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  g.matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    
    dLine(x0, y0, x1, y1, 0xff0000);
    listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

static void
matchBlockRightToLeft(UInt x0, UInt y0)
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
	  g.depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  g.matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    //listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

void
saveMapToFile(void){
  int poop;
  const char *fn = "depthThing.dat";
  poop = open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  epf("poop=%d\n", poop);
  if (poop < 0) {
    fatal("Open failed, %s: %s", fn, syserr());
  }
  int n = sizeof(g.diffFromMean);
  Assert(n == sizeof(double) * ImageWidth * ImageHeight);
  int w = write(poop, g.diffFromMean, n);
  if (w != n) {
    fatal("Write failed, %s: %s", fn, syserr());
  }
  close(poop);
}

void
getMapFromFile(void){
  int poop;
  const char *fn = "depthThing.dat";
  poop = open(fn, O_RDONLY);
  epf("poop=%d\n", poop);
  if (poop < 0) {
    fatal("Open failed, %s: %s", fn, syserr());
  }
  int n = sizeof(g.depthMapOffset);
  Assert(n == sizeof(double) * ImageWidth * ImageHeight);
  int r = read(poop, g.depthMapOffset, n);
  if (r != n) {
    fatal("Read failed, %s: %s", fn, syserr());
  }
  close(poop);
}

static void
doBlockMatch(void)
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
      if(g.matchesForPixel[y][x] != 0){
	g.depthMap[y][x] /= g.matchesForPixel[y][x];
      }else{
	g.depthMap[y][x] = 0;
      }
    }
  }

  double meanDepthLen = 0;
  int totalPixlesAdded = 0;
  
  for(int y = 0; y < (int)ImageHeight; y++){
    for(int x = 0; x < (int)ImageWidth; x++){
      if(g.matchesForPixel[y][x] != 0){
	totalPixlesAdded++;
	meanDepthLen += g.depthMap[y][x];
      }
    }
  }
  meanDepthLen /= totalPixlesAdded;
  if(saveFile){
    for(int y = 0; y < (int)ImageHeight; y++){
      for(int x = 0; x < (int)ImageWidth; x++){
	g.diffFromMean[y][x] = g.depthMap[y][x]-meanDepthLen;
      }
    }
    saveMapToFile();
  }
  
  if(readFile){
    getMapFromFile();
    for(int y = 0; y < (int)ImageHeight; y++){
      for(int x = 0; x < (int)ImageWidth; x++){
	g.depthMap[y][x] -= g.depthMapOffset[y][x];
      }
    }
  }
  
  
  double maxValue = 0;
  for(int y = 0; y < (int)ImageHeight; y++){
    for(int x = 0; x < (int)ImageWidth; x++){
      if(g.matchesForPixel[y][x] != 0){
	if(g.depthMap[y][x] > maxValue){
	  maxValue = g.depthMap[y][x];
	}
      }
    }
  }
  
 

  
  for(int y = 0; y < (int)ImageHeight; y++){
    for(int x = 0; x < (int)ImageWidth; x++){
      if(g.matchesForPixel[y][x] != 0){
	g.blockMatchOutput[y][x][0] = 0;
	g.blockMatchOutput[y][x][1] = 0;
	g.blockMatchOutput[y][x][2] = 0;
	double val = g.depthMap[y][x];
	
	val -= meanDepthLen;
	val *= -1;
	val *= 40;
	epf("val = %f", val);
	if(val > 0){
	  if(val > 255){
	    val = 255;
	  }
	  g.blockMatchOutput[y][x][0] = (int)(val);
	}else{
	  val *= -1;
	  if(val > 255){
	    val = 255;
	  }
	  g.blockMatchOutput[y][x][2] = (int)(val);
	}
	
      }else{
	g.blockMatchOutput[y][x][0] = 0;
	g.blockMatchOutput[y][x][1] = 255;
	g.blockMatchOutput[y][x][2] = 0;
      }
    }
  }
  epf("depth map generated");
}
