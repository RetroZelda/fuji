#include "Fuji.h"
#include "Display_Internal.h"
#include "DebugMenu.h"
#include "MFView.h"
#include "MFSystem.h"

MFDisplaySettings gDisplay;

MFVector gClearColour = MakeVector(0.f,0.f,0.22f,1.f);

const float MFAspect_1x1 = 1.0f/1.0f;
const float MFAspect_4x3 = 4.0f/3.0f;
const float MFAspect_16x9 = 16.0f/9.0f;
const float MFAspect_16x10 = 16.0f/10.0f;

void MFDisplay_InitModule()
{
	MFCALLSTACK;

	int error;

	DebugMenu_AddMenu("Display Options", "Fuji Options");

	// create the display
	error = MFDisplay_CreateDisplay(gDefaults.display.displayWidth, gDefaults.display.displayHeight, 32, 60, true, false, false, false);
	if(error) return;
}

void MFDisplay_DeinitModule()
{
	MFCALLSTACK;

	MFDisplay_DestroyDisplay();
}

void MFDisplay_GetDisplayRect(MFRect *pRect)
{
	pRect->x = 0.0f;
	pRect->y = 0.0f;
	pRect->width = (float)gDisplay.width;
	pRect->height = (float)gDisplay.height;
}
