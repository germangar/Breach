/*
 */

#include "gs_local.h"

//==================================================
//
//		TEAMS
//
//==================================================

typedef struct
{
	const char *name;
	byte_vec4_t color;
} gs_teamdef_t;

static gs_teamdef_t gs_teamDefs[] =
{
	{
		"WORLD",
		{ 255, 255, 255, 255 },
	},

	{
		"ALPHA",
		{ 255, 0, 0, 255 },
	},

	{
		"BETA",
		{ 0, 0, 255, 255 },
	},

	{
		"SPECTATOR",
		{ 255, 255, 255, 255 },
	}
};

/*
* GS_Teams_Init
* Just a security check to prevent programing mistakes
*/
void GS_Teams_Init( void )
{
#define NUMTEAMS ( sizeof( gs_teamDefs )/sizeof( gs_teamdef_t ) )
	assert( NUMTEAMS == GS_NUMTEAMS );
	assert( NUMTEAMS <= MAX_TEAMS );
#undef NUMTEAMS
}

/*
* GS_TeamName
*/
const char *GS_TeamName( int team )
{
	assert( team >= 0 && team < GS_NUMTEAMS );
	return gs_teamDefs[team].name;
}

/*
* GS_TeamColor
*/
int GS_TeamColor( int team )
{
	assert( team >= 0 && team < GS_NUMTEAMS );
	return COLOR_RGBA( gs_teamDefs[team].color[0], gs_teamDefs[team].color[1], gs_teamDefs[team].color[2], gs_teamDefs[team].color[3] );
	//return (int *)gs_teamDefs[team].color;
}

/*
* GS_Teams_TeamFromName
*/
int GS_Teams_TeamFromName( const char *teamname )
{
	const char *s;
	int i;

	if( !teamname || !teamname[0] )
		return -1; //invalid

	for( i = 0; i < GS_NUMTEAMS; i++ )
	{
		s = gs_teamDefs[i].name;
		if( !Q_stricmp( s, teamname ) )
			return i;
	}
	return -1; //invalid
}

/*
* GS_IsTeamDamage
*/
qboolean GS_IsTeamDamage( entity_state_t *targ, entity_state_t *attacker )
{
	if( !GS_TeamBased() )
		return qfalse;

	if( targ->team && attacker->team &&
		targ->team == attacker->team &&
		targ->number != attacker->number )
		return qtrue;

	return qfalse;
}