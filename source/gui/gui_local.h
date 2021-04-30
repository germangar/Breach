/*
 */

#ifndef __GUI_LOCAL_H__
#define __GUI_LOCAL_H__

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_keycodes.h"

#include "../cgame/ref.h"
#include "..\qcommon\version.h"
#include "ui_ref.h"
#include "ui_public.h"
#include "ui_syscalls.h"
#include "gui_items.h"
#include "gui_windows.h"
#include "gui_atoms.h"


#define CHAR_SEPARATOR ';'

#define UI_VIRTUALSCR_WIDTH 800
#define UI_VIRTUALSCR_HEIGHT 600

void UI_Error( const char *format, ... );
void UI_Printf( const char *format, ... );
void UI_FillRect( int x, int y, int w, int h, vec4_t color );

#define UI_Malloc( size ) trap_MemAlloc( size, __FILE__, __LINE__ )
#define UI_Free( data ) trap_MemFree( data, __FILE__, __LINE__ )

char *_UI_CopyString( const char *in, const char *filename, int fileline );
#define UI_CopyString(in) _UI_CopyString(in,__FILE__,__LINE__)

typedef struct
{
	int vidWidth;
	int vidHeight;
	int protocol_version;

	unsigned int time;

	int cursorX;
	int cursorY;

	int clientState;
	int serverState;
	int keydest;

	// button
	struct shader_s *whiteShader;
	struct shader_s *windowShader;
	struct shader_s *itemShader;
	struct shader_s *itemFocusShader;

	// checkbox
	struct shader_s *itemCheckBoxShader;
	struct shader_s *itemCheckBoxFocusShader;
	struct shader_s *itemCheckBoxSelectedShader;

	// slider
	struct shader_s *itemSliderLeft;
	struct shader_s *itemSliderRight;
	struct shader_s *itemSliderBar;
	struct shader_s *itemSliderHandle;

	// listbox
	struct shader_s *itemListBoxFocusShader;
	struct shader_s *itemListBoxSelectedShader;

	struct shader_s *listbox_up_left;
	struct shader_s *listbox_up_right;
	struct shader_s *listbox_down_left;
	struct shader_s *listbox_down_right;
	struct shader_s *listbox_left;
	struct shader_s *listbox_right;
	struct shader_s *listbox_up;
	struct shader_s *listbox_down;
	struct shader_s *listbox_back;

	// scrollbar
	struct shader_s *scrollUpShader;
	struct shader_s *scrollDownShader;
	struct shader_s *scrollBarShader;
	struct shader_s *scrollHandleShader;

	struct mufont_s *fontSystemSmall;
	struct mufont_s *fontSystemMedium;
	struct mufont_s *fontSystemBig;

	qboolean backGround; // has to draw the ui background
	qboolean backGroundTrackStarted;
	qboolean demoplaying;
} ui_local_t;

extern ui_local_t uis;

int UI_API( void );
void UI_Init( int vidWidth, int vidHeight, int protocol );
void UI_Shutdown( void );
void UI_Refresh( unsigned int time, int clientState, int serverState, int keydest, qboolean demoplaying, qboolean backGround );
void UI_DrawConnectScreen( char *serverName, char *rejectmessage, int downloadType, char *downloadfilename, float downloadPercent, int downloadSpeed, int connectCount, qboolean demoplaying );
void UI_KeyEvent( int key, qboolean down );
void UI_AddToServerList( char *adr, char *info );
void UI_CharEvent( int key );
void UI_MouseMove( int dx, int dy );
void UI_SetActiveGUI( qboolean active );

void UI_InitMenus(void );

#endif // __GUI_LOCAL_H__
