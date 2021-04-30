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

#include "g_local.h"


typedef struct
{
	char *name;
	void ( *spawn )( gentity_t *ent );
} spawn_t;

static void G_WorldSpawn( gentity_t *ent );

void G_Func_BrushModel( gentity_t *ent );
void G_Func_Plat( gentity_t *ent );
void G_Func_Door( gentity_t *ent );
void G_Func_Door_Rotating( gentity_t *ent );
void G_Func_Button( gentity_t *ent );
void G_Func_Rotating( gentity_t *ent );

spawn_t	spawns[] = {
	{ "worldspawn", G_WorldSpawn },
	{ "info_player_start", G_info_player_start },
	{ "info_player_deathmatch", G_info_player_start },
	{ "info_spawnpoint", G_Spawnpoint_Spawn },

	// triggers
	{ "trigger_multiple", G_Trigger_Spawn },
	{ "trigger_waterblend", G_Trigger_WaterBlend_Spawn },

	// brush models
	{ "func_plat", G_Func_Plat },
	{ "func_brushmodel", G_Func_BrushModel },
	{ "func_group", G_FreeEntity },
	{ "func_door", G_Func_Door },
	{ "func_door_rotating", G_Func_Door_Rotating },
	{ "func_button", G_Func_Button },
	{ "func_rotating", G_Func_Rotating },

	// targets

	// misc
	{ "misc_model", G_FreeEntity },
	{ "misc_portal_surface", G_misc_portal_surface },
	{ "misc_portal_camera", G_misc_portal_camera },
	{ "misc_skyportal", G_skyportal },
	{ "props_skyportal", G_skyportal },

	{ NULL, NULL }
};

/*
* ED_NewString : FIXME: LEGACY
*/
static char *ED_NewString( const char *string )
{
	char *newb, *new_p;
	size_t i, l;

	l = strlen( string ) + 1;
	newb = ( char * )G_LevelMalloc( l );

	new_p = newb;

	for( i = 0; i < l; i++ )
	{
		if( string[i] == '\\' && i < l-1 )
		{
			i++;
			if( string[i] == 'n' )
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '/';
				*new_p++ = string[i];
			}
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}

/*
* G_GetEntitySpawnKey
*/
const char *G_GetEntitySpawnKey( const char *key, gentity_t *self )
{
	static char value[MAX_TOKEN_CHARS];
	char keyname[MAX_TOKEN_CHARS];
	char *token;
	const char *data = NULL;

	value[0] = 0;

	if( self )
		data = self->spawnString;

	if( data && data[0] && key && key[0] )
	{
		// get the opening bracket
		token = COM_Parse( &data );

		if( token[0] != '{' )
			GS_Error( "G_GetEntitySpawnKey: Expecting '{' instead of '%s'\n", token );

		// go through all the dictionary pairs
		while( 1 )
		{
			// parse key
			token = COM_Parse( &data );
			if( token[0] == '}' )
				break;

			if( !data )
				GS_Error( "G_GetEntitySpawnKey: EOF without closing brace" );

			Q_strncpyz( keyname, token, sizeof( keyname ) );

			// parse value
			token = COM_Parse( &data );
			if( !data )
				GS_Error( "G_GetEntitySpawnKey: EOF without closing brace" );

			if( token[0] == '}' )
				GS_Error( "G_GetEntitySpawnKey: closing brace without data" );

			if( !Q_stricmp( key, keyname ) )
			{
				Q_strncpyz( value, token, sizeof( value ) );
				break;
			}
		}
	}

	return value;
}

/*
* G_SpawnEntity
*/
qboolean G_SpawnEntity( gentity_t *ent )
{
	const char *token;
	spawn_t	*s;
	gsitem_t *item;

	// FIXME: This method is slow. Just using it now for simplicity.

	// classname
	token = G_GetEntitySpawnKey( "classname", ent );
	if( token[0] )
	{
		// worldspawn entities are special and can be only one of them
		if( !Q_stricmp( token, "worldspawn" ) && ent->s.number != ENTITY_WORLD )
			return qfalse;

		ent->classname = ED_NewString( token );
	}
	else // this is impossible to happen
		GS_Error( "G_ParseEntity: Entity without a classname defined\n" );

	// origin
	token = G_GetEntitySpawnKey( "origin", ent );
	if( token[0] )
		sscanf( token, "%f %f %f", &ent->s.ms.origin[0], &ent->s.ms.origin[1], &ent->s.ms.origin[2] );

	// angles
	token = G_GetEntitySpawnKey( "angles", ent );
	if( token[0] )
		sscanf( token, "%f %f %f", &ent->s.ms.angles[0], &ent->s.ms.angles[1], &ent->s.ms.angles[2] );

	// model
	token = G_GetEntitySpawnKey( "model", ent );
	if( token[0] )
		ent->model = ED_NewString( token );

	// model2
	token = G_GetEntitySpawnKey( "model2", ent );
	if( token[0] )
		ent->model2 = ED_NewString( token );

	// spawnflags
	token = G_GetEntitySpawnKey( "spawnflags", ent );
	if( token[0] )
		ent->spawnflags = atoi( token );

	// target
	token = G_GetEntitySpawnKey( "target", ent );
	if( token[0] )
		ent->target = ED_NewString( token );

	// targetname
	token = G_GetEntitySpawnKey( "targetname", ent );
	if( token[0] )
		ent->targetname = ED_NewString( token );

	// killtarget
	token = G_GetEntitySpawnKey( "killtarget", ent );
	if( token[0] )
		ent->killtarget = ED_NewString( token );

	// message
	token = G_GetEntitySpawnKey( "message", ent );
	if( token[0] )
		ent->message = ED_NewString( token );

	// wait
	token = G_GetEntitySpawnKey( "wait", ent );
	if( token[0] )
		ent->wait = atof( token );

	// mass
	token = G_GetEntitySpawnKey( "mass", ent );
	if( token[0] )
		ent->mass = atoi( token );

	// count
	token = G_GetEntitySpawnKey( "count", ent );
	if( token[0] )
		ent->count = atoi( token );

	// health
	token = G_GetEntitySpawnKey( "health", ent );
	if( token[0] )
		ent->health = atoi( token );

	// damage
	token = G_GetEntitySpawnKey( "damage", ent );
	if( token[0] )
		ent->damage = atoi( token );

	// team
	token = G_GetEntitySpawnKey( "team", ent );
	ent->s.team = GS_Teams_TeamFromName( token );
	if( ent->s.team == -1 )
		ent->s.team = TEAM_NOTEAM;

	// delay
	token = G_GetEntitySpawnKey( "delay", ent );
	if( token[0] )
		ent->damage = atof( token );

	// gametype inhibition key
	token = G_GetEntitySpawnKey( "gametype", ent );
	if( token[0] )
	{
		//if( !strstr( gametype, gs.gametypeName ) )
		//	return qfalse;
	}

	// no need to continue with the worldspawn
	//if( ent == worldEntity ) 
	//	return qtrue;

	// FIXME: Make item entities use normal spawn functions
	if( ( item = GS_FindItemByClassname( ent->classname ) ) != NULL )
	{
		// item entities may be inhibited by the gametype
		if( !( level.gametype.spawnableItemsMask & item->type ) )
			return qfalse;

		if( !( item->flags & ITFLAG_PICKABLE ) )
			return qfalse;

		G_Item_Spawn( ent, item );

		return qtrue; // do not try to call spawn function for items
	}

	// proceed with calling the spawn function of this entity
	for( s = spawns; s->name; s++ )
	{
		if( !Q_stricmp( s->name, ent->classname ) )
		{
			s->spawn( ent );
			return qtrue;
		}
	}

	// see if there's a spawn definition in the gametype scripts
//	if( G_asCallMapEntitySpawnScript( ent->classname, ent ) )
//		return qtrue; // handled by the script

	if( sv_cheats->integer || developer->integer ) // mappers load their maps with devmap
		GS_Printf( "%s doesn't have a spawn function\n", ent->classname );

	return qfalse;
}

/*
* G_ParseEntity
*/
qboolean G_ParseEntity( const char **entitiesData )
{
	char keyName[MAX_TOKEN_CHARS];
	const char *token;
	int keysFound = 0;

	assert( entitiesData && *entitiesData );

	// get the opening bracket
	token = COM_Parse( entitiesData );

	if( token[0] != '{' )
		GS_Error( "G_ParseEntity: Expecting '{' instead of '%s'\n", token );

	// skip the rest of the entity up to the closing bracket,
	// but verify tokens come in key/content pairs.
	while( 1 )
	{
		// key name or closing bracket
		token = COM_Parse( entitiesData );

		if( token[0] == '}' ) // got closing bracket in the right place
			break;

		if( !token[0] )
			GS_Error( "G_ParseEntity: End of file without a closing bracket\n" );

		Q_strncpyz( keyName, token, sizeof( keyName ) );

		// key content
		token = COM_Parse( entitiesData );

		if( token[0] == '}' )
			GS_Error( "G_ParseEntity: Expecting key '%s' content instead of '}'\n", keyName );

		if( !token[0] )
			GS_Error( "G_ParseEntity: Expecting key '%s' content and found end of file\n", keyName );

		keysFound++;
	}

	if( !keysFound ) // if the entity has no content, skip it
	{
		GS_Printf( "WARNING: G_ParseEntity: Entity had no content defined. Inhibited.\n" );
		return qfalse;
	}

	// we have verified the entity definition, now we can spawn it
	return qtrue;
}

/*
* G_WorldSpawn
*/
void G_WorldSpawn( gentity_t *ent )
{
	int gravity;
	int farplanedist;
	const char *s;

	ent->s.ms.type = MOVE_TYPE_PUSHER;
	ent->s.solid = SOLID_SOLID;
	ent->s.cmodeltype = CMODEL_BRUSH;
	VectorClear( ent->s.ms.origin );
	VectorClear( ent->s.ms.angles );
	GClip_SetBrushModel( ent, "*0" );
	trap_PureModel( "*0" );

	farplanedist = atof( G_GetEntitySpawnKey( "farplanedist", ent ) );
	if( farplanedist > 0 || g_minculldistance->integer )
	{
		if( g_minculldistance->integer )
			level.farplanedist = max( 32, g_minculldistance->integer );
		else
			level.farplanedist = max( 512, farplanedist );
	}

	s = G_GetEntitySpawnKey( "music", ent );
	if( s[0] )
		trap_ConfigString( CS_AUDIOTRACK, s );

	gravity = (int)gs.environment.gravity;
	s = G_GetEntitySpawnKey( "gravity", ent );
	if( atoi( s ) > 0 )
		gravity = atoi( s );

	trap_ConfigString( CS_GRAVITY, va( "%i %f %f %f", gravity, gs.environment.gravityDir[0], gs.environment.gravityDir[1], gs.environment.gravityDir[2] ) );

	// fixme? : a perfect match is required between client and server,
	// so get it back with the same imprecision it has in the client
	sscanf( trap_GetConfigString( CS_GRAVITY ), "%i %f %f %f", &gravity, &gs.environment.gravityDir[0], &gs.environment.gravityDir[1], &gs.environment.gravityDir[2] );
	gs.environment.gravity = gravity;
	VectorNormalizeFast( gs.environment.gravityDir );
}
