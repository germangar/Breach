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

const field_t fields[] = {
	{ "classname", FOFS( classname ), F_LSTRING },
	{ "origin", FOFS( s.ms.origin ), F_VECTOR },
	{ "model", FOFS( model ), F_LSTRING },
	{ "model2", FOFS( model2 ), F_LSTRING },
	{ "spawnflags", FOFS( spawnflags ), F_INT },
	{ "target", FOFS( target ), F_LSTRING },
	{ "targetname", FOFS( targetname ), F_LSTRING },
	{ "killtarget", FOFS( killtarget ), F_LSTRING },
	{ "message", FOFS( message ), F_LSTRING },
	{ "wait", FOFS( wait ), F_FLOAT },
//	{ "color", FOFS( color ), F_VECTOR },
	{ "angles", FOFS( s.ms.angles ), F_VECTOR },
	{ "mass", FOFS( mass ), F_INT },
	{ "count", FOFS( count ), F_INT },
	{ "health", FOFS( health ), F_FLOAT },
	{ "dmg", FOFS( damage ), F_INT },
	{ "delay", FOFS( delay ), F_INT },

	// temp spawn vars -- only valid when the spawn function is called
	{ "lip", STOFS( lip ), F_INT, FFL_SPAWNTEMP },
	{ "distance", STOFS( distance ), F_INT, FFL_SPAWNTEMP },
	{ "radius", STOFS( radius ), F_FLOAT, FFL_SPAWNTEMP },
	{ "roll", STOFS( roll ), F_FLOAT, FFL_SPAWNTEMP },
	{ "height", STOFS( height ), F_INT, FFL_SPAWNTEMP },
	{ "phase", STOFS( phase ), F_FLOAT, FFL_SPAWNTEMP },
	{ "pausetime", STOFS( pausetime ), F_FLOAT, FFL_SPAWNTEMP },
	{ "item", STOFS( item ), F_LSTRING, FFL_SPAWNTEMP },
	{ "gravity", STOFS( gravity ), F_LSTRING, FFL_SPAWNTEMP },
	{ "music", STOFS( music ), F_LSTRING, FFL_SPAWNTEMP },
	{ "fov", STOFS( fov ), F_FLOAT, FFL_SPAWNTEMP },
	{ "minyaw", STOFS( minyaw ), F_FLOAT, FFL_SPAWNTEMP },
	{ "maxyaw", STOFS( maxyaw ), F_FLOAT, FFL_SPAWNTEMP },
	{ "minpitch", STOFS( minpitch ), F_FLOAT, FFL_SPAWNTEMP },
	{ "maxpitch", STOFS( maxpitch ), F_FLOAT, FFL_SPAWNTEMP },
	{ "nextmap", STOFS( nextmap ), F_LSTRING, FFL_SPAWNTEMP },
	{ "team", STOFS( team ), F_LSTRING, FFL_SPAWNTEMP },
	{ "shader", STOFS( shader ), F_LSTRING, FFL_SPAWNTEMP },
	{ "angle", STOFS( anglehack ), F_FLOAT, FFL_SPAWNTEMP },
	{ "speed", STOFS( speed ), F_FLOAT, FFL_SPAWNTEMP },
	{ "accel", STOFS( accel ), F_FLOAT, FFL_SPAWNTEMP },
	{ "noents", STOFS( noents ), F_INT, FFL_SPAWNTEMP },
	{ "farplanedist", STOFS( farplanedist ), F_INT, FFL_SPAWNTEMP },
	{ "scale", STOFS( scale ), F_FLOAT, FFL_SPAWNTEMP },
	{ "gametype", STOFS( gametype ), F_LSTRING, FFL_SPAWNTEMP },

	{ NULL, 0, F_INT, 0 }
};


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

//targets
void G_Target_ConsolePrint_Activate( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	Com_Printf( "%s\n", ent->message );
}
void G_Target_ConsolePrint( gentity_t *ent )
{
	ent->activate = G_Target_ConsolePrint_Activate;
	if( !ent->message )
		ent->message = "Target print";
}

void movemyangles( gentity_t *ent )
{
	int i;

	//ent->s.angles[PITCH] += aspeed * (game.frametime * 0.001f);
	//ent->s.angles[YAW] += aspeed * (game.frametime * 0.001f);
	//ent->s.angles[ROLL] += aspeed * (game.frametime * 0.001f);
	for( i = 0; i < 3; i++ )
		AngleNormalize180( ent->s.ms.angles[i] );

	ent->nextthink = level.time + 1;
	GClip_LinkEntity( ent );
}

void G_Spawn_Rotated_Box( gentity_t *ent )
{
	ent->s.type = ET_MODEL;
	ent->s.solid = SOLID_SOLID;
	ent->s.cmodeltype = CMODEL_BBOX_ROTATED;
	VectorSet( ent->s.local.mins, -st.radius, -st.radius, -st.radius );
	VectorSet( ent->s.local.maxs, st.radius, st.radius, st.radius );
	ent->s.ms.type = MOVE_TYPE_NONE;
	ent->netflags &= ~SVF_NOCLIENT;
	ent->s.modelindex1 = trap_ModelIndex( "models/items/ammo/pack/pack.md3" );
	ent->think = movemyangles;
	ent->nextthink = level.time + 1000;
	GClip_LinkEntity( ent );
}

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
	{ "target_print", G_Target_ConsolePrint }, // only used for testing

	{ "misc_model", G_FreeEntity },
	{ "misc_portal_surface", G_misc_portal_surface },
	{ "misc_portal_camera", G_misc_portal_camera },
	{ "misc_skyportal", G_skyportal },
	{ "props_skyportal", G_skyportal },

	{ "misc_rotated_box", G_Spawn_Rotated_Box }, // only used for testing

	{ NULL, NULL }
};

//===============
//G_CallSpawn
//
//Finds the spawn function for the entity and calls it
//===============
qboolean G_CallSpawn( gentity_t *ent )
{
	spawn_t	*s;
	gsitem_t *item;

	if( !ent->classname )
	{
		if( developer->integer )
			GS_Printf( "G_CallSpawn: NULL classname\n" );
		return qfalse;
	}

	// worldspawn entities are special and can be only one of them
	if( !Q_stricmp( ent->classname, "worldspawn" ) && ent->s.number != ENTITY_WORLD )
		return qfalse;

	if( ent == worldEntity )
		return qtrue;

	// check for Q3TA-style inhibition key
	if( st.gametype )
	{
		//if( !strstr( st.gametype, gs.gametypeName ) )
		//	return qfalse;
	}

	// assign a team from the team name in the editor
	if( st.team )
	{
		ent->s.team = GS_Teams_TeamFromName( st.team );
		if( ent->s.team == -1 )
			ent->s.team = TEAM_NOTEAM;
	}

	// item entities may be inhibited by the gametype
	if( ( item = GS_FindItemByClassname( ent->classname ) ) != NULL )
	{
		if( !( level.gametype.spawnableItemsMask & item->type ) )
			return qfalse;

		if( !( item->flags & ITFLAG_PICKABLE ) )
			return qfalse;

		G_Item_Spawn( ent, item );
		return qtrue;
	}

	// check normal spawn functions
	for( s = spawns; s->name; s++ )
	{
		if( !Q_stricmp( s->name, ent->classname ) )
		{
			s->spawn( ent );
			return qtrue;
		}
	}

	if( sv_cheats->integer ) // mappers load their maps with devmap
		GS_Printf( "%s doesn't have a spawn function\n", ent->classname );

	return qfalse;
}

//=============
//ED_NewString
//=============
static char *ED_NewString( char *string )
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

//===============
//ED_ParseField
//
//Takes a key/value pair and sets the binary values
//===============
static void ED_ParseField( char *key, char *value, gentity_t *ent )
{
	const field_t *f;
	qbyte *b;
	vec3_t vec;

	for( f = fields; f->name; f++ )
	{
		if( !Q_stricmp( f->name, key ) )
		{ // found it
			if( f->flags & FFL_SPAWNTEMP )
				b = (qbyte *)&st;
			else
				b = (qbyte *)ent;

			switch( f->type )
			{
			case F_LSTRING:
				*(char **)( b+f->ofs ) = ED_NewString( value );
				break;
			case F_VECTOR:
				sscanf( value, "%f %f %f", &vec[0], &vec[1], &vec[2] );
				( (float *)( b+f->ofs ) )[0] = vec[0];
				( (float *)( b+f->ofs ) )[1] = vec[1];
				( (float *)( b+f->ofs ) )[2] = vec[2];
				break;
			case F_INT:
				*(int *)( b+f->ofs ) = atoi( value );
				break;
			case F_FLOAT:
				*(float *)( b+f->ofs ) = atof( value );
				break;
			case F_IGNORE:
				break;
			default:
				break; // FIXME: Should this be error?
			}
			return;
		}
	}

	if( developer->integer )
		GS_Printf( "%s is not a field\n", key );
}

//====================
//ED_ParseEntity
//
//Parses an edict out of the given string, returning the new position
//ed should be a properly initialized empty entity.
//====================
/*static*/ const char *ED_ParseEntity( const char *data, gentity_t *ent )
{
	qboolean init;
	char keyname[256];
	char *com_token;

	init = qfalse;
	memset( &st, 0, sizeof( st ) );

	ent->s.number = ENTNUM( ent );

	// go through all the dictionary pairs
	while( 1 )
	{
		// parse key
		com_token = COM_Parse( &data );
		if( com_token[0] == '}' )
			break;
		if( !data )
			GS_Error( "ED_ParseEntity: EOF without closing brace" );

		Q_strncpyz( keyname, com_token, sizeof( keyname ) );

		// parse value
		com_token = COM_Parse( &data );
		if( !data )
			GS_Error( "ED_ParseEntity: EOF without closing brace" );

		if( com_token[0] == '}' )
			GS_Error( "ED_ParseEntity: closing brace without data" );

		init = qtrue;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded
		if( keyname[0] == '_' )
			continue;

		ED_ParseField( keyname, com_token, ent );
	}

	if( !init )
		G_FreeEntity( ent );

	return data;
}

void G_WorldSpawn( gentity_t *ent )
{
	int gravity;

	ent->s.ms.type = MOVE_TYPE_PUSHER;
	ent->s.solid = SOLID_SOLID;
	ent->s.cmodeltype = CMODEL_BRUSH;
	VectorClear( ent->s.ms.origin );
	VectorClear( ent->s.ms.angles );
	GClip_SetBrushModel( ent, "*0" );
	trap_PureModel( "*0" );

	if( st.nextmap )
		Q_strncpyz( level.nextmap, st.nextmap, sizeof( level.nextmap ) );

	if( st.farplanedist || g_minculldistance->integer )
	{
		if( g_minculldistance->integer )
			level.farplanedist = max( 32, g_minculldistance->integer );
		else
			level.farplanedist = max( 512, st.farplanedist );
	}

	if( st.music )
		trap_ConfigString( CS_AUDIOTRACK, st.music );

	gravity = (int)gs.environment.gravity;
	if( st.gravity )
	{
		if( atoi( st.gravity ) > 0 )
			gravity = atoi( st.gravity );
	}

	trap_ConfigString( CS_GRAVITY, va( "%i %f %f %f", gravity, gs.environment.gravityDir[0], gs.environment.gravityDir[1], gs.environment.gravityDir[2] ) );

	// fixme? : a perfect match is required between client and server,
	// so get it back with the same imprecision it has in the client
	sscanf( trap_GetConfigString( CS_GRAVITY ), "%i %f %f %f", &gravity, &gs.environment.gravityDir[0], &gs.environment.gravityDir[1], &gs.environment.gravityDir[2] );
	gs.environment.gravity = gravity;
	VectorNormalizeFast( gs.environment.gravityDir );
}
