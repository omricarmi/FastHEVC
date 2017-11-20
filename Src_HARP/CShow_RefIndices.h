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

class CShow_RefIndices
{

public:
  Mat Img;  //holds visualized image (no blending included)
  int Zoom; //Zoom of one CTU
  Point ImgDim;
  bool isInterestArea;
  bool isMarkingBlocks;
  bool isDrawingStoreUnits;
  bool isDrawingVectors;
  char text[500];
  int call; //keeping track of calls to this class
  vector<Point2f> KeypointsCur, KeypointsRef;
  vector<Point> MV_StartPoints;
  vector<Point> MV_EndPoints;

  //Interest area
  Point CTB_Span; //x and y span of observed LCUs (may be selection or all LCUs)
  Point CTB_UpperLeft; //from (inclusive)
  Point CTB_LowerRight; //to (exclusive)

  CShow_RefIndices(TComPic*& pcPic)
  {
    //---------------------------------------------
    // CHANGE HERE
    //---------------------------------------------

    isMarkingBlocks = true;
    isDrawingVectors = false;
    isInterestArea = false; // false means "full image"
    this->isDrawingVectors = isDrawingVectors;
    this->isMarkingBlocks = isMarkingBlocks;
    this->isDrawingStoreUnits = false;
    double BlendingAlpha = 0.7;

    CTB_UpperLeft  = Point(0,2);
    CTB_LowerRight = Point(1,3);
    //---------------------------------------------

    call = 0;
    sprintf(text, "====%s====", "CShow_RefIndices");
    int textlen = strlen(text);
    cout << text << endl;

    UInt WidthInLCUs  = pcPic->getPicSym()->getFrameWidthInCU();
    UInt HeightInLCUs = pcPic->getPicSym()->getFrameHeightInCU();

    if(isInterestArea) //only interest-area
    {
      Zoom = 64;
      CTB_Span = CTB_LowerRight - CTB_UpperLeft;
      ImgDim.x = CTB_Span.x*64*Zoom;
      ImgDim.y = CTB_Span.y*64*Zoom;
    }
    else //full image
    {
      Zoom = 3;
      CTB_UpperLeft  = Point(0,0);
      CTB_LowerRight = Point(WidthInLCUs, HeightInLCUs);
      CTB_Span = CTB_LowerRight - CTB_UpperLeft;
      ImgDim.x = (CTB_Span.x)*64*Zoom;
      ImgDim.y = (CTB_Span.y)*64*Zoom;
    }

    Img = Mat(ImgDim.y, ImgDim.x, CV_8UC3, BLACK);

    //---------------------------------------------
    // DRAWING EACH QUADTREE PARTITION
    //---------------------------------------------
    for ( UInt LCU_Index = 0; LCU_Index < Global.NumCTUs; LCU_Index++ )
    {
      //---------------------------------------------
      // GETTING ROI ON ZOOMED CTU REGION
      //---------------------------------------------
      // note: we need to expand the roi by the zoom factor so we can draw text properly
      UInt uiWidthInLCUs  = pcPic->getPicSym()->getFrameWidthInCU();
      UInt uiHeightInLCUs = pcPic->getPicSym()->getFrameHeightInCU();
      Int iCUAddr     = pcPic->getCU( LCU_Index )->getAddr();
      Point CTB_Pos  = Point(iCUAddr % uiWidthInLCUs, iCUAddr / uiWidthInLCUs);

      //don't dive into this CTU if it lies outside the interest area
      if( checkRange(CTB_Pos.x, true, 0, CTB_UpperLeft.x, CTB_LowerRight.x) == false ||
          checkRange(CTB_Pos.y, true, 0, CTB_UpperLeft.y, CTB_LowerRight.y) == false)
        continue;

      //CTB_Anc holds anchor (in pixels) relative to upper left corner of interest area!
      Point CTB_Anc = (CTB_Pos - CTB_UpperLeft) * 64 * Zoom;
      //CTB_Roi holds image ROI of the current CTU
      Mat CTB_Roi = Img(Rect(CTB_Anc.x, CTB_Anc.y, 64*Zoom, 64*Zoom));

      //---------------------------------------------
      // LETS DIVE INTO A RECURSION
      //---------------------------------------------
      TComDataCU* pcCU = pcPic->getCU( LCU_Index );
      xDrawCU( pcCU, 0, 0, CTB_Roi, CTB_Anc);

      //-
      //---------------------------------------------
      // FINALIZE: DRAW YELLOW CTU BORDER AND WRITE INDEX
      //---------------------------------------------
      //      line(CTB_Roi, Point(0,0), Point(64*Zoom-1,0), YELLOW, 10); //top
      //      line(CTB_Roi, Point(0,0), Point(0, 64*Zoom-1), YELLOW, 10);//left
      //      line(CTB_Roi, Point(64*Zoom-1,64*Zoom-1), Point(64*Zoom-1, 0), YELLOW, 10);//right
      //      line(CTB_Roi, Point(64*Zoom-1,64*Zoom-1), Point(0, 64*Zoom-1), YELLOW, 10);//right
      //-
      sprintf(text, "%d", LCU_Index);
      writeText(CTB_Roi, text, YELLOW, Point(7,7), 1.0);
    }
    //---------------------------------------------
    //---------------------------------------------
    //---------------------------------------------

    //---------------------------------------------
    // DRAWING THE MV'S WE COLLECTED
    //---------------------------------------------
    if(isDrawingVectors)
    {
      vector<Point>::const_iterator it1 = MV_StartPoints.begin();
      vector<Point>::const_iterator it2 = MV_EndPoints.begin();

      for( ; it1 != MV_StartPoints.end(); ++it1, ++it2)
      {
        Point S, E;
        S = *it1;
        E = *it2;
        int LineSize = 2;
        line(Img, S, E, GREEN_2, LineSize, CV_AA); //top

        double angle; // Draws the spin of the arrow
        angle = atan2( (double) S.y - E.y, (double) S.x - E.x );

        double spinSize = 15;
        S.x = (int) (E.x + spinSize * cos(angle + 3.1416 / 7));
        S.y = (int) (E.y + spinSize * sin(angle + 3.1416 / 7));
        line( Img, S, E, GREEN_2, LineSize, CV_AA, 0 );

        S.x = (int) (E.x + spinSize * cos(angle - 3.1416 / 7));
        S.y = (int) (E.y + spinSize * sin(angle - 3.1416 / 7));
        line( Img, S, E, GREEN_2, LineSize, CV_AA, 0 );
      }
    }

    //---------------------------------------------
    // BLENDING IN THE CUR FRAME
    //---------------------------------------------
    bool isBlending = true;
    Mat Blended;
    if(isBlending)
    {
      Mat Tmp;
      if (Global.isEncoder)
        copy_PicYuv2Mat(pcPic->getPicYuvOrg(), Tmp, INTER_LINEAR);
      else //decoder
        copy_PicYuv2Mat(pcPic->getPicYuvRec(), Tmp, INTER_LINEAR);
      convertToCV_8UC3(Tmp);
      Mat CurFrame = Mat(Global.HeightInLCUs*64, Global.WidthInLCUs*64, CV_8UC3, BLACK); //CTUs expand beyond image!
      Tmp.copyTo(CurFrame(Rect(0, 0, Global.DimX, Global.DimY)));
      cvtColor(CurFrame, CurFrame, CV_YCrCb2RGB);
      cvtColor(CurFrame, CurFrame, CV_RGB2GRAY);
      Mat pointers[] = { CurFrame, CurFrame, CurFrame };
      Mat Gray;
      merge(pointers, 3, Gray);
      Mat InterestArea = Gray(Rect(CTB_UpperLeft.x*64, CTB_UpperLeft.y*64, CTB_Span.x*64, CTB_Span.y*64));
      Mat Resized;
      cv::resize(InterestArea, Resized, Size(ImgDim.x, ImgDim.y),0,0, INTER_NEAREST);

      double beta =  1.0 - BlendingAlpha;
      addWeighted( Img, BlendingAlpha, Resized, beta, 0.0, Blended);
      Img = Blended;
    }

    Mat BorderCrop = Mat(Global.DimX*Zoom, Global.DimY*Zoom, CV_8UC3, BLACK);
    Img(Rect(0, 0, Global.DimX*Zoom, Global.DimY*Zoom)).copyTo(BorderCrop);
    Img = BorderCrop;

    cout << string(textlen, '=') << endl; //==================
    call++;
  };


  void xDrawCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Mat &CTB_Roi, Point &CTB_Anc)
  {
    TComPic* pcPic     = pcCU->getPic();
    UInt NumPartInCTB  = pcPic->getNumPartInCU();      //number of StorageUnits (4x4) in CTU
    UInt NumPartInCB = NumPartInCTB >> (uiDepth<<1); //number of StorageUnits (4x4) in current CU
    UInt NextCU_Increment   = NumPartInCB>>2; //increment (in StorageUnits) if CU is split further

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
          xDrawCU( pcCU, uiAbsZorderIdx, uiDepth+1, CTB_Roi, CTB_Anc);
        }
      }
      return;
    }
    //we arrived at the final partition depth for this CU
    int offs = 1; //used for text line feed
    const char *PartitionSizeStrings[] =   {"SIZE_2Nx2N","SIZE_2NxN","SIZE_Nx2N", "SIZE_NxN",
        "SIZE_2NxnU","SIZE_2NxnD","SIZE_nLx2N","SIZE_nRx2N"};
    const char *FalseTrueStrings[] = {"false", "true"};

    //---------------------------------------------
    // GETTING SOME INITIAL INFOS
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

    //unused
    int uiCUPelX = pcCU->getCUPelX();
    int uiCUPelY = pcCU->getCUPelY();

    //from Zscan to Raster, then getting x and y indices
    int RasterPartIdx  = g_auiZscanToRaster[uiAbsZorderIdx];
    Point AbsPart = Point(RasterPartIdx % (64/4), RasterPartIdx / (64/4)); // 64/4 = partition ticks (=StorageUnits-Ticks) in a LCU

    //---------------------------------------------
    // ANCHOR AND DIM INFO OF CU INSIDE THE CTU
    //---------------------------------------------
    //final pixel positions, extraction of CU roi
    int CB_Width = 64 >> uiDepth; //width of CU in pixels
    Point CB_Anc  = Point(AbsPart.x, AbsPart.y ) * 4 * Zoom; //times 4 since one Smallest-Partition spans 4x4 pixels!
    Point CB_Dims = Point(CB_Width , CB_Width ) * Zoom;
    Mat CB_Roi = CTB_Roi(Rect(CB_Anc.x, CB_Anc.y, CB_Dims.x, CB_Dims.y));

    //---------------------------------------------
    // INTRA / INTER COLORING OF CU
    //---------------------------------------------
    if(isMarkingBlocks)
    {
      if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTRA)
        CB_Roi = BLACK;
      else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER && pcCU->isSkipped(uiAbsZorderIdx))
        CB_Roi = WHITE; //we fill this later
      else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER)
        CB_Roi = WHITE; //we fill this later
      else
        CB_Roi = GRAY; //used for outer bounds CU's
    }

    //---------------------------------------------
    // DRAWING STORAGE-UNITS GRID
    //---------------------------------------------
    Point StorageUnits = Point(CB_Width/4, CB_Width/4);
    if(StorageUnits.x * StorageUnits.y != NumPartInCB)
      exit(-1);
    if(isDrawingStoreUnits)
      for(int y = 0; y<StorageUnits.y; y++)
        for(int x = 0; x<StorageUnits.x; x++)
        {
          Point SU_Anc  = Point(x*4, y*4 ) * Zoom; //times 4 since one Smallest-Partition spans 4x4 pixels!
          Point SU_Dims = Point(4 , 4 ) * Zoom;
          Mat SU_Roi = CB_Roi(Rect(SU_Anc.x, SU_Anc.y, SU_Dims.x, SU_Dims.y));

          //drawing StorageUnit borders
          Scalar color2(0,0,0);
          line(SU_Roi, Point(0,0), Point(SU_Dims.x-1,0), color2,1); //top
          line(SU_Roi, Point(0,0), Point(0, SU_Dims.y-1), color2,1);//left
          line(SU_Roi, Point(SU_Dims.x-1,SU_Dims.y-1), Point(SU_Dims.x-1, 0), color2,1);//right
          line(SU_Roi, Point(SU_Dims.x-1,SU_Dims.y-1), Point(0, SU_Dims.y-1), color2,1);//right
        }


    PartSize ePartSize = pcCU->getPartitionSize( uiAbsZorderIdx );
    UInt NumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
    //NextPU_Increment: increment in terms of StorageUnits
    UInt NextPU_Increment = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;

    //----------------------------------------------------
    //  PROCESSING PU'S
    //----------------------------------------------------
    if(pcCU->getPredictionMode(uiAbsZorderIdx) != MODE_INTRA) //avoid intra PU crosses
      for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsZorderIdx; uiPartIdx < NumPU; uiPartIdx++, uiSubPartIdx += NextPU_Increment )
      {
        //----------------------------------------------------
        // GETTING MERGE INFO
        //----------------------------------------------------
        bool MergeFlag    = pcCU->getMergeFlag( uiSubPartIdx );
        UInt MergeIndex = pcCU->getMergeIndex(uiSubPartIdx);

        //----------------------------------------------------
        // GETTING PU SIZES AND COUNT TO NEXT PU (PU OFFSET?)
        //----------------------------------------------------
        UInt ruiPartAddr;
        Int PU_Width,  PU_Height;
        myGetPartIndexAndSize( pcCU, uiAbsZorderIdx, uiDepth, uiPartIdx, //uiPartIdx = PUIdx: PU index, maybe just one, maybe up to four
            ruiPartAddr, PU_Width, PU_Height ); //ruiPartAddr is the same as the above uiSubPartIdx

        // ruiPartAddr is a relative index inside the current CU (no practical use here)
        // however, encoder code makes heavy use of this for addressing MV, MVField, etc.!
        if(ruiPartAddr != (uiSubPartIdx - uiAbsZorderIdx))
          CB_Roi = Scalar(64,128,64);

        //----------------------------------------------------
        // WE ARE ABLE TO RECONSTRUCT PU ANCHOR IN PIXELS RELATIVE TO THE CTU
        //----------------------------------------------------
        RasterPartIdx  = g_auiZscanToRaster[uiAbsZorderIdx+ruiPartAddr];
        int SUnitsPerRow = 64/4; // 64/4 = 16 StorageUnits in a CTU row

        Point PU_Anc  = Point(RasterPartIdx % (SUnitsPerRow), RasterPartIdx / (SUnitsPerRow)) * 4 *Zoom;
        Point PU_Dims = Point(PU_Width , PU_Height ) * Zoom;
        Mat PU_Roi = CTB_Roi(Rect(PU_Anc.x, PU_Anc.y, PU_Dims.x, PU_Dims.y));

        //----------------------------------------------------
        // WE ARE ABLE TO DETERMINE THE REFIDX OF THIS PU
        //----------------------------------------------------
        UInt AvailableRefs =  pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0);
        int RefIdx = pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiAbsZorderIdx+ruiPartAddr);
        if(RefIdx == 0)
          PU_Roi = REFIDX0;
        else if(RefIdx == 1)
          PU_Roi = REFIDX1;
        else if(RefIdx == 2)
          PU_Roi = REFIDX2;
        else if(RefIdx == 3)
          PU_Roi = REFIDX3;
        else
          PU_Roi = GRAY;

        if (RefIdx == AvailableRefs-1) //last one always yellow
            PU_Roi = REFIDX3;

        //----------------------------------------------------
        // DRAWING PU BORDERS
        //----------------------------------------------------
        if(isMarkingBlocks)
        {
          line(PU_Roi, Point(0,0), Point(PU_Dims.x-1,0), WHITE,2); //top
          line(PU_Roi, Point(0,0), Point(0, PU_Dims.y-1), WHITE,2);//left
          line(PU_Roi, Point(PU_Dims.x-1,PU_Dims.y-1), Point(PU_Dims.x-1, 0), WHITE,2);//right
          line(PU_Roi, Point(PU_Dims.x-1,PU_Dims.y-1), Point(0, PU_Dims.y-1), WHITE,2);//right
        }

        //----------------------------------------------------
        // WE ARE ABLE TO DETERMINE THE MV OF THIS PU
        //----------------------------------------------------
        TComMv cMv = pcCU->getCUMvField(REF_PIC_LIST_0)->getMv( uiAbsZorderIdx+ruiPartAddr );
        Point Mv(cMv.getHor(), cMv.getVer());
        if(Mv.x != 0 || Mv.y != 0) //if this PU has motion (taking care not to register zero-MVs from intra and stuff)
        {
          //we need anchors relative to the whole image!
          Point MV_Anc = CTB_Anc + PU_Anc + Point(PU_Dims.x / 2, PU_Dims.y / 2); //PU anchor is relative to CTU
          Point MV_Dims = Point(cMv.getHor()*Zoom / 4, cMv.getVer()*Zoom / 4 ); //division by 4: since MV has quadpel precision
          //if(isDrawingVectors)
          //   line(Img, MV_Anc, MV_Anc+MV_Dims, GREEN_2, 2, CV_AA); //top

          MV_StartPoints.push_back(MV_Anc);
          MV_EndPoints.push_back(MV_Anc+MV_Dims);

          //----------------------------------------------------
          // SAVING MV INFO FOR LATER PROCESSING
          //----------------------------------------------------



          Point2f KeypCur(float(MV_Anc.x)/Zoom, float(MV_Anc.y)/Zoom); //need to get rid of zoom for export
          KeypointsCur.push_back(Point2f(KeypCur));
          Point2f KeypRef = KeypCur +  Point2f(float(cMv.getHor())/ 4, float(cMv.getVer())/ 4);
          KeypointsRef.push_back(Point2f(KeypRef));
        }

        //----------------------------------------------------
        // WRITING PU TEXT
        //----------------------------------------------------
        if(isInterestArea)
        {
          sprintf(text, "---PU %d/%d, %dx%d---", uiPartIdx+1, NumPU, PU_Width, PU_Height);
          writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
          sprintf(text, "Merge Flag/Idx: %s, %d", FalseTrueStrings[MergeFlag], MergeIndex);
          writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
          sprintf(text, "ruiPartAddr: %d", ruiPartAddr);
          writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
          //sprintf(text, "Anchor: %d,%d", PU_Anc.x, PU_Anc.y);
          //writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
          sprintf(text, "MV hor/ver: %d/%d", Mv.x, Mv.y);
          writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
          //sprintf(text, "MV Anc hor/ver: %d/%d", MV_Anc.x, MV_Anc.y);
          //writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
        }
      }

    //----------------------------------------------------
    // DRAWING CU BORDERS
    //----------------------------------------------------
    if(isMarkingBlocks)
    {
      line(CB_Roi, Point(0,0), Point(CB_Dims.x-1,0), BLACK,3); //top
      line(CB_Roi, Point(0,0), Point(0, CB_Dims.y-1), BLACK,3);//left
      line(CB_Roi, Point(CB_Dims.x-1,CB_Dims.y-1), Point(CB_Dims.x-1, 0), BLACK,3);//right
      line(CB_Roi, Point(CB_Dims.x-1,CB_Dims.y-1), Point(0, CB_Dims.y-1), BLACK,3);//right
    }

    //----------------------------------------------------
    // WRITING CTU TEXT
    //----------------------------------------------------
    if(isInterestArea)
    {
      offs++;
      sprintf(text, "CTU Addr: %d", pcCU->getAddr()); //CTU Index
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "CU Depth: %d", pcCU->getDepth(uiAbsZorderIdx));
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "CU Dims: %dx%d", pcCU->getWidth(uiAbsZorderIdx), pcCU->getHeight(uiAbsZorderIdx));
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "AbsZorderIdx in CTU: %d", uiAbsZorderIdx);
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "Increment to next CU: %d", NextCU_Increment);
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "PU Partitions: %s", PartitionSizeStrings[pcCU->getPartitionSize(uiAbsZorderIdx)]);
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "Increment to next PU: %d", NextPU_Increment);
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "NumStorUnitsInCTB: %d", pcPic->getNumPartInCU());
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "NumStorUnitsInCB: %d", NumPartInCB);
      writeText(CB_Roi, text, WHITE, Point(0,offs++)*20);
    }
    //off we go to next CU!
  } //end xDrawCU

};

