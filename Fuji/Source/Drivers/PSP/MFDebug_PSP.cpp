#include "Fuji_Internal.h"

#if MF_DEBUG == MF_DRIVER_PSP

#include "MFInput_Internal.h"

#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>

MF_API void MFDebug_Message(const char *pMessage)
{
	fprintf(stderr, "%s\n", pMessage);
}

MF_API void MFDebug_DebugAssert(const char *pReason, const char *pMessage, const char *pFile, int line)
{
	MFDebug_Message(MFStr("%s(%d) : Assertion Failure.",pFile,line));
	MFDebug_Message(MFStr("Failed Condition: %s\n%s", pReason, pMessage));
	MFCallstack_Log();

	const char *pString;

	pspDebugScreenSetXY(35 - 9, 13);
	pspDebugScreenPrintf("Assertion Failure!");

	pString = MFStr("%s(%d)", pFile, line);
	pspDebugScreenSetXY(35 - (MFString_Length(pString)>>1), 15);
	pspDebugScreenPrintf(pString);

	pString = MFStr("Failed Condition: %s", pReason);
	pspDebugScreenSetXY(35 - (MFString_Length(pString)>>1), 17);
	pspDebugScreenPrintf(pString);

	pspDebugScreenSetXY(35 - (MFString_Length(pMessage)>>1), 18);
	pspDebugScreenPrintf(pMessage);

	while(!MFInput_WasPressed(Button_P2_Start, IDD_Gamepad, 0))
	{
		MFInput_Update();

		if(gpEngineInstance->bQuit || MFInput_WasPressed(Button_P2_Select, IDD_Gamepad, 0))
			sceKernelExitGame();
	}
}

#endif
