/*
   Copyright (C) 1997-2001 Id Software, Inc.

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

#include "ui_local.h"

#define S_UI_MENU_MUSIC				"sounds/music/ui"

#define UI_SHADER_BACKGROUND		"2d/ui/videoback"
#define UI_SHADER_CURSOR			"2d/ui/cursor"

ui_local_t uis;

//======================================================================

/*
   ============
   UI_API
   ============
 */
int UI_API( void )
{
	return UI_API_VERSION;
}

/*
   ============
   UI_Error
   ============
 */
void UI_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Error( msg );
}

/*
   ============
   UI_Printf
   ============
 */
void UI_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Print( msg );
}

/*
=================
UI_CopyString
=================
*/
char *_UI_CopyString( const char *in, const char *filename, int fileline )
{
	char *out;

	out = trap_MemAlloc( strlen( in )+1, filename, fileline );
	strcpy( out, in );
	return out;
}

//=============================================================================

/*
   =================
   UI_AddToServerList
   =================
 */
void UI_AddToServerList( char *adr, char *info )
{
}

void UI_SetActiveGUI( qboolean active )
{
	if( !active )
	{
		trap_CL_SetKeyDest( key_game );
		trap_Key_ClearStates();
	}
	else
	{
		trap_CL_SetKeyDest( key_menu );
		trap_Key_ClearStates();
	}
}


/*
   =================
   UI_MouseMove
   =================
 */
void UI_MouseMove( int dx, int dy )
{
	uis.cursorX += dx;
	uis.cursorY += dy;

	clamp( uis.cursorX, 0, UI_VIRTUALSCR_WIDTH );
	clamp( uis.cursorY, 0, UI_VIRTUALSCR_HEIGHT );
}


/*
   =================
   UI_KeyEvent
   =================
 */
void UI_KeyEvent( int key, qboolean down )
{
	if( !uis.activeWindows )
		return;

	if( !down )
	{
		if( key != K_MOUSE1 )
			return;
	}

	switch( key )
	{
	case K_ESCAPE:
		if( !uis.backGround ) 
		{
			UI_CloseWindow( "all" );
		}
		break;

	case K_MOUSE1:
		if( uis.itemDrag && (uis.itemFocus != uis.itemDrag) )
		{
			if( !down )
				UI_Item_ClickEvent( uis.itemDrag, down );
		}
		else if( uis.itemFocus )
		{
			UI_Item_ClickEvent( uis.itemFocus, down );
		}
		else if( uis.windowFocus )
		{
			UI_Window_ClickEvent( uis.windowFocus, down );
		}
		break;

	case K_MOUSE2:
		break;

	case K_MWHEELUP:
	case KP_UPARROW:
	case K_UPARROW:
		if( uis.windowFocus )
		{
		}
		break;
	case K_TAB:
		if( uis.windowFocus )
		{
		}
		break;

	case K_MWHEELDOWN:
	case KP_DOWNARROW:
	case K_DOWNARROW:
		if( uis.windowFocus )
		{
		}
		break;
	case KP_LEFTARROW:
	case K_LEFTARROW:
		if( uis.windowFocus )
		{
		}
		break;
	case KP_RIGHTARROW:
	case K_RIGHTARROW:
		if( uis.windowFocus )
		{
		}
		break;

	case K_MOUSE3:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:

	case KP_ENTER:
	case K_ENTER:
		if( uis.windowFocus )
		{
		}
		break;
	}
}

/*
   =================
   UI_CharEvent
   =================
 */
void UI_CharEvent( int key )
{
	if( uis.activeWindows )
	{
	}
}

/*
   =================
   UI_DrawConnectScreen
   =================
 */
void UI_DrawConnectScreen( char *serverName, char *rejectmessage, char *downloadfilename, int connectCount )
{
	qboolean loopbackhost;
	char str[MAX_QPATH];
	const char *mapname, *message;
	qboolean backGround = qfalse;

	trap_S_StopBackgroundTrack();
	mapname = trap_GetConfigString( CS_MAPNAME );

	// note : connect screen background is not enclosed in the virtual window
	if( backGround )
	{   
		trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight, 0, 0, 1, 1, colorBlack, uis.whiteShader );
	}

	// draw server name if not local host
	loopbackhost = !serverName || !serverName[0] || !Q_stricmp( serverName, "loopback" );
	if( !loopbackhost )
	{
		Q_snprintfz( str, sizeof( str ), "Connecting to %s", serverName );
		UI_DrawStringWidth( 16,
			UI_VIRTUALSCR_HEIGHT - ( 8 + trap_SCR_strHeight( uis.fontSystemSmall ) + trap_SCR_strHeight( uis.fontSystemBig ) + trap_SCR_strHeight( uis.fontSystemMedium ) ),
			ALIGN_LEFT_BOTTOM, str, 0, uis.fontSystemBig, colorWhite );
	}

	if( rejectmessage )
	{
		Q_snprintfz( str, sizeof( str ), "Refused: %s", rejectmessage );
		UI_DrawStringWidth( 16, UI_VIRTUALSCR_HEIGHT - ( 8 + trap_SCR_strHeight( uis.fontSystemSmall ) + trap_SCR_strHeight( uis.fontSystemBig ) ),
			ALIGN_LEFT_BOTTOM, str, 0, uis.fontSystemMedium, colorWhite );
	}

	if( downloadfilename )
	{
		Q_snprintfz( str, sizeof( str ), "Downloading %s", downloadfilename );
		UI_DrawStringWidth( 16,
			UI_VIRTUALSCR_HEIGHT - ( 8 + trap_SCR_strHeight( uis.fontSystemSmall ) + trap_SCR_strHeight( uis.fontSystemBig ) ),
			ALIGN_LEFT_BOTTOM, str, 0, uis.fontSystemMedium, colorWhite );
	}

	if( mapname && mapname[0] )
	{
		message = trap_GetConfigString( CS_MESSAGE );
		if( message && message[0] )
			UI_DrawStringWidth( 16, UI_VIRTUALSCR_HEIGHT - ( 8 + trap_SCR_strHeight( uis.fontSystemSmall ) ),
				ALIGN_LEFT_BOTTOM, message, 0, uis.fontSystemBig, colorWhite );
	}
}

/*
   =================
   UI_DrawBackground
   =================
 */
void UI_DrawBackground( void )
{
	trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight,
		0, 0, 1, 1, colorWhite, trap_R_RegisterPic( UI_SHADER_BACKGROUND ) );

	UI_DrawModelRealScreen( 0, 0, uis.vidWidth, uis.vidHeight, 30, trap_R_RegisterModel( "models/items/ammo/pack/pack.md3" ), 0, 0, 0, tv( -25, 35, 0 ) );

	if( !uis.backGroundTrackStarted )
	{
		trap_S_StartBackgroundTrack( S_UI_MENU_MUSIC, S_UI_MENU_MUSIC );
		uis.backGroundTrackStarted = qtrue;
	}
}

/*
   =================
   UI_Refresh
   =================
 */
void UI_Refresh( unsigned int time, int clientState, int serverState, int keydest, qboolean backGround )
{
	uis.time = time;
	uis.clientState = clientState;
	uis.serverState = serverState;
	uis.backGround = backGround;
	uis.keydest = keydest;

	if( uis.backGround )
	{
		UI_DrawBackground();
	}
	else
	{
		uis.backGroundTrackStarted = qfalse;
	}

	if( uis.activeWindows )
	{
		UI_SetUpVirtualWindow();

		// Execute the windows tree
		uis.windowFocus = UI_FindWindowsFocus();
		uis.itemFocus = NULL;
		if( uis.windowFocus ) 
		{
			if( UI_IsTopWindow( uis.windowFocus ) || uis.windowFocus->mergeablefocus ) 
			{
				uis.itemFocus = UI_RecurseFindItemFocus( uis.windowFocus->items_headnode );
			}
		}

		if( uis.itemDrag )
		{
			uis.itemDrag->ClickEvent( uis.itemDrag, qtrue );
		}

		UI_DrawActiveWindows( uis.windowFocus );
	}

	// draw cursor
	if( uis.keydest == key_menu )
	{
		UI_DrawStretchPic( uis.cursorX, uis.cursorY, 32, 32, ALIGN_CENTER_MIDDLE,
			0, 0, 1, 1, colorWhite, trap_R_RegisterPic( UI_SHADER_CURSOR ) );
	}
}


static void UI_RegisterFonts( void )
{
	cvar_t *con_fontSystemSmall = trap_Cvar_Get( "con_fontSystemSmall", DEFAULT_FONT_SMALL, CVAR_ARCHIVE );
	cvar_t *con_fontSystemMedium = trap_Cvar_Get( "con_fontSystemMedium", DEFAULT_FONT_MEDIUM, CVAR_ARCHIVE );
	cvar_t *con_fontSystemBig = trap_Cvar_Get( "con_fontSystemBig", DEFAULT_FONT_BIG, CVAR_ARCHIVE );

	uis.fontSystemSmall = trap_SCR_RegisterFont( con_fontSystemSmall->string );
	if( !uis.fontSystemSmall )
	{
		uis.fontSystemSmall = trap_SCR_RegisterFont( DEFAULT_FONT_SMALL );
		if( !uis.fontSystemSmall )
			UI_Error( "Couldn't load default font \"%s\"", DEFAULT_FONT_SMALL );
	}
	uis.fontSystemMedium = trap_SCR_RegisterFont( con_fontSystemMedium->string );
	if( !uis.fontSystemMedium )
		uis.fontSystemMedium = trap_SCR_RegisterFont( DEFAULT_FONT_MEDIUM );

	uis.fontSystemBig = trap_SCR_RegisterFont( con_fontSystemBig->string );
	if( !uis.fontSystemBig )
		uis.fontSystemBig = trap_SCR_RegisterFont( DEFAULT_FONT_BIG );

}

static void UI_Cache( void )
{
	// precache sounds
	//trap_S_RegisterSound( S_UI_MENU_IN_SOUND );

	// precache images
	uis.whiteShader = trap_R_RegisterPic( "2d/white" );
	uis.windowShader = trap_R_RegisterPic( "2d/ui/window_default" );
	uis.itemShader = trap_R_RegisterPic( "2d/ui/item_default" );
	uis.itemFocusShader = trap_R_RegisterPic( "2d/ui/item_default_focus" );

	// checkbox
	uis.itemCheckBoxShader = trap_R_RegisterPic( "2d/ui/checkbox_idle" );
	uis.itemCheckBoxFocusShader = trap_R_RegisterPic( "2d/ui/checkbox_focus" );
	uis.itemCheckBoxSelectedShader = trap_R_RegisterPic( "2d/ui/checkbox_selected" );

	// listbox
	uis.itemListBoxFocusShader = trap_R_RegisterPic( "2d/ui/listbox_focus" );
	uis.itemListBoxSelectedShader = trap_R_RegisterPic( "2d/ui/listbox_selected" );
	// default listbox background
	uis.listbox_up_left = trap_R_RegisterPic( "2d/ui/listbox_up_left" );
	uis.listbox_up_right = trap_R_RegisterPic( "2d/ui/listbox_up_right" );
	uis.listbox_down_left = trap_R_RegisterPic( "2d/ui/listbox_down_left" );
	uis.listbox_down_right = trap_R_RegisterPic( "2d/ui/listbox_down_right" );
	uis.listbox_left = trap_R_RegisterPic( "2d/ui/listbox_left" );
	uis.listbox_right = trap_R_RegisterPic( "2d/ui/listbox_right" );
	uis.listbox_up = trap_R_RegisterPic( "2d/ui/listbox_up" );
	uis.listbox_down = trap_R_RegisterPic( "2d/ui/listbox_down" );
	uis.listbox_back = trap_R_RegisterPic( "2d/ui/listbox_back" );

	// slider
	uis.itemSliderLeft = trap_R_RegisterPic( "2d/ui/slider_left" );
	uis.itemSliderRight = trap_R_RegisterPic( "2d/ui/slider_right" );
	uis.itemSliderBar = trap_R_RegisterPic( "2d/ui/slider_bar" );
	uis.itemSliderHandle = trap_R_RegisterPic( "2d/ui/slider_handler" );

	// scroll bar
	uis.scrollUpShader = trap_R_RegisterPic( "2d/ui/scrollbar_up" );
	uis.scrollDownShader = trap_R_RegisterPic( "2d/ui/scrollbar_down" );
	uis.scrollBarShader = trap_R_RegisterPic( "2d/ui/scrollbar_bar" );
	uis.scrollHandleShader = trap_R_RegisterPic( "2d/ui/scrollbar_handler" );

	trap_R_RegisterPic( UI_SHADER_CURSOR );

	// precache fonts
	UI_RegisterFonts();
}

/*
   =================
   UI_Init
   =================
 */
void UI_Init( int vidWidth, int vidHeight, int protocol )
{
	cvar_t *ui_startmap;
	cvar_t *ui_startdemo;

	memset( &uis, 0, sizeof( uis ) );

	ui_startmap = trap_Cvar_Get( "ui_startmap", UI_ListNameForPosition( UI_RefreshMapListCvar(), 0 ), CVAR_ARCHIVE );
	ui_startdemo = trap_Cvar_Get( "ui_startdemo", UI_ListNameForPosition( UI_RefreshDemoListCvar(), 0 ), CVAR_ARCHIVE ); 

	uis.vidWidth = vidWidth;
	uis.vidHeight = vidHeight;
	uis.protocol_version = protocol;

	uis.cursorX = uis.vidWidth / 2;
	uis.cursorY = uis.vidHeight / 2;

	UI_Cache();
	UI_Playermodel_Init(); // create a list with the available player models
	UI_InitTemporaryBoneposesCache();

	UI_InitWindows();
	
	UI_InitMenus();
}

/*
   =================
   UI_Shutdown
   =================
 */
void UI_Shutdown( void )
{
	trap_S_StopBackgroundTrack();

	UI_FreeWindows();

	trap_Cmd_RemoveCommand( "menu_main" );
}

//======================================================================

#ifndef UI_HARD_LINKED
// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Error( msg );
}

void Com_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Print( msg );
}
#endif
