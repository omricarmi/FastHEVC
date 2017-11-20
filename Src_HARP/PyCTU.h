// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include "HARP_Defines.h"
#include "CLibGlobal.h"
#include "PyFactory.h"

class CPyAppendCUs
{

  PyObject *PyList_FIN_CUs;

public:

  CPyAppendCUs(TComDataCU* pcCU, PyObject *PyList_FIN_CUs)
{
    if(not Global.isObsPOC())
      return;

    this->PyList_FIN_CUs = PyList_FIN_CUs;
    xProcessCU( pcCU, 0, 0);
};



  void xProcessCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth)
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
          xProcessCU( pcCU, uiAbsZorderIdx, uiDepth+1);
        }
      }
      return;
    }

    //---------------------------------------------
    // WE ARRIVED AT THE FINAL DEPTH FOR THIS CU
    //---------------------------------------------
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
      // GETTING PU SIZES AND COUNT TO NEXT PU (PU OFFSET?)
      //----------------------------------------------------
      UInt ruiPartAddr;
      Int PU_Width,  PU_Height;
      myGetPartIndexAndSize( pcCU, uiAbsZorderIdx, uiDepth, uiPartIdx, //uiPartIdx = PUIdx: PU index, maybe just one, maybe up to four
          ruiPartAddr, PU_Width, PU_Height ); //ruiPartAddr is the same as the above uiSubPartIdx

      //----------------------------------------------------
      // GETTING MERGE INFO
      //----------------------------------------------------
      bool MergeFlag    = pcCU->getMergeFlag( uiSubPartIdx );
      UInt MergeIndex = pcCU->getMergeIndex(uiSubPartIdx);

      //----------------------------------------------------
      // WE ARE ABLE TO RECONSTRUCT PU ANCHOR IN PIXELS RELATIVE TO THE CTU
      //----------------------------------------------------
      int RasterPartIdx  = g_auiZscanToRaster[uiAbsZorderIdx+ruiPartAddr];
      int SUnitsPerRow = 64/4; // 64/4 = 16 StorageUnits in a CTU row

      Point PU_Anc  = Point(RasterPartIdx % (SUnitsPerRow), RasterPartIdx / (SUnitsPerRow)) * 4;
      Point PU_Dims = Point(PU_Width , PU_Height );
    }
    //off we go to next CU!
  } //end xDrawCU
};
