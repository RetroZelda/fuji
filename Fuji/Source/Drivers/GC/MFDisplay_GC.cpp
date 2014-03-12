#include "Fuji_Internal.h"

#if MF_DISPLAY == MF_DRIVER_GC

#include "Display_Internal.h"
#include "MFRenderer_Internal.h"

extern MFRect gCurrentViewport;

int MFDisplay_CreateDisplay(int width, int height, int bpp, int rate, bool vsync, bool triplebuffer, bool wide, bool progressive)
{
	gCurrentViewport.x = 0.0f;
	gCurrentViewport.y = 0.0f;
	gCurrentViewport.width = (float)gDisplay.width;
	gCurrentViewport.height = (float)gDisplay.height;

	MFRenderer_CreateDisplay();

	return 0;
}

void MFDisplay_ResetDisplay()
{
	MFRenderer_ResetDisplay();
}

void MFDisplay_DestroyDisplay()
{
	MFRenderer_DestroyDisplay();
}

float MFDisplay_GetNativeAspectRatio()
{
	return 4.f / 3.f;
}

bool MFDisplay_IsWidescreen()
{
	return false;
}

#endif
