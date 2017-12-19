// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include <TLibCommon/TComPic.h>
#include "TLibCommon/TypeDef.h"

#include "HARP_Defines.h"
#include "CHelper.h"
#include "CShow.h"
#include <string>
#include "json-forwards.h"
#include "json.h"
#include <fstream>

void exportImagePNG(Mat Image, string nickname, string dirName) ;

class CuData {
public:

    // POC, CTU info
    UInt POC;
    UInt  CTUIdx;      //CTU address (running index)
    Point CTUPos;       //absolute CTU position in pixels

    // CU position in frame, width, depth
    Point Pos;          //absolute CU position in pixels
    UInt  Width;        //CU dimension
    UInt  Depth;        //CU final depth

    // CU abs/raster partition indices, RelPos
    // everything is relative to upper left CTU corner
    UInt  AbsPartIdx;   //SU index, zorder scan
    UInt  RastPartIdx;  //SU index, raster scan
    Point RelPos;       //CU position relative to CTU

    // Mode
    string Mode;

    //QP
    UInt QP;

    // image yuv values
    Mat orgYuv;

    //
    string imgSrc;


    // default constructor
    CuData(Mat Img, sCU CU){
        CTUIdx = CU.CTUIdx;
        CTUPos = CU.CTUPos;
        Pos = CU.Pos;
        Width = CU.Width;
        Depth = CU.Depth;
        AbsPartIdx = CU.AbsPartIdx;
        RastPartIdx = CU.RastPartIdx;
        RelPos = CU.RelPos;
        Mode = CU.Mode;
        QP = CU.QP;
        POC = CU.POC;
        imgSrc = "";

        //generate where to save the data
        stringstream ss;
        ss << "CNNOutput/" << "F" << POC << "/CTU" << CTUIdx;
        string destDir = ss.str();
        system(("mkdir -p " + destDir).c_str());
        stringstream ss2;
        ss2 << "CU" << RelPos.x << "_" << RelPos.y;
        string imgName = ss2.str();
        imgSrc = destDir + string("/") + imgName;

        //save the image to file
        Mat CU_Roi = Img(Rect((CU.CTUPos.x + CU.RelPos.x), (CU.CTUPos.y + CU.RelPos.y), CU.Width, CU.Width));
        exportImagePNG(CU_Roi,imgName,destDir);

        Json::Value root;
        root["CTUIdx"] = CU.CTUIdx;
        root["CTUPos"]["x"] = CU.CTUPos.x;
        root["CTUPos"]["y"] = CU.CTUPos.y;
        root["Pos"]["x"] = CU.Pos.x;
        root["Pos"]["y"] = CU.Pos.y;
        root["Width"] = CU.Width;
        root["Depth"] = CU.Depth;
        root["AbsPartIdx"] = CU.AbsPartIdx;
        root["RastPartIdx"] = CU.RastPartIdx;
        root["RelPos"]["x"] = CU.RelPos.x;
        root["RelPos"]["y"] = CU.RelPos.y;
        root["Mode"] = CU.Mode;
        root["QP"] = CU.QP;
        root["POC"] = CU.POC;
        root["imgSrc"] = imgSrc + ".png";

        ofstream file((imgSrc + ".json").c_str());
        file << root << endl;
        file.close();

    }



};

class CShow_PredResiReco
{

public:
    //FastHEVC
    Mat OrgYuv; //holds the original yuv pixels
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
        //FastHEVC
        copy_internal_picyuv(HM_2_MAT, pcPic->getPicYuvOrg(), OrgYuv, Rect(), TEXT_LUMA);
        copy_internal_picyuv(HM_2_MAT, pcPic->getPicYuvPred(), TmpPredYuv, Rect(), TEXT_LUMA);
        copy_internal_picyuv(HM_2_MAT, pcPic->getPicYuvResi(), TmpResiYuv, Rect(), TEXT_LUMA);
        copy_internal_picyuv(HM_2_MAT, pcPic->getPicYuvRec(), TmpRecoYuv, Rect(), TEXT_LUMA);

        TmpResiYuv = TmpResiYuv * (ResiEnhanceFactor / 2)  + 127; //scale to 8 bits and shift "mean" to 127

        //FastHEVC
        convertToCV_8UC3(OrgYuv);
        convertToCV_8UC3(TmpPredYuv);
        convertToCV_8UC3(TmpResiYuv);
        convertToCV_8UC3(TmpRecoYuv);

        cv::resize(TmpPredYuv, PredYuv, Size(0, 0), Zoom, Zoom, INTER_NEAREST);
        cv::resize(TmpResiYuv, ResiYuv, Size(0, 0), Zoom, Zoom, INTER_NEAREST);
        cv::resize(TmpRecoYuv, RecoYuv, Size(0, 0), Zoom, Zoom, INTER_NEAREST);

        //FastHEVC
        stringstream ss;
        ss << "CNNOutput/" << "F" << pcPic->getPOC();
        string destDir = ss.str();
        system(("mkdir -p " + destDir).c_str());
        stringstream ss2;
        ss2 << "F" << pcPic->getPOC() << "-X-" << Global.DimX << "-Y-" << Global.DimY;
        exportImagePNG(OrgYuv,ss2.str(),destDir);


        //---------------------------------------------
        // DRAWING EACH QUADTREE PARTITION
        //---------------------------------------------
        for ( UInt CTU_Index = 0; CTU_Index < Global.NumCTUs; CTU_Index++ )
        {
            //---------------------------------------------
            // GETTING ROI ON ZOOMED CTU REGION
            //---------------------------------------------
            // note: we need to expand the ROI by the zoom factor so we can draw text properly
            UInt uiWidthInLCUs  = pcPic->getPicSym()->getFrameWidthInCtus();
            UInt uiHeightInLCUs = pcPic->getPicSym()->getFrameHeightInCtus();
            Int iCUAddr     = pcPic->getCtu( CTU_Index )->getCtuRsAddr();
            Point CTU_Pos  = Point(iCUAddr % uiWidthInLCUs, iCUAddr / uiWidthInLCUs);

            //FastHEVC
            stringstream ss;
            ss << "CNNOutput/" << "F" << pcPic->getPOC() << "/CTU" << CTU_Index;
            string destDir = ss.str();
            system(("mkdir -p " + destDir).c_str());
            //crop to CTU or less not enough
            // TODO handle case of horizon modulo from 64 in the start of the frame and not the end (left not right as we saw on analyzer)
            int spareX = Global.DimX - CTU_Pos.x * 64;
            int spareY = Global.DimY - CTU_Pos.y * 64;
            Mat CU_Roi = OrgYuv(Rect(CTU_Pos.x * 64, CTU_Pos.y * 64, (spareX < 64) ? spareX : 64, (spareY < 64) ? spareY : 64));
            stringstream ss2;
            ss2 << "CTU-" <<CTU_Index << "-X-" << CTU_Pos.x*64 << "-Y-" << CTU_Pos.y*64;
            exportImagePNG(CU_Roi,ss2.str(),destDir);

            //---------------------------------------------
            // LETS DIVE INTO A RECURSION
            //---------------------------------------------
            TComDataCU* pcCU = pcPic->getCtu( CTU_Index );
            xDrawCU( pcCU, 0, 0, CTU_Pos);
        }


        sprintf(text, "Residual enhanced by x%d", ResiEnhanceFactor);
//    writeText(ResiYuv, text, YELLOW, Point(30,30), 3.0);

        cout << string(textlen, '=') << endl; //==================
        call++;
    };

    void xDrawCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Point CTU_Pos)
    {
        TComPic* pcPic     = pcCU->getPic();
        UInt NumPartInCTU  = pcPic->getNumPartitionsInCtu();    //number of StorageUnits (4x4) in CTU
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

        // FastHEVC
        CuData cuData(OrgYuv,CU);

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

void exportImagePNG(Mat Image, string nickname, string dirName) {
    string FN;
    FN = Global.outputDataset + "/" + dirName + "/" + nickname + ".png";

    //format given by provided filename!
    int Quality = 0; //0 = slower
    vector<int> params;
    params.push_back(16);
//  params.push_back(CV_IMWRITE_PNG_COMPRESSION); //TODO by omricarmi on 19/12/2017: check why dont work
    params.push_back(Quality);

    imwrite(FN.c_str(), Image, params);
    cout << "Exported: " << FN << endl;
}