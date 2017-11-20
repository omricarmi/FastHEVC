// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <vector>

#include <vector>
#include <opencv/cv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <QString>
#include <QProcess>
#include "HARP_Defines.h"

using namespace std;
using namespace cv;

//==========================================================================================================================
//====================================================== OPENCV ============================================================
//==========================================================================================================================

void getMatrixType(cv::Mat M, const char* string);
void writeJPG_Direkt(Mat Image, string FileName);

inline void inspectMatrixAndSave(cv::Mat M, string FN)
{
  getMatrixType(M, FN.c_str());
  writeJPG_Direkt(M, FN.c_str());
}

//------------------------------------------------------------------
// getMatrixType
//------------------------------------------------------------------
inline void getMatrixType(cv::Mat M, const char* string)
{
  if (string != NULL)
    std::cout << "----Type of matrix " << string << "----" << endl;
  else
    std::cout << "----Type of matrix ----" << endl;

  cv::Size sizeM = M.size();
  int rows = sizeM.height;
  int cols = sizeM.width;
  int depth = M.depth();
  char depthstr[50];
  switch (depth)
  {
  case CV_8U:
    sprintf(depthstr, "CV_8U");
    break;
  case CV_8S:
    sprintf(depthstr, "CV_8S");
    break;
  case CV_16U:
    sprintf(depthstr, "CV_16U");
    break;
  case CV_16S:
    sprintf(depthstr, "CV_16S");
    break;
  case CV_32S:
    sprintf(depthstr, "CV_32S");
    break;
  case CV_32F:
    sprintf(depthstr, "CV_32F");
    break;
  case CV_64F:
    sprintf(depthstr, "CV_64F");
    break;
  default:
    sprintf(depthstr, "USERTYPE");
    break;
  }

  cout << "Cols x Rows = " << cols << "x" << rows << ", ";
  cout << "Type = " << depthstr << ", Dims = " << M.dims << ", NumChannels = " << M.channels() << "\n";
}

//------------------------------------------------------------------
// printMatrix
//------------------------------------------------------------------
inline void printMatrix(cv::Mat M, const char* string)
{
  stringstream sout(stringstream::in | stringstream::out);
  cv::Size sizeM = M.size();
  int rows = sizeM.height;
  int cols = sizeM.width;

  if (string != NULL)
    sout << "----Matrix " << string << "----" << endl;
  else
    sout << "----Matrix ----" << endl;

  //getMatrixType(M, string);
  sout << setw(5) << fixed; // << setprecision( 3 );// << right << fixed;

  if (M.channels() == 1) //2D matrix, like H or F
  {
    for (int row = 0; row < rows; ++row)
    {
      for (int col = 0; col < cols; ++col)
      {
        if (M.depth() == 6)
          sout << (double) M.at<double>(row, col) << " ";
        else if (M.depth() == 5)
          sout << (double) M.at<float>(row, col) << " ";
        else
          assert(0);
      }
      sout << endl;
    }
    sout << flush;
  }
  else //uh oh, some strange 3D points matrix, originating from vector<Point2f> (OpenCV likes those!)
  {
    //indexing separate channels is tough!
    vector<Mat> splitted;
    split(M, splitted);
    for (int ch = 0; ch < M.channels(); ch++)
    {
      sout << "--------Channel " << ch << ": ----------" << endl;
      for (int row = 0; row < rows; ++row)
      {
        for (int col = 0; col < cols; ++col)
        {
          if (col > 100)
            continue;

          if (M.depth() == 6)
            sout << (double) splitted[ch].at<double>(row, col) << " ";
          else if (M.depth() == 5)
            sout << (double) splitted[ch].at<float>(row, col) << " ";
          else
            assert(0);
          sout << "\t";
        }

      }
      sout << endl;
    }
    sout << flush;
  }

  cout << sout.str();
  //return sout.str();
}

inline bool doesFileExist(string FN)
{
  std::ifstream infile(FN.c_str());
  return infile.good();
}

inline void sleepFor(float seconds)
{
  unsigned int microseconds = (unsigned int) seconds*1000*1000;
  usleep(microseconds);
}

Mat invertImage(Mat Image);
void convertToCV_8UC3(Mat &Image);

//------------------------------------------------------------------
// writeJPG
//------------------------------------------------------------------

inline void writeJPG_Direkt(Mat Image, string FileName)
{
  //format given by provided filename!
  int Quality = JPG_QUALITY; //100 = best
  vector<int> params;
  params.push_back(CV_IMWRITE_JPEG_QUALITY);
  params.push_back(Quality);

  //  //INVERTING
  //  if (loadINI("HARP", "invert") == true)
  //    Image = invertImage(Image);

#ifdef JPG_DIMX
  float Ratio = float(JPG_DIMX)/image.cols;
  int JPEG_DimY = int(Ratio*image.rows);
  cv::resize(image, image, cv::Size2i(JPG_DIMX, JPEG_DimY));
#endif

  imwrite(FileName, Image, params);
  cout << "Wrote: " << FileName << endl;

}

inline void resizeToHeight(Mat &Img, int Height)
{
  float RatioY = float(Img.rows)/Height;
  int Width = int(Img.cols/RatioY);
  resize(Img, Img, cv::Size2i(Width, Height), 0, 0, INTER_NEAREST);
}

inline void resizeToWidth(Mat &Img, int Width)
{
  float RatioX = float(Img.cols)/Width;
  int Height = int(Img.rows/RatioX);
  resize(Img, Img, cv::Size2i(Width, Height), 0, 0, INTER_NEAREST);
}

inline void padToWidth(Mat &Img, int Width)
{
  int CurWidth = Img.cols;
  Mat Pad = Mat(Img.rows,Width-CurWidth, CV_8UC3, WHITE);
  hconcat(Img, Pad, Img);
}

//------------------------------------------------------------------
// writeText
//------------------------------------------------------------------
inline void writeText(Mat Matrix, QString text, Scalar color = Scalar(0, 0, 0), Point textOrg = Point(3, 3.0), double fontScale = 1.0, int thickness = 2)
{
  //int fontFace = FONT_HERSHEY_SIMPLEX;
  int fontFace = FONT_HERSHEY_PLAIN;

  int baseline = 0;
  //Size textSize = getTextSize(text, fontFace, fontScale, thickness, &baseline);
  baseline += thickness;

  //RegEx for ' ' or ',' or '.' or ':' or '\t': QRegExp rx("(\\ |\\,|\\.|\\:|\\t)");
  QRegExp rx("(\\n)");
  QStringList query = text.split(rx);

  foreach (QString textrow,query) //C++11 feature man!
  {
    textOrg.y += 20*fontScale;
    putText(Matrix, textrow.toLocal8Bit().constData(), textOrg, fontFace, fontScale, color, thickness, 3);
  }
}

inline void writeText(Mat Matrix, QString text, double fontScale = 1.0, Scalar color = Scalar(0, 0, 0), Point textOrg = Point(3, 3.0), int thickness = 2)
{
  writeText(Matrix, text, color, textOrg, fontScale, thickness);
}

//------------------------------------------------------------------
// convert YCrCb to RGB, return copy
//------------------------------------------------------------------
inline Mat getRGB(Mat &YCrCb)
{
  Mat RGB;
  cv::cvtColor(YCrCb, RGB, CV_YCrCb2RGB);
  return RGB;
}

inline Mat getRGB(Mat &Y, Mat &U, Mat &V)
{
  Mat YUV;
  Mat pointers[] =
  { Y, U, V };
  merge(pointers, 3, YUV); //now its RGB!

  Mat RGB;
  cv::cvtColor(YUV, RGB, CV_YCrCb2RGB);
  return RGB;
}

inline void convertToCV_8UC3(Mat &Image)
{
  if (Image.channels() != 3) //grayscale to color
  {
    Mat tmp;
    Mat pointers[] =
    { Image, Image, Image };
    merge(pointers, 3, tmp); //now its RGB!
    Image = tmp.clone();
  }

  if (Image.depth() != CV_8U)
    Image.convertTo(Image, CV_8U);
}

//------------------------------------------------------------------
// invertImage
//------------------------------------------------------------------
inline Mat invertImage(Mat Image)
{
  Mat cloned = Image.clone();
  cloned.convertTo(Image, CV_8U);

  Mat inverted = cloned.clone(); //reserving space
  inverted = Scalar(255, 255, 255);
  inverted -= cloned;
  return inverted;
}

//==========================================================================================================================
//====================================================== YUV READER ========================================================
//==========================================================================================================================

//------------------------------------------------------------------
// CYuvReader
//------------------------------------------------------------------
class CYuvReader
{
public:

  CYuvReader(const char *file, unsigned short w, unsigned short h)
{
    width = w;
    height = h;
    pSourcefile = fopen(file, "rb");

    if(pSourcefile == NULL)
    {
      cout << "Couldn't open file " << file << endl;
      endOfFile = 0;
    }
    else
    {
      fseek(pSourcefile, 0, SEEK_END);
      endOfFile = ftell(pSourcefile);
      fseek(pSourcefile, 0, SEEK_SET);
    }
}

  Mat getMatY()
  {
    if(pSourcefile == NULL)
      return Mat();

    int pos = ftell(pSourcefile);
    assert(pos < endOfFile);
    unsigned char *samples = new unsigned char[width*height];
    int length = fread(samples, sizeof(unsigned char), width*height, pSourcefile);
    assert(length==width*height);
    Mat tmp = Mat(height, width, CV_8UC1,samples);
    Mat M = tmp.clone();
    fseek(pSourcefile, width*height/2, SEEK_CUR);
    delete[] samples;
    return M;
  }

  void getMatYUV444(Mat &Y, Mat &U, Mat &V, int DimX, int DimY)
  {
    if(pSourcefile == NULL)
      assert(0);
    int pos = ftell(pSourcefile);
    assert(pos < endOfFile);

    unsigned char *y = (unsigned char*) malloc (DimX*DimY);
    unsigned char *u = (unsigned char*) malloc (DimX*DimY/4);
    unsigned char *v = (unsigned char*) malloc (DimX*DimY/4);

    fread(y, sizeof(unsigned char), width*height, pSourcefile);
    fread(u, sizeof(unsigned char), width*height/4, pSourcefile);
    fread(v, sizeof(unsigned char), width*height/4, pSourcefile);

    Mat tmp;
    tmp = Mat(height, width, CV_8UC1,y);
    Y = tmp.clone();
    tmp = Mat(height/2, width/2, CV_8UC1,u);
    cv::resize(tmp, U, Size(width, height));
    tmp = Mat(height/2, width/2, CV_8UC1,v);
    cv::resize(tmp, V, Size(width, height));

    delete[] y;
    delete[] u;
    delete[] v;
  }

  void getMatYUV420(Mat &Y, Mat &U, Mat &V, int DimX, int DimY)
  {
    if(pSourcefile == NULL)
      assert(0);
    int pos = ftell(pSourcefile);
    assert(pos < endOfFile);

    unsigned char *y = (unsigned char*) malloc (DimX*DimY);
    unsigned char *u = (unsigned char*) malloc (DimX*DimY/4);
    unsigned char *v = (unsigned char*) malloc (DimX*DimY/4);

    int BytesRead[3];
    BytesRead[0] = fread(y, sizeof(unsigned char), width*height, pSourcefile);
    BytesRead[1] = fread(u, sizeof(unsigned char), width*height/4, pSourcefile);
    BytesRead[2] = fread(v, sizeof(unsigned char), width*height/4, pSourcefile);

    if(BytesRead[0] != width*height || BytesRead[1] != width*height/4 || BytesRead[2] != width*height/4) //ERROR!
    {
      cout << "ERROR: " << BytesRead[0] << " " << BytesRead[1] << " " << BytesRead[2] << endl;
      assert(0);
    }

    Mat tmp;
    tmp = Mat(height, width, CV_8UC1,y);
    Y = tmp.clone();
    tmp = Mat(height/2, width/2, CV_8UC1,u);
    U = tmp.clone();
    tmp = Mat(height/2, width/2, CV_8UC1,v);
    V = tmp.clone();

    delete[] y;
    delete[] u;
    delete[] v;
  }

  int getNumFrames()
  {
    return endOfFile/(width*height*1.5);
  }

  void skipFrames(int SkipFrames)
  {
    fseek(pSourcefile, SkipFrames*width*height*1.5, SEEK_SET);
  }

  unsigned int endOfFile;
  unsigned short height;
  unsigned short width;
  FILE *pSourcefile;
};


