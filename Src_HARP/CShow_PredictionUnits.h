// (c) 2014 Dominic Springer, Andreas Heindel
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include "TLibCommon/TypeDef.h"

#include "HARP_Defines.h"
#include "CHelper.h"
#include "CShow.h"

class CShow_PredictionUnits
{
public:
  Mat Img;  //holds visualized image (no blending included)
  int Zoom; //zoom factor for nice text rendering
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
  vector<Point2f> MVs;

  //Interest area
  Point CTU_Span; //x and y span of observed LCUs (may be selection or all LCUs)
  Point CTU_UpperLeft; //from (inclusive)
  Point CTU_LowerRight; //to (exclusive)

  CShow_PredictionUnits(TComPic* pcPic)
  {
    //---------------------------------------------
    // CHANGE HERE
    //---------------------------------------------
    isMarkingBlocks = true;
    isDrawingVectors = true;
    int Vis_MVsize = 2; //line width for MV drawing
    //Scalar Vis_MVcolor = GREEN; //color for MV drawing
    Scalar Vis_MVcolor = YELLOW; //color for MV drawing

    isInterestArea = false; // false means "full image"
    this->isDrawingVectors = isDrawingVectors;
    this->isMarkingBlocks = isMarkingBlocks;
    this->isDrawingStoreUnits = false;

    CTU_UpperLeft  = Point(0,2);
    CTU_LowerRight = Point(1,3);
    //---------------------------------------------

    call = 0;
    sprintf(text, "====%s====", "CShow_PredictionUnits");
    int textlen = strlen(text);
    cout << text << endl;

    if(isInterestArea) //only interest-area
    {
      Zoom = 64;
      CTU_Span = CTU_LowerRight - CTU_UpperLeft;
      ImgDim.x = CTU_Span.x*64*Zoom;
      ImgDim.y = CTU_Span.y*64*Zoom;
    }
    else //full image
    {
      Zoom = 3;
      CTU_UpperLeft  = Point(0,0);
      CTU_LowerRight = Point(Global.WidthInLCUs, Global.HeightInLCUs);
      CTU_Span = CTU_LowerRight - CTU_UpperLeft;
      ImgDim.x = (CTU_Span.x)*64*Zoom;
      ImgDim.y = (CTU_Span.y)*64*Zoom;
    }

    Img = Mat(ImgDim.y, ImgDim.x, CV_8UC3, BLACK);

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

      //don't dive into this CTU if it lies outside the interest area
      if( checkRange(CTU_Pos.x, true, 0, CTU_UpperLeft.x, CTU_LowerRight.x) == false ||
          checkRange(CTU_Pos.y, true, 0, CTU_UpperLeft.y, CTU_LowerRight.y) == false)
        continue;

      Point CTU_Anc = (CTU_Pos - CTU_UpperLeft) * 64 * Zoom; //anchor (in pixels) relative to upper left corner
      Mat CTU_Roi = Img(Rect(CTU_Anc.x, CTU_Anc.y, 64*Zoom, 64*Zoom)); //image ROI of the current CTU

      //---------------------------------------------
      // LETS DIVE INTO A RECURSION
      //---------------------------------------------
      TComDataCU* pcCU = pcPic->getCU( CTU_Index );
      xDrawCU( pcCU, 0, 0, CTU_Roi, CTU_Anc);

      sprintf(text, "%d", CTU_Index);
      writeText(CTU_Roi, text, YELLOW, Point(7,7), 1.0);
    }

    //---------------------------------------------
    // DRAWING THE MV'S WE COLLECTED
    //---------------------------------------------
    if(isDrawingVectors)
    {
      vector<Point>::const_iterator it1 = MV_StartPoints.begin();
      vector<Point>::const_iterator it2 = MV_EndPoints.begin();
      vector<Point2f>::const_iterator it3 = MVs.begin();

      for( ; it1 != MV_StartPoints.end(); ++it1, ++it2, ++it3)
      {
        Point S, E;
        S = *it1;
        E = *it2;
        double spinSize = 3*Vis_MVsize;
        line(Img, S, E, Vis_MVcolor, Vis_MVsize, CV_AA);

        double angle;
        angle = atan2( (double) S.y - E.y, (double) S.x - E.x );

        S.x = (int) (E.x + spinSize * cos(angle + 3.1416 / 7));
        S.y = (int) (E.y + spinSize * sin(angle + 3.1416 / 7));
        line( Img, S, E, Vis_MVcolor, Vis_MVsize, CV_AA, 0 );

        S.x = (int) (E.x + spinSize * cos(angle - 3.1416 / 7));
        S.y = (int) (E.y + spinSize * sin(angle - 3.1416 / 7));
        line( Img, S, E, Vis_MVcolor, Vis_MVsize, CV_AA, 0 );
      }
    }

    //---------------------------------------------
    // BLENDING IN THE CUR FRAME
    //---------------------------------------------

    bool isBlending = true;
    Mat Blended;
    Mat Resized1;
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

      cv::resize(CurFrame, Resized1, Size(ImgDim.x, ImgDim.y),0,0, INTER_LINEAR);

      cvtColor(CurFrame, CurFrame, CV_RGB2GRAY);
      Mat pointers[] = { CurFrame, CurFrame, CurFrame };
      Mat Gray;
      merge(pointers, 3, Gray);
      Mat InterestArea = Gray(Rect(CTU_UpperLeft.x*64, CTU_UpperLeft.y*64, CTU_Span.x*64, CTU_Span.y*64));

      Mat Resized2;
      cv::resize(InterestArea, Resized2, Size(ImgDim.x, ImgDim.y),0,0, INTER_LINEAR);
      double alpha = 0.5;
      double beta =  1.0 - alpha;
      addWeighted( Img, alpha, Resized2, beta, 0.0, Blended);
      Img = Blended;
    }

    Mat BorderCrop = Mat(Global.DimX*Zoom, Global.DimY*Zoom, CV_8UC3, BLACK);
    Img(Rect(0, 0, Global.DimX*Zoom, Global.DimY*Zoom)).copyTo(BorderCrop);
    Img = BorderCrop;

    cout << string(textlen, '=') << endl; //==================
    call++;
  };

  void xDrawCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Mat &CTU_Roi, Point &CTU_Anc)
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
          xDrawCU( pcCU, uiAbsZorderIdx, uiDepth+1, CTU_Roi, CTU_Anc);
        }
      }
      return;
    }
    int offs = 1;
    //we arrived at the final partition depth for this CU

    //---------------------------------------------
    // GETTING CU INFOS
    //---------------------------------------------
    sCU CU = get_CU_FIN(pcCU, uiAbsZorderIdx);
    //assert(AbsPart.x == CU.ResPos_SU.x && AbsPart.y == CU.ResPos_SU.y);

    Mat CU_Roi = CTU_Roi(Rect(CU.RelPos.x *Zoom, CU.RelPos.y *Zoom, CU.Width *Zoom, CU.Width *Zoom));

    //---------------------------------------------
    // INTRA / INTER COLORING OF CU
    //---------------------------------------------
    bool isMergeMode = pcCU->getMergeFlag(uiAbsZorderIdx);
    if(isMarkingBlocks)
    {
      if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTRA)
        CU_Roi = RED;
      else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER && pcCU->isSkipped(uiAbsZorderIdx))
        CU_Roi = GREEN;
      else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER && isMergeMode)
        CU_Roi = DARKGREEN;
      else if(pcCU->getPredictionMode(uiAbsZorderIdx) == MODE_INTER)
        CU_Roi = BLUE;
      else
        CU_Roi = GRAY; //used for outer bounds CU's
    }


    PartSize ePartSize = pcCU->getPartitionSize( uiAbsZorderIdx );
    UInt NumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
    //NextPU_Increment: increment in terms of StorageUnits
    UInt NextPU_Increment = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;

    //----------------------------------------------------
    //  PROCESSING PU'S
    //----------------------------------------------------
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
      // assert(ruiPartAddr != (uiSubPartIdx - uiAbsZorderIdx)); //just making sure we understood everything
      if(ruiPartAddr != (uiSubPartIdx - uiAbsZorderIdx))
        CU_Roi = Scalar(64,128,64);

      //----------------------------------------------------
      // WE ARE ABLE TO RECONSTRUCT PU ANCHOR IN PIXELS RELATIVE TO THE CTU
      //----------------------------------------------------
      int RasterPartIdx  = g_auiZscanToRaster[uiAbsZorderIdx+ruiPartAddr];
      int SUnitsPerRow = 64/4; // 64/4 = 16 StorageUnits in a CTU row
      // Point Anchor = Point(RasterPartIdx % (SUnitsPerRow), RasterPartIdx / (SUnitsPerRow))*4;

      //int CU_Width = 64 >> uiDepth; //width of CU in pixels
      Point PU_Anc  = Point(RasterPartIdx % (SUnitsPerRow), RasterPartIdx / (SUnitsPerRow)) * 4 *Zoom;
      Point PU_Dims = Point(PU_Width , PU_Height ) * Zoom;
      Mat PU_Roi = CTU_Roi(Rect(PU_Anc.x, PU_Anc.y, PU_Dims.x, PU_Dims.y));

      //----------------------------------------------------
      // WE ARE ABLE TO DETERMINE THE MV OF THIS PU
      //----------------------------------------------------
      TComMv cMv = pcCU->getCUMvField(REF_PIC_LIST_0)->getMv( uiAbsZorderIdx+ruiPartAddr );
      Point Mv(cMv.getHor(), cMv.getVer());
      if(Mv.x != 0 || Mv.y != 0) //if this PU has motion (taking care not to register zero-MVs from intra and stuff)
      {
        //we need anchors relative to the whole image!
        Point MV_Anc = CTU_Anc + PU_Anc + Point(PU_Dims.x / 2, PU_Dims.y / 2); //PU anchor is relative to CTU
        int Magni = 1;
        Point MV_Dims = Point(cMv.getHor()*Zoom*Magni / 4, cMv.getVer()*Zoom*Magni / 4 ); //division by 4: since MV has quadpel precision

        MVs.push_back(Point2f((float)(cMv.getHor()) / 4, (float)(cMv.getVer())/ 4 ));
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
      // DRAWING PU BORDERS
      //----------------------------------------------------
      {
        line(PU_Roi, Point(0,0), Point(PU_Dims.x-1,0), WHITE,2); //top
        line(PU_Roi, Point(0,0), Point(0, PU_Dims.y-1), WHITE,2);//left
        line(PU_Roi, Point(PU_Dims.x-1,PU_Dims.y-1), Point(PU_Dims.x-1, 0), WHITE,2);//right
        line(PU_Roi, Point(PU_Dims.x-1,PU_Dims.y-1), Point(0, PU_Dims.y-1), WHITE,2);//bottom
      }
    }

    //----------------------------------------------------
    // DRAWING CU BORDERS
    //----------------------------------------------------
    if(isMarkingBlocks)
    {
      int dist = CU.Width*Zoom;
      line(CU_Roi, Point(0,0), Point(dist-1,0), BLACK,3); //top
      line(CU_Roi, Point(0,0), Point(0, dist-1), BLACK,3);//left
      line(CU_Roi, Point(dist-1,dist-1), Point(dist-1, 0), BLACK,3);//right
      line(CU_Roi, Point(dist-1,dist-1), Point(0, dist-1), BLACK,3);//right
    }

    //----------------------------------------------------
    // WRITING CTU TEXT
    //----------------------------------------------------
    if(isInterestArea)
    {
      offs++;
      sprintf(text, "CTU Addr: %d", pcCU->getAddr()); //CTU Index
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "CU Depth: %d", pcCU->getDepth(uiAbsZorderIdx));
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "CU Dims: %dx%d", pcCU->getWidth(uiAbsZorderIdx), pcCU->getHeight(uiAbsZorderIdx));
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "AbsZorderIdx in CTU: %d", uiAbsZorderIdx);
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "Increment to next CU: %d", NextCU_Increment);
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "PU Partitions: %s", PartitionSizeStrings[pcCU->getPartitionSize(uiAbsZorderIdx)]);
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "Increment to next PU: %d", NextPU_Increment);
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "NumStorUnitsInCTU: %d", pcPic->getNumPartInCU());
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
      sprintf(text, "NumStorUnitsInCU: %d", NumPartInCU);
      writeText(CU_Roi, text, WHITE, Point(0,offs++)*20);
    }
    //off we go to next CU!
  } //end xDrawCU
};

