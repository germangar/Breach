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

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>

#if defined ( __FreeBSD__ )
#include <machine/param.h>
#endif

#include "../qcommon/qcommon.h"
#include "glob.h"

cvar_t *nostdout;

unsigned sys_frame_time;

uid_t saved_euid;
qboolean stdin_active = qtrue;

// =======================================================================
// General routines
// =======================================================================

#ifndef DEDICATED_ONLY
extern void GLimp_Shutdown( void );
extern void IN_Shutdown( void );
#endif

static void signal_handler( int sig )
{
	static int try = 0;

	switch( try++ )
	{
	case 0:
		if( sig == SIGINT || sig == SIGTERM )
		{
			Com_Printf( "Received signal %d, exiting...\n", sig );
			Com_Quit();
		}
		else
		{
			Com_Error( ERR_FATAL, "Received signal %d\n", sig );
		}
		break;
#ifndef DEDICATED_ONLY
	case 1:
		Com_Printf( "Received signal %d, exiting...\n", sig );
		IN_Shutdown();
		GLimp_Shutdown();
		break;
#endif
	default:
		Com_Printf( "Received signal %d, exiting...\n", sig );
		break;
	}
	_exit( 1 );
}

static void InitSig( void )
{
	signal( SIGHUP, signal_handler );
	signal( SIGQUIT, signal_handler );
	signal( SIGILL, signal_handler );
	signal( SIGTRAP, signal_handler );
	signal( SIGIOT, signal_handler );
	signal( SIGBUS, signal_handler );
	signal( SIGFPE, signal_handler );
	signal( SIGSEGV, signal_handler );
	signal( SIGTERM, signal_handler );
	signal( SIGINT, signal_handler );
}

void Sys_ConsoleOutput( char *string )
{
	if( nostdout && nostdout->integer )
		return;

	fputs( string, stdout );
}

/*
   =================
   Sys_Quit
   =================
 */
void Sys_Quit( void )
{
	fcntl( 0, F_SETFL, fcntl( 0, F_GETFL, 0 ) & ~FNDELAY );

	Qcommon_Shutdown();

	_exit( 0 );
}

/*
   =================
   Sys_Init
   =================
 */
void Sys_Init( void )
{
}

/*
   =================
   Sys_Error
   =================
 */
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char string[1024];

	// change stdin to non blocking
	fcntl( 0, F_SETFL, fcntl( 0, F_GETFL, 0 ) & ~FNDELAY );

	CL_Shutdown();

	va_start( argptr, format );
	Q_vsnprintfz( string, sizeof( string ), format, argptr );
	va_end( argptr );
	fprintf( stderr, "Error: %s\n", string );

	Qcommon_Shutdown();

	_exit( 1 );
}

/*
   ================
   Sys_Microseconds
   ================
 */
static unsigned long sys_secbase;
quint64 Sys_Microseconds( void )
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday( &tp, &tzp );

	if( !sys_secbase )
	{
		sys_secbase = tp.tv_sec;
		return tp.tv_usec;
	}

	// TODO handle the wrap
	return (quint64)( tp.tv_sec - sys_secbase )*1000000 + tp.tv_usec;
}

/*
   ================
   Sys_Milliseconds
   ================
 */
unsigned int Sys_Milliseconds( void )
{
	return Sys_Microseconds() / 1000;
}

/*
================
Sys_XTimeToSysTime

Sub-frame timing of events returned by X
Ported from Quake III Arena source code.
================
*/
int Sys_XTimeToSysTime( unsigned long xtime )
{
	int ret, time, test;

	// some X servers (like suse 8.1's) report weird event times
	// if the game is loading, resolving DNS, etc. we are also getting old events
	// so we only deal with subframe corrections that look 'normal'
	ret = xtime - (unsigned long)(sys_secbase * 1000);
	time = Sys_Milliseconds();
	test = time - ret;

	if( test < 0 || test > 30 ) // in normal conditions I've never seen this go above
		return time;
	return ret;
}

/*
   ================
   Sys_Sleep
   ================
 */
void Sys_Sleep( unsigned int millis )
{
	usleep( millis * 1000 );
}

static void floating_point_exception_handler( int whatever )
{
	signal( SIGFPE, floating_point_exception_handler );
}

char *Sys_ConsoleInput( void )
{
	static char text[256];
	int len;
	fd_set fdset;
	struct timeval timeout;

	if( !dedicated || !dedicated->integer )
		return NULL;

	if( !stdin_active )
		return NULL;

	FD_ZERO( &fdset );
	FD_SET( 0, &fdset ); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if( select( 1, &fdset, NULL, NULL, &timeout ) == -1 || !FD_ISSET( 0, &fdset ) )
		return NULL;

	len = read( 0, text, sizeof( text ) );
	if( len == 0 )
	{           // eof!
		Com_Printf( "EOF from stdin, console input disabled...\n" );
		stdin_active = qfalse;
		return NULL;
	}

	if( len < 1 )
		return NULL;

	text[len-1] = 0; // rip off the /n and terminate

	return text;
}

/*
   =================
   Sys_GetSymbol
   =================
 */
#ifdef SYS_SYMBOL
void *Sys_GetSymbol( const char *moduleName, const char *symbolName )
{
	// FIXME: Does not work on Debian64 for unknown reasons (dlsym always returns NULL)
	void *const module = dlopen( moduleName, RTLD_NOW );
	if( module )
	{
		void *const symbol = dlsym( module, symbolName );
		dlclose( module );
		return symbol;
	}
	else
		return NULL;
}
#endif // SYS_SYMBOL

//===============================================================================

/*
   =================
   Sys_AppActivate
   =================
 */
void Sys_AppActivate( void )
{
}

/*
   =================
   Sys_SendKeyEvents
   =================
 */
void Sys_SendKeyEvents( void )
{
	// grab frame time
	sys_frame_time = Sys_Milliseconds();
}

/*****************************************************************************/

int main( int argc, char **argv )
{
	unsigned int oldtime, newtime, time;

	InitSig();

	Qcommon_Init( argc, argv );

	fcntl( 0, F_SETFL, fcntl( 0, F_GETFL, 0 ) | FNDELAY );

	nostdout = Cvar_Get( "nostdout", "0", 0 );
	if( !nostdout->integer )
	{
		fcntl( 0, F_SETFL, fcntl( 0, F_GETFL, 0 ) | FNDELAY );
	}

	oldtime = Sys_Milliseconds();
	while( qtrue )
	{
		// find time spent rendering last frame
		do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
			if( time > 0 )
				break;
#ifdef PUTCPU2SLEEP
			Sys_Sleep( 0 );
#endif
		}
		while( 1 );
		oldtime = newtime;

		Qcommon_Frame( time );
	}
}
