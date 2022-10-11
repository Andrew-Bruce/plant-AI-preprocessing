

class plantImage {
public:
  UInt8** RGB_row_pointers;
  UInt8** HSV_row_pointers;
  
  UInt8 ***hsvData;//[][][3]
  UInt8 ***rgbData;//[][][3]
  bool **mask;
  int **floodedMask;
  std::vector<Chunk> chunkInfo;
  std::vector<Chunk> badChunkInfo;

  int width;
  int height;
  

  double **depthMapOffset;
  double **diffFromMean;

  UInt8 **blockMatchOutput_row_pointers;
  UInt8 ***blockMatchOutputData;//[][][3]

  double **depthMap;
  UInt8 **matchesForPixel;

  
  void convertRgbImageToHsv(void);
  void convertHsvImageToRgb(void);


  
  void normalizeImage(bool doSat, bool doVal, bool ignoreMask);
  void normalizeBlock(UInt xx, UInt yy, UInt imgAvgSat, UInt imgAvgVal, UInt blkAvgSat, UInt blkAvgVal, bool doSat, bool doVal, bool ignoreMask);

  void getBlockAverageSatVal(int xx, int yy, UInt *satP, UInt *valP, UInt *pixelCountP, bool ignoreMask);

  void getImageAverageSatVal(UInt *satP, UInt *valP, bool ignoreMask);



  void drawBox(UInt x, UInt y, UInt bsz, UInt c)  __attribute__((unused));
  
  void dLine(UInt x0, UInt y0, UInt x1, UInt y1, UInt c);
  void hLine(UInt x, UInt y, UInt len, UInt c);
  void vLine(UInt32 x, UInt32 y, UInt32 len, UInt c);

  void hLineHSV(UInt x, UInt y, UInt len, UInt hue);
  void vLineHSV(UInt32 x, UInt32 y, UInt32 len, UInt hue);

  void drawRectangleHSV(UInt x, UInt y, UInt dx, UInt dy, UInt hue) __attribute__((unused));

  
  void setHsvPixel(UInt x, UInt y, UInt8 hue, UInt8 sat, UInt8 val);
  void setRgbPixel(UInt x, UInt y, UInt8 red, UInt8 green, UInt8 blue);


  double getMatchScore(UInt x0, UInt y0, UInt x1, UInt y1);
  void matchBlockLeftToRight(UInt x0, UInt y0);
  void matchBlockRightToLeft(UInt x0, UInt y0);
  void saveMapToFile(void);
  void getMapFromFile(void);
  void doBlockMatch(void);

  void thresholdHSV(void);
  void doFloodingUsingMask(void);
  int hLineNumChunkPixles(int x, int y, int len, int chunkNum);
  int vLineNumChunkPixels(int x, int y, int len, int chunkNum);
  void splitChunkInHalf(int chunkIndex, bool horizontalSplit, int newFloodValue);
  void floodFromThisPixel(int x, int y, int chunkNum);
  void refloodFromThisPixelInRange(int x, int y, int x0, int y0, int x1, int y1, int oldFloodValue, int newFloodValue);
  int refloodChunk(int chunkIndex, int startingNewFloodValue);
  void calculateChunks(int numChunks);
  int splitChunk(int chunkIndexToSplit, int startingNewChunkValue, bool horizontalSplit);
  void drawChunkRectangles(void);
  void destroyBadChunks(void);

  
  plantImage(void);
  plantImage(UInt8** RGB_row_pointers_, int width_, int height_);
};
