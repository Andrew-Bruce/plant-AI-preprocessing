#include "chunk.hpp"

Chunk::Chunk(void){
  
}

Chunk::Chunk(int _x, int _y, int _w, int _h, int _floodValue, int _pixelCount) {
  x = _x;
  y = _y;
  w = _w;
  h = _h;
  floodValue = _floodValue;
  pixelCount = _pixelCount;
}