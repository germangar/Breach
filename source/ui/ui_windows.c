/*
 */

#include "ui_local.h"

//======================================================================

#define WINDOW_MIN_WIDTH 128
#define WINDOW_MIN_HEIGHT 48

uiwindow_t *rootWindow = NULL;

uiwindow_t *UI_FindWindow( const char *name )
{
	uiwindow_t *window;

	for( window = rootWindow; window != NULL; window = window->nextRegistered )
	{
		if( !Q_stricmp( name, window->name ) )
			return window;
	}

	return NULL;
}

uiwindow_t *UI_RegisterWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic )
{
	uiwindow_t *window;

	window = UI_FindWindow( name );

	if( !window )
	{
		if( !name )
			UI_Error( "UI_RegisterWindow: no name provided\n" );
		else if( strlen( name ) < 3 )
			UI_Error( "UI_RegisterWindow: too short window name\n" );
		else if( strlen( name ) >= WINDOW_NAME_SIZE )
			UI_Error( "UI_RegisterWindow: too long window name\n" );
		else if( !Q_stricmp( name, "self" ) || !Q_stricmp( name, "all" ) )
			UI_Error( "UI_RegisterWindow: %s window name is forbidden\n", name );

		window = UI_Malloc( sizeof( uiwindow_t ) );
		memset( window, 0, sizeof( uiwindow_t ) );
		Q_strncpyz( window->name, name, sizeof( window->name ) );
		window->nextActive = NULL;
		window->nextRegistered = rootWindow;
		rootWindow = window;
	}

	// set up some default values
	window->blockdraw = blockdraw;
	window->blockfocus = blockfocus;

	window->x = x;
	window->y = y;
	window->w = w;
	window->h = h;

	clamp( window->w, WINDOW_MIN_WIDTH, UI_VIRTUALSCR_WIDTH );
	clamp( window->h, WINDOW_MIN_HEIGHT, UI_VIRTUALSCR_HEIGHT );
	clamp( window->x, -( window->w + 16 ), UI_VIRTUALSCR_WIDTH - 16 );
	clamp( window->y, 0, UI_VIRTUALSCR_HEIGHT - 16 );

	if( custompic )
	{
		window->custompic = custompic;
		window->picS1 = window->picT1 = 0;
		window->picS2 = window->picT2 = 1;
	}
	else
	{
		window->custompic = uis.windowShader;
		window->picS1 = window->picT1 = 0;
		window->picS2 = window->picT2 = 1;
	}
	Vector4Copy( colorWhite, window->color );

	return window;
}

void UI_InitWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic )
{
	UI_RegisterWindow( name, x, y, w, h, blockdraw, blockfocus, custompic );
}

static void UI_RecurseFreeWindow( uiwindow_t **win ) 
{
	uiwindow_t *window = *win;

	if( window )
	{
		if( window->nextRegistered )
			UI_RecurseFreeWindow( &window->nextRegistered );

		// free associated items
		if( window->items_headnode )
			UI_RecurseFreeItem( window->items_headnode );

		UI_Free( window );
		window = NULL;
	}
}

void UI_FreeWindows( void )
{
	uis.activeWindows = NULL;
	UI_RecurseFreeWindow( &rootWindow );
}

void UI_InitWindows( void )
{
	UI_FreeWindows();
}

static void UI_ActivateWindow( uiwindow_t *window )
{
	uiwindow_t *win = NULL;
	
	if( window )
	{
		UI_SetActiveGUI( qtrue );
		for( win = uis.activeWindows; win != NULL; win = win->nextActive )
		{
			if( win == window ) 
				return;
		}

		// activate it
		window->nextActive = uis.activeWindows;
		uis.activeWindows = window;
	}
}

static qboolean UI_DeactivateWindow( uiwindow_t *window )
{
	uiwindow_t *win;

	if( window )
	{
		if( window == uis.activeWindows )
		{
			uis.activeWindows = window->nextActive;
			return qtrue;
		}

		for( win = uis.activeWindows; win != NULL; win = win->nextActive )
		{
			if( win->nextActive && win->nextActive == window ) 
			{
				win->nextActive = window->nextActive;
				window->nextActive = NULL;
				return qtrue;
			}
		}
	}

	return qfalse;
}

static void UI_PushWindow( uiwindow_t *window )
{
	menuitem_private_t *item;
	qboolean wasActive;

	if( window )
	{
		UI_SetActiveGUI( qtrue );
		if( window == uis.activeWindows )
			return; // already on top

		wasActive = UI_DeactivateWindow( window );
		UI_ActivateWindow( window );

		// update the window items from the original sources
		if( !wasActive )
		{
			item = window->items_headnode;
			while( item )
			{
				if( item->Update )
					item->Update( &item->pub );

				item = item->next;
			}
		}
	}
}

void UI_Window_ClickEvent( uiwindow_t *window, qboolean down )
{
	if( window )
	{
		if( down )
		{
		}
		else
		{
			if( UI_IsTopWindow( window ) )
			{
			}
			else
			{
				UI_PushWindow( window );
			}
		}
	}
}

void UI_OpenWindow( const char *name )
{
	uiwindow_t *window;
	window = UI_FindWindow( name );
	if( window )
	{
		UI_PushWindow( window );
	}
	else
	{
		UI_Printf( "UI_OpenWindow: unknown window %s\n", name );
	}
}

void UI_CloseWindow( const char *name )
{
	if( name && !Q_stricmp( "all", name ) )
	{
		while( uis.activeWindows != NULL )
			UI_DeactivateWindow( uis.activeWindows );
		UI_SetActiveGUI( qfalse );
	}
	else
	{
		UI_DeactivateWindow( UI_FindWindow( name ) );
		if( uis.activeWindows == NULL )
			UI_SetActiveGUI( qfalse );
	}
}

void UI_ApplyWindowChanges( const char *name )
{
	uiwindow_t *window;
	window = UI_FindWindow( name );
	if( window )
	{
		menuitem_private_t *item;

		item = window->items_headnode;
		while( item )
		{
			if( item->ApplyChanges )
				item->ApplyChanges( &item->pub );

			item = item->next;
		}
	}
	else
	{
		UI_Printf( "UI_Window_ApplyChanges: unknown window %s\n", name );
	}
}

//======================================================================

//======================================================================

static void UI_DrawWindow( uiwindow_t *window, uiwindow_t *focus )
{
	if( window )
	{
		UI_DrawStretchPic( window->x, window->y, window->w, window->h, ALIGN_LEFT_TOP,
			window->picS1, window->picT1, window->picS2, window->picT2,
			UI_WindowColor( window, window->color ), window->custompic );

		//UI_DrawStringWidth( window->x + 8, window->y, ALIGN_LEFT_TOP, window->name, window->w,
		//	uis.fontSystemSmall, UI_WindowColor( window, colorWhite ) );

		//UI_DrawModel( window->x, window->y, window->w, window->h, ALIGN_LEFT_TOP, trap_R_RegisterModel( "models/items/ammo/pack/pack.md3" ), tv( -25, 35, 0 ) );


		UI_DrawItemsArray( window->items_headnode );
	}
}

static void UI_RecurseDrawActiveWindows( uiwindow_t *window, uiwindow_t *focus )
{
	if( window )
	{
		if( window->nextActive && !window->blockdraw )
			UI_RecurseDrawActiveWindows( window->nextActive, focus );

		UI_DrawWindow( window, focus );
	}
}

static uiwindow_t *UI_RecurseFindWindowsFocus( uiwindow_t *window )
{
	uiwindow_t *focus;

	focus = NULL;
	if( window )
	{
		// if this window is in focus it overrides
		// its children being in focus
		if( uis.cursorX >= window->x &&
			uis.cursorX <= (window->x + window->w) &&
			uis.cursorY >= window->y &&
			uis.cursorY <= (window->y + window->h) )
		{
			return window;
		}


		if( window->nextActive && !window->blockdraw && !window->blockfocus )
			focus = UI_RecurseFindWindowsFocus( window->nextActive );
	}

	return focus;
}

void UI_DrawActiveWindows( uiwindow_t *focus )
{
	UI_RecurseDrawActiveWindows( uis.activeWindows, focus );
}

uiwindow_t *UI_FindWindowsFocus( void )
{
	return UI_RecurseFindWindowsFocus( uis.activeWindows );
}

qboolean UI_IsTopWindow( uiwindow_t *focus )
{
	return ( ( focus == uis.activeWindows ) && ( focus != NULL ) );
}

//======================================================================


menuitem_private_t *UI_Window_CreateItem( const char *windowname, const char *itemname )
{
	uiwindow_t *window;
	menuitem_private_t *item;

	window = UI_FindWindow( windowname );
	if( !window )
	{
		UI_Printf( "Couldn't find window %s\n", windowname );
		return NULL;
	}

	item = UI_RegisterItem( itemname, &window->items_headnode );
	if( item ) 
	{
		item->window = window;
		Q_strncpyz( item->pub.windowname, window->name, sizeof( item->pub.windowname ) );
	}

	return item;
}
