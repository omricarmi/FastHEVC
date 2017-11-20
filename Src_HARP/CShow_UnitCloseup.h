// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <vector>

#include <opencv/cv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>

using namespace cv;
using namespace std;

#include "HARP_Defines.h"
#include "CHelper.h"

#include "TLibCommon/TypeDef.h"

class CShow_UnitCloseup
{

public:
  Mat Closeup;
  int Zoom;
  int ImgDimX;
  int ImgDimY;
  char text[500];
  bool isWritingText;
  Mat CurY;

  CShow_UnitCloseup(TComDataCU* pcCU)
  {
    if (not Global.isObsCTU())
      return;

    sprintf(text, "\n====%s====", "CShow_UnitCloseup");
    int textlen = strlen(text);
    cout << text << endl;

    //---CHANGE HERE ------------------------------
    Zoom = 32;   //need to scale up so that text is readable
    isWritingText = 1;
    //---------------------------------------------

    //---------------------------------------------
    // PREPARING CTU-IMAGES AND BACKGROUND IMAGE
    //---------------------------------------------
    ImgDimX = 64*Zoom;
    ImgDimY = 64*Zoom;
    Closeup = Mat(ImgDimY, ImgDimX, CV_8UC3, Scalar(0,0,0));

    TComPic* pcPic = pcCU->getPic();

    xDrawCU( pcCU, 0, 0);

    //---------------------------------------------
    // EXPORTING IMAGES
    //---------------------------------------------
    Global.exportImage(Closeup,  "UnitCloseup", true, true, false);
    cout << string(textlen, '=') << endl; //==================
  };

  void xDrawCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth)
  {
    TComPic* pcPic     = pcCU->getPic();
    UInt uiCurNumParts = pcPic->getNumPartInCU() >> (uiDepth<<1); //uiCurNumParts = Num Prediction Units
    UInt uiQNumParts   = uiCurNumParts>>2;

    //if small block signals: way to go, not deep enough
    if( pcCU->getDepth(uiAbsZorderIdx) > uiDepth )
    {
      for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
      {
        UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
        UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
        if(    ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() )
            && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
        {
          xDrawCU( pcCU, uiAbsZorderIdx, uiDepth+1);
        }
      }
      return;
    }

    //---------------------------------------------
    // WE ARE NOW AT THE FINAL DEPTH
    //---------------------------------------------
    //getting the pcPic, POC and iCUAddr
    TComPic* rpcPic = pcCU->getPic();
    int POC = rpcPic->getPOC();
    Int iCUAddr     = pcCU->getAddr(); //CU raster index in this slice

    //getting picture height and width
    Int  y, iWidth, iHeight;
    TComPicYuv* pcPicYuv = rpcPic->getPicYuvRec();
    iWidth  = pcPicYuv->getWidth();
    iHeight = pcPicYuv->getHeight();

    //LCU width and height, position of LCU in picture
    UInt uiWidthInLCUs  = rpcPic->getPicSym()->getFrameWidthInCU();
    UInt uiHeightInLCUs = rpcPic->getPicSym()->getFrameHeightInCU();
    UInt uiCol=0, uiLin=0, uiSubStrm=0;

    //Asserting things (just making sure :)
    assert(Global.HeightInLCUs == pcPic->getPicSym()->getFrameHeightInCU());
    assert(Global.WidthInLCUs  == pcPic->getPicSym()->getFrameWidthInCU());
    UInt uiTileCol;
    UInt uiTileStartLCU;
    UInt uiTileLCUX;
    UInt uiTileLCUY;
    UInt uiTileWidth;
    UInt uiTileHeight;
    uiTileCol = rpcPic->getPicSym()->getTileIdxMap(iCUAddr) % (rpcPic->getPicSym()->getNumColumnsMinus1()+1); // what column of tiles are we in?
    uiTileStartLCU = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iCUAddr))->getFirstCUAddr();
    uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
    uiTileLCUY = uiTileStartLCU / uiWidthInLCUs;
    assert( uiTileCol == 0 and uiTileStartLCU == 0 and uiTileLCUX == 0 and uiTileLCUY == 0);
    uiTileWidth = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iCUAddr))->getTileWidth();
    uiTileHeight = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iCUAddr))->getTileHeight();


    int CUWidth = 64 >> uiDepth; //width of CU in pixels

    //Find out the anchor partition of this CU at depth uiDepth
    int LCU_x     = iCUAddr % uiWidthInLCUs;
    int LCU_y     = iCUAddr / uiWidthInLCUs;

    Int iRastPartIdx  = g_auiZscanToRaster[uiAbsZorderIdx];
    int AbsPart_x = iRastPartIdx % (64/4); //(64/4) : partition ticks in a LCU
    int AbsPart_y = iRastPartIdx / (64/4);

    //setting LCU_x and LCU_y to zero since we only want to draw current CTU only
    LCU_x = 0;
    LCU_y = 0;

    int pixel_x = LCU_x*64+AbsPart_x*4;
    int pixel_y = LCU_y*64+AbsPart_y*4;

    //Region of interest (ROI) processing
    Point P_anc = Point(pixel_x, pixel_y ) * Zoom; //anchor
    Point P_dim = Point(CUWidth , CUWidth ) * Zoom;

    if(P_anc.x + P_dim.x > ImgDimX || P_anc.y + P_dim.y > ImgDimY )
      return; //allow small images with large zoom

    Mat roi = Closeup(Rect(P_anc.x, P_anc.y, P_dim.x, P_dim.y));

    Scalar color(128,128,128); //grey
    bool isMergeMode = pcCU->getMergeFlag(uiAbsZorderIdx);

    if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTRA)
      color = RED;
    else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER && pcCU->isSkipped(uiAbsZorderIdx))
      color = GREEN;
    else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER && isMergeMode)
      color = DARKGREEN;
    else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER)
      color = BLUE;
    else
      color = GRAY; //used for outer bounds CU's

    //coloring, drawing lines
    roi = color;
    line(roi, Point(0,0), Point(P_dim.x-1,0), cv::Scalar(0,0,0),3); //top
    line(roi, Point(0,0), Point(0, P_dim.y-1), cv::Scalar(0,0,0),3);//left
    line(roi, Point(P_dim.x-1,P_dim.y-1), Point(P_dim.x-1, 0), cv::Scalar(0,0,0),3);//right
    line(roi, Point(P_dim.x-1,P_dim.y-1), Point(0, P_dim.y-1), cv::Scalar(0,0,0),3);//right

    //getting number of partition units
    UChar iNumPart = 0;
    Char* SizeArray = pcCU->getPartitionSize();
    switch ( SizeArray[0] )
    {
    case SIZE_2Nx2N:        iNumPart = 1; break;
    case SIZE_2NxN:         iNumPart = 2; break;
    case SIZE_Nx2N:         iNumPart = 2; break;
    case SIZE_NxN:          iNumPart = 4; break;
    case SIZE_2NxnU:        iNumPart = 2; break;
    case SIZE_2NxnD:        iNumPart = 2; break;
    case SIZE_nLx2N:        iNumPart = 2; break;
    case SIZE_nRx2N:        iNumPart = 2; break;
    default: assert (0); break;
    }

    //text output for debugging
    if(isWritingText)
    {
      int uiCUPelX = pcCU->getCUPelX();
      int uiCUPelY = pcCU->getCUPelY();

      char text[500];
      int offs = 20;
      sprintf(text, "iCUAddr: %d", pcCU->getAddr());
      writeText(roi, text, Scalar(255,255,255), Point(0,offs));
      sprintf(text, "m_uiAbsIdxInLCU: %d", uiAbsZorderIdx);
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*2));
      sprintf(text, "PredMode:%d", pcCU->getPredictionMode(uiAbsZorderIdx));
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*3));
      sprintf(text, "CUPelX: %d", pcCU->getCUPelX());
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*4));
      sprintf(text, "CUPelY: %d", pcCU->getCUPelY());
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*5));
      sprintf(text, "depth(idx): %d, uiDepth: %d", pcCU->getDepth(uiAbsZorderIdx), uiDepth);
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*6));
      sprintf(text, "m_pePartSize(idx): %s", PartitionSizeStrings[pcCU->getPartitionSize(uiAbsZorderIdx)]);
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*7));
      sprintf(text, "m_puhWidth(idx): %d", pcCU->getWidth(uiAbsZorderIdx));
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*8));
      sprintf(text, "skipped: %s", FalseTrueStrings[pcCU->isSkipped(uiAbsZorderIdx)]);
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*9));
      sprintf(text, "NumPredUnits: %d", iNumPart);
      writeText(roi, text, Scalar(255,255,255), Point(0,offs*10));

    }
  }
};
