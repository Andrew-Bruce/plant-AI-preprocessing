int
hLineNumChunkPixles(int x, int y, int len, int chunkNum)
{
  int score = 0;
  Assert(x+len-1 < (int)ImageWidth);
  for(int dx = 0; dx < len; dx++) {
    if(g.floodedMask[y][x+dx] == chunkNum) {
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
    if(g.floodedMask[y+dy][x] == chunkNum) {
      score++;
    }
  }
  return score;
}

static void
splitChunkInHalf(int chunkIndex, bool horizontalSplit, int newFloodValue)
{
  
  int n = g.chunkInfo.size();
  assert(chunkIndex <= n);
  int floodValue = g.chunkInfo[chunkIndex].floodValue;
  int chunkX = g.chunkInfo[chunkIndex].x;
  int chunkY = g.chunkInfo[chunkIndex].y;
  int chunkW = g.chunkInfo[chunkIndex].w;
  int chunkH = g.chunkInfo[chunkIndex].h;
  epf("splitting chunk %d new half %d\n", floodValue, newFloodValue);
  int halfPixels = g.chunkInfo[chunkIndex].pixelCount/2;
  
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
    bottomHalfPixels = g.chunkInfo[chunkIndex].pixelCount - topHalfPixels;
    
    
    g.chunkInfo[chunkIndex].h = bestHLinePosY-chunkY+1;
    g.chunkInfo[chunkIndex].pixelCount = topHalfPixels;

    int newX = chunkX;
    int newY = bestHLinePosY+1;
    int newW = chunkW;
    int newH = (chunkY+chunkH)-(newY);
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    g.chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++){
      for(int x = newX; x < newX+newW; x++){
	if (g.floodedMask[y][x] == floodValue){
	  g.floodedMask[y][x] = newFloodValue;
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
    
    
    g.chunkInfo[chunkIndex].w = bestVLinePosX-chunkX;
    g.chunkInfo[chunkIndex].pixelCount = topHalfPixels;
    
    int newX = bestVLinePosX;
    int newY = chunkY;
    int newW = (chunkX+chunkW)-bestVLinePosX;
    int newH = chunkH;
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    g.chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++) {
      for(int x = newX; x < newX+newW; x++) {
	if (g.floodedMask[y][x] == floodValue) {
	  g.floodedMask[y][x] = newFloodValue;
	}
      }
    }
  }
  
}

void
floodFromThisPixel(int x, int y, int chunkNum) {
  Assert((x >= 0)&&(x < (int)ImageWidth)&&(y >= 0)&&(y < (int)ImageHeight));
  if (g.mask[y][x]) {
    g.floodedMask[y][x] = chunkNum;
  }
  
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= 0)&&(testX < (int)ImageWidth)&&(testY >= 0)&&(testY < (int)ImageHeight)) {
	  if (g.floodedMask[testY][testX] == 0) {
	    if (g.mask[testY][testX]) {
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
  if (g.floodedMask[y][x] == oldFloodValue) {
    g.floodedMask[y][x] = newFloodValue;
  }
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= x0)&&(testX < x1)&&(testY >= y0)&&(testY < y1)) {
	  if (g.floodedMask[testY][testX] == oldFloodValue) {
	    assert(g.mask[testY][testX]);
	    refloodFromThisPixelInRange(testX, testY, x0, y0, x1, y1, oldFloodValue, newFloodValue);
	  }
	}
      }
    }
  }
}

int
refloodChunk(int chunkIndex, int startingNewFloodValue) {
  int x0 = g.chunkInfo[chunkIndex].x;
  int y0 = g.chunkInfo[chunkIndex].y;
  int x1 = x0 + g.chunkInfo[chunkIndex].w;
  int y1 = y0 + g.chunkInfo[chunkIndex].h;
  int oldFloodValue = g.chunkInfo[chunkIndex].floodValue;
  
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (g.floodedMask[y][x] == oldFloodValue) {
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, oldFloodValue, startingNewFloodValue);
	epf("reflooded %d to new value %d", oldFloodValue, startingNewFloodValue);
	startingNewFloodValue++;
      }
    }
  }
  int highestValue = startingNewFloodValue-1;
  bool alreadyDoubleReflooded = false;
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (g.floodedMask[y][x] == highestValue) {
	assert(!alreadyDoubleReflooded);//should only go once
	alreadyDoubleReflooded = true;
	epf("re-reflooded %d to old value %d", highestValue, oldFloodValue);
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, highestValue, oldFloodValue);
      }
    }
  }

  epf("highest value is %d", highestValue);
  return highestValue;
}

void
calculateChunks(int numChunks) {
  g.chunkInfo.clear();
  g.badChunkInfo.clear();
  for (int chunk = 1; chunk < numChunks; chunk++) {
    int minX = 1000000;
    int maxX = -1000000;
    int minY = 1000000;
    int maxY = -1000000;
    int numPix = 0;
    for (int y = 0; y < (int)ImageHeight; y++) {
      for (int x = 0; x < (int)ImageWidth; x++) {
	if (g.floodedMask[y][x] == chunk) {
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
      epf("chunk value %d has no pixels", chunk);
      assert(false);
    }
    int w = maxX-minX+1;
    int h = maxY-minY+1;
    Chunk ck(minX, minY, w, h, chunk, numPix);
    
    if (numPix >= MinPixelsWorthSaving) {
      g.chunkInfo.push_back(ck);
      if(w*h > ck.maxChunkSize) {
	ck.maxChunkSize = w*h;
      }
    }else{
      g.badChunkInfo.push_back(ck);
    }
  }
}

int
splitChunk(int chunkIndexToSplit, int startingNewChunkValue, bool horizontalSplit) {
  splitChunkInHalf(chunkIndexToSplit, horizontalSplit, startingNewChunkValue);
  startingNewChunkValue++;
  epf("reflooding chunk %d", g.chunkInfo[chunkIndexToSplit].floodValue);
  startingNewChunkValue = refloodChunk(chunkIndexToSplit, startingNewChunkValue);
  epf("reflooding chunk %d", g.chunkInfo[g.chunkInfo.size()-1].floodValue);
  startingNewChunkValue = refloodChunk(g.chunkInfo.size()-1, startingNewChunkValue);
  return startingNewChunkValue;
}

void
drawChunkRectangles() {
  int red     = 0;
  int green   = (0xff * 120) / 360;
  for(int i = 0; i < (int)g.chunkInfo.size(); i++) {
    int borderColor = green;
    drawRectangleHSV(g.chunkInfo[i].x, g.chunkInfo[i].y, g.chunkInfo[i].w, g.chunkInfo[i].h, borderColor);
  }
  
  for(int i = 0; i < (int)g.badChunkInfo.size(); i++) {
    int borderColor = red;
    drawRectangleHSV(g.badChunkInfo[i].x, g.badChunkInfo[i].y, g.badChunkInfo[i].w, g.badChunkInfo[i].h, borderColor);
  }
  
}

void
destroyBadChunks(){
  int magenta = (0xff * 300) / 360;
  for(int i = 0; i < (int) g.badChunkInfo.size(); i++){
    Chunk *theChunk= &g.badChunkInfo[i];
    int floodVal = theChunk->floodValue;
    for(int y = theChunk->y; y < theChunk->y+theChunk->h; y++){
      for(int x = theChunk->x; x < theChunk->x+theChunk->w; x++){
	if(g.floodedMask[y][x] == floodVal){
	  g.mask[y][x] = false;
	  g.floodedMask[y][x] = 0;
	  g.hsvData[y][x][0] = magenta;
	  g.hsvData[y][x][1] = 255;
	  g.hsvData[y][x][2] = 255;
	}
      }
    }
  }
}

void
doFloodingUsingMask() {
  memset(g.floodedMask, 0, sizeof(g.floodedMask));
  epf("flooding idk\n");
  
  //goes through g.mask which is the output from thresholding. when it finds a pixel in the mask that isn't part of any chunk (floodedMask == 0) it floods from that pixel. floodedmask has 0 for a pixel not part of any chunk, and any other value is the ID of the chunk
  int chunkID = 1;
  for (UInt y = 0; y < ImageHeight; y++) {
    for (UInt x = 0; x < ImageWidth; x++) {
      if ((g.floodedMask[y][x] == 0)&&(g.mask[y][x] == true)){
	floodFromThisPixel(x, y, chunkID);
	chunkID++;
      }
    }
  }
  
  int numChunks = chunkID;
  epf("calculating initial chunks num of %d", numChunks);
  calculateChunks(numChunks);

  int maxSideLength = 400;
  for(int i = 0; i < (int)g.chunkInfo.size(); i++) {
    int w = g.chunkInfo[i].w;
    int h = g.chunkInfo[i].h;
    if (w>h) {
      if (w > maxSideLength) {
	epf("splitchunk");
	numChunks = splitChunk(i, numChunks, false);
	epf("num chunks is now %d", numChunks);
      }
    }else{
      if (h > maxSideLength) {
	epf("splitchunk");
	numChunks = splitChunk(i, numChunks, true);
	epf("num chunks is now %d", numChunks);
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
	if(g.floodedMask[y][x] != 0) {
	  g.hsvData[y][x][0] = colorIndex[g.floodedMask[y][x]%(sizeof(colorIndex)/sizeof(colorIndex[0]))];
	  g.hsvData[y][x][1] = 0xff;
	  g.hsvData[y][x][2] = 0xff;
	}
      }
    }
  }
  epf("calculating post split chunks max of %d", numChunks);
  calculateChunks(numChunks);
  //destroyBadChunks();
  if(colorChunks){
    drawChunkRectangles();
  }
}
