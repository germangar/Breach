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

/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block.

Ported over from Quake 1 and Quake 3.
==============================================================================
*/

#define TAG_FREE	0
#define TAG_LEVEL	1

#define	ZONEID		0x1d4a11
#define MINFRAGMENT 64

typedef struct memblock_s
{
	int		size;           // including the header and possibly tiny fragments
	int     tag;            // a tag of 0 is a free block
	struct memblock_s       *next, *prev;
	int     id;        		// should be ZONEID
} memblock_t;

typedef struct
{
	int		size;		// total bytes malloced, including header
	int		count, used;
	memblock_t	blocklist;		// start / end cap for linked list
	memblock_t	*rover;
} memzone_t;

static memzone_t *levelzone;

/*
* G_Z_ClearZone
*/
static void G_Z_ClearZone( memzone_t *zone, int size )
{
	memblock_t	*block;
	
	// set the entire zone to one free block
	zone->blocklist.next = zone->blocklist.prev = block =
		(memblock_t *)( (qbyte *)zone + sizeof(memzone_t) );
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->size = size;
	zone->rover = block;
	zone->used = 0;
	zone->count = 0;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}

/*
* G_Z_Free
*/
static void G_Z_Free( void *ptr, const char *filename, int fileline )
{
	memblock_t *block, *other;
	memzone_t *zone;

	if (!ptr)
		GS_Error( "G_Z_Free: NULL pointer" );

	block = (memblock_t *) ( (qbyte *)ptr - sizeof(memblock_t));
	if( block->id != ZONEID )
		GS_Error( "G_Z_Free: freed a pointer without ZONEID (file %s at line %i)", filename, fileline );
	if( block->tag == 0 )
		GS_Error( "G_Z_Free: freed a freed pointer (file %s at line %i)", filename, fileline );

	// check the memory trash tester
	if ( *(int *)((qbyte *)block + block->size - 4 ) != ZONEID )
		GS_Error( "G_Z_Free: memory block wrote past end" );

	zone = levelzone;
	zone->used -= block->size;
	zone->count--;

	block->tag = 0;		// mark as free

	other = block->prev;
	if( !other->tag )
	{
		// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if( block == zone->rover )
			zone->rover = other;
		block = other;
	}

	other = block->next;
	if( !other->tag )
	{
		// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if( other == zone->rover )
			zone->rover = block;
	}
}

/*
* G_Z_TagMalloc
*/
static void *G_Z_TagMalloc( int size, int tag, const char *filename, int fileline )
{
	int extra;
	memblock_t *start, *rover, *new, *base;
	memzone_t *zone;

	if( !tag )
		GS_Error( "G_Z_TagMalloc: tried to use a 0 tag (file %s at line %i)", filename, fileline );

	//
	// scan through the block list looking for the first free block
	// of sufficient size
	//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = (size + 3) & ~3;		// align to 32-bit boundary

	zone = levelzone;
	base = rover = zone->rover;
	start = base->prev;

	do
	{
		if( rover == start )	// scaned all the way around the list
			return NULL;
		if( rover->tag )
			base = rover = rover->next;
		else
			rover = rover->next;
	} while( base->tag || base->size < size );

	//
	// found a block big enough
	//
	extra = base->size - size;
	if( extra > MINFRAGMENT )
	{
		// there will be a free fragment after the allocated block
		new = (memblock_t *) ((qbyte *)base + size );
		new->size = extra;
		new->tag = 0;			// free block
		new->prev = base;
		new->id = ZONEID;
		new->next = base->next;
		new->next->prev = new;
		base->next = new;
		base->size = size;
	}

	base->tag = tag;				// no longer a free block
	zone->rover = base->next;	// next allocation will start looking here
	zone->used += base->size;
	zone->count++;
	base->id = ZONEID;

	// marker for memory trash testing
	*(int *)((qbyte *)base + base->size - 4) = ZONEID;

	return (void *) ((qbyte *)base + sizeof(memblock_t));
}

/*
* G_Z_Malloc
*/
static void *G_Z_Malloc( int size, const char *filename, int fileline )
{
	void	*buf;
	
	buf = G_Z_TagMalloc( size, TAG_LEVEL, filename, fileline );
	if( !buf )
		GS_Error( "G_Z_Malloc: failed on allocation of %i bytes", size );
	memset( buf, 0, size );

	return buf;
}

/*
* G_Z_Print
*/
static void G_Z_Print( memzone_t *zone )
{
	memblock_t	*block;

	GS_Printf( "zone size: %i  used: %i in %i blocks\n", zone->size, zone->used, zone->count );

	for( block = zone->blocklist.next; ; block = block->next )
	{
		//GS_Printf( "block:%p    size:%7i    tag:%3i\n", block, block->size, block->tag );

		if( block->next == &zone->blocklist )
			break;			// all blocks have been hit	
		if( (qbyte *)block + block->size != (qbyte *)block->next )
			GS_Printf( "ERROR: block size does not touch the next block\n" );
		if( block->next->prev != block )
			GS_Printf( "ERROR: next block doesn't have proper back link\n" );
		if( !block->tag && !block->next->tag )
			GS_Printf( "ERROR: two consecutive free blocks\n");
	}
}

//==============================================================================

/*
* G_LevelInitPool
*/
void G_LevelInitPool( size_t size )
{
	G_LevelFreePool();

	levelzone = ( memzone_t * )G_Malloc( size );
	G_Z_ClearZone( levelzone, size );
}

/*
* G_LevelFreePool
*/
void G_LevelFreePool( void )
{
	if( levelzone )
	{
		G_Free( levelzone );
		levelzone = NULL;
	}
}

/* 
* G_LevelMalloc
*/
void *_G_LevelMalloc( size_t size, const char *filename, int fileline )
{
	return G_Z_Malloc( size, filename, fileline );
}

/*
* G_LevelFree
*/
void _G_LevelFree( void *data, const char *filename, int fileline )
{
	G_Z_Free( data, filename, fileline );
}

/*
* G_LevelCopyString
*/
/*
char *_G_LevelCopyString( const char *in, const char *filename, int fileline )
{
	char *out;

	out = _G_LevelMalloc( strlen( in ) + 1, filename, fileline );
	strcpy( out, in );
	return out;
}
*/

/*
* G_LevelGarbageCollect
*/
/*
void G_LevelGarbageCollect( void )
{
	if( g_asGC_stats->integer == 2 )
		G_Z_Print( levelzone );
}
*/

//==============================================================================

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
		ent = G_Find( ent, FOFFSET( gentity_t, targetname ), targetname );
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
void G_PrintMsg( gclient_t *client, const char *format, ... )
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

	// mirror at server console
	if( dedicated->integer )
		GS_Printf( "%s", msg );

	s = va( "pr \"%s\"", msg );

	if( client == NULL || trap_GetClientState( CLIENTNUM( client ) ) >= CS_SPAWNED )
		trap_ServerCmd( CLIENTNUM( client ), s );
}

//============
//G_ChatMsg
//
//NULL sends to all the message to all clients
//============
void G_ChatMsg( gclient_t *client, const char *format, ... )
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

	// mirror at server console
	if( dedicated->integer )
		GS_Printf( "%s", msg );

	s = va( "ch \"%s\"", msg );

	if( client == NULL || trap_GetClientState( CLIENTNUM( client ) ) >= CS_SPAWNED )
		trap_ServerCmd( CLIENTNUM( client ), s );
}

//============
//G_CenterPrintMsg
//
//NULL sends to all the message to all clients
//============
void G_CenterPrintMsg( gclient_t *client, const char *format, ... )
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

	trap_ServerCmd( CLIENTNUM( client ), va( "cp \"%s\"", msg ) );
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
//		more miscelanea tools
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

