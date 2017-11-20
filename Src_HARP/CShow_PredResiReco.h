// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include "TLibCommon/TypeDef.h"

#include "HARP_Defines.h"
#include "CHelper.h"
#include "CShow.h"

class CShow_PredResiReco
{

public:
  Mat PredYuv, ResiYuv, RecoYuv;  //holds visualized image (no blending included)
  int Zoom; //zoom factor for nice text rendering
  Point ImgDim;
  char text[500];
  int call; //keeping track of calls to this class

  Point CTU_Span; //x and y span of observed LCUs (may be selection or all LCUs)
  Point CTU_UpperLeft; //from (inclusive)
  Point CTU_LowerRight; //to (exclusive)

  bool isDrawingCUs;
  float LineWidth;
  int ResiEnhanceFactor;

  CShow_PredResiReco(TComPic* pcPic)
  {
    //---------------------------------------------
    // CHANGE HERE
    //---------------------------------------------
    isDrawingCUs = true;
    LineWidth = 1;
    Zoom = 5;
    ResiEnhanceFactor = 2; //higher contrast for residual
    //---------------------------------------------
    call = 0;
    sprintf(text, "====%s====", "CShow_PredResiReco");
    int textlen = strlen(text);
    cout << text << endl;

    CTU_UpperLeft  = Point(0,0);
    CTU_LowerRight = Point(Global.WidthInLCUs, Global.HeightInLCUs);
    CTU_Span = CTU_LowerRight - CTU_UpperLeft;
    ImgDim.x = (CTU_Span.x)*64*Zoom;
    ImgDim.y = (CTU_Span.y)*64*Zoom;

    Mat TmpPredYuv, TmpResiYuv, TmpRecoYuv;
    copy_internal_picyuv(HM_2_MAT, pcPic->getPicYuvPred(), TmpPredYuv, Rect(), TEXT_LUMA);
    copy_internal_picyuv(HM_2_MAT, pcPic->getPicYuvResi(), TmpResiYuv, Rect(), TEXT_LUMA);
    copy_internal_picyuv(HM_2_MAT, pcPic->getPicYuvRec(), TmpRecoYuv, Rect(), TEXT_LUMA);

    TmpResiYuv = TmpResiYuv * (ResiEnhanceFactor / 2)  + 127; //scale to 8 bits and shift "mean" to 127

    convertToCV_8UC3(TmpPredYuv);
    convertToCV_8UC3(TmpResiYuv);
    convertToCV_8UC3(TmpRecoYuv);

    cv::resize(TmpPredYuv, PredYuv, Size(0, 0), Zoom, Zoom, INTER_NEAREST);
    cv::resize(TmpResiYuv, ResiYuv, Size(0, 0), Zoom, Zoom, INTER_NEAREST);
    cv::resize(TmpRecoYuv, RecoYuv, Size(0, 0), Zoom, Zoom, INTER_NEAREST);

    //---------------------------------------------
    // DRAWING EACH QUADTREE PARTITION
    //---------------------------------------------
    for ( UInt CTU_Index = 0; CTU_Index < Global.NumCTUs; CTU_Index++ )
    {
      //---------------------------------------------
      // GETTING ROI ON ZOOMED CTU REGION
      //---------------------------------------------
      // note: we need to expand the ROI by the zoom factor so we can draw text properly
      UInt uiWidthInLCUs  = pcPic->getPicSym()->getFrameWidthInCU();
      UInt uiHeightInLCUs = pcPic->getPicSym()->getFrameHeightInCU();
      Int iCUAddr     = pcPic->getCU( CTU_Index )->getAddr();
      Point CTU_Pos  = Point(iCUAddr % uiWidthInLCUs, iCUAddr / uiWidthInLCUs);

      //---------------------------------------------
      // LETS DIVE INTO A RECURSION
      //---------------------------------------------
      TComDataCU* pcCU = pcPic->getCU( CTU_Index );
      xDrawCU( pcCU, 0, 0, CTU_Pos);
    }


    sprintf(text, "Residual enhanced by x%d", ResiEnhanceFactor);
    writeText(ResiYuv, text, YELLOW, Point(30,30), 3.0);

    cout << string(textlen, '=') << endl; //==================
    call++;
  };

  void xDrawCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Point CTU_Pos)
  {
    TComPic* pcPic     = pcCU->getPic();
    UInt NumPartInCTU  = pcPic->getNumPartInCU();    //number of StorageUnits (4x4) in CTU
    UInt NumPartInCU = NumPartInCTU >> (uiDepth<<1); //number of StorageUnits (4x4) in current CU
    UInt NextCU_Increment   = NumPartInCU>>2; //increment (in StorageUnits) if CU is split further

    //if small block signals: way to go, not deep enough
    if( pcCU->getDepth(uiAbsZorderIdx) > uiDepth ) //if upper left StorageUnit says "Final depth dude!"
    {
      for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=NextCU_Increment )
      {
        //pcCU->getCUPelX() : get absolute pixel index if LCU(!) in frame
        UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
        UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
        if(    ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() )
            && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
        {
          xDrawCU( pcCU, uiAbsZorderIdx, uiDepth+1, CTU_Pos);
        }
      }
      return;
    }
    int offs = 1;
    //we arrived at the final partition depth for this CU

    sCU CU = get_CU_FIN(pcCU, uiAbsZorderIdx);
    drawSingleCU(PredYuv, CU);
    drawSingleCU(ResiYuv, CU);
    drawSingleCU(RecoYuv, CU);

    //off we go to next CU!
  } //end xDrawCU

  void drawSingleCU(Mat Img, sCU CU)
  {
    Mat CU_Roi = Img(Rect((CU.CTUPos.x + CU.RelPos.x) *Zoom, (CU.CTUPos.y + CU.RelPos.y)*Zoom, CU.Width *Zoom, CU.Width *Zoom));

    //----------------------------------------------------
    // DRAWING CU BORDERS
    //----------------------------------------------------
    if(isDrawingCUs)
    {
      int dist = CU.Width*Zoom;
      line(CU_Roi, Point(0,0), Point(dist-1,0), BLACK,LineWidth); //top
      line(CU_Roi, Point(0,0), Point(0, dist-1), BLACK,LineWidth);//left
      line(CU_Roi, Point(dist-1,dist-1), Point(dist-1, 0), BLACK,LineWidth);//right
      line(CU_Roi, Point(dist-1,dist-1), Point(0, dist-1), BLACK,LineWidth);//right
    }

  }
};

