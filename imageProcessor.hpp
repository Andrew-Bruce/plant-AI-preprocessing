


static const UInt ImageWidth  = 1023;
static const UInt ImageHeight = 683;


class image {
public:
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
  
  image(void);
  image(const char* filename, int* widthOutput, int* heightOutput);
};
