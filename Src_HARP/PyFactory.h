// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include "TLibCommon/TypeDef.h"
#include "TLibEncoder/TEncCu.h"

#include "HARP_Defines.h"
#include "CHelper.h"
#include "CShow.h"

#include "PyNDArray.h"
#include "CGlobal.h"
#include "PyHelper.h"

#include "CGlobal.h"

#define DATATREE_DEBUG

#define ME_TRANSL 0
#define ME_AFFINE 1
//#define ME_NUM    2

// ====================================================================================================================
// WARNING: all PyDict_SetItem calls probably leak memory when combined with PyString_FromString, PyInt_FromLong etc.
// FIXME ASAP!!
// ====================================================================================================================



// IMPORTANT NOTES:
// GENERAL RULE: List STEALS, Dict INCREFS!!! (when using SetItem)
// PyString_FromString: returns NEW reference
// PyInt_FromLong: returns NEW reference

// PyList_SetItem: STEALS the object (does NOT INCREF) and discards the reference to the old object at this position
// PyDict_SetItem: Does NOT steal, i.e. INCREFs the object
// PyList_Append:  Does NOT steal, i.e. INCREFs the object
// PyTuple_GetItem: returns BORROWED REFERENCE, i.e. DOES NOT DECREF the object

//GREAT EXPLANATION
//The objects you create with PyInt_FromLong() and you add to the list should be kept in a local variable.
//
//The reason are the ownership rules: PyInt_FromLong() generates a reference that you own. In the call to PyTuple_SetItem(), you lose this ownership again, because PyTuple_SetItem() "steals" it from you, so you don't have to care about. But PyList_Append() doesn't do so, it increases the refcount. In order to have the object GC'ed correctly, you have to release your ownership by DECREF'ing.
//
//So, instead of PyList_Append(item, PyInt_FromLong(jp)), you do the following:
//
//PyObject * jpo = PyInt_FromLong(jp);
//// do some error checking here
//PyList_Append(item, jpo);
//Py_DECREF(jpo);

// ==========================================================================================================================
// =================================================  NUMPY <-> YUV CONVERSION ==============================================
// ==========================================================================================================================

//------------------------------------------------------------------------------
//  TCOM YUV  (to numpy)
//------------------------------------------------------------------------------
inline PyObject* convert_Yuv2Py_Luminance(TComYuv* pcYuv)
{
  int Height  = pcYuv->getHeight();
  int Width   = pcYuv->getWidth();
  int Stride  = pcYuv->getStride();
  Pel *Src  = pcYuv->getLumaAddr();

  npy_intp dimarray[2] = {Width, Height};
  PyObject* poDst = PyArray_SimpleNew(2, dimarray, NPY_SHORT);
  Pel* Dst = (Pel*) PyArray_DATA(poDst);

  const npy_intp* _strides = PyArray_STRIDES(poDst);
  int stride_y = _strides[0];
  int stride_x = _strides[1];

  for (int y = 0; y < Height; y++)
  {
    ::memcpy(Dst, Src, sizeof(Pel) * Width);
    Src += Stride;
    Dst += Width;
  }

  return poDst;
}

//------------------------------------------------------------------------------
//  TCOM YUV  (from numpy)
//------------------------------------------------------------------------------
inline void convert_Py2Yuv_Luminance(PyObject* poSrc, TComYuv* pcYuv)
{
  assert (PyArray_TYPE(poSrc) == NPY_SHORT);
  assert (PyArray_NDIM(poSrc) == 2);
  const npy_intp* _sizes   = PyArray_DIMS(poSrc);
  const npy_intp* _strides = PyArray_STRIDES(poSrc);
  int stride_y = _strides[0];
  int stride_x = _strides[1];

  int Height  = pcYuv->getHeight();
  int Width   = pcYuv->getWidth();
  int Stride  = pcYuv->getStride();
  Pel *Dst  = pcYuv->getLumaAddr();
  assert(Height == _sizes[0]);
  assert(Width  == _sizes[1]);

  Pel* Src = (Pel*) PyArray_DATA(poSrc);

  for (int y = 0; y < Height; y++)
  {
    ::memcpy(Dst, Src, sizeof(Pel) * Width);
    Dst += Stride;
    Src += Width;
  }
}

//------------------------------------------------------------------------------
//  TCOM YUV  (from numpy, small area special version)
//------------------------------------------------------------------------------
inline void convert_Py2Yuv_Luminance_Part(PyObject* poSrc, TComYuv* pcYuv, UInt uiPartAddr, UInt Width, UInt Height)
{
  // see copyPartToPartLuma() for related code
  assert (PyArray_TYPE(poSrc) == NPY_SHORT);
  assert (PyArray_NDIM(poSrc) == 2);
  const npy_intp* _sizes   = PyArray_DIMS(poSrc);
  const npy_intp* _strides = PyArray_STRIDES(poSrc);

  int Stride = pcYuv->getStride();
  Pel *Dst   = pcYuv->getLumaAddr(uiPartAddr);
  assert(Height == _sizes[0]);
  assert(Width  == _sizes[1]);


  Pel* Src = (Pel*) PyArray_DATA(poSrc);

  for (int y = 0; y < Height; y++)
  {
    ::memcpy(Dst, Src, sizeof(Pel) * Width);
    Dst += Stride;
    Src += Width;
  }
}

//------------------------------------------------------------------------------
//  TCOM PICYUV
//------------------------------------------------------------------------------
inline PyObject* convert_PicYuv2Py(TComPicYuv* pcPicYuv, int Channel)
{
  int Margin, Stride, Width, Height;
  Pel* Src;

  if(Channel == TEXT_LUMA)
  {
    Margin = pcPicYuv->getLumaMargin();
    Width =  pcPicYuv->getWidth();
    Height =  pcPicYuv->getHeight();
    Src = pcPicYuv->getBufY();
    Stride = pcPicYuv->getStride();
  }
  else  //CHROMA
  {
    Margin = pcPicYuv->getChromaMargin();
    Width =  pcPicYuv->getWidth() / 2;
    Height =  pcPicYuv->getHeight() / 2;
    Stride = Width + (pcPicYuv->getChromaMargin() * 2);
    if(Channel == TEXT_CHROMA_U)
      Src = pcPicYuv->getBufU();
    else if (Channel == TEXT_CHROMA_V)
      Src = pcPicYuv->getBufV();
    else assert(0);
  }
  Src += Margin + Margin*Stride; //getting rid of first margin in the upper left

  //we could also go via pcPicYuv->getLumaAddr( pcCU->getAddr(), uiZOrder );
  //we would not need to go via pcPicYuv->getBufY()
  //untested, but way more elegant...

  npy_intp dimarray[2] = {Height, Width};
  PyObject* poDst = PyArray_SimpleNew(2, dimarray, NPY_SHORT);
  Pel* Dst = (Pel*) PyArray_DATA(poDst);

  for (int y = 0; y < Height; y++)
  {
    ::memcpy(Dst, Src, sizeof(Pel) * Width);
    Src += Stride;
    Dst += Width;
  }

  return poDst;
}

//------------------------------------------------------------------------------
//  TCOM PICYUV
//------------------------------------------------------------------------------
inline PyObject* convert_PicYuv2Py_Y_SingleCU(TComPicYuv* pcPicYuv, TComDataCU* pcCU, UInt AbsPartIdx)
{
  UInt uiPartOffset = 0; //we are only interested in CU, not in its parts
  UInt    uiZOrder  = pcCU->getZorderIdxInCU() + uiPartOffset;
  Pel*    Src       = pcPicYuv->getLumaAddr( pcCU->getAddr(), AbsPartIdx );

  //-
  //HACK
  //  Src = pcPicYuv->getBufY();
  //  pcPicYuv->getBufY();
  //  int Margin = pcPicYuv->getLumaMargin();
  //  int Stride = pcPicYuv->getStride();
  //  Src += Margin + Margin*Stride;
  //-


  UInt  iSrcStride  = pcPicYuv->getStride();
  int CU_Width = 64 >> pcCU->getDepth(AbsPartIdx);
  int Width  = CU_Width;
  int Height = CU_Width;

  npy_intp dimarray[2] = {Height, Width};
  PyObject* poDst = PyArray_SimpleNew(2, dimarray, NPY_SHORT);
  Pel* Dst = (Pel*) PyArray_DATA(poDst);

  for (int y = 0; y < Height; y++)
  {
    ::memcpy(Dst, Src, sizeof(Pel) * Width);
    Src += iSrcStride;
    Dst += Width;
  }

  return poDst;
}

//------------------------------------------------------------------------------
//  TCOM PICYUV
//------------------------------------------------------------------------------
inline PyObject* pyf_create_PIC( TComPicYuv* pcPicYuv )
{
  PyObject* PoDict = PyDict_New();

  PyObject* poY = convert_PicYuv2Py(pcPicYuv, TEXT_LUMA);
  PyObject* poU = convert_PicYuv2Py(pcPicYuv, TEXT_CHROMA_U);
  PyObject* poV = convert_PicYuv2Py(pcPicYuv, TEXT_CHROMA_V);
  PyDict_SetItem(PoDict, PyString_FromString("Y"), poY);
  PyDict_SetItem(PoDict, PyString_FromString("U"), poU);
  PyDict_SetItem(PoDict, PyString_FromString("V"), poV);

  Py_DECREF(poY);
  Py_DECREF(poU);
  Py_DECREF(poV);

  return PoDict;
}

// ==========================================================================================================================
// =================================================  CREATE HARP DATA TREE OBJECTS =========================================
// ==========================================================================================================================

//------------------------------------------------------------------------------
//  POC
//------------------------------------------------------------------------------
inline PyObject* pyf_create_POC( TComPic* rpcPic, int SliceIdx, bool withRefList )
{
  assert(SliceIdx == 0);

  //welcome to this world!
  PyObject* PoDict = PyDict_New();

  //HM interfacing
  UInt Width_inCTUs  = rpcPic->getPicSym()->getFrameWidthInCU();
  UInt Height_inCTUs = rpcPic->getPicSym()->getFrameHeightInCU();
  TComPicYuv* YuvRec = rpcPic->getPicYuvRec(); //YuvRec because we may be on decoder side, also
  int DimX = YuvRec->getWidth();
  int DimY = YuvRec->getHeight();
  int POC = rpcPic->getPOC();

  //Object name
  PyDict_SetItemString(PoDict, "Name", PyString_FromString("POC"));

  //Input YUV
  Dict_addString(PoDict, "InputYUV", Global.FN_InputYUV.toStdString().c_str());

  //POCIdx
  PyDict_SetItemString(PoDict, "Idx", PyInt_FromLong(POC));

  //Size in CTUs
  PyObject *PoTuple1 = pyf_getTuple(Width_inCTUs, Height_inCTUs);
  PyDict_SetItem(PoDict, PyString_FromString("Size_inCTUs"), PoTuple1);
  Py_DECREF(PoTuple1);

  //Size
  PyObject *PoTuple2 = pyf_getTuple(DimX, DimY);
  PyDict_SetItem(PoDict, PyString_FromString("Size"), PoTuple2);
  Py_DECREF(PoTuple2);

  if (Global.isEncoder)
  {
    //YuvOrg
    PyObject* poYuvOrg = pyf_create_PIC(rpcPic->getPicYuvOrg());
    PyDict_SetItem(PoDict, PyString_FromString("YuvOrg"), poYuvOrg);
    Py_DECREF(poYuvOrg);
  }

  //IMPORTANT NOTE:
  // pyf_create_POC() may be called for current POC and also(!) for RefPOCs
  // if we are doing a RefPOC, YuvRec is valid (finished already)
  // if we are doing CurPOC, YuvRec is not yet ready here!
  // solution: YuvRec will be overwritten at end of compressSlice()!

  //YuvRec
  PyObject* poYuvRec = pyf_create_PIC(rpcPic->getPicYuvRec());
  PyDict_SetItem(PoDict, PyString_FromString("YuvRec"), poYuvRec);
  Py_DECREF(poYuvRec);

  //RPS index of slice (TBD: doublecheck RPS mechanism)
  PyDict_SetItemString(PoDict, "RPS_idx", PyInt_FromLong(rpcPic->getSlice(SliceIdx)->getRPSidx()));

  //Ref POCs, LIST0
  if (withRefList)
  {
    PyObject *poRefList0 = PyList_New(0);
    PyDict_SetItem(PoDict, PyString_FromString("List0_Refs"), poRefList0);
    Py_DECREF(poRefList0);
    TComSlice* pcSlice = rpcPic->getSlice(SliceIdx);
    int NumRefFrames =  pcSlice->getNumRefIdx(REF_PIC_LIST_0); //same way RDO iterates through ref list
    for(int i = 0; i < NumRefFrames; i++)
    {
      TComPic* pcPic_ref = pcSlice->getRefPic(REF_PIC_LIST_0, i);
      PyObject *poRefPOC = pyf_create_POC(pcPic_ref, 0, false);
      PyDict_SetItemString(poRefPOC, "RefPos", PyInt_FromLong(i));
      PyDict_SetItemString(poRefPOC, "Meta_RPS_Size", PyInt_FromLong(NumRefFrames));
      PyDict_SetItemString(poRefPOC, "Meta_RPS_NumNegPics", PyInt_FromLong(pcSlice->getRPS()->getNumberOfNegativePictures()));
      PyDict_SetItemString(poRefPOC, "Meta_RPS_NumPosPics", PyInt_FromLong(pcSlice->getRPS()->getNumberOfPositivePictures()));


//      cout << "m_numberOfPictures:" << RefPicSet->m_numberOfPictures << endl;
//      cout << "m_numRefIdc:" << RefPicSet->m_numRefIdc << endl;
//      cout << "getRPSidx():" << slice->getRPSidx() << endl;
//      cout << "getRPS()->getNumberOfNegativePictures():" << slice->getRPS()->getNumberOfNegativePictures() << endl;
//      cout << "getRPS()->getNumberOfPositivePictures():" << slice->getRPS()->getNumberOfPositivePictures() << endl;

      PyList_Append(poRefList0, poRefPOC);
      Py_DECREF(poRefPOC);
    }
  }

  //List of CTUs
  PyObject* poList= PyList_New(0);
  PyDict_SetItem(PoDict, PyString_FromString("CTUs"), poList);
  Py_DECREF(poList);

  return PoDict;
}

//------------------------------------------------------------------------------
//  CTU
//------------------------------------------------------------------------------
inline PyObject* pyf_create_CTU(TComDataCU* pcCU)
{
  //welcome to this world!
  PyObject* PoDict = PyDict_New();

  //HM interfacing
  TComPic* rpcPic = pcCU->getPic();
  UInt Width_inCTUs  = rpcPic->getPicSym()->getFrameWidthInCU();
  UInt Height_inCTUs = rpcPic->getPicSym()->getFrameHeightInCU();
  int CTUIdx  = pcCU->getAddr();
  int Width   = CTU_DIM;
  int CTUPosx = (CTUIdx % Width_inCTUs) * CTU_DIM;
  int CTUPosy = (CTUIdx / Width_inCTUs) * CTU_DIM;

  //Object name
  PyDict_SetItemString(PoDict, "Name", PyString_FromString("CTU"));

  //CTU Idx
  //PyDict_SetItem(PoDict, PyString_FromString("CTUIdx"), PyInt_FromLong(CTUIdx));
  PyDict_SetItemString(PoDict, "Idx", PyInt_FromLong(CTUIdx));

  //CTU Size
  PyObject *PoTuple1 = pyf_getTuple(Width, Width);
  PyDict_SetItemString(PoDict, "Size", PoTuple1);
  Py_DECREF(PoTuple1);

  //CTU Anchor
  PyObject *PoTuple2 = pyf_getTuple(CTUPosx, CTUPosy);
  PyDict_SetItemString(PoDict, "Pos", PoTuple2);
  Py_DECREF(PoTuple2);

  //-
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  //	Global.PyList_RDO_CUs = PyList_New(0); //keep a pointer to this goodie
  //	PyDict_SetItemString(PoDict, "RDO_CUs", Global.PyList_RDO_CUs);
  //	Py_DECREF(Global.PyList_RDO_CUs);
  //
  //	Global.PyList_FIN_CUs = PyList_New(0); //keep a pointer to this goodie
  //	PyDict_SetItemString(PoDict, "CUs", Global.PyList_FIN_CUs);
  //	Py_DECREF(Global.PyList_FIN_CUs);
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //-

  //CUs_RDO, CUs_POST, CUs_FINAL
  PyObject *List;
  List = PyList_New(0);
  PyDict_SetItemString(PoDict, "CUs_RDO", List);
  Py_DECREF(List);
  List = PyList_New(0);
  PyDict_SetItemString(PoDict, "CUs_POST", List);
  Py_DECREF(List);
  List = PyList_New(0);
  PyDict_SetItemString(PoDict, "CUs_FINAL", List);
  Py_DECREF(List);

  return PoDict;
}

//------------------------------------------------------------------------------
//  CU
//------------------------------------------------------------------------------
inline PyObject* pyf_create_CU_internal(sCU &CU);

inline PyObject* pyf_create_CU(string IDStr, TComDataCU* pcCU, TEncCu* EncCu, UInt AbsPartIdx)
{
  PyObject* PoCU;
  if (IDStr.compare("CUs_RDO") == 0) //if same as
  {
    if ( AbsPartIdx != 0 )
      THROW_ERROR("pyf_create_CU not correctly used");
    sCU CU = get_CU_RDO(pcCU);
    PoCU = pyf_create_CU_internal(CU);

    // RDO SPECIAL: ADD CHECKS-LIST
    PyDict_DelItem(PoCU, PyString_FromString("PUs")); //not used for CUs_RDO
    PyObject *poList = PyList_New(0);
    PyDict_SetItemString(PoCU, "CHECKs", poList);
    Py_DECREF(poList);

    //-
    //		TRACING MEMORY LEAK
    //		Mat test(100*3,100*3, CV_8UC3, Scalar(0,0,128));
    //		PyObject *PoMat2 = Global.Converter->toNDArray(test);
    //		PyDict_SetItem(PoCU, PyString_FromString("TestMAT"), PoMat2);
    //		Py_DECREF(PoMat2);
    //-
  }
  else if (IDStr.compare("CUs_FINAL") == 0 or IDStr.compare("CUs_POST") == 0 ) //if same as
  {
    sCU CU = get_CU_FIN(pcCU, AbsPartIdx);
    PoCU = pyf_create_CU_internal(CU);

    PyObject *po_Y_Resi  = convert_PicYuv2Py_Y_SingleCU(pcCU->getPic()->getPicYuvResi(), pcCU, AbsPartIdx);
    PyObject *po_Y_Reco  = convert_PicYuv2Py_Y_SingleCU(pcCU->getPic()->getPicYuvRec(), pcCU, AbsPartIdx);
    PyObject *po_Y_Pred  = convert_PicYuv2Py_Y_SingleCU(pcCU->getPic()->getPicYuvPred(), pcCU, AbsPartIdx);
    PyObject *po_Y_Orig  = convert_PicYuv2Py_Y_SingleCU(pcCU->getPic()->getPicYuvOrg(), pcCU, AbsPartIdx);

    Dict_addObject(PoCU, "Y_Resi",  po_Y_Resi);
    Dict_addObject(PoCU, "Y_Reco",  po_Y_Reco);
    Dict_addObject(PoCU, "Y_Pred",  po_Y_Pred);
    Dict_addObject(PoCU, "Y_Orig",  po_Y_Orig);
  }

  return PoCU;
}

inline PyObject* pyf_create_CU_internal(sCU &CU)
{

  // create dictionary
  PyObject* PoDict = PyDict_New();

  // Object name
  PyDict_SetItemString(PoDict, "Name", PyString_FromString("CU"));

  // POC, CTU info
  //	PyDict_SetItem(PoDict, PyString_FromString("POCIdx"), PyInt_FromLong(CU.POC));
  //	PyDict_SetItem(PoDict, PyString_FromString("CTUIdx"), PyInt_FromLong(CU.CTUIdx));

  // CU position in frame
  PyObject *PoTuple = pyf_getTuple(CU.Pos.x, CU.Pos.y);
  PyDict_SetItem(PoDict, PyString_FromString("Pos"), PoTuple);
  Py_DECREF(PoTuple);

  // CU position relative to CTU origin
  PyObject *PoTuple2 = pyf_getTuple(CU.RelPos.x, CU.RelPos.y);
  PyDict_SetItem(PoDict, PyString_FromString("RelPos"), PoTuple2);
  Py_DECREF(PoTuple2);

  // CU Size
  PyObject *PoTuple1 = pyf_getTuple(CU.Width, CU.Width);
  PyDict_SetItemString(PoDict, "Size", PoTuple1);
  Py_DECREF(PoTuple1);

  // Depth
  PyDict_SetItem(PoDict, PyString_FromString("Depth"), PyInt_FromLong(CU.Depth));

  // CU abs/raster partition indices
  PyDict_SetItem(PoDict, PyString_FromString("AbsPartIdx"), PyInt_FromLong(CU.AbsPartIdx));
  PyDict_SetItem(PoDict, PyString_FromString("RastPartIdx"), PyInt_FromLong(CU.RastPartIdx));

  // Mode
  PyDict_SetItemString(PoDict, "Mode", PyString_FromString(CU.Mode.c_str()));

  // QP
  PyDict_SetItemString(PoDict, "QP", PyInt_FromLong(CU.QP));

  // List of PUs
  PyObject *poList = PyList_New(0); //keep a pointer to this goodie
  PyDict_SetItemString(PoDict, "PUs", poList);
  Py_DECREF(poList);

  return PoDict;
}

//------------------------------------------------------------------------------
//  PU
//------------------------------------------------------------------------------
inline PyObject* pyf_create_PU_internal(string IDStr, sPU &PU);

inline PyObject* pyf_create_PU(string IDStr, TComDataCU* pcCU, UInt iPartIdx, UInt AbsPartIdx, UInt Depth )
{
  if (IDStr.compare("CUs_RDO") == 0) //if same as
  {
    if ( AbsPartIdx != 0 or Depth != 0)
      THROW_ERROR("pyf_create_PU not correctly used");
    sCU CU = get_CU_RDO(pcCU);
    sPU PU;
    PU.CU = CU;

    //HM Interfacing
    UInt  uiPartAddr;
    Int   PU_Width, PU_Height;
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, PU_Width, PU_Height );
    UInt AbsPartIdx    = pcCU->getZorderIdxInCU();
    int RasterPartIdx  = g_auiZscanToRaster[AbsPartIdx+uiPartAddr];
    int SUnitsPerRow = CTU_DIM/4; // 64/4 = 16 StorageUnits in a CTU row

    PU.RelPos = getRelPos(RasterPartIdx);
    PU.Pos  = PU.CU.CTUPos + PU.RelPos;
    PU.Size = Point(PU_Width , PU_Height );
    PU.uiPartAddr  = uiPartAddr;
    PU.AbsPartIdx  = AbsPartIdx+uiPartAddr;
    PU.RastPartIdx = RasterPartIdx;

    return pyf_create_PU_internal(IDStr, PU);
  }
  else if ( (IDStr.compare("CUs_POST") == 0) or (IDStr.compare("CUs_FINAL") == 0)) //if same as
  {
    sCU CU = get_CU_FIN(pcCU, AbsPartIdx);
    sPU PU;
    PU.CU = CU;

    UInt uiPartAddr;
    Int PU_Width,  PU_Height;
    myGetPartIndexAndSize( pcCU, AbsPartIdx, Depth, iPartIdx, //uiPartIdx = PUIdx: PU index, maybe just one, maybe up to four
        uiPartAddr, PU_Width, PU_Height ); //ruiPartAddr is the same as the above uiSubPartIdx

    int RasterPartIdx  = g_auiZscanToRaster[AbsPartIdx+uiPartAddr];
    int SUnitsPerRow = 64/4; // 64/4 = 16 StorageUnits in a CTU row

    PU.RelPos = getRelPos(RasterPartIdx);
    PU.Pos  = PU.CU.CTUPos + PU.RelPos;
    PU.Size = Point(PU_Width , PU_Height );
    PU.uiPartAddr  = uiPartAddr;
    PU.AbsPartIdx  = AbsPartIdx+uiPartAddr;
    PU.RastPartIdx = RasterPartIdx;

    PyObject* poPU = pyf_create_PU_internal(IDStr, PU);
    return poPU;
  }
  else
    THROW_ERROR("Unknown CU list");
}

inline PyObject* pyf_create_PU_internal(string IDStr, sPU &PU)
{
  //debug break point
//  if (PU.CU.Mode.compare("Merge") == 0)
//    assert(0);

  // create dictionary
  PyObject* PoDict = PyDict_New();
  Dict_addString(PoDict, "Name", "PU"); //Object name
  Dict_addString(PoDict, "Mode", PU.CU.Mode.c_str()); //Mode (for convenience)
  Dict_addObject(PoDict,  "Pos", pyf_getTuple(PU.Pos.x, PU.Pos.y)); //PU position in frame
  Dict_addObject(PoDict,  "RelPos", pyf_getTuple(PU.RelPos.x, PU.RelPos.y)); // PU position relative to CTU origin
  Dict_addObject(PoDict,  "RelPos_inCU", pyf_getTuple(PU.RelPos.x - PU.CU.RelPos.x, PU.RelPos.y - PU.CU.RelPos.y)); // PU position relative to CU
  Dict_addObject(PoDict,  "Size", pyf_getTuple(PU.Size.x, PU.Size.y)); //PU size
  Dict_addLong(PoDict,   "uiPartAddr",  PU.uiPartAddr);  //
  Dict_addLong(PoDict,   "AbsPartIdx",  PU.AbsPartIdx);  // PU abs/raster partition indices
  Dict_addLong(PoDict,   "RastPartIdx", PU.RastPartIdx); // PU abs/raster partition indices

  //List of RDO MEs
  if ((IDStr.compare("CUs_RDO") == 0))
  {
    PyObject *poList = PyList_New(0);
    PyDict_SetItemString(PoDict, "CHECKs", poList);
    Py_DECREF(poList);
  }

  return PoDict;
}

//------------------------------------------------------------------------------
//  RDO TEST COLLECTIONS
//------------------------------------------------------------------------------
inline PyObject* pyf_create_CHECK_PartSize(TComDataCU* pcCU, string Type, PartSize ePartSize)
{
  // create dictionary
  PyObject* PoDict = PyDict_New();

  //Object name
  Dict_addString(PoDict, "Name", "CHECK_PartSize");
  Dict_addString(PoDict, "Type", Type.c_str());
  Dict_addString(PoDict, "PartSize", PartitionSizeStrings[ePartSize]);

  // List of PUs
  PyObject *poList = PyList_New(0); //keep a pointer to this goodie
  Dict_addObject(PoDict, "PUs", poList);
  return PoDict;
}

//-
//------------------------------------------------------------------------------
//  APPEND INFO TO ME
//------------------------------------------------------------------------------
// NO LONGER USED!!
inline void pyf_appendto_ME(PyObject* poME,  TComMv _cMVTemp, TComMv _cMvPred, Int _CandIdx,
    UInt _uiMvBits, UInt _uiMvCost, UInt _ruiCost_SADonly, UInt _uiBitsTemp, UInt _uiCostTemp)
{
  assert(0);

  //MV (found)
  PyObject *PoTuple = pyf_getTuple(_cMVTemp.getHor(), _cMVTemp.getVer());
  PyDict_SetItem(poME, PyString_FromString("MV_Found"), PoTuple);
  Py_DECREF(PoTuple);


  //MV Predictor
  PyObject *PoTuple2 = pyf_getTuple(_cMvPred.getHor(), _cMvPred.getVer());
  PyDict_SetItem(poME, PyString_FromString("MV_Pred"), PoTuple2);
  Py_DECREF(PoTuple2);

  PyDict_SetItem(poME, PyString_FromString("MV_CandIdx"), PyInt_FromLong(_CandIdx));

  PyDict_SetItem(poME, PyString_FromString("ME_Bits_MVdiff"), PyInt_FromLong(_uiMvBits));
  PyDict_SetItem(poME, PyString_FromString("ME_Cost_MVdiff"), PyInt_FromLong(_uiMvCost));
  PyDict_SetItem(poME, PyString_FromString("ME_Cost_SADonly"), PyInt_FromLong(_ruiCost_SADonly));
  PyDict_SetItem(poME, PyString_FromString("ME_Bits_Final"), PyInt_FromLong(_uiBitsTemp));
  PyDict_SetItem(poME, PyString_FromString("ME_Cost_Final"), PyInt_FromLong(_uiCostTemp));
}

//------------------------------------------------------------------------------
//  EncodeRes
//------------------------------------------------------------------------------
// NO LONGER USED!!
inline PyObject* pyf_create_EncodeResidual(TComDataCU* pcCU, int MyPredInfoBits)
{
  // create dictionary
  PyObject *PoDict = PyDict_New();

  PyDict_SetItemString(PoDict, "Name", PyString_FromString("EncodeRes"));
  PyDict_SetItem(PoDict, PyString_FromString("PredInfoBits"), PyInt_FromLong(MyPredInfoBits));



  return PoDict;
}

//------------------------------------------------------------------------------
//  BITS
//------------------------------------------------------------------------------

inline PyObject* pyf_create_CU_BITS(float Bits_SkipFlag, float Bits_SkipMergeIndex, float Bits_PredMode, float Bits_PartSize,
    float Bits_IntraIPCM, float Bits_PredInfoAllPUs, float Bits_Coeffs)
{
  // create dictionary
  PyObject *PoDict = PyDict_New();

  // Bits
  Dict_addFloat(PoDict, "SkipFlag", Bits_SkipFlag);
  Dict_addFloat(PoDict, "SkipMergeIndex", Bits_SkipMergeIndex); //carefull: only of meaning if skip is active
  Dict_addFloat(PoDict, "PredMode", Bits_PredMode);
  Dict_addFloat(PoDict, "PartSize", Bits_PartSize);
  Dict_addFloat(PoDict, "IntraIPCM", Bits_IntraIPCM);
  Dict_addFloat(PoDict, "PredInfoAllPUs", Bits_PredInfoAllPUs);
  Dict_addFloat(PoDict, "Coeffs", Bits_Coeffs);

  return PoDict;
}

//-

// ==========================================================================================================================
// ========================================================= THE HARP DATA TREE =============================================
// ==========================================================================================================================

//Debug and test HDT nodes with:
//pyf_Wrapper_save_CPickle_FN(CTU, "test_CTU.hdt");
//pyf_Wrapper_save_CPickle_FN(LastCU, "test_LastCU.hdt");

class CDataTree
{
public:
  enum EDetail { eNONE=0, ePOC=1, eCTU=2, eCU=3, ePU=4, eRDO=5, eRDO_ME=6, eALL=10};

  PyObject* POC;
  EDetail Detail;
  bool ExportHDT = true;

  CDataTree() {}

  void init()
  {
    import_array();
    Detail = (EDetail) pyf_INI_load_INT("HARP", "DataTreeDetail");
    ExportHDT = (EDetail) pyf_INI_load_INT("HARP", "ExportHDT");
  }

  void savePOC(char *FN)
  {
    if (Detail >= ePOC) //safety check
    {
      if (ExportHDT)
      {
        INIT_TIMER
        START_TIMER;

        cout << "Exporting PKL: " << FN << endl;
        pyf_Wrapper_save_CPickle_FN(POC, FN);

        //-
        //			sprintf(FN, "%sPOC_%05d.harp", Global.TmpDir.c_str(), Global.CurrentPOC);
        //			cout << "Exporting HARP DataTree: " << FN << endl;
        //			pyf_Wrapper_saveMarshalFN(POC, FN);
        //
        //			sprintf(FN, "%sPOC_%05d.harppkl", Global.TmpDir.c_str(), Global.CurrentPOC);
        //			cout << "Exporting PKL: " << FN << endl;
        //			pyf_callFunc_pickleObject(POC, FN);
        //-
        STOP_TIMER("Export");
      }
    }
  }

  void clear()
  {
    if (Detail >= ePOC)
      Py_DECREF(POC);
  }

  PyObject *getLastCU(string IDStr, PyObject *CTU_parent)
  {
    PyObject *List = PyDict_GetItemString(CTU_parent, IDStr.c_str());
    assert(List != NULL);
    Py_ssize_t Size = PyList_Size(List);
    assert(Size != 0);
    PyObject *LastCU = PyList_GetItem(List, Size-1);
    return LastCU;
  }

  PyObject *getLastCHECK(PyObject *CU_parent)
  {
    PyObject *List = PyDict_GetItemString(CU_parent, "CHECKs");
    assert(List != NULL);
    Py_ssize_t Size = PyList_Size(List);
    assert(Size != 0);
    PyObject *LastTEST = PyList_GetItem(List, Size-1);
    return LastTEST;
  }

  PyObject *getLastPU(PyObject *parent) //parent might be either CU or CHECK
  {
    PyObject *List = PyDict_GetItemString(parent, "PUs");
    assert(List != NULL);
    Py_ssize_t Size = PyList_Size(List);
    assert(Size != 0);
    PyObject *LastPU = PyList_GetItem(List, Size-1);
    return LastPU;
  }

  PyObject *getCTU(Py_ssize_t Idx)
  {
    PyObject *List = PyDict_GetItemString(POC, "CTUs");
    assert(List != NULL);
    PyObject *CTU = PyList_GetItem(List, Idx);
    return CTU;
  }


  //------------------------------------------------------------------------------
  // SLICE INFO
  //------------------------------------------------------------------------------
  PyObject* HARP_SliceInfo_create()
  {
    PyObject* SliceInfo = PyDict_New();
    PyDict_SetItemString(this->POC, "SliceInfo", SliceInfo);
    Py_DECREF(SliceInfo);
    return SliceInfo;
  }

  PyObject* HARP_SliceInfo_get()
  {
    PyObject *SliceInfo = PyDict_GetItemString(POC, "SliceInfo");
    assert(SliceInfo != NULL);
    return SliceInfo;
  }

  void HARP_SliceInfo_add__Type_QP(TComSlice*  pcSlice)
  {
    PyObject *SliceInfo = this->HARP_SliceInfo_get();
    Char c = (pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B');
    char SliceType[100];
    sprintf(SliceType, "%c" , c);

    // SliceInfo
    Dict_addString(SliceInfo, "SliceType", SliceType);
    Dict_addLong(SliceInfo, "SliceQpBase", pcSlice->getSliceQpBase());
    Dict_addLong(SliceInfo, "SliceQp", pcSlice->getSliceQp());
  }

  //------------------------------------------------------------------------------
  // POC
  //------------------------------------------------------------------------------
  PyObject *HARP_POC_get()
  {
    return this->POC;
  }

  void HARP_POC_add__Bits_PNSR_SSD(int NumAccessUnits, float  RBSPBits, float POCBits,
                                float PSNR_Y, float PSNR_U, float PSNR_V, float SSD_Y, float SSD_U, float SSD_V )
  {
    if (Detail >= ePOC)
    {
      PyObject *po_POC = HARP_POC_get();

      Dict_addLong(po_POC, "NumAccessUnits", NumAccessUnits);
      Dict_addLong(po_POC, "Bits_RBSP", RBSPBits);
      Dict_addLong(po_POC, "Bits_POC", POCBits);

      PyObject* po_PSNR_SSD = PyDict_New();
      PyDict_SetItemString(po_POC, "PSNR_SSD", po_PSNR_SSD);
      Py_DECREF(po_PSNR_SSD);

      Dict_addFloat(po_PSNR_SSD, "PSNR_Y", PSNR_Y);
      Dict_addFloat(po_PSNR_SSD, "PSNR_U", PSNR_U);
      Dict_addFloat(po_PSNR_SSD, "PSNR_V", PSNR_V);
      Dict_addFloat(po_PSNR_SSD, "SSD_Y", SSD_Y);
      Dict_addFloat(po_PSNR_SSD, "SSD_U", SSD_U);
      Dict_addFloat(po_PSNR_SSD, "SSD_V", SSD_V);
    }
  }

  void create_POC(TComPic* rpcPic, int SliceIdx, bool withRefList )
  {
    if (Detail >= ePOC)
      POC = pyf_create_POC(rpcPic, SliceIdx, true); //create dictionary
  }

  void overwrite_YuvRec(TComPic* rpcPic)
  {
    if (Detail >= ePOC)
    {
      //YuvRec already exists as a dummy, we delete it, then recreate it
      PyDict_DelItem(POC, PyString_FromString("YuvRec"));
      PyObject* poYuvRec = pyf_create_PIC(rpcPic->getPicYuvRec());
      PyDict_SetItem(POC, PyString_FromString("YuvRec"), poYuvRec);
      Py_DECREF(poYuvRec);
    }
  }

  // ADD A MAT IMG (CV_8UC3) TO THE HDT
  void add_CSHOW(Mat VisImg, string Name)
  {
    if (Detail >= ePOC)
    {
      PyObject *PoVisImg = Global.Converter->toNDArray(VisImg);
      PyDict_SetItem(POC, PyString_FromString(Name.c_str()), PoVisImg);
      Py_DECREF(PoVisImg);
    }
  }

  void create_CTU(TComDataCU* pcCU)
  {
    if (Detail >= eCTU )
    {
      PyObject *List = PyDict_GetItemString(POC, "CTUs");
      PyObject *PyDict = pyf_create_CTU(pcCU); //create dictionary
      PyList_Append(List, PyDict); //append dict to list
      Py_DECREF(PyDict); //list now "owns" the dict
    }
  }

  void create_CU(string IDStr, TComDataCU* pcCU, TEncCu* EncCu, UInt AbsPartIdx)
  {
    if (Global.CurrentPOC == 1)
      int a = 0;

    if (Detail >= eCU)
    {
      PyObject *CTU = getCTU(pcCU->getAddr());
      PyObject *List = PyDict_GetItemString(CTU, IDStr.c_str());
      PyObject *PyDict = pyf_create_CU(IDStr, pcCU, EncCu, AbsPartIdx);
      PyList_Append(List, PyDict); //append dict to list
      Py_DECREF(PyDict); //list now "owns" the dict
    }
  }

  void create_CHECK_PartSize(string IDStr, TComDataCU* pcCU, string Type, PartSize ePartSize)
  {
    assert (IDStr.compare("CUs_RDO") == 0);
    if (Detail >= ePU) // and Global.isObsCTU())
    {
      PyObject *CTU = getCTU(pcCU->getAddr());
      PyObject *LastCU  = getLastCU("CUs_RDO", CTU); //IDStr selects CUs_RDO, CUs_POST or CUs_FINAL
      PyObject *List = PyDict_GetItemString(LastCU, "CHECKs");
      PyObject *PyDict = pyf_create_CHECK_PartSize(pcCU, Type, ePartSize);
      PyList_Append(List, PyDict); //append dict to list
      Py_DECREF(PyDict); //list now "owns" the dict
    }
  }

  void append_CHECK_PartSize(string IDStr, TComDataCU* pcCU, TComYuv* PredYuv, TComYuv* RecYuv, TComYuv* ResiYuv)
  {
    assert (IDStr.compare("CUs_RDO") == 0);
    if (Detail >= ePU) // and Global.isObsCTU())
    {
      assert (IDStr.compare("CUs_RDO") == 0);

      PyObject *CurCTU   = getCTU(pcCU->getAddr());
      PyObject *CurCU    = getLastCU("CUs_RDO", CurCTU);
      PyObject *CurCHECK = getLastCHECK(CurCU);

      assert (IDStr.compare("CUs_RDO") == 0);
      PyObject *poPredYuv = convert_Yuv2Py_Luminance(PredYuv);
      PyObject *poRecYuv  = convert_Yuv2Py_Luminance(RecYuv);
      PyObject *poResiYuv = convert_Yuv2Py_Luminance(ResiYuv);

      Dict_addObject(CurCHECK, "PredYuv", poPredYuv);
      Dict_addObject(CurCHECK, "RecYuv",  poRecYuv);
      Dict_addObject(CurCHECK, "ResiYuv", poResiYuv);

      Dict_addLong(CurCHECK, "TotalCost", pcCU->getTotalCost());
      Dict_addLong(CurCHECK, "TotalBits", pcCU->getTotalBits());
      Dict_addLong(CurCHECK, "TotalDistortion", pcCU->getTotalDistortion());
    }
  }

  PyObject * create_PU(string IDStr, TComDataCU* pcCU, UInt iPartIdx, UInt AbsPartIdx, UInt Depth )
  {
    PyObject *PyDict;

    if (Detail >= ePU)
    {
      PyObject *CTU = getCTU(pcCU->getAddr());
      PyObject *LastCU  = getLastCU(IDStr, CTU); //IDStr selects CUs_RDO, CUs_POST or CUs_FINAL
      PyObject *List;

      if(IDStr.compare("CUs_RDO") == 0)
      {
        if (1) //Global.isObsCTU())
        {
          PyObject *LastCHECK= getLastCHECK(LastCU);
          List = PyDict_GetItemString(LastCHECK, "PUs");
          PyDict = pyf_create_PU(IDStr, pcCU, iPartIdx, AbsPartIdx, Depth);
          PyList_Append(List, PyDict); //append dict to list
          Py_DECREF(PyDict); //list now "owns" the dict
        }
      }
      else if(IDStr.compare("CUs_POST") == 0)
      {
        List = PyDict_GetItemString(LastCU, "PUs");
        PyDict = pyf_create_PU(IDStr, pcCU, iPartIdx, AbsPartIdx, Depth);
        PyList_Append(List, PyDict); //append dict to list
        Py_DECREF(PyDict); //list now "owns" the dict
      }
      else if(IDStr.compare("CUs_FINAL") == 0)
      {
        List = PyDict_GetItemString(LastCU, "PUs");
        PyDict = pyf_create_PU(IDStr, pcCU, iPartIdx, AbsPartIdx, Depth);
        PyList_Append(List, PyDict); //append dict to list
        Py_DECREF(PyDict); //list now "owns" the dict
      }
    }

    return PyDict;
  }

  void append_PU(string IDStr, PyObject *CurPU, TComDataCU* pcCU, UInt Bits_PUPredInfo)
  {
    if (Detail >= ePU)
    {
      // create dictionary
      PyObject *PoDict = PyDict_New();

      //we call append_PU only for inter PUs currently, so Inter-Only is assumed below
      Dict_addString(PoDict, "Name", "Motion"); //Object name

      UInt AbsPartIdx = PyInt_AsLong(PyDict_GetItem(CurPU, PyString_FromString("AbsPartIdx")));
      UInt uiPartAddr = PyInt_AsLong(PyDict_GetItem(CurPU, PyString_FromString("uiPartAddr")));
      UInt WorkaroundIdx;
      if (IDStr.compare("CUs_RDO") == 0) //if same as
        WorkaroundIdx = uiPartAddr;
      if (IDStr.compare("CUs_POST") == 0) //if same as
        WorkaroundIdx = AbsPartIdx;
      if (IDStr.compare("CUs_FINAL") == 0) //if same as
        WorkaroundIdx = AbsPartIdx;

      //we did not include append_PU for intra yet
      assert(pcCU->getPredictionMode(WorkaroundIdx) != MODE_INTRA);


      if (pcCU->getPredictionMode(WorkaroundIdx) != MODE_INTRA)
      {
        TComMv Mv;
        int RefIdx;
        Mv = pcCU->getCUMvField(REF_PIC_LIST_0)->getMv( WorkaroundIdx );
        RefIdx = pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(WorkaroundIdx);

        if (IDStr.compare("CUs_POST") == 0 or IDStr.compare("CUs_FINAL") == 0)
        {
          //assert(0);  //for release "breakpoint" :)
          Dict_addLong(PoDict, "Bits_PUPredInfo", Bits_PUPredInfo);
        }

//        //debug
//        if (IDStr.compare("CUs_FINAL") == 0)
//          cout << IDStr << endl << flush;
//          int a = 0;

        Dict_addObject(PoDict,  "MV", pyf_getTuple(Mv.getHor(), Mv.getVer())); //MV xy
        Dict_addLong(PoDict, "RefIdx", RefIdx);


#ifdef ACTIVATE_NEXT_MC
        //-
        Dict_addObject(PoDict,  "H_GT_bb", Global.Converter->toNDArray(Mv.get_H_GT_bb())); //H_GT_bb
        Dict_addObject(PoDict,  "H_GT_gg", Global.Converter->toNDArray(Mv.get_H_GT_gg())); //H_GT_bb
        Dict_addObject(PoDict,  "H_est", Global.Converter->toNDArray(Mv.get_H_est())); //H_est
        Dict_addObject(PoDict,  "H_estqu", Global.Converter->toNDArray(Mv.get_H_estqu())); //H_estqu
        //-
        if( Mv.isAffine == true)
          Dict_addString(PoDict,  "Type", "AFFINE");
        else Dict_addString(PoDict, "Type", "TRANSL");

#elif defined ACTIVATE_NEXT_RDO
        Dict_addLong(PoDict,  "NewMC", Mv.isNewMC);
        //assert(Mv.isNewMC == 0);
//        if (Mv.isNewMC)
//          if(IDStr.compare("CUs_RDO") != 0)
//            cout << IDStr << endl;
            //assert(IDStr.compare("CUs_RDO") == 0);
        Dict_addString(PoDict, "Type", "TRANSL");
#endif
      }

      Dict_addObject(CurPU, "Motion", PoDict);
    }
  }

  PyObject* create_CHECK_MotionType(string IDStr, int TypeME, TComDataCU* pcCU, Int iPartIdx)
  {
    PyObject *PoDict;
    assert (IDStr.compare("CUs_RDO") == 0);
    if (Detail >= eRDO_ME ) //and Global.isObsCTU())
    {
      PyObject *CTU = getCTU(pcCU->getAddr());
      PyObject *LastCU     = getLastCU("CUs_RDO", CTU); //IDStr selects CUs_RDO, CUs_POST or CUs_FINAL
      PyObject *LastCHECK  = getLastCHECK(LastCU);
      PyObject *LastPU     = getLastPU(LastCHECK);
      PyObject *List = PyDict_GetItemString(LastPU, "CHECKs");

      PoDict = PyDict_New();
      Dict_addString(PoDict, "Name", "CHECK_MotionType");
      if( TypeME == ME_TRANSL)
        Dict_addString(PoDict, "Type", "TRANSL");
      if( TypeME == ME_AFFINE)
        Dict_addString(PoDict, "Type", "AFFINE");
      Dict_addLong(PoDict, "PUIdx", iPartIdx );

      PyList_Append(List, PoDict); //append dict to list
      Py_DECREF(PoDict); //list now "owns" the dict
    }

    return PoDict;
  }

  void appendTo_CHECK_MotionType(string IDStr, PyObject *poCurParent, Int iRefIdx, Int CandIdx, TComMv _cMVTemp, TComMv _cMvPred,
      double Costs_MvBits, double Costs_Sideinfo, double Costs_HadSAD, double Costs_Final, double uiCostTemp, double uiBitsTemp)
  {

    if (Detail >= eRDO_ME ) //and Global.isObsCTU())
    {
      PyObject *PoDict = poCurParent;
      Dict_addInt_asTuple(PoDict, "cMVTemp", _cMVTemp.getHor(), _cMVTemp.getVer());
      Dict_addInt_asTuple(PoDict, "cMVPred", _cMvPred.getHor(), _cMvPred.getVer());
      Dict_addLong(PoDict, "RefIdx", iRefIdx);
      Dict_addLong(PoDict, "CandIdx", CandIdx);
      Dict_addFloat(PoDict, "Costs_MvBits", Costs_MvBits);
      Dict_addFloat(PoDict, "Costs_Sideinfo", Costs_Sideinfo );
      Dict_addFloat(PoDict, "Costs_HadSAD", Costs_HadSAD);
      Dict_addFloat(PoDict, "Costs_Final", Costs_Final);
      Dict_addFloat(PoDict, "uiCostTemp", uiCostTemp);
      Dict_addFloat(PoDict, "uiBitsTemp", uiBitsTemp);
    }
  }

  void create_BITS(bool isFinalEncode, TComDataCU* pcCU, float Bits_SkipFlag, float Bits_SkipMergeIndex, float Bits_PredMode, float Bits_PartSize,
      float Bits_IntraIPCM, float Bits_PredInfoAllPUs, float Bits_Coeffs)
  {
    if (Detail >= eALL)
    {
      int test = pcCU->getAddr();
      PyObject *CTU = getCTU(pcCU->getAddr());
      string IDStr;
      if (isFinalEncode)
        IDStr = "CUs_FINAL";
      else
        IDStr = "CUs_POST";
      PyObject *poCU  = getLastCU(IDStr, CTU);
      PyObject *poBITS = pyf_create_CU_BITS(Bits_SkipFlag, Bits_SkipMergeIndex, Bits_PredMode, Bits_PartSize, Bits_IntraIPCM, Bits_PredInfoAllPUs, Bits_Coeffs);
      Dict_addObject(poCU, "Bits", poBITS);
    }
  }


};





inline PyObject* pyf_get_Arguments_Dict_1(PyObject *CurPOC, PyObject *CurCTU, PyObject *CurPU, sCU CU, sPU PU, int iRefIdxTemp, TComMv& _cMVTemp)
{
  PyObject* PoArgsDict = PyDict_New();
  PyObject* po_PUPosX = PyInt_FromLong(PU.Pos.x);
  PyObject* po_PUPosY = PyInt_FromLong(PU.Pos.y);
  PyObject* po_PUSizeX = PyInt_FromLong(PU.Size.x);
  PyObject* po_PUSizeY = PyInt_FromLong(PU.Size.y);
  PyObject* po_RefIdx = PyInt_FromLong(iRefIdxTemp);
  PyObject* po_MV = pyf_getTuple(_cMVTemp.getHor(), _cMVTemp.getVer()); //MV xy

  PyDict_SetItemString(PoArgsDict, "POC", CurPOC);
  PyDict_SetItemString(PoArgsDict, "RefIdx", po_RefIdx);
  PyDict_SetItemString(PoArgsDict, "PU_PosX", po_PUPosX);
  PyDict_SetItemString(PoArgsDict, "PU_PosY", po_PUPosY);
  PyDict_SetItemString(PoArgsDict, "PU_SizeX", po_PUSizeX);
  PyDict_SetItemString(PoArgsDict, "PU_SizeY", po_PUSizeY);
  PyDict_SetItemString(PoArgsDict, "PU",  CurPU);
  PyDict_SetItemString(PoArgsDict, "CTU",  CurCTU);
  PyDict_SetItemString(PoArgsDict, "MV", po_MV);

  //PyObject* Po_H_GT_gg = Global.Converter->toNDArray(_cMVTemp.get_H_GT_gg());
  //PyDict_SetItemString(PoArgsDict, "H_GT_gg", Po_H_GT_gg);

  //only decref those we created above!
  //PoArgsDict is handled by caller!!
  Py_DECREF(po_PUPosX);
  Py_DECREF(po_PUPosY);
  Py_DECREF(po_PUSizeX);
  Py_DECREF(po_PUSizeY);
  Py_DECREF(po_RefIdx);
  Py_DECREF(po_MV);

  //DONT!!
//  Py_DECREF(CurPOC);
//  Py_DECREF(CurPU);
//  Py_DECREF(CurCTU);

  return PoArgsDict;
}

