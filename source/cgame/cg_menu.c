/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

#define CHAR_SEPARATOR ';'
char *CG_ListNameForPosition( char *namesList, int position )
{
	static char buf[MAX_STRING_CHARS];
	char separator = CHAR_SEPARATOR;
	char *s, *t;
	int count;

	if( !namesList )
		return NULL;

	// set up the tittle from the spinner names
	s = namesList;
	t = s;
	count = 0;
	buf[0] = 0;
	while( *s && ( s = strchr( s, separator ) ) )
	{
		if( count == position )
		{
			*s = 0;
			if( !strlen( t ) )
				GS_Error( "UI_NameInStringList: empty name in list\n" );
			if( strlen( t ) > MAX_STRING_CHARS - 1 )
				GS_Printf( "WARNING: UI_NameInStringList: name is too long\n" );

			Q_strncpyz( buf, t, sizeof( buf ) );
			*s = separator;
			break;
		}

		count++;
		s++;
		t = s;
	}

	if( buf[0] == 0 )
		return NULL;

	return buf;
}

static void Menu_Action_CloseAndApply( struct menuitem_public_s *item )
{
	trap_UI_ApplyWindowChanges( item->windowname );
	trap_UI_CloseWindow( "all" );
}
/*
static void Menu_Action_SetCvarToggle( struct menuitem_public_s *item )
{
	if( item && item->targetString )
	{
		trap_Cvar_SetValue( item->targetString, ( item->value != 0 ) );
	}
}

static void Menu_Action_GetCvarToggle( struct menuitem_public_s *item )
{
	if( item && item->targetString )
	{
		item->value = ( trap_Cvar_Value( item->targetString ) != 0 );
	}
}*/

qboolean demoavi = qfalse;

static void Demoavi_Update( struct menuitem_public_s *item )
{
	item->value = ( demoavi != qfalse );
}

static void Demoavi_Set( struct menuitem_public_s *item )
{
	if( item->value != demoavi )
		trap_Cmd_ExecuteText( EXEC_NOW, "demoavi" );
	demoavi = ( qboolean )( item->value != qfalse );
}

static void Menu_Action_JoinTeam( struct menuitem_public_s *item )
{
	trap_Cmd_ExecuteText( EXEC_NOW, "join" );
	trap_UI_CloseWindow( item->windowname );
	trap_UI_OpenWindow( "team_menu" );
}

static void Menu_Action_Spectate( struct menuitem_public_s *item )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "spec" );
	trap_UI_CloseWindow( "all" );
}

static void Menu_Action_Disconnect( struct menuitem_public_s *item )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect" );
	trap_UI_CloseWindow( "all" );
}

static void Menu_Action_SelectClass( struct menuitem_public_s *item )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "class %s", CG_ListNameForPosition( item->listNames, item->value ) ) );
}

static void Menu_Spectator( void )
{
	trap_UI_InitWindow( "spectator_menu", 300, 200, 200, 200, qfalse, qtrue, NULL );

	trap_UI_InitItemButton( "spectator_menu", "jointeam", 32, 32, ALIGN_LEFT_TOP, "Join", NULL, Menu_Action_JoinTeam, NULL );

	trap_UI_InitItemButton( "spectator_menu", "disconnect", 32, 64, ALIGN_LEFT_TOP, "Disconnect", NULL, Menu_Action_Disconnect, NULL );
}

static void Menu_Team( void )
{
	trap_UI_InitWindow( "team_menu", 300, 200, 200, 200, qfalse, qtrue, NULL );

	trap_UI_InitItemButton( "team_menu", "spectate", 32, 32, ALIGN_LEFT_TOP, "Spectate", NULL, Menu_Action_Spectate, NULL );
	trap_UI_InitItemButton( "team_menu", "disconnect", 32, 64, ALIGN_LEFT_TOP, "Disconnect", NULL, Menu_Action_Disconnect, NULL );

	trap_UI_InitItemSpinner( "team_menu", "class", 32, 96, ALIGN_LEFT_TOP, NULL, "soldier;sniper;engineer;", NULL, Menu_Action_SelectClass, qfalse, NULL );
	trap_UI_InitItemButton( "team_menu", "ok", 32, 128, ALIGN_LEFT_TOP, "Ok", NULL, Menu_Action_CloseAndApply, NULL );

}

static void Menu_Demo( void )
{
	menuitem_public_t *item;

	trap_UI_InitWindow( "demo_menu", 300, 0, 200, 48, qtrue, qtrue, NULL );

	item = trap_UI_InitItemButton( "demo_menu", "disconnect", 12, 24, ALIGN_LEFT_MIDDLE, "Stop", NULL, Menu_Action_Disconnect, NULL );
	item->w = 82;
	item->h = 24;

	item = trap_UI_InitItemStatic( "demo_menu", "capture", 160, 24, ALIGN_RIGHT_MIDDLE, "capture", cgm.fontSystemSmall );

	item = trap_UI_InitItemCheckBox( "demo_menu", "demoavi", 166, 24, ALIGN_LEFT_MIDDLE, Demoavi_Update, Demoavi_Set, qtrue, NULL );
}

void CG_IngameMenu( void )
{
	static qboolean precache_menus = qfalse;

	if( !precache_menus )
	{
		if( cgs.demoPlaying )
		{
			Menu_Demo();
		}
		else
		{
			Menu_Spectator();
			Menu_Team();
		}
		precache_menus = qtrue;
	}

	if( cgs.demoPlaying )
	{
		trap_UI_OpenWindow( "demo_menu" );
	}
	else if( cg.predictedEntityState.team != TEAM_SPECTATOR )
	{
		trap_UI_OpenWindow( "team_menu" );
	}
	else
	{
		trap_UI_OpenWindow( "spectator_menu" );
	}
}

