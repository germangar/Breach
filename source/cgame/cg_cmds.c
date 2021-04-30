/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"


//==================================================
// CLIENT COMANDS
// execute or forward to server.
//==================================================

/*
* CG_Cmd_UseWeapon_f
*/
static void CG_Cmd_UseWeapon_f( void )
{
	gsitem_t *item;
	int typeMask = 0xFF;

	if( !cg.frame.valid || cgs.demoPlaying )
		return;

	item = GS_Cmd_UseItem( &cg_entities[cgs.playerNum].current, &cg.predictedPlayerState, trap_Cmd_Args(), typeMask );
	if( item )
	{
		if( item->type & IT_WEAPON )
			CG_Predict_ChangeWeapon( item->tag );

		trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd use %s", item->shortname ) );
	}
}

/*
* CG_Cmd_NextWeapon_f
*/
static void CG_Cmd_NextWeapon_f( void )
{
	gsitem_t *item;

	if( !cg.frame.valid || cgs.demoPlaying )
		return;

	item = GS_Cmd_NextWeapon_f( &cg_entities[cgs.playerNum].current, &cg.predictedPlayerState, cg.predictedNewWeapon );
	if( item )
	{
		CG_Predict_ChangeWeapon( item->tag );
		trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd use %s", item->shortname ) );
	}
}

/*
* CG_Cmd_PrevWeapon_f
*/
static void CG_Cmd_PrevWeapon_f( void )
{
	gsitem_t *item;

	if( !cg.frame.valid || cgs.demoPlaying )
		return;

	item = GS_Cmd_PrevWeapon_f( &cg_entities[cgs.playerNum].current, &cg.predictedPlayerState, cg.predictedNewWeapon );
	if( item )
	{
		CG_Predict_ChangeWeapon( item->tag );
		trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd use %s", item->shortname ) );
	}
}

//==================================================
// CLIENT COMANDS MANAGER
//==================================================

typedef struct cgame_cmd_s
{
	char *name;
	struct cgame_cmd_s *next;
} cgame_cmd_t;

cgame_cmd_t *cgameCommandsHead = NULL;

/*
* CG_Cmd_RegisterCommand - Keep a local list of command names for unregistering them
*/
void CG_Cmd_RegisterCommand( const char *cmdname, void ( *func )(void) )
{
	cgame_cmd_t *command = cgameCommandsHead;

	if( !cmdname || !cmdname[0] )
		return;

	while( command )
	{
		if( !Q_stricmp( cmdname, command->name ) )
			return;
		command = command->next;
	}

	// didn't find it. Register a new one
	trap_Cmd_AddCommand( (char *)cmdname, func );

	command = cgameCommandsHead;
	cgameCommandsHead = ( cgame_cmd_t * )CG_Malloc( sizeof( cgame_cmd_t ) );
	cgameCommandsHead->name = GS_CopyString( cmdname );
	cgameCommandsHead->next = command;
}

/*
* CG_Cmd_UnregisterCommands
*/
static void CG_Cmd_UnregisterCommands( void )
{
	cgame_cmd_t *c, *command = cgameCommandsHead;

	while( command )
	{
		trap_Cmd_RemoveCommand( command->name );
		c = command;
		command = command->next;
		CG_Free( c->name );
		CG_Free( c );
	}
}

/*
* CG_InitCommands
*/
void CG_InitCommands( void )
{
	// add local commands
	CG_Cmd_RegisterCommand( "use", CG_Cmd_UseWeapon_f );
	CG_Cmd_RegisterCommand( "wnext", CG_Cmd_NextWeapon_f );
	CG_Cmd_RegisterCommand( "wprev", CG_Cmd_PrevWeapon_f );
	CG_Cmd_RegisterCommand( "help_hud", Cmd_CG_PrintHudHelp_f );
}

/*
* CG_ShutdownCommands
*/
void CG_ShutdownCommands( void )
{
	CG_Cmd_UnregisterCommands();
}

//==================================================
// SERVER COMMANDS
// received from server
//==================================================

/*
* CG_SVCMD_Print
*/
static void CG_SVCMD_Print( void )
{
	GS_Printf( "%s", trap_Cmd_Argv( 1 ) );
}

/*
* CG_SVCMD_ChatPrint
*/
static void CG_SVCMD_ChatPrint( void )
{
	GS_Printf( "%s", trap_Cmd_Argv( 1 ) );
	trap_S_StartGlobalSound( CG_LocalSound( SOUND_CHAT ), CHAN_AUTO, 1.0f );
}

//=================
// server commands manager
//=================

typedef struct
{
	char *name;
	void ( *func )( void );
} cg_svcmd_t;

cg_svcmd_t cg_svCmds[] =
{
	{ "pr", CG_SVCMD_Print },
	{ "ch", CG_SVCMD_ChatPrint },

	{ NULL }
};

/*
* CG_ServerCommand
*/
void CG_ServerCommand( void )
{
	cg_svcmd_t *svcmd;
	char *string;

	string = trap_Cmd_Argv( 0 );
	for( svcmd = cg_svCmds; svcmd->name; svcmd++ )
	{
		if( !Q_stricmp( string, svcmd->name ) )
		{
			svcmd->func();
			break;
		}
	}
}
