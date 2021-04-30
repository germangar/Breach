/*
   Copyright (C) 2002-2003 Victor Luchits

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 */

#include "client.h"
#include "../gui/ui_public.h"

// Structure containing functions exported from user interface DLL
ui_export_t *uie;

EXTERN_API_FUNC void *GetUIAPI( void * );

mempool_t *ui_mempool;
static void *module_handle;

/*
   ===============
   CL_UIModule_Print
   ===============
 */
static void CL_UIModule_Print( const char *msg )
{
	Com_Printf( "%s", msg );
}

/*
   ===============
   CL_UIModule_Error
   ===============
 */
static void CL_UIModule_Error( const char *msg )
{
	Com_Error( ERR_FATAL, "%s", msg );
}

//===============
//CL_UIModule_GetConfigString
//===============
static const char *CL_UIModule_GetConfigString( int i )
{
	if( i < 0 || i >= MAX_CONFIGSTRINGS )
	{
		Com_DPrintf( S_COLOR_RED "CL_UIModule_GetConfigString: i > MAX_CONFIGSTRINGS" );
		return NULL;
	}
	return cl.configstrings[i];
}

/*
===============
CL_UIModule_MemAlloc
===============
*/
static void *CL_UIModule_MemAlloc(size_t size, const char *filename, int fileline )
{
	return _Mem_Alloc( ui_mempool, size, MEMPOOL_USERINTERFACE, 0, filename, fileline );
}

/*
===============
CL_UIModule_MemFree
===============
*/
static void CL_UIModule_MemFree( void *data, const char *filename, int fileline )
{
	_Mem_Free( data, MEMPOOL_USERINTERFACE, 0, filename, fileline );
}

/*
   ==============
   CL_UIModule_Init
   ==============
 */
void CL_UIModule_Init( void )
{
	int apiversion;
	ui_import_t import;
	void *( *builtinAPIfunc )(void *) = NULL;

#ifdef UI_HARD_LINKED
	builtinAPIfunc = GetUIAPI;
#endif

	CL_UIModule_Shutdown();

	ui_mempool = _Mem_AllocPool( NULL, "User Iterface", MEMPOOL_USERINTERFACE, __FILE__, __LINE__ );

	import.Error = CL_UIModule_Error;
	import.Print = CL_UIModule_Print;

	import.Cvar_Get = Cvar_Get;
	import.Cvar_Set = Cvar_Set;
	import.Cvar_SetValue = Cvar_SetValue;
	import.Cvar_ForceSet = Cvar_ForceSet;
	import.Cvar_String = Cvar_String;
	import.Cvar_Value = Cvar_Value;

	import.Cmd_Argc = Cmd_Argc;
	import.Cmd_Argv = Cmd_Argv;
	import.Cmd_Args = Cmd_Args;

	import.Cmd_AddCommand = Cmd_AddCommand;
	import.Cmd_RemoveCommand = Cmd_RemoveCommand;
	import.Cmd_ExecuteText = Cbuf_ExecuteText;
	import.Cmd_Execute = Cbuf_Execute;

	import.FS_FOpenFile = FS_FOpenFile;
	import.FS_Read = FS_Read;
	import.FS_Write = FS_Write;
	import.FS_Print = FS_Print;
	import.FS_Tell = FS_Tell;
	import.FS_Seek = FS_Seek;
	import.FS_Eof = FS_Eof;
	import.FS_Flush = FS_Flush;
	import.FS_FCloseFile = FS_FCloseFile;
	import.FS_GetFileList = FS_GetFileList;
	import.FS_GetGameDirectoryList = FS_GetGameDirectoryList;
	import.FS_FirstExtension = FS_FirstExtension;

	import.CL_SetKeyDest = CL_SetKeyDest;
	import.CL_ResetServerCount = CL_ResetServerCount;
	import.CL_GetClipboardData = CL_GetClipboardData;
	import.CL_FreeClipboardData = CL_FreeClipboardData;

	import.Key_ClearStates = Key_ClearStates;
	import.Key_GetBindingBuf = Key_GetBindingBuf;
	import.Key_KeynumToString = Key_KeynumToString;
	import.Key_SetBinding = Key_SetBinding;
	import.Key_IsDown = Key_IsDown;

	import.R_ClearScene = R_ClearScene;
	import.R_AddEntityToScene = R_AddEntityToScene;
	import.R_AddLightToScene = R_AddLightToScene;
	import.R_AddPolyToScene = R_AddPolyToScene;
	import.R_RenderScene = R_RenderScene;
	import.R_EndFrame = R_EndFrame;
	import.R_ModelBounds = R_ModelBounds;
	import.R_RegisterModel = R_RegisterModel;
	import.R_RegisterPic = R_RegisterPic;
	import.R_RegisterSkin = R_RegisterSkin;
	import.R_RegisterSkinFile = R_RegisterSkinFile;
	import.R_LerpTag = R_LerpTag;
	import.R_DrawStretchPic = R_DrawStretchPic;
	import.R_TransformVectorToScreen = R_TransformVectorToScreen;
	import.R_SkeletalGetNumBones = R_SkeletalGetNumBones;
	import.R_SkeletalGetBoneInfo = R_SkeletalGetBoneInfo;
	import.R_SkeletalGetBonePose = R_SkeletalGetBonePose;

	import.CM_LoadMapMessage = CM_LoadMapMessage;

	import.S_RegisterSound = CL_SoundModule_RegisterSound;
	import.S_StartLocalSound = CL_SoundModule_StartLocalSound;
	import.S_StartBackgroundTrack = CL_SoundModule_StartBackgroundTrack;
	import.S_StopBackgroundTrack = CL_SoundModule_StopBackgroundTrack;

	import.SCR_RegisterFont = SCR_RegisterFont;
	import.SCR_DrawString = SCR_DrawString;
	import.SCR_DrawStringWidth = SCR_DrawStringWidth;
	import.SCR_DrawClampString = SCR_DrawClampString;
	import.SCR_strHeight = SCR_strHeight;
	import.SCR_strWidth = SCR_strWidth;
	import.SCR_StrlenForWidth = SCR_StrlenForWidth;

	import.GetConfigString = CL_UIModule_GetConfigString;

	import.VID_GetModeInfo = VID_GetModeInfo;

	import.Milliseconds = Sys_Milliseconds;

	import.Mem_Alloc = CL_UIModule_MemAlloc;
	import.Mem_Free = CL_UIModule_MemFree;

	uie = (ui_export_t *)Com_LoadGameLibrary( "ui", "GetUIAPI", &module_handle, &import, builtinAPIfunc, cls.sv_pure, NULL );
	if( !uie )
		Com_Error( ERR_DROP, "Failed to load UI dll" );

	apiversion = uie->API();
	if( apiversion != UI_API_VERSION )
	{
		Com_UnloadGameLibrary( &module_handle );
		Mem_FreePool( &ui_mempool );
		uie = NULL;
		Com_Error( ERR_FATAL, "UI version is %i, not %i", apiversion, UI_API_VERSION );
	}

	uie->Init( viddef.width, viddef.height, APP_PROTOCOL_VERSION );
}

/*
   ===============
   CL_UIModule_Shutdown
   ===============
 */
void CL_UIModule_Shutdown( void )
{
	if( !uie )
		return;

	uie->Shutdown();
	Com_UnloadGameLibrary( &module_handle );
	Mem_FreePool( &ui_mempool );
	uie = NULL;
}

/*
   ===============
   CL_UIModule_Refresh
   ===============
 */
void CL_UIModule_Refresh( qboolean backGround )
{
	if( uie )
		uie->Refresh( cls.realtime, Com_ClientState(), Com_ServerState(), cls.key_dest, cls.demo.playing, backGround );
}

/*
   ===============
   CL_UIModule_DrawConnectScreen
   ===============
 */
void CL_UIModule_DrawConnectScreen( void )
{
	if( uie )
	{
		int downloadType, downloadSpeed;
		
		if( cls.download.web )
			downloadType = 2;
		else if( cls.download.filenum )
			downloadType = 1;
		else
			downloadType = 0;

		if( downloadType )
		{
			size_t downloadedSize;
			unsigned int downloadTime;

			downloadedSize = (size_t)( cls.download.size * cls.download.percent ) - cls.download.baseoffset;
			downloadTime = Sys_Milliseconds() - cls.download.timestart;

			downloadSpeed = downloadTime ? ( downloadedSize / 1024.0f ) / ( downloadTime * 0.001f ) : 0;	
		}
		else
		{
			downloadSpeed = 0;
		}

		uie->DrawConnectScreen( cls.servername, cls.rejected ? cls.rejectmessage : NULL,
			downloadType, cls.download.name, cls.download.percent * 100.0f, downloadSpeed,
			cls.connect_count, cls.demo.playing );
	}
}

/*
   ===============
   CL_UIModule_Keydown
   ===============
 */
void CL_UIModule_KeyEvent( int key, qboolean down )
{
	if( uie )
		uie->KeyEvent( key, down );
}

/*
   ===============
   CL_UIModule_CharEvent
   ===============
 */
void CL_UIModule_CharEvent( int key )
{
	if( uie )
		uie->CharEvent( key );
}

/*
   ===============
   CL_UIModule_MouseMove
   ===============
 */
void CL_UIModule_MouseMove( int dx, int dy )
{
	if( uie )
		uie->MouseMove( dx, dy );
}

/*
   ===============
   CL_UIModule_DeactivateUI
   ===============
 */
void CL_UIModule_DeactivateUI( void )
{
	CL_UI_CloseWindow( "all" );
}

/*
   ===============
   CL_UIModule_AddToServerList
   ===============
 */
void CL_UIModule_AddToServerList( char *adr, char *info )
{
	if( uie )
		uie->AddToServerList( adr, info );
}


/*
CL_UI_InitWindow
*/
void CL_UI_InitWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic )
{
	if( uie )
		uie->InitWindow( name, x, y, w, h, blockdraw, blockfocus, custompic );
}

/*
CL_UI_OpenWindow
*/
void CL_UI_OpenWindow( const char *name )
{
	if( uie )
		uie->OpenWindow( name );
}

/*
CL_UI_CloseWindow
*/
void CL_UI_CloseWindow( const char *name )
{
	if( uie )
		uie->CloseWindow( name );
}

void CL_UI_ApplyWindowChanges( const char *name )
{
	if( uie )
		uie->ApplyWindowChanges( name );
}

/*
CL_UI_InitItemWindowDragger
*/
struct menuitem_public_s *CL_UI_InitItemWindowDragger( const char *windowname, const char *itemname, int x, int y, int align )
{
	if( uie )
		return uie->InitItemWindowDragger( windowname, itemname, x, y, align );
	return NULL;
}

/*
CL_UI_InitItemStatic
*/
struct menuitem_public_s *CL_UI_InitItemStatic( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font )
{
	if( uie )
		return uie->InitItemStatic( windowname, itemname, x, y, align, tittle, font );
	return NULL;
}

/*
CL_UI_InitItemButton
*/
struct menuitem_public_s *CL_UI_InitItemButton( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString )
{
	if( uie )
		return uie->InitItemButton( windowname, itemname, x, y, align, tittle, font, Action, targetString );
	return NULL;
}

/*
CL_UI_InitItemCheckBox
*/
struct menuitem_public_s *CL_UI_InitItemCheckBox( const char *windowname, const char *itemname, int x, int y, int align, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	if( uie )
		return uie->InitItemCheckBox( windowname, itemname, x, y, align, Update, Apply, alwaysApply, targetString );
	return NULL;
}

/*
CL_UI_InitItemSpinner
*/
struct menuitem_public_s *CL_UI_InitItemSpinner( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *spinnerNames, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	if( uie )
		return uie->InitItemSpinner( windowname, itemname, x, y, align, font, spinnerNames, Update, Apply, alwaysApply, targetString );
	return NULL;
}

/*
CL_UI_InitItemSlider
*/
struct menuitem_public_s *CL_UI_InitItemSlider( const char *windowname, const char *itemname, int x, int y, int align, int min, int max, float stepsize, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	if( uie )
		return uie->InitItemSlider( windowname, itemname, x, y, align, min, max, stepsize, Update, Apply, alwaysApply, targetString );
	return NULL;
}

/*
CL_UI_InitItemListbox
*/
struct menuitem_public_s *CL_UI_InitItemListbox( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *namesList, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, void *Action, char *targetString )
{
	if( uie )
		return uie->InitItemListbox( windowname, itemname, x, y, align, font, namesList, Update, Apply, alwaysApply, Action, targetString );
	return NULL;
}
