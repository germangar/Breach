/*
 */

#include "gui_local.h"

//======================================================================

#define WINDOW_MIN_WIDTH 128
#define WINDOW_MIN_HEIGHT 48

/*
* cWindowManager
*/

class cWindow *cWindowManager::FindWindowByName( const char *name )
{
	class cWindow *window;

	for( window = this->rootWindow; window != NULL; window = window->nextRegistered )
	{
		if( !Q_stricmp( name, window->name ) )
			return window;
	}

	return NULL;
}

class cWindow *cWindowManager::RegisterWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic )
{
	class cWindow *window;

	window = this->FindWindowByName( name );

	if( !window )
	{
		if( !name )
			UI_Error( "cWindowManager::RegisterWindow: no name provided\n" );
		else if( strlen( name ) < 3 )
			UI_Error( "cWindowManager::RegisterWindow: too short window name\n" );
		else if( strlen( name ) >= WINDOW_NAME_SIZE )
			UI_Error( "cWindowManager::RegisterWindow: too long window name\n" );
		else if( !Q_stricmp( name, "self" ) || !Q_stricmp( name, "all" ) )
			UI_Error( "cWindowManager::RegisterWindow: %s window name is forbidden\n", name );

		window = ( class cWindow * )UI_Malloc( sizeof( cWindow ) );
		memset( window, 0, sizeof( cWindow ) );
		Q_strncpyz( window->name, name, sizeof( window->name ) );
		window->nextActive = NULL;
		window->nextRegistered = this->rootWindow;
		this->rootWindow = window;
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

void cWindowManager::RecurseFreeWindow( class cWindow **win ) 
{
	class cWindow *window = *win;

	if( window )
	{
		if( window->nextRegistered )
			this->RecurseFreeWindow( &window->nextRegistered );

		// free associated items
/*		if( window->items_headnode )
			UI_RecurseFreeItem( window->items_headnode );*/

		guiItemManager.RecurseFreeItems( window->menuItemsHeadnode );

		UI_Free( window );
		window = NULL;
	}
}

void cWindowManager::FreeWindows( void )
{
	this->activeWindows = NULL;
	this->RecurseFreeWindow( &this->rootWindow );
}

void cWindowManager::RecurseDrawActiveWindows( class cWindow *window )
{
	if( window )
	{
		if( window->nextActive && !window->blockdraw )
			this->RecurseDrawActiveWindows( window->nextActive );

		window->Draw();
	}
}

void cWindowManager::DrawActiveWindows( void )
{
	if( this->activeWindows )
		this->RecurseDrawActiveWindows( this->activeWindows );
}

class cWindow *cWindowManager::RecurseFindWindowsFocus( class cWindow *window )
{
	class cWindow *focus;

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
			focus = this->RecurseFindWindowsFocus( window->nextActive );
	}

	return focus;
}

void cWindowManager::UpdateFocusedWindow( void )
{
	this->focusedWindow = this->RecurseFindWindowsFocus( this->activeWindows );
}

void cWindowManager::OpenWindow( const char *name )
{
	class cWindow *window = this->FindWindowByName( name );

	if( window )
		window->Push();
	else
		UI_Printf( "cWindowManager::OpenWindow: unknown window %s\n", name );
}

void cWindowManager::CloseWindow( const char *name )
{
	class cWindow *window;

	if( name && !Q_stricmp( "all", name ) )
	{
		while( this->activeWindows != NULL )
			this->activeWindows->Deactivate();

		UI_SetActiveGUI( qfalse );
	}
	else
	{
		window = this->FindWindowByName( name );
		if( window != NULL )
			window->Deactivate();

		if( this->activeWindows == NULL )
			UI_SetActiveGUI( qfalse );
	}
}

cWindowManager guiWindowManager;

/*
* cWindow
*/

void cWindow::Activate( void )
{
	class cWindow *win = NULL;

	UI_SetActiveGUI( qtrue );
	for( win = guiWindowManager.activeWindows; win != NULL; win = win->nextActive )
	{
		if( win == this )
			return;
	}

	// activate it
	this->nextActive = guiWindowManager.activeWindows;
	guiWindowManager.activeWindows = this;
}

qboolean cWindow::Deactivate( void )
{
	class cWindow *win;

	if( this == guiWindowManager.activeWindows )
	{
		guiWindowManager.activeWindows = this->nextActive;
		return qtrue;
	}

	for( win = guiWindowManager.activeWindows; win != NULL; win = win->nextActive )
	{
		if( win->nextActive && win->nextActive == this ) 
		{
			win->nextActive = this->nextActive;
			this->nextActive = NULL;
			return qtrue;
		}
	}

	return qfalse;
}

void cWindow::Push( void )
{
	class cMenuItem *item;
	qboolean wasActive;

	UI_SetActiveGUI( qtrue );
	if( this == guiWindowManager.activeWindows )
		return; // already on top

	wasActive = this->Deactivate();
	this->Activate();

	// update the window items from the original sources
	if( !wasActive )
	{
		item = this->menuItemsHeadnode;
		while( item )
		{
			item->Update();
			item = item->next;
		}
	}
}

void cWindow::ClickEvent( qboolean down )
{
	if( down )
	{
	}
	else
	{
		if( this->IsTopWindow() )
		{
		}
		else
		{
			this->Push();
		}
	}
}

class cMenuItem *cWindow::CreateMenuItem( const char *itemname )
{
	class cMenuItem *item;

	item = guiItemManager.RegisterItem( itemname, &this->menuItemsHeadnode );
	if( item )
	{
		item->parentWindow = this;
		Q_strncpyz( item->pub.windowname, this->name, sizeof( item->pub.windowname ) );
	}

	return item;
}

qboolean cWindow::IsTopWindow( void )
{
	return (qboolean)( ( this == guiWindowManager.activeWindows ) && ( guiWindowManager.activeWindows != NULL ) );
}

float *cWindow::CurrentColor( void )
{
	vec4_t backColorMask = { 0.9f, 0.9f, 0.9f, 1.0f };
	int i;

	if( !this->IsTopWindow() )
	{
		for( i = 0; i < 4; i++ ) 
			this->currentColor[i] = this->color[i] * backColorMask[i];
	}
	else
	{
		Vector4Copy( this->color, this->currentColor );
	}

	return this->currentColor;
}

void cWindow::ApplyChanges( void )
{
	class cMenuItem *item;

	item = this->menuItemsHeadnode;
	while( item )
	{
		item->ApplyChanges();
		item = item->next;
	}
}

void cWindow::Draw( void )
{
	UI_DrawStretchPic( this->x, this->y, this->w, this->h, ALIGN_LEFT_TOP,
			this->picS1, this->picT1, this->picS2, this->picT2,
			this->CurrentColor(), this->custompic );

		//UI_DrawStringWidth( window->x + 8, window->y, ALIGN_LEFT_TOP, window->name, window->w,
		//	uis.fontSystemSmall, UI_WindowColor( window, colorWhite ) );

		//UI_DrawModel( window->x, window->y, window->w, window->h, ALIGN_LEFT_TOP, trap_R_RegisterModel( "models/items/ammo/pack/pack.md3" ), tv( -25, 35, 0 ) );

		guiItemManager.RecurseDrawItems( this->menuItemsHeadnode );
}

//======================================================================

// wrap

class cMenuItem *cWindowManager::CreateMenuItem( const char *windowname, const char *itemname )
{
	class cWindow *window;
	class cMenuItem *item;

	window = guiWindowManager.FindWindowByName( windowname );
	if( !window )
	{
		UI_Printf( "Couldn't find window %s\n", windowname );
		return NULL;
	}

	item = window->CreateMenuItem( itemname );

	return item;
}

//--------------------------------------------------
// fixme: temporary, for allowing the ui to export
//--------------------------------------------------

void UI_InitWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic )
{
	guiWindowManager.RegisterWindow( name, x, y, w, h, blockdraw, blockfocus, custompic );
}

void UI_OpenWindow( const char *name )
{
	guiWindowManager.OpenWindow( name );
}

void UI_CloseWindow( const char *name )
{
	guiWindowManager.CloseWindow( name );
}

void UI_ApplyWindowChanges( const char *name )
{
	class cWindow *window = guiWindowManager.FindWindowByName( name );
	if( !window )
	{
		UI_Printf( "UI_ApplyWindowChanges: unknown window %s\n", name );
		return;
	}
	
	window->ApplyChanges();
}
