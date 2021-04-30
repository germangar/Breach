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
// g_utils.c -- misc utility functions for game module

#include "g_local.h"


static qbyte	*levelpool = NULL;			// Vic: I'm tempted to rename this to liverpool...
static size_t	levelpool_size = 0;
static size_t	levelpool_pointer = 0;

/*
* G_LevelInitPool
*/
void G_LevelInitPool( size_t size )
{
	G_LevelFreePool();

	if( !size )
		size = levelpool_size;
	assert( size );

	levelpool = ( qbyte * )G_Malloc( size );
	memset( levelpool, 0, size );

	levelpool_size = size;
	levelpool_pointer = 0;
}

/*
* G_LevelFreePool
*/
void G_LevelFreePool( void )
{
	if( levelpool )
	{
		G_Free( levelpool );
		levelpool = NULL;
	}
}

//=============
//G_LevelMemInit
// Note that we never release allocated memory
//=============
void *_G_LevelMalloc( size_t size, const char *filename, int fileline )
{
	qbyte *pointer;

	if( levelpool_pointer + size > levelpool_size ) {
		GS_Error( "G_LevelMalloc: out of memory (alloc %i bytes at %s:%i)\n", size, filename, fileline );
		return NULL;
	}

	pointer = levelpool + levelpool_pointer;
	levelpool_pointer += size;

	return ( void * )pointer;
}

//=============
//G_LevelFree
//=============
void _G_LevelFree( void *data, const char *filename, int fileline )
{
}

//=============
//G_Find
//
//Searches all active entities for the next one that holds
//the matching string at fieldofs (use the FOFS() macro) in the structure.
//
//Searches beginning at the entity after from, or the beginning if NULL
//NULL will be returned if the end of the list is reached.
//
//=============
gentity_t *G_Find( gentity_t *from, size_t fieldofs, char *match )
{
	char *s;

	if( !from )
		from = game.entities;
	else
		from++;

	for(; from < &game.entities[game.numentities]; from++ )
	{
		if( !from->s.local.inuse )
			continue;
		if( from == worldEntity )
			continue;
		s = *(char **) ( (qbyte *)from + fieldofs );
		if( !s )
			continue;
		if( !Q_stricmp( s, match ) )
			return from;
	}

	return NULL;
}

//=================
//findradius
//
//Returns entities that have origins within a spherical area
//
//findradius (origin, radius)
//=================
gentity_t *findradius( gentity_t *from, vec3_t org, float rad )
{
	vec3_t eorg;
	int j;

	if( !from )
		from = worldEntity;
	else
		from++;

	for(; from < &game.entities[game.numentities]; from++ )
	{
		if( !from->s.local.inuse )
			continue;
		if( from->s.solid == SOLID_NOT )
			continue;
		if( from == worldEntity )
			continue;
		for( j = 0; j < 3; j++ )
			eorg[j] = org[j] - ( from->s.ms.origin[j] + ( from->s.local.mins[j] + from->s.local.maxs[j] ) * 0.5 );
		if( VectorLengthFast( eorg ) > rad )
			continue;
		return from;
	}

	return NULL;
}

//=================
//G_FindBoxInRadius
//Returns entities that have their boxes within a spherical area
//=================
gentity_t *G_FindBoxInRadius( gentity_t *from, vec3_t org, float rad )
{
	int j;
	vec3_t mins, maxs;

	if( !from )
		from = game.entities;
	else
		from++;

	for(; from < &game.entities[game.numentities]; from++ )
	{
		if( !from->s.local.inuse )
			continue;
		if( from->s.solid == SOLID_NOT )
			continue;
		if( from == worldEntity )
			continue;
		// make absolute mins and maxs
		for( j = 0; j < 3; j++ )
		{
			mins[j] = from->s.ms.origin[j] + from->s.local.mins[j];
			maxs[j] = from->s.ms.origin[j] + from->s.local.maxs[j];
		}
		if( !BoundsAndSphereIntersect( mins, maxs, org, rad ) )
			continue;

		return from;
	}

	return NULL;
}

//=============
//G_PickTarget
//
//Searches all active entities for the next one that holds
//the matching string at fieldofs (use the FOFS() macro) in the structure.
//
//Searches beginning at the entity after from, or the beginning if NULL
//NULL will be returned if the end of the list is reached.
//
//=============
#define MAXCHOICES  8

gentity_t *G_PickTarget( char *targetname )
{
	gentity_t *ent = NULL;
	int num_choices = 0;
	gentity_t *choice[MAXCHOICES];

	if( !targetname )
	{
		GS_Printf( "G_PickTarget called with NULL targetname\n" );
		return NULL;
	}

	while( 1 )
	{
		ent = G_Find( ent, FOFS( targetname ), targetname );
		if( !ent )
			break;
		choice[num_choices++] = ent;
		if( num_choices == MAXCHOICES )
			break;
	}

	if( !num_choices )
	{
		GS_Printf( "G_PickTarget: target %s not found\n", targetname );
		return NULL;
	}

	return choice[rand() % num_choices];
}

vec3_t VEC_UP	    = { 0, -1, 0 };
vec3_t MOVEDIR_UP   = { 0, 0, 1 };
vec3_t VEC_DOWN	    = { 0, -2, 0 };
vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

void G_SetMovedir( vec3_t angles, vec3_t movedir )
{
	if( VectorCompare( angles, VEC_UP ) )
	{
		VectorCopy( MOVEDIR_UP, movedir );
	}
	else if( VectorCompare( angles, VEC_DOWN ) )
	{
		VectorCopy( MOVEDIR_DOWN, movedir );
	}
	else
	{
		AngleVectors( angles, movedir, NULL, NULL );
	}

	VectorClear( angles );
}


float vectoyaw( vec3_t vec )
{
	float yaw;

	if( vec[PITCH] == 0 )
	{
		yaw = 0;
		if( vec[YAW] > 0 )
			yaw = 90;
		else if( vec[YAW] < 0 )
			yaw = -90;
	}
	else
	{
		yaw = RAD2DEG( atan2( vec[YAW], vec[PITCH] ) );
		if( yaw < 0 )
			yaw += 360;
	}

	return yaw;
}

//=================
//G_Spawn
//
//Either finds a free entity, or allocates a new one.
//Try to avoid reusing an entity that was recently freed, because it
//can cause the client to think the entity morphed into something else
//instead of being removed and recreated, which can cause interpolated
//angles and bad trails.
//=================
gentity_t *G_Spawn( void )
{
	int i;
	gentity_t *e;

	assert( level.canSpawnEntities );

	e = &game.entities[gs.maxclients];
	for( i = gs.maxclients; i < game.numentities; i++, e++ )
	{
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if( !e->s.local.inuse && e != worldEntity && ( level.mapTimeStamp == game.serverTime || trap_Milliseconds() > e->freetimestamp + 400 ) )
		{
			G_InitEntity( e );
			return e;
		}
	}

	if( i == MAX_EDICTS )
		GS_Error( "G_Spawn: no free edicts" );

	if( &game.entities[game.numentities] == worldEntity )
		GS_Error( "G_Spawn: no free edicts" );

	game.numentities++;
	G_InitEntity( e );

	return e;
}

//============
//G_AddEvent
//
//============
void G_AddEvent( gentity_t *ent, int event, int parm, qboolean highPriority )
{
	if( !ent || ent == worldEntity || !ent->s.local.inuse )
	{
		return;
	}
	if( !event )
	{
		return;
	}
	// replace the most outdated low-priority event
	if( !highPriority )
	{
		int oldEventNum = -1;

		if( !ent->eventPriority[0] && !ent->eventPriority[1] )
		{
			oldEventNum = ( ent->numEvents + 1 ) & 2;
		}
		else if( !ent->eventPriority[0] )
		{
			oldEventNum = 0;
		}
		else if( !ent->eventPriority[1] )
		{
			oldEventNum = 1;
		}

		// no luck
		if( oldEventNum == -1 )
		{
			return;
		}

		ent->s.events[oldEventNum] = event;
		ent->s.eventParms[oldEventNum] = parm;
		ent->eventPriority[oldEventNum] = qfalse;
		return;
	}

	ent->s.events[ent->numEvents & 1] = event;
	ent->s.eventParms[ent->numEvents & 1] = parm;
	ent->eventPriority[ent->numEvents & 1] = highPriority;
	ent->numEvents++;
}

//============
//G_SpawnEvent
//
//============
gentity_t *G_SpawnEvent( int event, int parm, vec3_t origin )
{
	gentity_t *ent;

	ent = G_Spawn();
	ent->s.type = ET_EVENT;
	ent->s.solid = SOLID_NOT;
	ent->s.cmodeltype = CMODEL_NOT;
	ent->netflags &= ~SVF_NOCLIENT;
	if( origin != NULL )
		VectorCopy( origin, ent->s.ms.origin );
	ent->think = NULL;
	ent->pain = NULL;
	ent->die = NULL;
	ent->activate = NULL;
	ent->touch = NULL;
	ent->clearSnap = G_FreeEntity;
	ent->closeSnap = NULL;

	G_AddEvent( ent, event, parm, qtrue );

	GClip_LinkEntity( ent );

	return ent;
}

//============
//G_TurnEntityIntoEvent
//============
void G_TurnEntityIntoEvent( gentity_t *ent, int event, int parm )
{
	ent->s.type = ET_EVENT;
	ent->s.solid = SOLID_NOT;
	ent->s.cmodeltype = CMODEL_NOT;
	ent->think = NULL;
	ent->pain = NULL;
	ent->die = NULL;
	ent->activate = NULL;
	ent->touch = NULL;
	ent->clearSnap = G_FreeEntity;
	ent->closeSnap = NULL;

	G_AddEvent( ent, event, parm, qtrue );

	GClip_LinkEntity( ent );
}


//============
//G_PrintMsg
//
//NULL sends to all the message to all clients
//============
void G_PrintMsg( gentity_t *ent, const char *format, ... )
{
	char msg[1024];
	va_list	argptr;
	char *s, *p;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	// double quotes are bad
	p = msg;
	while( ( p = strchr( p, '\"' ) ) != NULL )
		*p = '\'';

	s = va( "pr \"%s\"", msg );

	if( !ent )
	{
		int i;

		for( i = 0; i < gs.maxclients; i++ )
		{
			ent = game.entities + i;
			if( !ent->s.local.inuse )
				continue;
			if( !ent->client )
				continue;
			trap_ServerCmd( ENTNUM( ent ), s );
		}

		// mirror at server console
		if( dedicated->integer )
			GS_Printf( "%s", msg );
		return;
	}

	if( ent->s.local.inuse && ent->client )
		trap_ServerCmd( ENTNUM( ent ), s );
}

//============
//G_ChatMsg
//
//NULL sends to all the message to all clients
//============
void G_ChatMsg( gentity_t *ent, const char *format, ... )
{
	char msg[1024];
	va_list	argptr;
	char *s, *p;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	// double quotes are bad
	p = msg;
	while( ( p = strchr( p, '\"' ) ) != NULL )
		*p = '\'';

	s = va( "ch \"%s\"", msg );

	if( !ent )
	{
		int i;
		for( i = 0; i < gs.maxclients; i++ )
		{
			ent = game.entities + i;
			if( !ent->s.local.inuse )
				continue;
			if( trap_GetClientState( ENTNUM( ent ) ) < CS_SPAWNED )
				continue;
			trap_ServerCmd( ENTNUM( ent ), s );
		}

		// mirror at server console
		if( dedicated->integer )
			GS_Printf( "%s", msg );
		return;
	}

	if( ent->s.local.inuse && trap_GetClientState( ENTNUM( ent ) ) >= CS_SPAWNED )
		trap_ServerCmd( ENTNUM( ent ), s );
}

//============
//G_CenterPrintMsg
//
//NULL sends to all the message to all clients
//============
void G_CenterPrintMsg( gentity_t *ent, const char *format, ... )
{
	char msg[1024];
	va_list	argptr;
	char *p;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	// double quotes are bad
	p = msg;
	while( ( p = strchr( p, '\"' ) ) != NULL )
		*p = '\'';

	trap_ServerCmd( ENTNUM( ent ), va( "cp \"%s\"", msg ) );
}

//==============================================================================
//
//Kill box
//
//==============================================================================

//=================
//KillBox
//
//Kills all entities that would touch the proposed new positioning
//of ent.  Ent should be unlinked before calling this!
//=================
qboolean KillBox( gentity_t *ent )
{
	trace_t	tr;

	while( 1 )
	{
		GS_Trace( &tr, ent->s.ms.origin, ent->s.local.mins, ent->s.local.maxs, ent->s.ms.origin, ENTITY_WORLD, MASK_PLAYERSOLID, 0 );
		if( tr.fraction == 1.0f && !tr.startsolid )
			break;

		if( tr.ent == ENTNUM( worldEntity ) )
			return qfalse; // found the world

		if( tr.ent < 1 )
			break;

		// nail it
		G_TakeDamage( &game.entities[tr.ent], ent, ent, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );

		// if we didn't kill it, fail
		if( game.entities[tr.ent].s.solid )
			return qfalse;
	}

	return qtrue;   // all clear
}

//==============================================================================
//
//		Warsow: more miscelanea tools
//
//==============================================================================

//=============
//G_DropSpawnpointToFloor
//=============
void G_DropSpawnpointToFloor( gentity_t *ent )
{
	vec3_t start, end;
	trace_t	trace;
	gsplayerclass_t *playerClass = GS_PlayerClassByIndex( 0 );

	if( ent->spawnflags & 1 )  //  floating items flag
		return;

	VectorCopy( ent->s.ms.origin, start );
	start[2] += 1;
	VectorCopy( ent->s.ms.origin, end );
	end[2] -= 16000;

	GS_Trace( &trace, start, playerClass->mins, playerClass->maxs, end, ENTNUM( ent ), MASK_PLAYERSOLID, 0 );
	if( !trace.startsolid && trace.fraction < 1.0f )
	{
		VectorCopy( trace.endpos, ent->s.ms.origin );
	}
}

/*
* G_ListNameForPosition
*/
char *G_ListNameForPosition( const char *namesList, int position, const char separator )
{
	static char buf[MAX_STRING_CHARS];
	const char *s, *t;
	char *b;
	int count, len;

	if( !namesList )
		return NULL;

	// set up the tittle from the spinner names
	s = namesList;
	t = s;
	count = 0;
	buf[0] = 0;
	b = buf;
	while( *s && ( s = strchr( s, separator ) ) )
	{
		if( count == position )
		{
			len = s - t;
			if( len <= 0 )
				GS_Error( "G_NameInStringList: empty name in list\n" );
			if( len > MAX_STRING_CHARS - 1 )
				GS_Printf( "WARNING: G_NameInStringList: name is too long\n" );
			while( t <= s )
			{
				if( *t == separator || t == s )
				{
					*b = 0;
					break;
				}
				
				*b = *t;
				t++;
				b++;
			}

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

/*
* G_AllocCreateNamesList
*/
char *G_AllocCreateNamesList( const char *path, const char *extension, const char separator )
{
	char separators[2];
	char name[MAX_CONFIGSTRING_CHARS];
	char buffer[MAX_STRING_CHARS], *s, *list;
	int numfiles, i, j, found, length, fulllength;

	if( !extension || !path )
		return NULL;

	if( extension[0] != '.' || strlen( extension ) < 2 )
		return NULL;

	if( ( numfiles = trap_FS_GetFileList( path, extension, NULL, 0, 0, 0 ) ) == 0 ) 
		return NULL;

	separators[0] = separator;
	separators[1] = 0;

	//
	// do a first pass just for finding the full len of the list
	//

	i = 0;
	found = 0;
	length = 0;
	fulllength = 0;
	do 
	{
		if( ( j = trap_FS_GetFileList( path, extension, buffer, sizeof( buffer ), i, numfiles ) ) == 0 ) 
		{
			// can happen if the filename is too long to fit into the buffer or we're done
			i++;
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length + 1 ) 
		{
			length = strlen( s );

			if( strlen( path ) + 1 + length >= MAX_CONFIGSTRING_CHARS ) 
			{
				GS_Printf( "Warning: G_AllocCreateNamesList :file name too long: %s\n", s );
				continue;
			}

			Q_strncpyz( name, s, sizeof( name ) );
			COM_StripExtension( name );

			fulllength += strlen( name ) + 1;
			found++;
		}
	} while( i < numfiles );

	if( !found )
		return NULL;

	//
	// Allocate a string for the full list and do a second pass to copy them in there
	//

	fulllength += 1;
	list = G_Malloc( fulllength );

	i = 0;
	length = 0;
	do 
	{
		if( ( j = trap_FS_GetFileList( path, extension, buffer, sizeof( buffer ), i, numfiles ) ) == 0 ) 
		{
			// can happen if the filename is too long to fit into the buffer or we're done
			i++;
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length + 1 ) 
		{
			length = strlen( s );

			if( strlen( path ) + 1 + length >= MAX_CONFIGSTRING_CHARS ) 
				continue;

			Q_strncpyz( name, s, sizeof( name ) );
			COM_StripExtension( name );

			Q_strncatz( list, name, fulllength );
			Q_strncatz( list, separators, fulllength );
		}
	} while( i < numfiles );

	return list;
}

