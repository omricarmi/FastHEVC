// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include <sys/time.h>
#include <vector>
#include <iostream>
#include <iterator>
#include <fstream>

#include <vector>
#include <opencv/cv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QApplication>
#include <QTextEdit>

#include "PyNDArray.h"
#include "HARP_Defines.h"
#include "CHelper.h"


using namespace std;
using namespace cv;

static bool InitDone = false;

// ---------------------------
// GLOBAL, ACCESS FROM ANYWHERE (YES, I KNOW)
// ---------------------------
class CGlobal
{
public:
  string TmpDir; //make sure it ends with "/"

  // ---------------------------
  // PYTHON
  // ---------------------------
  NDArrayConverter *Converter;

  // ---------------------------
  // INTERNAL
  // ---------------------------
  bool isEncoder;  //PicYuvOrg is not available on decoder side
  int CurrentCTU;  //encoder continually places current CTU index here
  int CurrentPOC;  //encoder continually places current POC index here
  bool initDone;
  int DimX, DimY;
  int NumCTUs;
  int WidthInLCUs;
  int HeightInLCUs;

  char tmptxt[500];
  QString FN_InputYUV;
  QApplication *App;

  CGlobal();
  ~CGlobal();
  void init_general();
  void init_python();
  void initTmpDir();
  void printVersion();
  void setCurrentPOC(int POC);
  int getCurrentPOC();
  void setCurrentCTU(int CTU);
  int getCurrentCTU();

  bool isObsCTU();
  void exportImage(Mat Image, string nickname, bool withPOCIdx = false, bool withCTUIdx = false, bool exportToSinglePics = true);
};

extern CGlobal Global;
