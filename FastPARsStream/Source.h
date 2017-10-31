

#ifndef SOURCE_H
#define SOURCE_H


#define _CRTDBG_MAP_ALLOC 
//#include <stdlib.h>  
#include <crtdbg.h>  
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#include <windows.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string>
#include <time.h>
//#include <iostream>
#include "nidaqmxNew.h"
#include "StageMoves.h"
#include "GageFuncs.h"
#include "DAQmxErrors.h"
#include <math.h>
#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"
#include "MiscFuncs.h"
#include "HilbertMath.h"
#include "MediaFuncs.h"




int runPARS();
int runPARSRT();
int runPAM();
int runFree();


#endif