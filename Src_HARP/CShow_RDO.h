// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <vector>

#include "HARP_Defines.h"
#include "CHelper.h"
#include "TLibCommon/TypeDef.h"
#include "TLibEncoder/TEncSearch.h"
#include "CShow.h"

using namespace std;
using namespace cv;

//------------------------------------------------------------------
// Img Matrix
//------------------------------------------------------------------
class CImgMatrix
{



public:
  int Rows, Cols, RowHeight, MaxRows, RowsPerImg;
  int CurCol;
  int LabelHeight; //pixel height of label stripes (for a row and all Mats in this row)
  vector<vector<Mat> > Matrix;
  vector<vector<QString> > LabelMatrix;
  vector<Scalar> RowColorList;
  QStringList RowLabelList;

  CImgMatrix()
  {
    Rows = 0;
    CurCol = 0;
    Cols = -1; //set by owner

    RowHeight = 64*4; //EVERY entry in the row is scaled to this height individually
    MaxRows = 100; //limit Final to X rows
    RowsPerImg = 50; //rows written to one jpg
    LabelHeight = 40;
  }

  void reset()
  {
    for(int r=0; r<Rows; r++)
    {
      Matrix[r].clear();
      LabelMatrix[r].clear();
    }
    Matrix.clear();
    LabelMatrix.clear();
    RowLabelList.clear();
    RowColorList.clear();
    Rows = 0;
    CurCol = 0;
  }

  void pushImg(Mat Img, QString Info) //always sets in last row
  {
    if(Rows == MaxRows) //we are at the end and over the last image
      putText(Img, "DEFECT", Point(0.0, 20), FONT_HERSHEY_PLAIN, 1.0, RED, 2, 3);

    if(CurCol == Cols-1) //All bad things must come to an end, Jesse
    {
      cout << "Warning CImgMatrix: to less columns, image lost" << endl;
      return;
    }

    LabelMatrix[Rows-1][CurCol] = Info;
    Matrix[Rows-1][CurCol] = Img;
    CurCol++;
  }

  void createEmptyRow(QString RowLabel, Scalar RowColor)
  {
    CurCol = 0; //carriage return
    if(Rows >= MaxRows) //All bad things must come to an end, Jesse
      return;

    RowLabelList.append(RowLabel);
    RowColorList.push_back(RowColor);

    assert(Cols != -1); //somebody forgot to initialize...

    vector<Mat> Row;
    vector<QString> LabelRow;
    for(int c=0; c<Cols; c++)
    {
      //the top label of each column
      LabelRow.push_back(QString());

      //the image of each column
      Mat Proxy = Mat(); //Mat(64,64, CV_8UC3, Scalar(128,128,128)); //simple 64x64 gray patch with number
      Row.push_back(Proxy); //push_back here: append to right
    }
    Matrix.push_back(Row);
    LabelMatrix.push_back(LabelRow);
    Rows++;

  }

  Mat assembleOneRow(int r)
  {
    Mat ImgRow;

    QString HeadlineLabel = RowLabelList[r];

    for(int c=0; c<Cols; c++)
    {
      //get the img at this position
      Mat CurImg = Matrix[r][c];

      //convert to correct format (maybe just single channel, or 16 Bit, or ...)
      convertToCV_8UC3(CurImg);

      QString ColumnLabel = LabelMatrix[r][c];
      if(c == 0)
      {
        bool test = CurImg.empty();
        //        assert(CurImg.empty() == false); //first Mat must never be empty
      }
      if(CurImg.empty()) //empty entries may happen (may now be obsolete with pushImg)
        continue;


      //scale image to desired height
      int ImgHeight = CurImg.rows;
      resizeToHeight(CurImg, RowHeight);

      //add top label
      Mat TopLabel(LabelHeight,CurImg.cols, CV_8UC3, RowColorList[r]);
      putText(TopLabel, ColumnLabel.toLocal8Bit().constData(), Point(3.0, 30), FONT_HERSHEY_PLAIN, 1.6, Scalar(0, 0, 0), 2, 3);
      vconcat(TopLabel, CurImg, CurImg);

      //inserting col-spacer
      Mat Spacer(RowHeight+LabelHeight,20, CV_8UC3, RowColorList[r]);

      if(c == 0){
        ImgRow = CurImg; //first column
        hconcat(Spacer, ImgRow, ImgRow); //left border
        hconcat(ImgRow, Spacer, ImgRow); //right border
      }
      else {
        hconcat(ImgRow, CurImg, ImgRow);
        hconcat(ImgRow, Spacer, ImgRow);
      }

      //Mat test = Matrix[r][c].t();
      //LeftImg.push_back(Matrix[r][c].t());
    }

    //inserting top v-spacer
    Mat Spacer(LabelHeight,ImgRow.cols, CV_8UC3, RowColorList[r]);
    putText(Spacer, HeadlineLabel.toLocal8Bit().constData(), Point(3.0, 30), FONT_HERSHEY_PLAIN, 2.0, Scalar(0, 0, 0), 2, 3);
    //resizeToWidth(Spacer, ImgRow.cols);
    vconcat(Spacer, ImgRow, ImgRow);

    //inserting bottom v-spacer
    Spacer = RowColorList[r];
    Spacer(Rect(0, Spacer.rows/2, Spacer.cols, Spacer.rows/2)) = WHITE;
    vconcat(ImgRow, Spacer, ImgRow);

    return ImgRow;
  }

  void saveFinal(string FN)
  {
    //cout << "ImgMatrix getFinal: " << Rows << "x" << Cols << " images" <<  endl;

    Mat Final;

    //preprocessing: compile a vector of large row-images

    vector<Mat> VectorOfRows;
    for(int r=0; r<Rows; r++)
    {
      //assemble one row
      Mat ImgRow = assembleOneRow(r);
      VectorOfRows.push_back(ImgRow);
      Matrix[r].clear(); //free memory, don't need those Mats any more
    }

    //if there is something to write
    if(Rows > 0)
    {
      //let us see what the maximal row width is
      int RowDimX = 0;
      int RowDimY = VectorOfRows[0].rows;
      for(int r=0; r<Rows; r++)
      {
        if (VectorOfRows[r].cols > RowDimX)
          RowDimX = VectorOfRows[r].cols;
      }

      //let us pad all rows which are too short
      for(int r=0; r<Rows; r++)
      {
        if (VectorOfRows[r].cols < RowDimX)
          padToWidth(VectorOfRows[r], RowDimX);
        assert(RowDimY == VectorOfRows[r].rows ); //all rows must be fixed in height!
      }

      //we now create a >>reduced<< Mat image for saving to file
      int NumTotalRows = (Rows < MaxRows) ? Rows : MaxRows;

      int NumRuns = int(ceil(float(NumTotalRows) / RowsPerImg));
      int cnt = 0;
      for(int run = 0; run < NumRuns; run++)
      {
        int TmpNum = (NumTotalRows < RowsPerImg) ? NumTotalRows : RowsPerImg;
        Final = Mat(TmpNum*RowDimY,RowDimX, CV_8UC3, WHITE);
        for(int r=0; r < TmpNum && cnt < NumTotalRows-1; r++, cnt++)  //-1: get rid of the last (defect) row
        {
          VectorOfRows[cnt].copyTo(Final(Rect(0,r*RowDimY, RowDimX, RowDimY)));
        }

        char text[500];
        sprintf(text, "VisRDO__Part%02d", run);
        Global.exportImage(Final, text, true, true, false);
      }
    }
  }

  void appendToLastHeadline(QString String)
  {
    QString &LastHeadline = this->RowLabelList.last();
    LastHeadline.append(String);
  }
};


class CShow_RDO
{
public:
  int LCU_DimX;
  int LCU_DimY;
  int call;
  CImgMatrix ImgMatrix;
  int CurrentImgRow;
  char text[500];

  CShow_RDO()
  {
    string ClassName = "CShow_RDO";
    LCU_DimX = LCU_DimY = 64;
    ImgMatrix.Cols = 24;
    call++;
  }

  void initAnalysis(TComDataCU* pcCU)
  {
    if (not Global.isObsCTU())
      return;

    cout << "Visualizing RDO..." << flush;
  }

  void finalizeAnalysis(TComDataCU* pcCU)
  {
    if (not Global.isObsCTU())
      return;

    //---------------------------------------------
    // EXPORTING IMAGES
    //---------------------------------------------
    // since ImgMatrix takes over splitting, we do this manually!
    char POCStr[500], CTUStr[500];
    sprintf(POCStr, "POC%05d_", Global.getCurrentPOC());
    sprintf(CTUStr, "_CTU%04d", Global.getCurrentCTU());
    ImgMatrix.saveFinal(Global.TmpDir + POCStr + "CheckRDCostInter" + CTUStr + IMAGE_FORMAT);


    ImgMatrix.reset();
  }

  void start_predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv,
      TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv)
  {
    if (not Global.isObsCTU())
      return;
    cout << "."  << flush;
    //Create Headline
    Mat Vis1, Vis2, VisText;

    int Width = 64 >> pcCU->getDepth(0); //width of CU in pixels
    QString Headline = QString("predInterSearch(): POC%1, CTU%2, %3x%4, Depth%5")
	                .arg(Global.CurrentPOC).arg(Global.CurrentCTU)
	                .arg(Width).arg(Width).arg(pcCU->getDepth(0));

    Scalar RowColor = BRBA_YELLOW;
    this->ImgMatrix.createEmptyRow(Headline, RowColor);

    //VISUALIZE CURRENT CU
    getVis_CurrentCU(pcCU, Vis1);
    ImgMatrix.pushImg(Vis1, "Current CU");

    getVis_YUVBuffer(pcCU, pcOrgYuv, "getVis_YUVBuffer ORG", Vis1, VisText);
    ImgMatrix.pushImg(Vis1, "ORG");

  }

  void append1_predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv,
      TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv, Bool bUseRes, Bool bUseMRG,
      Int iPartIdx )
  {
    if (not Global.isObsCTU())
      return;
    Mat Vis1, Vis2, VisText;

    Int         iWidth, iHeight;
    UInt        uiPartAddr;
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );
    PartSize ePartSize = pcCU->getPartitionSize(0);
    QString PartSizeString = PartitionSizeStrings[ePartSize];

    getVis_CurrentPU(pcCU, iPartIdx, "getVis_CurrentPU", Vis1, VisText);
    ImgMatrix.pushImg(Vis1, QString("CurPU: %1").arg(PartSizeString));
    Mat KeepImg = VisText.clone();
  }

  void append2_predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv)
  {
    Mat Vis1, Vis2, VisText;
    getVis_YUVBuffer(pcCU, rpcPredYuv, "getVis_YUVBuffer PRED", Vis1, VisText);
    ImgMatrix.pushImg(Vis1, "PRED");

    getVis_subtractYBuffers(pcCU, pcOrgYuv, rpcPredYuv, "getVis_YUVBuffer DIFFERENCE", Vis1, VisText);
    ImgMatrix.pushImg(Vis1, "ORG-PRED Y");

    UInt HAD_SAD = calculateHADs_Y(rpcPredYuv, pcOrgYuv, 0, rpcPredYuv->m_iHeight, rpcPredYuv->m_iWidth);

    QString PUInfoStr = QString(" | Final HAD-SAD: %1").arg(HAD_SAD);
    ImgMatrix.appendToLastHeadline(PUInfoStr);
  }

  // pcCU             pcYuvOrg,          pcYuvPred,          rpcYuvResi,
  void createImg_encodeRes( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv* pcPredYuv, TComYuv*& rpcResiYuv,
      // rpcYuvResiBest,        rpcYuvRec,				  //encodeRes may decide against coefficient coding
      TComYuv*& rpcResiBestYuv, TComYuv*& rpcRecYuv, bool skipCoeffCoding)
  {
    Mat Vis_Image, Vis_Text;
    Mat Y, U, V;

    //uncomment to show only non-merge related runs
    //    if(pcCU->getMergeFlag( 0 ) == true)
    //      return;

    //Create Headline
    PartSize ePartSize = pcCU->getPartitionSize(0);
    QString PartSizeString = PartitionSizeStrings[ePartSize];
    //    if(Global.isCurrentTestAffine)
    //      PartSizeString += "_AFFINE";
    QString Headline = QString("encodeRes(): POC%1, CTU%2, %3, Depth%4, Merge=%5, skipCoeffCoding=%6")
                .arg(Global.CurrentPOC).arg(Global.CurrentCTU)
                .arg(PartSizeString).arg(pcCU->getDepth(0)).arg(pcCU->getMergeFlag( 0 )).arg(skipCoeffCoding);
    this->ImgMatrix.createEmptyRow(Headline, BRBA_GREEN);

    //VISUALIZE CURRENT CU
    getVis_CurrentCU(pcCU, Vis_Image);
    ImgMatrix.pushImg(Vis_Image, "Current CU");

    //-
    //    getVis_YUVBuffer(pcCU, pcOrgYuv, "getVis_YUVBuffer ORG", Vis_Image, Vis_Text);
    //    ImgMatrix.pushImg(Vis_Image, "YUV ORG");
    //    getVis_YUVBuffer(pcCU, pcPredYuv, "getVis_YUVBuffer PRED", Vis_Image, Vis_Text);
    //    ImgMatrix.pushImg(Vis_Image, "YUV PRED");
    //-

    //PRED BUFFER
    getVis_SingleYUVBuffers(pcPredYuv, Y, U, V, false); //no rescale!
    ImgMatrix.pushImg(Y, "PRED Y");
    ImgMatrix.pushImg(U, "PRED U");
    ImgMatrix.pushImg(V, "PRED V");

    //RESI BUFFER
    getVis_SingleYUVBuffers(rpcResiYuv, Y, U, V, true);
    ImgMatrix.pushImg(Y, "RESI Y");
    ImgMatrix.pushImg(U, "RESI U");
    ImgMatrix.pushImg(V, "RESI V");

    //RESIBEST BUFFER
    getVis_SingleYUVBuffers(rpcResiBestYuv, Y, U, V, true);
    ImgMatrix.pushImg(Y, "RESIBEST Y");

    //REC BUFFER
    getVis_YUVBuffer(pcCU, rpcRecYuv, "getVis_YUVBuffer REC", Vis_Image, Vis_Text);
    ImgMatrix.pushImg(Vis_Image, "REC");
  }

};

