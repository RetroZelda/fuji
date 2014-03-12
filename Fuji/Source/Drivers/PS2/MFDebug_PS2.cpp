#include "Fuji_Internal.h"

#if MF_DEBUG == MF_DRIVER_PS2

#include "MFInput_Internal.h"

#include <stdio.h>

MF_API void MFDebug_Message(const char *pMessage)
{
	printf("%s\n", pMessage);
}

MF_API void MFDebug_DebugAssert(const char *pReason, const char *pMessage, const char *pFile, int line)
{
	MFDebug_Message(MFStr("%s(%d) : Assertion Failure.",pFile,line));
	MFDebug_Message(MFStr("Failed Condition: %s\n%s", pReason, pMessage));
	MFCallstack_Log();

	// draw some shit on the screen..

	while(!MFInput_WasPressed(Button_P2_Start, IDD_Gamepad, 0))
	{
		MFInput_Update();
	}
}

#endif
