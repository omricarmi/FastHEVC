// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <vector>
#include <string>

#include "opencv/cv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"


#include "CGlobal.h"
#include "CShow.h"
#include "HARP_Defines.h"

#ifdef ACTIVATE_CALLGRIND
#include <valgrind/callgrind.h>


using namespace std;
using namespace cv;

inline void CG_startCallGraph()
{
#ifdef CG_CALLGRAPH_INTER_ONLY
  if (Global.CurrentPOC == 0)
    return;
#endif
  cout << "STARTING CALLGRIND INSTRUMENTATION" << endl;
  CALLGRIND_START_INSTRUMENTATION;

}

inline void CG_stopCallGraph()
{
#ifdef CG_CALLGRAPH_INTER_ONLY
  if (Global.CurrentPOC == 0)
    return;
#endif
  cout << "STOPPING CALLGRIND INSTRUMENTATION" << endl;
  CALLGRIND_STOP_INSTRUMENTATION;
  CALLGRIND_DUMP_STATS;
}

#endif
