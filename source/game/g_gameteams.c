/*
Copyright (C) 2007 German Garcia
 */

#include "g_local.h"

cvar_t *g_teams_maxplayers;

//=================
//
//=================
void G_Teams_Init( void )
{
	g_teams_maxplayers = trap_Cvar_Get( "g_teams_maxplayers", "6", CVAR_ARCHIVE );
}

enum
{
	ER_TEAM_OK,
	ER_TEAM_UNKNOWN,
	ER_TEAM_INVALID,
	ER_TEAM_CURRENT,
	ER_TEAM_FULL
};

//=================
//G_GameTypes_DenyJoinTeam
//=================
static int G_Teams_DenyJoinTeam( gentity_t *ent, int team )
{
	if( team < 0 || team >= GS_NUMTEAMS )
	{
		return ER_TEAM_UNKNOWN;
	}

	if( team == ent->client->team )
		return ER_TEAM_CURRENT;

	if( GS_TeamBased() && team == TEAM_NOTEAM )
		return ER_TEAM_INVALID;

	return ER_TEAM_OK;
}

//=================
//G_Teams_JoinTeam - checks that client can join the given team and then joins it
//=================
qboolean G_Teams_JoinTeam( gentity_t *ent, int team )
{
	int error;

	if( !ent->client )
		return qfalse;

	if( ( error = G_Teams_DenyJoinTeam( ent, team ) ) )
	{
		switch( error )
		{
		default:
			GS_Printf( "WARNING: G_Teams_JoinTeamTeam got an unknown error code\n" );
		case ER_TEAM_UNKNOWN:
			G_PrintMsg( ent->client, "Unknown team name\n" );
			break;
		case ER_TEAM_INVALID:
			G_PrintMsg( ent->client, "Team %s is not a valid option\n", GS_TeamName( team ) );
			break;
		case ER_TEAM_FULL:
			G_PrintMsg( ent->client, "Team %s is FULL\n", GS_TeamName( team ) );
			break;
		case ER_TEAM_CURRENT:
			G_PrintMsg( ent->client, "Team %s is your current team\n", GS_TeamName( team ) );
			break;
		}

		return qfalse;
	}

	// ok, can join, proceed
	ent->client->team = team;

	// when joining a team always assign the default playerclass
	ent->client->playerClassIndex = GS_PlayerClassByName( "soldier" )->index;

	// temporarily ghost him
	G_Client_Respawn( ent, qtrue );
	G_SpawnQueue_AddClient( ent );
	return qtrue;
}

//=================
//G_Teams_JoinAnyTeam - find us a team since we are too lazy to do ourselves
//=================
static qboolean G_Teams_JoinAnyTeam( gentity_t *ent, qboolean silent )
{
	int team;

	// by now just try all
	for( team = TEAM_NOTEAM; team < GS_NUMTEAMS; team++ )
	{
		if( G_Teams_JoinTeam( ent, team ) )
		{
			if( !silent )
			{
				G_PrintMsg( NULL, "%s%s joined the %s team.\n", ent->client->netname,
				           S_COLOR_WHITE, GS_TeamName( ent->client->team ) );
			}
			return qtrue;
		}
	}

	return qfalse;
}

//=================
//G_Teams_Join_Cmd
//=================
void G_Teams_Join_Cmd( gentity_t *ent )
{
	char *t;
	int team;

	t = trap_Cmd_Argv( 1 );
	if( !t || *t == 0 )
	{
		G_Teams_JoinAnyTeam( ent, qfalse );
		return;
	}

	team = GS_Teams_TeamFromName( t );

	if( G_Teams_JoinTeam( ent, team ) )
	{
		G_PrintMsg( NULL, "%s%s joined the %s%s team.\n", ent->client->netname, S_COLOR_WHITE,
			GS_TeamName( ent->client->team ), S_COLOR_WHITE );
	}
}

//======================================================================
//
//TEAM COMMUNICATIONS
//
//======================================================================

void G_Say_Team( gentity_t *who, char *msg, qboolean checkflood )
{
	char outmsg[256];
	char buf[256];
	char *p;
	gentity_t *cl_ent;
	char current_color[3];

	if( checkflood )
	{
		if( G_CheckFlood( who ) )
			return;
	}

	if( !GS_TeamBased() )
	{
		Cmd_Say_f( who, qfalse, qfalse );
		return;
	}

	Q_strncpyz( current_color, S_COLOR_WHITE, sizeof( current_color ) );

	memset( outmsg, 0, sizeof( outmsg ) );

	if( *msg == '\"' )
	{
		msg[strlen( msg ) - 1] = 0;
		msg++;
	}

	for( p = outmsg; *msg && ( p - outmsg ) < sizeof( outmsg ) - 3; msg++ )
	{
		if( *msg == '%' )
		{
			buf[0] = 0;
			switch( *++msg )
			{
			case '%':
				*p++ = *msg;
				break;
			default:
				// Maybe add a warning here?
				*p++ = '%';
				*p++ = *msg;
			}
			if( strlen( buf ) + ( p - outmsg ) < sizeof( outmsg ) - 3 )
			{
				Q_strncatz( outmsg, buf, sizeof( outmsg ) );
				p += strlen( buf );
			}
		}
		else if( *msg == '^' )
		{
			*p++ = *msg++;
			*p++ = *msg;
			Q_strncpyz( current_color, p-2, sizeof( current_color ) );
		}
		else
		{
			*p++ = *msg;
		}
	}
	*p = 0;

	for( cl_ent = game.entities; ENTNUM( cl_ent ) < gs.maxclients; cl_ent++ )
	{
		if( !cl_ent->s.local.inuse )
			continue;

		if( cl_ent->s.team == who->s.team )
		{
			G_ChatMsg( cl_ent->client, "%s[TEAM]%s %s%s: %s\n", S_COLOR_YELLOW, S_COLOR_WHITE, who->client->netname,
			           S_COLOR_YELLOW, outmsg );
		}
	}
}
