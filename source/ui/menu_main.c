/*
*/

#include "ui_local.h"

static void QuitFunc( void *unused )
{
	trap_CL_SetKeyDest( key_console );
	trap_Cmd_ExecuteText( EXEC_APPEND, "quit" );
}

void CloseWindow( menuitem_public_t *item )
{
	if( item  ) 
	{
		if( item->targetString && Q_stricmp( item->targetString, "self" ) )
		{
			UI_CloseWindow( item->targetString );
		}
		else if( item->windowname )
		{
			UI_CloseWindow( item->windowname );
		}
	}
}

void CloseAndSaveWindow( menuitem_public_t *item )
{
	if( item  ) 
	{
		if( item->targetString && Q_stricmp( item->targetString, "self" ) )
		{
			UI_ApplyWindowChanges( item->targetString );
			UI_CloseWindow( item->targetString );
		}
		else if( item->windowname )
		{
			UI_ApplyWindowChanges( item->windowname );
			UI_CloseWindow( item->windowname );
		}
	}
}

void OpenWindow( menuitem_public_t *item )
{
	if( item && item->targetString ) 
	{
		UI_OpenWindow( item->targetString );
	}
}

void OKAndStartMap( menuitem_public_t *item )
{
	if( item ) 
	{
		const char *s = trap_Cvar_String( "ui_startmap" );
		if( s && s[0] )
		{
			if( trap_Cvar_Value( "sv_cheats" ) )
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "devmap %s", s ) );
			else
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "map %s", s ) );
		}
	}
}

void OKAndStartDemo( menuitem_public_t *item )
{
	if( item ) 
	{
		const char *s = trap_Cvar_String( "ui_startdemo" );
		if( s && s[0] )
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo %s", s ) );
	}
}

void SetCvarToggle( menuitem_public_t *item )
{
	if( item && item->targetString )
	{
		trap_Cvar_SetValue( item->targetString, ( item->value != 0 ) );
	}
}

void GetCvarToggle( menuitem_public_t *item )
{
	if( item && item->targetString )
	{
		item->value = ( trap_Cvar_Value( item->targetString ) != 0 );
	}
}

void GetCvarValue( menuitem_public_t *item )
{
	if( item && item->targetString )
	{
		int i = trap_Cvar_Value( item->targetString );
		if( i >= item->minvalue && i < item->maxvalue )
			item->value = i;
	}
}

void SetCvarValue( menuitem_public_t *item )
{
	if( item && item->targetString )
	{
		trap_Cvar_SetValue( item->targetString, item->value );
	}
}

void GetCvarStringToTittle( menuitem_public_t *item )
{
	if( item && item->targetString )
	{
		const char *s = trap_Cvar_String( item->targetString );
		if( s && s[0] )
			Q_strncpyz( item->tittle, s, sizeof( item->tittle ) );
	}
}

void SetCvarStringFromTittle( menuitem_public_t *item )
{
	if( item && item->targetString )
	{
		trap_Cvar_ForceSet( item->targetString, item->tittle );
	}
}

void GetCvarStringToNamesListPosition( menuitem_public_t *item )
{
	if( item && item->targetString && item->listNames )
	{
		char *t;
		const char *s = trap_Cvar_String( item->targetString );
		if( s && s[0] )
		{
			int i = 0;
			while( ( t = UI_ListNameForPosition( item->listNames, i ) ) != NULL )
			{
				if( !Q_stricmp( t, s ) )
				{
					item->value = (int)i;
					return;
				}

				i++;
			}
		}
	}
}

void SetCvarStringFromNamesListPosition( menuitem_public_t *item )
{
	if( item && item->targetString )
	{
		if( UI_ListNameForPosition( item->listNames, item->value ) != NULL )
			trap_Cvar_ForceSet( item->targetString, UI_ListNameForPosition( item->listNames, item->value ) );
		else
			trap_Cvar_ForceSet( item->targetString, "" );
	}
}


void M_Menu_Main_f( void )
{
	menuitem_public_t *item;

	UI_InitWindow( "test", 20, 30, 340, 320, qfalse, qfalse, NULL );

	UI_InitItemStatic( "test", "test1_tittle", 32, 16, ALIGN_LEFT_TOP, "hello world", NULL );

	// load map
	UI_InitItemStatic( "test", "test1_spinner_tittle", 30, 70, ALIGN_RIGHT_TOP, "map", uis.fontSystemSmall );
	item = UI_InitItemListbox( "test", "test1_spinner", 32, 70, ALIGN_LEFT_TOP, NULL, UI_RefreshMapListCvar(), GetCvarStringToNamesListPosition, SetCvarStringFromNamesListPosition, qtrue, OKAndStartMap, "ui_startmap" );
	item->h = 110;
	item->w = 220;
	UI_InitItemButton( "test", "test1_loadmap", 260, 70, ALIGN_LEFT_TOP, "Load", NULL, OKAndStartMap, NULL );

	UI_InitItemCheckBox( "test", "devmap_checkbox", 260, 130, ALIGN_LEFT_MIDDLE, GetCvarToggle, SetCvarToggle, qtrue, "sv_cheats" );
	UI_InitItemStatic( "test", "test1_devmaptittle", 276, 130, ALIGN_LEFT_MIDDLE, "cheats", uis.fontSystemSmall );
	
	UI_InitItemButton( "test", "test1_opendemos", 92, 240, ALIGN_LEFT_TOP, "Demos", NULL, OpenWindow, "demos" );
	UI_InitItemButton( "test", "test1_openquit", 188, 240, ALIGN_LEFT_TOP, "Quit", NULL, OpenWindow, "quit_window" );

	//UI_InitItemButton( "test", "test1_close", 200, 240, ALIGN_LEFT_TOP, "Close", NULL, CloseWindow, "self" );

	UI_InitWindow( "demos", 80, 40, 700, 500, qfalse, qfalse, NULL );

	
	//UI_InitItemButton( "demos", "demos_ok", 212, 196, ALIGN_LEFT_TOP, "OK", NULL, CloseAndSaveWindow, "self" );
	//UI_InitItemStatic( "demos", "demos_checkbox_tittle", 72, 48, ALIGN_RIGHT_TOP, "show fps", uis.fontSystemSmall );
	//UI_InitItemCheckBox( "demos", "demos_checkbox", 72, 48, ALIGN_LEFT_TOP, GetCvarToggle, SetCvarToggle, "cg_showFPS" );
	//UI_InitItemSlider( "demos", "demos_slider", 62, 128, ALIGN_LEFT_TOP, 0, 32, 1, GetCvarValue, SetCvarValue, "r_subdivisions" );
	UI_InitItemWindowDragger( "demos", "demos_dragger", 350, 8, ALIGN_CENTER_TOP );


	// load demo
	item = UI_InitItemListbox( "demos", "demos_listbox", 350, 36, ALIGN_CENTER_TOP, NULL, UI_RefreshDemoListCvar(), GetCvarStringToNamesListPosition, SetCvarStringFromNamesListPosition, qtrue, OKAndStartDemo, "ui_startdemo" );
	item->w = 600;
	item->h = 400;
	UI_InitItemButton( "demos", "test2_loaddemo", 650, 450, ALIGN_RIGHT_TOP, "Load Demo", NULL, OKAndStartDemo, NULL );
	UI_InitItemButton( "demos", "demos_close", 50, 450, ALIGN_LEFT_TOP, "Cancel", NULL, CloseWindow, "self" );

	UI_InitWindow( "quit_window", 160, 260, 300, 140, qfalse, qtrue, NULL );

	UI_InitItemStatic( "quit_window", "quit_window_tittle", 150, 32, ALIGN_CENTER_TOP, "Are you sure you want to quit?", NULL );
	UI_InitItemButton( "quit_window", "quit_window_yes", 152, 80, ALIGN_LEFT_TOP, "YES", uis.fontSystemBig, QuitFunc, NULL );
	UI_InitItemButton( "quit_window", "quit_window_no", 140, 80, ALIGN_RIGHT_TOP, "NO", uis.fontSystemBig, CloseWindow, "self" );

	UI_OpenWindow( "test" );
}

void UI_InitMenus( void )
{
	trap_Cmd_AddCommand( "menu_main", M_Menu_Main_f );
}
