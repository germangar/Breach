
#include "gui_local.h"

ui_import_t UI_IMPORT;

ui_export_t *GetUIAPI( ui_import_t *import )
{
	static ui_export_t globals;

	UI_IMPORT = *import;

	globals.API = UI_API;

	globals.Init = UI_Init;
	globals.Shutdown = UI_Shutdown;

	globals.Refresh = UI_Refresh;
	globals.DrawConnectScreen = UI_DrawConnectScreen;

	globals.KeyEvent = UI_KeyEvent;
	globals.CharEvent = UI_CharEvent;
	globals.MouseMove = UI_MouseMove;

	globals.InitWindow = UI_InitWindow;
	globals.OpenWindow = UI_OpenWindow;
	globals.CloseWindow = UI_CloseWindow;
	globals.ApplyWindowChanges = UI_ApplyWindowChanges;
	globals.InitItemStatic = UI_InitItemStatic;
	globals.InitItemButton = UI_InitItemButton;
	globals.InitItemCheckBox = UI_InitItemCheckBox;
	globals.InitItemSpinner = UI_InitItemSpinner;
	globals.InitItemSlider = UI_InitItemSlider;
	globals.InitItemWindowDragger = UI_InitItemWindowDragger;
	globals.InitItemListbox = UI_InitItemListbox;

	globals.AddToServerList = UI_AddToServerList;

	return &globals;
}

#if defined ( HAVE_DLLMAIN ) && !defined ( UI_HARD_LINKED )
int _stdcall DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
