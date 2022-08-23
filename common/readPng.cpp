#include <opencv2/opencv.hpp> 

UInt8 *
readPng(const char *fn, UInt8 *rgbData, UInt32 *width, UInt32 *height)
{
  cv::Mat img = cv::imread(fn);
  int w = img.cols;
  int h = img.rows;
  *width = w;
  *height = h;

  int nbytes = w*h*3;
  rgbData = (UInt8 *) Malloc(nbytes);
  UInt8 *rp = rgbData;

  for(int y = 0; y < h; y++){
    for(int x = 0; x < w; x++){;
      *rp++ = img.at<cv::Vec3b>(y,x)[2];//reversed since bgr
      *rp++ = img.at<cv::Vec3b>(y,x)[1];
      *rp++ = img.at<cv::Vec3b>(y,x)[0];
    }
  }

  return rgbData;
}
