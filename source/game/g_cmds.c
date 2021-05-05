/*
 */
#include "g_local.h"

/*
* G_CheckFlood
*/
qboolean G_CheckFlood( gentity_t *ent )
{
	return qfalse;
}

/*
/* G_CheckCheat
*/
qboolean G_CheckCheat( gentity_t *ent )
{
	if( !sv_cheats->integer )
	{
		G_PrintMsg( ent, "Cheats are not allowed in this server\n" );
		return qtrue;
	}
	return qfalse;
}

/*
* Cmd_FakeClient_f
*/
void Cmd_FakeClient_f( gentity_t *ent )
{

	gentity_t *fakeclient;

	if( G_CheckCheat( ent ) )
		return;

	fakeclient = G_SpawnFakeClient( "fakeplayer" );
	if( fakeclient )
		G_Teams_Join_Cmd( fakeclient );
	else
		G_PrintMsg( ent, "Couldn't spawn a fakeclient\n" );
}

/*
* Cmd_UseWeapon_f
*/
void Cmd_UseWeapon_f( gentity_t *ent )
{
	gsitem_t *item;
	int typeMask = 0xFF;

	if( !ent->client )
		return;

	item = GS_Cmd_UseItem( &ent->s, &ent->client->ps, trap_Cmd_Args(), typeMask );
	if( item )
	{
		G_Item_Use( ent, item );
	}
}

/*
* Cmd_NextWeapon_f
*/
void Cmd_NextWeapon_f( gentity_t *ent )
{
	gsitem_t *item;

	if( !ent->client )
		return;

	item = GS_Cmd_NextWeapon_f( &ent->s, &ent->client->ps, 0 );
	if( item )
	{
		G_Item_Use( ent, item );
	}
}

/*
* Cmd_PrevWeapon_f
*/
void Cmd_PrevWeapon_f( gentity_t *ent )
{
	gsitem_t *item;

	if( !ent->client )
		return;

	item = GS_Cmd_PrevWeapon_f( &ent->s, &ent->client->ps, 0 );
	if( item )
	{
		G_Item_Use( ent, item );
	}
}

/*
* Cmd_NoClip_f
*/
void Cmd_NoClip_f( gentity_t *ent )
{
	if( G_CheckCheat( ent ) )
		return;

	if( ent->s.solid )
	{
		ent->s.solid = SOLID_NOT;
		ent->s.ms.type = MOVE_TYPE_FREEFLY;
		G_PrintMsg( ent, "Clipping OFF\n" );
	}
	else
	{
		ent->s.solid = SOLID_PLAYER;
		ent->s.ms.type = MOVE_TYPE_STEP;
		G_PrintMsg( ent, "Clipping ON\n" );
	}
}

/*
* Cmd_Spec_f
*/
void Cmd_Spec_f( gentity_t *ent )
{
	if( G_Teams_JoinTeam( ent, TEAM_SPECTATOR ) )
	{
		G_PrintMsg( NULL, "%s%s joined the %s%s team.\n", ent->client->netname, S_COLOR_WHITE,
			GS_TeamName( ent->s.team ), S_COLOR_WHITE );
	}
}

/*
* Cmd_Class_f
*/
void Cmd_Class_f( gentity_t *ent )
{
	gsplayerclass_t *playerClass;
	int i;

	if( ent->client->team == TEAM_NOTEAM || ent->client->team == TEAM_SPECTATOR )
	{
		G_PrintMsg( ent, "Join a team first\n" );
		return;
	}

	if( trap_Cmd_Argc() < 2 )
	{
		G_PrintMsg( ent, "You must specify a class name. Valid names are:\n" );
		goto print_classes_help;
	}

	playerClass = GS_PlayerClassByName( trap_Cmd_Argv( 1 ) );
	if( !playerClass )
	{
		G_PrintMsg( ent, "Invalid class name. Valid names are:\n" );
		goto print_classes_help;
	}

	if( !Q_stricmp( playerClass->classname, "noclass" ) )
	{
		if( G_CheckCheat( ent ) )
			goto print_classes_help;
	}

	ent->client->playerClassIndex = playerClass->index;
	G_PrintMsg( ent, "You will respawn as %s\n", playerClass->classname );

	return;

print_classes_help:
	for( i = 1; ( playerClass = GS_PlayerClassByIndex( i ) ) != NULL; i++ )
	{
		G_PrintMsg( ent, "- %s\n", playerClass->classname );
	}
}

/*
* Cmd_Say_f
*/
void Cmd_Say_f( gentity_t *ent, qboolean arg0, qboolean checkflood )
{
	char *p;
	char text[2048];

	if( checkflood )
	{
		if( G_CheckFlood( ent ) )
			return;
	}

	if( trap_Cmd_Argc() < 2 && !arg0 )
		return;

	Q_snprintfz( text, sizeof( text ), "%s%s: ", ent->client->netname, S_COLOR_GREEN );

	if( arg0 )
	{
		Q_strncatz( text, trap_Cmd_Argv( 0 ), sizeof( text ) );
		Q_strncatz( text, " ", sizeof( text ) );
		Q_strncatz( text, trap_Cmd_Args(), sizeof( text ) );
	}
	else
	{
		p = trap_Cmd_Args();

		if( *p == '"' )
		{
			if( p[strlen( p )-1] == '"' )
				p[strlen( p )-1] = 0;
			p++;
		}
		Q_strncatz( text, p, sizeof( text ) );
	}

	// don't let text be too long for malicious reasons
	if( strlen( text ) > 150 )
		text[150] = 0;

	Q_strncatz( text, "\n", sizeof( text ) );

	G_ChatMsg( NULL, "%s", text );
}

/*
* Cmd_SayCmd_f
*/
static void Cmd_SayCmd_f( gentity_t *ent )
{
	Cmd_Say_f( ent, qfalse, qtrue );
}

/*
* Cmd_SayTeam_f
*/
static void Cmd_SayTeam_f( gentity_t *ent )
{
	G_Say_Team( ent, trap_Cmd_Args(), qtrue );
}

/*
* Cmd_Origin_f
*/
static void Cmd_Origin_f( gentity_t *ent )
{
	if( G_CheckCheat( ent ) )
		return;
	G_PrintMsg( ent, "origin: %f, %f, %f\n", ent->s.ms.origin[0], ent->s.ms.origin[1], ent->s.ms.origin[2] );
}

//==================
//Cmd_CvarInfo_f - Contains a cvar name and string provided by the client
//==================
static void Cmd_CvarInfo_f( gentity_t *ent )
{
	if( trap_Cmd_Argc() < 2 )
	{
		G_PrintMsg( ent, "Cmd_CvarInfo_f: invalid argument count\n" );
		return;
	}

	GS_Printf( "%s's cvar '%s' is '%s'\n", ent->client->netname, trap_Cmd_Argv( 1 ), trap_Cmd_Argv( 2 ) );
}

//===========================================================
//	client commands
//===========================================================
typedef struct
{
	char name[MAX_QPATH];
	void ( *func )( gentity_t *ent );
} g_gamecommands_t;

g_gamecommands_t g_Commands[MAX_GAMECOMMANDS];

/*
* G_AddCommand
*/
void G_AddCommand( char *name, void (*callback)(gentity_t *) )
{
	int i;
	char temp[MAX_QPATH];

	Q_strncpyz( temp, name, sizeof( temp ) );

	//see if we already had it in game side
	for( i = 0; i < MAX_GAMECOMMANDS; i++ )
	{
		if( !g_Commands[i].name[0] )
			break;
		if( !Q_stricmp( g_Commands[i].name, temp ) )
		{
			// update func if different
			if( g_Commands[i].func != callback )
				g_Commands[i].func = callback;
			return;
		}
	}

	if( i == MAX_GAMECOMMANDS )
	{
		GS_Error( "G_AddCommand: Couldn't find a free g_Commands spot for the new command. (increase MAX_GAMECOMMANDS)\n" );
		return;
	}

	// we don't have it, add it
	g_Commands[i].func = callback;
	strcpy( g_Commands[i].name, temp );
	trap_ConfigString( CS_GAMECOMMANDS + i, g_Commands[i].name );
}

/*
* G_InitGameCommands
*/
void G_InitGameCommands( void )
{
	int i;
	for( i = 0; i < MAX_GAMECOMMANDS; i++ )
	{
		g_Commands[i].func = NULL;
		g_Commands[i].name[0] = 0;
	}

	G_AddCommand( "use", Cmd_UseWeapon_f );
	G_AddCommand( "origin", Cmd_Origin_f );
	G_AddCommand( "say", Cmd_SayCmd_f );
	G_AddCommand( "say_team", Cmd_SayTeam_f );
	G_AddCommand( "spec", Cmd_Spec_f );
	G_AddCommand( "noclip", Cmd_NoClip_f );
	G_AddCommand( "wnext", Cmd_NextWeapon_f );
	G_AddCommand( "wprev", Cmd_PrevWeapon_f );
	G_AddCommand( "join", G_Teams_Join_Cmd );
	G_AddCommand( "class", Cmd_Class_f );

	// temp stuff for testing
	G_AddCommand( "fakeclient", Cmd_FakeClient_f );
	G_AddCommand( "cvarinfo", Cmd_CvarInfo_f );
}

/*
* G_ClientCommand
*/
void G_ClientCommand( int clientNum )
{
	gentity_t *ent;
	char *cmd;
	int i;

	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "G_ClientDisconnect: Bad entity number" );

	if( trap_GetClientState( clientNum ) < CS_SPAWNED )
		return;

	ent = &game.entities[clientNum];

	cmd = trap_Cmd_Argv( 0 );

	for( i = 0; i < MAX_GAMECOMMANDS; i++ )
	{
		if( !g_Commands[i].name[0] )
			break;
		if( !Q_stricmp( g_Commands[i].name, cmd ) )
		{
			if( g_Commands[i].func )
				g_Commands[i].func( ent );
			return;
		}
	}

	// unknown as a command. Echo it as msg to the player
	G_PrintMsg( ent, "unknown command: %s\n", trap_Cmd_Argv( 0 ) );
}
