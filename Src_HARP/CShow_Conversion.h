// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <vector>
#include <string>

#include "opencv/cv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"

using namespace std;
using namespace cv;

//------------------------------------------------------------------
// copy_internal_PICYUV
//------------------------------------------------------------------
enum EDirection{ HM_2_MAT, MAT_2_HM};
inline void copy_internal_picyuv(EDirection Direction, TComPicYuv* pcPicYuv, Mat &Img, Rect ROI, TextType Channel)
{
  int Margin, iStride, iWidth, iHeight;
  Pel* ptrYUV;

  if(Channel == TEXT_LUMA)
  {
    Margin = pcPicYuv->getLumaMargin();
    iWidth =  pcPicYuv->getWidth();
    iHeight =  pcPicYuv->getHeight();
    ptrYUV = pcPicYuv->getBufY();
    iStride = pcPicYuv->getStride();

  }
  else  //CHROMA
  {
    Margin = pcPicYuv->getChromaMargin();
    iWidth =  pcPicYuv->getWidth() / 2;
    iHeight =  pcPicYuv->getHeight() / 2;
    iStride = iWidth + (pcPicYuv->getChromaMargin() * 2);
    if(Channel == TEXT_CHROMA_U) ptrYUV = pcPicYuv->getBufU();
    else if (Channel == TEXT_CHROMA_V) ptrYUV = pcPicYuv->getBufV();
    else assert(0);
  }

  ptrYUV += Margin + Margin*iStride; //getting rid of first margin in the upper left

  if(Direction == HM_2_MAT )
  {
    Img = Mat(iHeight, iWidth, CV_16SC1, Scalar(0));
    for (int y = 0; y < iHeight; y++)
    {
      unsigned char* ptrMat = Img.ptr<unsigned char>(y);
      ::memcpy(ptrMat, ptrYUV, sizeof(Pel) * iWidth);
      ptrYUV += iStride;
    }
  }
  else if(Direction == MAT_2_HM )
  {
    for (int y = 0; y < iHeight; y++)
    {
      unsigned char* ptrMat = Img.ptr<unsigned char>(y);
      ::memcpy(ptrYUV, ptrMat, sizeof(Pel) * iWidth);
      ptrYUV += iStride;
    }
  }
}

//------------------------------------------------------------------
// copy_internal_YUV
//------------------------------------------------------------------
inline void  copy_internal_yuv(EDirection Direction, TComYuv* pcYuv, Mat &Img, Rect ROI, TextType Channel)
{
  int iStride, iWidth, iHeight;
  Pel* ptrYUV;

  if(Channel == TEXT_LUMA)
  {
    iHeight = pcYuv->m_iHeight;
    iWidth  = pcYuv->m_iWidth;
    iStride = pcYuv->m_iWidth;
    ptrYUV  = pcYuv->m_apiBufY;
  }
  else  //CHROMA
  {
    iHeight = pcYuv->m_iCHeight;
    iWidth  = pcYuv->m_iCWidth;
    iStride = pcYuv->m_iCWidth;

    if(Channel == TEXT_CHROMA_U) ptrYUV  = pcYuv->m_apiBufU;
    else if (Channel == TEXT_CHROMA_V) ptrYUV = pcYuv->m_apiBufV;
    else assert(0);
  }

  if(Direction == HM_2_MAT )
  {
    Img = Mat(iHeight, iWidth, CV_16SC1, Scalar(0));
    for (int y = 0; y < iHeight; y++)
    {
      unsigned char* ptrMat = Img.ptr<unsigned char>(y);
      ::memcpy(ptrMat, ptrYUV, sizeof(Pel) * iWidth);
      ptrYUV += iStride;
    }
  }
  else if(Direction == MAT_2_HM )
  {
    for (int y = 0; y < Img.rows; y++)
    {
      unsigned char* ptrMat = Img.ptr<unsigned char>(y);
      ::memcpy(ptrYUV, ptrMat, sizeof(Pel) * Img.cols);
      ptrYUV += iStride;
    }

  }
}

//------------------------------------------------------------------
// copy_Yuv2Mat
//------------------------------------------------------------------
inline void copy_Yuv2Mat(TComYuv* pcYuv, Mat &Image, int Interpolation)
{
  Mat Y, U, V;
  copy_internal_yuv(HM_2_MAT, pcYuv, Y, Rect(), TEXT_LUMA);
  copy_internal_yuv(HM_2_MAT, pcYuv, U, Rect(), TEXT_CHROMA_U);
  copy_internal_yuv(HM_2_MAT, pcYuv, V, Rect(), TEXT_CHROMA_V);

  Mat Ur, Vr;
  cv::resize(U, Ur, Size(0, 0), 2.0, 2.0, Interpolation);
  cv::resize(V, Vr, Size(0, 0), 2.0, 2.0, Interpolation);

  //Merging to color image
  Mat pointers[] = { Y, Ur, Vr };
  merge(pointers, 3, Image);

  //note: Image has 16 bit bitdepth!
}

//------------------------------------------------------------------
// copy_PicYuv2Mat
//------------------------------------------------------------------
inline void copy_PicYuv2Mat(TComPicYuv* pcPicYuv, Mat &Image, int Interpolation)
{
  Mat Y, U, V;
  copy_internal_picyuv(HM_2_MAT, pcPicYuv, Y, Rect(), TEXT_LUMA);
  copy_internal_picyuv(HM_2_MAT, pcPicYuv, U, Rect(), TEXT_CHROMA_U);
  copy_internal_picyuv(HM_2_MAT, pcPicYuv, V, Rect(), TEXT_CHROMA_V);

  Mat Ur, Vr;
  cv::resize(U, Ur, Size(0, 0), 2.0, 2.0, Interpolation);
  cv::resize(V, Vr, Size(0, 0), 2.0, 2.0, Interpolation);

  //Merging to color image
  Mat pointers[] = { Y, Ur, Vr };
  merge(pointers, 3, Image);

  //note: Image has 16 bit bitdepth!
}

//------------------------------------------------------------------
// copy_Mat2PicYuv
//------------------------------------------------------------------
inline void copy_Mat2PicYuv(Mat Image, TComPicYuv* pcPicYuv, int Interpolation)
{
  int iWidth = Image.size().width;
  int iHeight = Image.size().height;

  vector<Mat> splitted;
  split(Image, splitted);
  Mat Y, U, V;
  Y = splitted[0];
  resize(splitted[1], U, cv::Size(iWidth / 2, iHeight / 2), 0, 0, Interpolation);
  resize(splitted[2], V, cv::Size(iWidth / 2, iHeight / 2), 0, 0, Interpolation);

  copy_internal_picyuv(MAT_2_HM, pcPicYuv, Y, Rect(), TEXT_LUMA);
  copy_internal_picyuv(MAT_2_HM, pcPicYuv, U, Rect(), TEXT_CHROMA_U);
  copy_internal_picyuv(MAT_2_HM, pcPicYuv, V, Rect(), TEXT_CHROMA_V);
}

//------------------------------------------------------------------
// just for testing
//------------------------------------------------------------------
inline float sumAllPelsY(TComYuv* pcYuv)
{
  int Height  = pcYuv->getHeight();
  int Width   = pcYuv->getWidth();
  int Stride  = pcYuv->getStride();
  Pel *Src  = pcYuv->getLumaAddr();

  float sum = 0;
  for (int y = 0; y < Height; y++)
  {
    for (int x = 0; x < Width; x++)
    {
      sum += Src[x];
    }
    Src += Stride;
  }

  return sum;
}
