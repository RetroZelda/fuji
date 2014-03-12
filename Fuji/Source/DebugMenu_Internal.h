#if !defined(_DEBUGMENU_INTERNAL_H)
#define _DEBUGMENU_INTERNAL_H

#define MENU_FONT_HEIGHT 20.0f

#if !defined(_PSP)
	#define MENU_X		100.0f
	#define MENU_Y		100.0f
	#define MENU_WIDTH	640.0f-MENU_X*2.0f
	#define MENU_HEIGHT	480.0f-MENU_Y*2.0f
#else
	#define MENU_X		50.0f
	#define MENU_Y		20.0f
	#define MENU_WIDTH	480.0f-MENU_X*2.0f
	#define MENU_HEIGHT	272.0f-MENU_Y*2.0f
#endif

#include "DebugMenu.h"

// internal functions
MFInitStatus DebugMenu_InitModule(int moduleId, bool bPerformInitialisation);
void DebugMenu_DeinitModule();

void DebugMenu_Update();
void DebugMenu_Draw();

// the root menu
extern Menu rootMenu;
extern MenuObject *pCurrentMenu;

#endif
