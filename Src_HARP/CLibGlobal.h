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
#include "CShow_RDO.h"
#include "PyFactory.h"

using namespace std;
using namespace cv;

// ---------------------------
// GLOBAL, ACCESS FROM ANYWHERE
// ---------------------------
class CLibGlobal
{
public:
  CShow_RDO Show_RDO;
  bool isNotGetPredError;

  CLibGlobal(){
    cout << "CLibGlobal instantiated" << endl;}

  ~CLibGlobal(){
    cout << "CLibGlobal destroyed" << endl;}

  //bool isFinalEncode; // required context for encodePUWise()
  bool insideCompressSlice; //RDO is the word!
  bool insideCompressCU;    //RDO is the word!

  bool isInside_RDO(){
    return insideCompressSlice && insideCompressCU;}

  bool isInside_POST_RDO(){
    return insideCompressSlice && not insideCompressCU;} //only true if inside compressSlice and inside EncodeCU

  bool isInside_FINAL(){
    return not insideCompressSlice && not insideCompressCU;} //only true if inside encodeSlice and inside EncodeCU

  //HARP DATA TREE FTW!
  CDataTree DataTree;
};

extern CLibGlobal LibGlobal;
