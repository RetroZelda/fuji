#include "Fuji_Internal.h"

#if MF_SYSTEM == MF_DRIVER_NULL

#include <time.h>

void MFSystem_InitModulePlatformSpecific()
{
	gpEngineInstance->currentPlatform = FP_Unknown;
}

void MFSystem_DeinitModulePlatformSpecific()
{
}

void MFSystem_HandleEventsPlatformSpecific()
{
}

void MFSystem_UpdatePlatformSpecific()
{
}

void MFSystem_DrawPlatformSpecific()
{
}

MF_API uint64 MFSystem_ReadRTC()
{
	return (uint64)clock();
}

MF_API uint64 MFSystem_GetRTCFrequency()
{
	return CLOCKS_PER_SEC;
}

MF_API const char * MFSystem_GetSystemName()
{
	return "null";
}

#endif
