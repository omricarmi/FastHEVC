// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

// PRIMARY CONTROL =========================================
#define HARP_VERSION "2.00"     //HARP version

//PLEASE NOTE:
//we cleaned up RDO, in order to attach to it with HARP.
//NOTE THAT currently this comes with -0.02dB deficiency compared to original code.
//Will be fixed in v2.01!! v2.01 scheduled for 5th of October 2016

#define ACTIVATE_NEXT_RDO_BITCORRECT //see above, will be fixed ASAP

// EXTENDED FUNCTIONS ===========================================
#define EXTENDED_xEncodeCU   //will log detailed sideinfo bits
// ==========================================================

// JPG/PNG CONTROL =========================================
#define IMAGE_FORMAT ".jpg" //choose between .jpg and .png
#define JPG_QUALITY 95      //name says it all
#define VIS_TEXT_SIZE 600,600   //size of text info image patch
// ==========================================================

// RESTRICTIONS =============================================
// WARNING: do NOT change, or you  will break the code
#define CTU_DIM 64 //VAD, CTU size
#define SU_DIM 4   //VAD, interal storage unit size
#define ACTIVATE_HARP
#define LOGBITS
//#define ACTIVATE_CALLGRIND       //activate callgrind profiling (linux-only)
//#define CG_CALLGRAPH_INTER_ONLY  //skip Intra POCs during Callgrind profiling
// ==========================================================

// TIMER STUFF ==============================================
#define INIT_TIMER struct timeval tp; \
        double sec, usec, start, end, Seconds;
#define START_TIMER \
		    /*cout << "TIMER: starting...\n";*/ \
		    gettimeofday( &tp, NULL ); \
        sec = static_cast<double>( tp.tv_sec ); \
        usec = static_cast<double>( tp.tv_usec )/1E6; \
        start = sec + usec;
#define STOP_TIMER(text) \
		gettimeofday( &tp, NULL ); \
        sec = static_cast<double>( tp.tv_sec ); \
        usec = static_cast<double>( tp.tv_usec )/1E6; \
        end = sec + usec; \
        Seconds = end - start; \
        cout << "TIMER stopped: " << text << " took  "; \
        cout << Seconds << " secs" << endl << flush;
// ==========================================================

// MACROS FOR MEASURING BITS ================================
#ifdef LOGBITS
#define HARP_LOGBITS(Dest, origline) \
		{ \
			UInt BitsBefore = m_pcEntropyCoder->getNumberOfWrittenBits(); \
			origline; \
			Dest = m_pcEntropyCoder->getNumberOfWrittenBits() - BitsBefore; \
		}
#else //deactivate overhead
#define HARP_LOGBITS(Dest, origline) \
		origline;
#endif
// ==========================================================

// VARIOUS ==================================================
//#define FOR(i,length) for(int i=0; i<(int)(length); i++)
#define C_STR toStdString().c_str()  //QString to *char conversion
#define CTUSIZE 64 // KNOW WHAT YOU ARE DOING!
#define THROW_ERROR(error) {std::cout << endl << "ERROR: " << endl << error << endl << "Exiting..." << endl << endl << endl << flush; assert(0); exit(-1);}
#define THROW_ERROR_PYTHON(error, moreinfo) {PyErr_Print(); std::cout << endl << "PYTHON ERROR: " << endl << error << " (" << moreinfo << ")" << endl << "Exiting..." << endl << endl << endl << flush; assert(0); exit(-1);}
#define PY_ASSERT_RETVAL(line) \
		line; \
		if (poRetVal == NULL) THROW_ERROR_PYTHON("Call failed", "");
#define PY_ASSERT(line) \
    assert (line == 0);
// ==========================================================


// BASIC COLOR CODES ========================================
#define WHITE Scalar(255,255,255)
#define GRAY  Scalar(128,128,128)
#define BLACK Scalar(0,0,0)
#define YELLOW Scalar(0,255,255)
#define GREEN Scalar(0,255,0)
#define DARKGREEN Scalar(0,128,0)
#define RED Scalar(0,0,255)
#define BLUE Scalar(255,0,0)
#define GREEN_2 Scalar(9,241,9)
#define DEEPSKYBLUE Scalar(255,191,0)
#define DARKORANGE Scalar(0,140,255)
#define OLIVE Scalar(0x00,0x80,0x80)
#define MEDIUMPURPLE Scalar(0xD8,0x70,0x93)
#define MAGENTA Scalar(255,0,255)
#define BRBA_YELLOW Scalar(13,201,250)
#define BRBA_BLUE Scalar(194,189,44)
#define BRBA_RED Scalar(52,97,220)
#define BRBA_GREEN Scalar(63,168,125)
#define TITHONIAN Scalar(247,241,217)
#define AALENIAN  Scalar(221,217,154)
#define HETTANGIAN Scalar(211,179,78)

#define REFIDX0  Scalar(255.0, 255.0, 0.0)
#define REFIDX1  Scalar(238.0, 130.0, 238.0)
#define REFIDX2  Scalar(30.0, 105.0, 210.0)
#define REFIDX3  Scalar(0.0, 255.0, 255.0)


// ==========================================================

#include "CGlobal.h" //pull in all global stuff wherever HARP_Defines.h is interesting


