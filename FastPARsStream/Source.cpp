#include "Source.h"

LPCTSTR saveDirectory = _T("C:\\Users\\PARS1\\Desktop\\PARS\\Data\\Oct 19_EOMRTTest\\");

bool useMTTrans = false;

int main()
{
	int scanMode;

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);


	illuWelcome();
	
	// Primary program loop
	while (1)
	{
		// New Scan, get desired scan
		scanMode = getScanMode();
		
		switch (scanMode)
		{
		case 0:					// Exit program
			return 0;
		case 1: runPARS();		// PARS single capture mode
			break;
		case 2: runPARSRT();	// PARS real-time capture mode
			break;
		/*case 3: runPAM();
			break;
		case 4: runFree();
			break;*/
		default: 
			_ftprintf(stdout, "Invalid selection\n\n");
		}
	}
	
	//_CrtDumpMemoryLeaks();
	return 0;
}

int runPARS()
{
	PARSWelcome(saveDirectory);
	initializeGageSingleCap(); // Setup Gage card
	collectData(); // Collect data
	checkScanComplete(); // Check to see when capture is completed
	saveGageData(saveDirectory); // Transfer and save all that data
	releaseGageSingleCap(); // Release Gage card

	return 0;
}

int runPARSRT()
{
	PARSRTWelcome();

			
		initializeGageStream(); // Setup Gage card
		OpenRTWindow(); // Launch view window
		initializeWindowVars(false); // Set ploting variables

		while (!gageStreamRealtime())
		{
			if (updateScopeWindowFast())
			{
				break;
			}
		}

		releaseGageRT(); // Release Gage card		
	

	return 0;
}

int runPAM()
{
	return 0;
}

int runFree()
{
	return 0;
}