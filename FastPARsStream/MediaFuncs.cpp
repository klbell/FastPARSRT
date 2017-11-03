#include "MediaFuncs.h"

// Settings

float satLevel = 0.5; // percetn of max where signal saturation is forced
bool oldAverage = true;


// Other Variables
sf::RenderWindow *window;
int captureCount = 0;
int captureSize;

// Fast Variables
int16 *xdata, *ydata;
float halfX, halfY, rangeX, rangeY, rangeSig;
int16 * peakRawData;
int16* actualData;

uInt8 colormap[256][3];

const int imageWidth = 600;
const int imageHeight = 600;

int interpLevel = 2;

const int interpHeight = 300; // Must be wdith / interplvl
const int interpWidth = 300;

int testgrid[interpHeight][interpWidth];
int rendergrid[interpHeight][interpWidth];
int testgridCount[interpHeight][interpWidth];

int *triggerdata, *logicDataloc;
BOOL *logicData;

int maxX = 0, maxY = 0, maxSig = 0, minX = 0, minY = 0, minSig = 0;



void OpenRTWindow()
{
	window = new sf::RenderWindow(sf::VideoMode(imageWidth, imageHeight), "IS Scope RT");
}

void initializeWindowVars(bool MT)
{
	// Get segment count
	//uInt32 segmentCount = getSegmentCount();
	captureSize = getCaptureSize();

	triggerdata = new int[captureSize];
	logicData = new BOOL[captureSize];
	logicDataloc = new int[captureSize];
	xdata = new int16[captureSize];
	ydata = new int16[captureSize];	
	peakRawData = new int16[captureSize];
	
	// Load colormap
	loadColorMap();
}

void minMaxExtractFast(void*  pWorkBuffer, uInt32 u32TransferSize)
{
	// Convert in data
	actualData = (int16*)pWorkBuffer;
	int totalLength = u32TransferSize * 4;

	// Extract trigger signal
	int16 tempmaxdata = 0;
	size_t sizetrue = 0;

	
	BOOL bthreshold, bderivative, blogictemp;

	for (int n = 0, m = 0; n < u32TransferSize; n++, m++)
	{
		triggerdata[m] = actualData[4*n+3];
		if (triggerdata[m] > tempmaxdata)
		{
			tempmaxdata = triggerdata[m];
		}
	}

	// Find values above threshold
	int thresVal = 0.3*tempmaxdata; //watch out
	logicData[0] = 0;
	for (int n = 1; n < u32TransferSize; n++)
	{
		bthreshold = triggerdata[n] > thresVal;
		bderivative = (triggerdata[n] - triggerdata[n-1]) > 0;

		blogictemp = bthreshold*bderivative;

		logicData[n] = blogictemp;

		if (blogictemp != 0)
		{
			sizetrue++;
		}
	}
	


	// Find values where logicData is true, and record locations
	for (int n = 0, m = 0; n < u32TransferSize; n++)
	{
		if (logicData[n] != 0)
		{
			logicDataloc[m] = n;
			m++;
		}
	}

	// This is where we check for doubles


	// Extract mirror data
	int tempLoc, tempLocEnd;
	int16 xMax = 0, yMax = 0, xMin = 0, yMin = 0, tempX, tempY;	
	

	for (int n = 0; n < sizetrue; n++)
	{
		tempLoc = logicDataloc[n];
		tempX = actualData[4*tempLoc+1];
		tempY = actualData[4*tempLoc+2];
		xdata[n] = tempX;
		ydata[n] = tempY;

		if (tempX > maxX)
		{
			maxX = tempX;
		}
		if (tempY > maxY)
		{
			maxY = tempY;
		}
		if (tempX < minX)
		{
			minX = tempX;
		}
		if (tempY < minY)
		{
			minY = tempY;
		}
	}
	rangeX = maxX - minX;
	rangeY = maxY - minY;
	halfX = (maxX - minX) / 2;
	halfY = (maxY - minY) / 2;

	//This is where we would clean and apply our corrections

	int16 minVal, maxVal, tempVal;

	// Extract signal data
	for (int n = 0; n < sizetrue - 1; n++)
	{
		minVal = 0;
		maxVal = 0;
		tempLoc = logicDataloc[n];
		tempLocEnd = logicDataloc[n + 1] - 1;
		for (int m = tempLoc; m < tempLocEnd; m++)
		{
			tempVal = actualData[4*m];
			if (tempVal > maxVal)
			{
				maxVal = tempVal;
			}
			if (tempVal < minVal)
			{
				minVal = tempVal;
			}
		}
		peakRawData[n] = maxVal - minVal;
	}
	peakRawData[sizetrue - 1] = 0; //For now - fix later?

		
	minVal = 0;
	maxVal = 0;

	for (int n = 0; n < sizetrue; n++)
	{
		tempVal = peakRawData[n];
		if (tempVal > maxVal)
		{
			maxVal = tempVal;
		}
		if (tempVal < minVal)
		{
			minVal = tempVal;
		}
	}
	rangeSig = maxVal - minVal;
	minSig = minVal;
	captureCount = sizetrue;
}

int updateScopeWindowFast()
{
	sf::Image scopeImage;
	sf::Texture scopeTexture;
	sf::Sprite background;
	sf::Color color;

	int xLoc, yLoc;
	int intensity;

	scopeImage.create(imageWidth, imageHeight, sf::Color::Black);

	// plot points on test grid
	for (int n = 0; n < captureCount; n++)
	{
		// Determine draw location
		xLoc = ((float)(xdata[n] - minX)*0.98 / rangeX) * interpWidth;
		yLoc = ((float)(ydata[n] - minY)*0.98 / rangeY) * interpHeight;

		// Determine pixel intensity
		intensity = (((float)(peakRawData[n] - minSig) / rangeSig)) * 255; //removed 1.5 scaling factor
		// intensity = min(240, intensity); - settings a threshold value

		testgrid[yLoc][xLoc] += intensity; //potential speed up
		testgridCount[yLoc][xLoc]++;
	}

	
	// Average grid pixels which have multiple occurrances
	for (int i = 0; i < interpHeight; i++)
	{
		for (int j = 0; j < interpWidth; j++)
		{
			if (testgridCount[i][j] > 1)
			{
				testgrid[i][j] = testgrid[i][j] / testgridCount[i][j];
			}
		}
	}

	
	int sUp, sDown, sRight, sLeft; // for fancy averaging
	float wUp, wDown, wLeft, wRight, wDen;
	float wMult = 2;

	if (oldAverage)
	{
		// Do basic spactial averaging and draw
		for (int i = 1; i < interpHeight - 1; i++)
		{
			for (int j = 1; j < interpWidth - 1; j++)
			{
				// spatial averaging
				if (testgridCount[i][j] == 0)
				{
					testgrid[i][j] = (testgrid[i - 1][j] + testgrid[i + 1][j] + testgrid[i][j - 1] + testgrid[i][j + 1]) / 4;
					for (int avg = 0; avg < 2; avg++)
					{
						testgrid[i][j] = (2 * testgrid[i][j] + testgrid[i - 1][j] + testgrid[i + 1][j] +
							testgrid[i][j - 1] + testgrid[i][j + 1]) / 6;
					}
				}


				intensity = testgrid[i][j];

				// Draw
				color.r = colormap[intensity][0];
				color.g = colormap[intensity][1];
				color.b = colormap[intensity][2];

				for (int a = i*interpLevel; a <= (i + 1)*interpLevel; a++) //most efficent way?
				{
					for (int b = j*interpLevel; b <= (j + 1)*interpLevel; b++)
					{
						scopeImage.setPixel(a, b, color);
					}
				}
			}
		}
	}
	

	sf::IntRect r1(0, 0, imageWidth, imageHeight);
	scopeTexture.loadFromImage(scopeImage, r1);

	// Draw stuff
	window->clear();
	background.setTexture(scopeTexture);
	window->draw(background);
	window->display();

	resetWindowVars();

	// Check if clossing
	if (checkWindowCommands())
	{
		delete window;
		delete[] triggerdata, logicData, logicDataloc, actualData, xdata, ydata, peakRawData;
		return 1;

	}
	else
	{
		return 0;
	}
}

int makeImageRealtime(void* pWorkBuffer)
{
	int16* actualData = (int16 *)pWorkBuffer;
	
	
	/*
	sf::CircleShape shape(100.f);
	shape.setFillColor(sf::Color::Green);

	
	sf::Event event;
	while (window->pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
			window->close();
	}
	
	window->clear();
	window->draw(shape);
	window->display();
	*/


	return 0;
}

void resetWindowVars()
{
	// Reset counter
	captureCount = 0; 

	// Zero test grid
	for (int i = 0; i < interpHeight; i++)
	{
		for (int j = 0; j < interpWidth; j++)
		{
			testgrid[i][j] = 0;
			testgridCount[i][j] = 0;
		}
	}	
}

int checkWindowCommands()
{
	sf::Event event;
	while (window->pollEvent(event))
	{
		// "close requested" event: we close the window
		if (event.type == sf::Event::Closed)
		{
			window->close();
			return 1;
		}			
	}
	return 0;
}



int16 hilbert(int16* peakRawData, uInt32 u32TransferSize)
{
		
	fftw_complex* in, *out;
	int n = (int)u32TransferSize;
	in = new fftw_complex[u32TransferSize];
	out = new fftw_complex[u32TransferSize];	

	for (int n = 0; n < u32TransferSize; n++)
	{
		in[n][0] = peakRawData[n];
		in[n][1] = 0;
	}

		
	fftw_plan plan_forward;	
	
	plan_forward = fftw_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

	for (int n = 0; n < u32TransferSize; n++)
	{
		_ftprintf(stdout, "%f\n", in[n][0]);
	}

	fftw_execute(plan_forward);
	
	return 0;
}

void loadColorMap()
{
	uInt8 colormapRead[257][3];
	FILE * colorFile;
	int inColorCount;
	int inInd;

	// open file
	colorFile = fopen("ColormapHot.txt", "r");	
	if (!colorFile)
	{
		_ftprintf(stdout,"Failled to load colormap from file");
		getchar();
	}

	// read in values
	int n = 0;

	for (int n = 0; n < 500; n++)
	{
		int checkScan = fscanf(colorFile, "%d %d %d", &colormapRead[n][0], &colormapRead[n][1], &colormapRead[n][2]);	
		if (checkScan == -1) // End of file
		{
			inColorCount = n-1;
			break;
		}
	}

	// Close file
	fclose(colorFile);	
	colorFile = NULL;

	
	float readMul = 256.0/(float)inColorCount;

	for (int m = 0; m < 256; m++)
	{
		inInd = (int)(m/readMul);
		colormap[m][0] = colormapRead[inInd][0];
		colormap[m][1] = colormapRead[inInd][1];
		colormap[m][2] = colormapRead[inInd][2];
	}
	
}
