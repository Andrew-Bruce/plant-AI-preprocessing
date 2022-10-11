unsigned char**
readJpeg(const char* filename, UInt* width, UInt* height);

void
writeJpeg(const char* filename, unsigned char** row_pointer, UInt width, UInt height);
