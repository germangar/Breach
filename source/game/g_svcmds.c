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


//==============================================================================
//
//PACKET FILTERING
//
//
//You can add or remove addresses from the filter list with:
//
//addip <ip>
//removeip <ip>
//
//The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".
//
//Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.
//
//listip
//Prints the current list of filters.
//
//writeip
//Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.
//
//filterban <0 or 1>
//
//If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.
//
//If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.
//
//==============================================================================

typedef struct
{
	unsigned mask;
	unsigned compare;
} ipfilter_t;

#define	MAX_IPFILTERS	1024

static ipfilter_t ipfilters[MAX_IPFILTERS];
static int numipfilters;

//=================
//StringToFilter
//=================
static qboolean StringToFilter( char *s, ipfilter_t *f )
{
	char num[128];
	int i, j;
	qbyte b[4];
	qbyte m[4];

	for( i = 0; i < 4; i++ )
	{
		b[i] = 0;
		m[i] = 0;
	}

	for( i = 0; i < 4; i++ )
	{
		if( *s < '0' || *s > '9' )
		{
			GS_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while( *s >= '0' && *s <= '9' )
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi( num );
		if( b[i] != 0 )
			m[i] = 255;

		if( !*s )
			break;
		s++;
	}

	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return qtrue;
}

//=================
//SV_FilterPacket
//=================
qboolean SV_FilterPacket( char *from )
{
	int i;
	unsigned in;
	qbyte m[4];
	char *p;

	i = 0;
	p = from;
	while( *p && i < 4 )
	{
		m[i] = 0;
		while( *p >= '0' && *p <= '9' )
		{
			m[i] = m[i]*10 + ( *p - '0' );
			p++;
		}
		if( !*p || *p == ':' )
			break;
		i++, p++;
	}

	in = *(unsigned *)m;

	for( i = 0; i < numipfilters; i++ )
		if( ( in & ipfilters[i].mask ) == ipfilters[i].compare )
			return ( qboolean )( filterban->integer != 0 );

	return ( qboolean )( filterban->integer == 0 );
}

//=================
//Cmd_AddIP_f
//=================
static void Cmd_AddIP_f( void )
{
	int i;

	if( trap_Cmd_Argc() < 3 )
	{
		GS_Printf( "Usage:  addip <ip-mask>\n" );
		return;
	}

	for( i = 0; i < numipfilters; i++ )
		if( ipfilters[i].compare == 0xffffffff )
			break; // free spot
	if( i == numipfilters )
	{
		if( numipfilters == MAX_IPFILTERS )
		{
			GS_Printf( "IP filter list is full\n" );
			return;
		}
		numipfilters++;
	}

	if( !StringToFilter( trap_Cmd_Argv( 2 ), &ipfilters[i] ) )
		ipfilters[i].compare = 0xffffffff;
}

//=================
//Cmd_RemoveIP_f
//=================
static void Cmd_RemoveIP_f( void )
{
	ipfilter_t f;
	int i, j;

	if( trap_Cmd_Argc() < 3 )
	{
		GS_Printf( "Usage:  sv removeip <ip-mask>\n" );
		return;
	}

	if( !StringToFilter( trap_Cmd_Argv( 2 ), &f ) )
		return;

	for( i = 0; i < numipfilters; i++ )
		if( ipfilters[i].mask == f.mask
		    && ipfilters[i].compare == f.compare )
		{
			for( j = i+1; j < numipfilters; j++ )
				ipfilters[j-1] = ipfilters[j];
			numipfilters--;
			GS_Printf( "Removed.\n" );
			return;
		}
	GS_Printf( "Didn't find %s.\n", trap_Cmd_Argv( 2 ) );
}

//=================
//Cmd_ListIP_f
//=================
static void Cmd_ListIP_f( void )
{
	int i;
	qbyte b[4];

	GS_Printf( "Filter list:\n" );
	for( i = 0; i < numipfilters; i++ )
	{
		*(unsigned *)b = ipfilters[i].compare;
		GS_Printf( "%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3] );
	}
}

//=================
//Cmd_WriteIP_f
//=================
static void Cmd_WriteIP_f( void )
{
	int file;
	char name[MAX_QPATH];
	char string[MAX_STRING_CHARS];
	qbyte b[4];
	int i;

	Q_strncpyz( name, "listip.cfg", sizeof( name ) );

	GS_Printf( "Writing %s.\n", name );

	if( trap_FS_FOpenFile( name, &file, FS_WRITE ) == -1 )
	{
		GS_Printf( "Couldn't open %s\n", name );
		return;
	}

	Q_snprintfz( string, sizeof( string ), "set filterban %d\n", filterban->integer );
	trap_FS_Write( string, strlen( string ), file );

	for( i = 0; i < numipfilters; i++ )
	{
		*(unsigned *)b = ipfilters[i].compare;
		Q_snprintfz( string, sizeof( string ), "sv addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3] );
		trap_FS_Write( string, strlen( string ), file );
	}

	trap_FS_FCloseFile( file );
}


static void Cmd_Restart_f( void )
{
	int i;
	for( i = 0; i < gs.maxclients; i++ )
	{
		if( trap_GetClientState( i ) < CS_SPAWNED )
			continue;

		game.clients[i].team = TEAM_SPECTATOR;
		G_Client_Respawn( &game.entities[i], qtrue );
	}

	G_InitLevel( level.mapname, level.mapString, level.mapStrlen, game.serverTime );
}

//=================
//G_AddCommands
//=================
void G_AddCommands( void )
{
	trap_Cmd_AddCommand( "addip", Cmd_AddIP_f );
	trap_Cmd_AddCommand( "removeip", Cmd_RemoveIP_f );
	trap_Cmd_AddCommand( "listip", Cmd_ListIP_f );
	trap_Cmd_AddCommand( "writeip", Cmd_WriteIP_f );

	trap_Cmd_AddCommand( "restart", Cmd_Restart_f );
}

//=================
//G_RemoveCommands
//=================
void G_RemoveCommands( void )
{
	trap_Cmd_RemoveCommand( "addip" );
	trap_Cmd_RemoveCommand( "removeip" );
	trap_Cmd_RemoveCommand( "listip" );
	trap_Cmd_RemoveCommand( "writeip" );

	trap_Cmd_RemoveCommand( "restart" );
}
