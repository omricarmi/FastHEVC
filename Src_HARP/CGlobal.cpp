// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#include "CGlobal.h"
#include "PyHelper.h"

CGlobal::CGlobal()
{
  //---------------------------------------------
  // DEFAULT VALUES
  //---------------------------------------------

  //you can change this, but note that only HDTs will be redirected... SinglePics, rec.yuv and str.bin will remain in current directory
  this->TmpDir = "./";  //make sure it ends with "/"

  this->CurrentCTU = -1;  //encoder continually places current CTU index here
  this->CurrentPOC = 0;  //encoder continually places current POC index here
  this->initDone = false;

  initTmpDir();
}

CGlobal::~CGlobal()
{
  pyf_finalize();
}

//IMPORTANT NOTE:: init() MUST be called INSTANTLY after
//all cmd line options have been parsed
void CGlobal::init_general()
{
  if(initDone)
    THROW_ERROR("CGlobal: multiple initialization attempted");
  initDone = true;
}

void CGlobal::init_python()
{
  pyf_initialize();
  Converter = new NDArrayConverter;
}


//Initializing Tmp directory
void CGlobal::initTmpDir()
{
  static bool FirstCall = true;
  if (FirstCall)
  {
    //-
    //    		if(this->HARP_CleanTmpDir)
    //    		{
    //    			system((string ("rm -rf ") + this->TmpDir).c_str());
    //    			cout << "HARP TmpDir cleaned\n";
    //    		}
    //-

    system((string ("mkdir -p ") + this->TmpDir).c_str());
    FirstCall = false;
  }
}

void CGlobal::printVersion()
{
  printf("===================== HARP =========================\n");
  printf("================= Version %s =====================\n", HARP_VERSION);
  printf("====================================================\n");
#ifdef ACTIVATE_NEXT
  cout << "WARNING: NEXT development branch is active!" << endl << endl;
#endif
}

void CGlobal::setCurrentPOC(int POC)
{
  this->CurrentPOC = POC;
}

int CGlobal::getCurrentPOC()
{
  return this->CurrentPOC;
}

void CGlobal::setCurrentCTU(int CTU)
{
  QCoreApplication::processEvents();
  this->CurrentCTU = CTU;
}

int CGlobal::getCurrentCTU()
{
  return this->CurrentCTU;
}

bool CGlobal::isObsCTU()
{

  //reduces performance by 15%
  //maybe there is a more efficient (native?) way to do this?
  if (pyf_is_HDT_POC(CurrentPOC) and pyf_is_VisRDO_CTU(CurrentCTU))
    return true;
  else return false;
}

void CGlobal::exportImage(Mat Image, string nickname, bool withPOCIdx, bool withCTUIdx, bool exportToSinglePics)
{
  char POCStr[500] = "";
  char CTUStr[500] = "";
  if(withPOCIdx)
    sprintf(POCStr, "POC_%05d_", this->getCurrentPOC());
  if(withCTUIdx)
    sprintf(CTUStr, "_CTU%04d", this->getCurrentCTU());

  string FN;
  if (exportToSinglePics)
    FN = this->TmpDir + "SinglePics/" + POCStr + nickname + CTUStr + IMAGE_FORMAT;
  else
    FN = this->TmpDir + POCStr + nickname + CTUStr + IMAGE_FORMAT;

  //format given by provided filename!
  int Quality = JPG_QUALITY; //100 = best
  vector<int> params;
  params.push_back(CV_IMWRITE_JPEG_QUALITY);
  params.push_back(Quality);

  //INVERTING
  if (loadINI("Visualization", "Invert") == true)
    Image = invertImage(Image);

  imwrite(FN.c_str(), Image, params);
  cout << "Exported: " << FN << endl;
}
