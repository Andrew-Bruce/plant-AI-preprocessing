void writePnmHeader(int fd, UInt width, UInt height);

void writePnmToStdout(UInt8* rgbPixels, UInt width, UInt height);

void writePpmToFile(UInt8 *rgbPixels, UInt width, UInt height, const char *filename);
