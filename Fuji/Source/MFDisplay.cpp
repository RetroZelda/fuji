#include "Fuji_Internal.h"
#include "MFDisplay_Internal.h"
#include "DebugMenu.h"
#include "MFView.h"
#include "MFSystem.h"

MFDisplaySettings gDisplay;
extern MFInitParams gInitParams;

bool gAppHasFocus = true;

MFInitStatus MFDisplay_InitModule(int moduleId, bool bPerformInitialisation)
{
	MFCALLSTACK;

	int error;

	DebugMenu_AddMenu("Display Options", "Fuji Options");

	// create the display
	if(gInitParams.display.displayRect.width == 0 || gInitParams.display.displayRect.height == 0)
		MFDisplay_GetDefaultRes(&gInitParams.display.displayRect);

	error = MFDisplay_CreateDisplay((int)gInitParams.display.displayRect.width, (int)gInitParams.display.displayRect.height, 32, 60, true, false, false, false);
	if(error)
		return MFIS_Failed;
	return MFIS_Succeeded;
}

void MFDisplay_DeinitModule()
{
	MFCALLSTACK;

	MFDisplay_DestroyDisplay();
}

MF_API void MFDisplay_GetDisplayRect(MFRect *pRect)
{
	pRect->x = 0.0f;
	pRect->y = 0.0f;
	if(gDisplay.windowed)
	{
		pRect->width = (float)gDisplay.width;
		pRect->height = (float)gDisplay.height;
	}
	else
	{
		pRect->width = (float)gDisplay.fullscreenWidth;
		pRect->height = (float)gDisplay.fullscreenHeight;
	}
}

MF_API bool MFDisplay_HasFocus()
{
	return gAppHasFocus;
}
