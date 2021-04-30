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
// sys_win.h

#include "../qcommon/qcommon.h"
#include "winquake.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#include <conio.h>

#include "../win32/conproc.h"

#define MINIMUM_WIN_MEMORY  0x0a00000
#define MAXIMUM_WIN_MEMORY  0x1000000

qboolean s_win95;

int starttime;
qboolean ActiveApp;
qboolean Minimized;

static HANDLE hinput, houtput;

unsigned sys_msg_time;
unsigned sys_frame_time;


static HANDLE qwclsemaphore;

#define	MAX_NUM_ARGVS	128
int argc;
char *argv[MAX_NUM_ARGVS];

cvar_t *sys_hwtimer;
static qboolean	hwtimer;
static int milli_offset = 0;
static qint64 micro_offset = 0;

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	CL_Shutdown();

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	MessageBox( NULL, msg, "Error", 0 /* MB_OK */ );
	if( qwclsemaphore )
		CloseHandle( qwclsemaphore );

	// shut down QHOST hooks if necessary
	DeinitConProc();

	Qcommon_Shutdown();

	exit( 1 );
}

void Sys_Quit( void )
{
	timeEndPeriod( 1 );

	CL_Shutdown();

	CloseHandle( qwclsemaphore );
	if( dedicated && dedicated->integer )
		FreeConsole();

	// shut down QHOST hooks if necessary
	DeinitConProc();

	Qcommon_Shutdown();

	exit( 0 );
}

//================================================================

/*
================
Sys_Milliseconds
================
*/
// pb : adapted High Res Performance Counter code from ezquake
static qint64 freq;

static void Sys_InitTime( void )
{
	QueryPerformanceFrequency( (LARGE_INTEGER *) &freq );
	hwtimer = qfalse;
	sys_hwtimer = Cvar_Get( "sys_hwtimer", "0", 0 );
	sys_hwtimer->modified = qtrue;
}

static unsigned int Sys_Milliseconds_TGT( void )
{
	static unsigned int base;
	static qboolean	initialized = qfalse;
	unsigned int now;

	if( !initialized )
	{
		// let base retain 16 bits of effectively random data which is used
		//for quickly generating random numbers
		base = timeGetTime() & 0xffff0000;
		initialized = qtrue;
	}

	now = timeGetTime();

	return now - base;
}

static quint64 Sys_Microseconds_QPC( void )
{
	static qboolean first = qtrue;
	static qint64 p_start;

	qint64 p_now;
	QueryPerformanceCounter( (LARGE_INTEGER *) &p_now );

	if( first )
	{
		first = qfalse;
		p_start = p_now;
	}

	return ( ( p_now - p_start ) * 1000000 ) / freq;
}

unsigned int Sys_Milliseconds( void )
{
	if( hwtimer )
		return ( Sys_Microseconds_QPC() + micro_offset ) / 1000;
	else
		return Sys_Milliseconds_TGT() + milli_offset;
}

quint64 Sys_Microseconds( void )
{
	if( hwtimer )
		return Sys_Microseconds_QPC() + micro_offset;
	else
		return (quint64)( Sys_Milliseconds_TGT() + milli_offset ) * 1000;
}

//==================
//Sys_HwTimerModified
// keep timer switches synchronized
//==================
static void Sys_HwTimerModified( void )
{
	static int hwtimer_old = -1;

	const unsigned int millis = Sys_Milliseconds_TGT();
	const qint64 micros = Sys_Microseconds_QPC();
	const qint64 drift = micros - millis * 1000;

	sys_hwtimer->modified = qfalse;

	if( sys_hwtimer->integer )
	{
		if( freq )
		{
			hwtimer = 1;
		}
		else
		{
			Com_Printf( "Can't enable hwtimer\n" );
			Cvar_SetValue( "sys_hwtimer", 0 );
			hwtimer = 0;
		}
	}
	else
	{
		hwtimer = 0;
	}

	if( hwtimer == hwtimer_old )
		return; // no need to update

	switch( hwtimer )
	{
	case 0:
		// switched from micro to milli precision
		milli_offset = max( milli_offset, drift / 1000 );
		break;
	case 1:
		// switched from milli to micro precision
		micro_offset = max( micro_offset, -drift );
		break;
	default:
		assert( 0 );
	}
	hwtimer_old = hwtimer;
}

void Sys_Sleep( unsigned int millis )
{
	Sleep( millis );
}

/*
=================
Sys_GetSymbol
=================
*/
#ifdef SYS_SYMBOL
void *Sys_GetSymbol( const char *moduleName, const char *symbolName )
{
	HMODULE module = GetModuleHandle( moduleName );
	return module
		? (void *) GetProcAddress( module, symbolName )
		: NULL;
}
#endif // SYS_SYMBOL

//===============================================================================

cvar_t *sys_affinity;

static char *Sys_GetAffinity_f( void )
{
	static char affinityString[33];
	DWORD_PTR procAffinity, sysAffinity;
	HANDLE proc = GetCurrentProcess();

	if( GetProcessAffinityMask( proc, &procAffinity, &sysAffinity ) )
	{
		SYSTEM_INFO sysInfo;
		DWORD i;
		CloseHandle( proc );
		GetSystemInfo( &sysInfo );
		for( i = 0; i < sysInfo.dwNumberOfProcessors; ++i )
		{
			affinityString[i] = '0' + ( procAffinity & 1 );
			procAffinity >>= 1;
		}
		affinityString[i] = '\0';
		return affinityString;
	}

	CloseHandle( proc );
	return NULL;
}

static qboolean Sys_SetAffinity_f( void *affinity )
{

	int result = 0;
	SYSTEM_INFO sysInfo;
	DWORD_PTR procAffinity = 0, i;
	HANDLE proc = GetCurrentProcess();
	char minValid[33], maxValid[33];
	const size_t len = strlen( (char *) affinity );

	if( !affinity )
		return 0;

	// create range of valid values for error printing
	GetSystemInfo( &sysInfo );
	for( i = 0; i < sysInfo.dwNumberOfProcessors; ++i )
	{
		minValid[i] = '0';
		maxValid[i] = '1';
	}
	minValid[i] = '\0';
	maxValid[i] = '\0';

	if( len == sysInfo.dwNumberOfProcessors )
	{
		// string is of valid length, parse in reverse direction
		const char *c;
		for( c = ( (char *) affinity ) + len - 1; c >= (char *) affinity; --c )
		{
			// parse binary digit
			procAffinity <<= 1;
			switch( *c )
			{
			case '0':
				// nothing to do
				break;
			case '1':
				// at least one digit must be 1
				result = 1;
				procAffinity |= 1;
				break;
			default:
				// invalid character found
				result = 2;
				goto abort;
			}
		}
		SetProcessAffinityMask( proc, procAffinity );
	}
abort:
	if( result == 2 )
		Com_Printf( "\"sys_affinity\" must be a non-zero bitmask between \"%s\" and \"%s\".\n", minValid, maxValid );
	CloseHandle( proc );
	return result;
}

void Sys_AffinityModified( void )
{
	if( !Sys_SetAffinity_f( sys_affinity->string ) )
	{
		Com_Printf( "%s is not a valid affinity mask for this processor\n", sys_affinity->string );
		Cvar_ForceSet( "sys_affinity", Sys_GetAffinity_f() );
	}
	sys_affinity->modified = qfalse;
}

void Sys_InitAffinity( void )
{
	char *af;

	af = Sys_GetAffinity_f();

	if( !af )
	{       // should never happen
		sys_affinity = Cvar_Get( "sys_affinity", "0", CVAR_ARCHIVE );
	}
	else
	{
		sys_affinity = Cvar_Get( "sys_affinity", af, CVAR_ARCHIVE );
		if( !strcmp( af, sys_affinity->string ) )
			Sys_AffinityModified();
	}
}

/*
================
Sys_Init
================
*/
void Sys_Init( void )
{
	OSVERSIONINFO vinfo;

	timeBeginPeriod( 1 );
	Sys_InitTime();
	Sys_InitAffinity();

	vinfo.dwOSVersionInfoSize = sizeof( vinfo );

	if( !GetVersionEx( &vinfo ) )
		Sys_Error( "Couldn't get OS info" );

	if( vinfo.dwMajorVersion < 4 )
		Sys_Error( "%s requires windows version 4 or greater", APPLICATION );
	if( vinfo.dwPlatformId == VER_PLATFORM_WIN32s )
		Sys_Error( "%s doesn't run on Win32s", APPLICATION );
	else if( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
		s_win95 = qtrue;

	if( dedicated->integer )
	{
		SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

		if( !AllocConsole() )
			Sys_Error( "Couldn't create dedicated server console" );
		hinput = GetStdHandle( STD_INPUT_HANDLE );
		houtput = GetStdHandle( STD_OUTPUT_HANDLE );

		// let QHOST hook in
		InitConProc( argc, argv );
	}
}

static char console_text[256];
static int console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput( void )
{
	INPUT_RECORD rec;
	int ch;
	DWORD dummy;
	DWORD numread, numevents;

	if( !dedicated || !dedicated->integer )
		return NULL;

	for(;; )
	{
		if( !GetNumberOfConsoleInputEvents( hinput, &numevents ) )
			Sys_Error( "Error getting # of console events" );

		if( numevents <= 0 )
			break;

		if( !ReadConsoleInput( hinput, &rec, 1, &numread ) )
			Sys_Error( "Error reading console input" );

		if( numread != 1 )
			Sys_Error( "Couldn't read console input" );

		if( rec.EventType == KEY_EVENT )
		{
			if( !rec.Event.KeyEvent.bKeyDown )
			{
				ch = rec.Event.KeyEvent.uChar.AsciiChar;

				switch( ch )
				{
				case '\r':
					WriteFile( houtput, "\r\n", 2, &dummy, NULL );

					if( console_textlen )
					{
						console_text[console_textlen] = 0;
						console_textlen = 0;
						return console_text;
					}
					break;

				case '\b':
					if( console_textlen )
					{
						console_textlen--;
						WriteFile( houtput, "\b \b", 3, &dummy, NULL );
					}
					break;

				default:
					if( ch >= ' ' )
					{
						if( console_textlen < sizeof( console_text )-2 )
						{
							WriteFile( houtput, &ch, 1, &dummy, NULL );
							console_text[console_textlen] = ch;
							console_textlen++;
						}
					}
					break;
				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput( char *string )
{
	DWORD dummy;
	char text[256];

	if( !dedicated || !dedicated->integer )
		return;

	if( console_textlen )
	{
		text[0] = '\r';
		memset( &text[1], ' ', console_textlen );
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile( houtput, text, console_textlen+2, &dummy, NULL );
	}

	WriteFile( houtput, string, (unsigned)strlen( string ), &dummy, NULL );

	if( console_textlen )
		WriteFile( houtput, console_text, console_textlen, &dummy, NULL );
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents( void )
{
	MSG msg;

	while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		if( !GetMessage( &msg, NULL, 0, 0 ) )
			Sys_Quit();
		sys_msg_time = msg.time;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// grab frame time
	sys_frame_time = timeGetTime(); // FIXME: should this be at start?
}

/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData( qboolean primary )
{
	char *data = NULL;
	char *cliptext;

	if( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if( ( cliptext = GlobalLock( hClipboardData ) ) != 0 )
			{
				data = Q_malloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
* Sys_SetClipboardData
*/
qboolean Sys_SetClipboardData( char *data )
{
	size_t size;
	HGLOBAL hglbCopy;
	UINT uFormat;
	LPCSTR cliptext = data;
	LPTSTR lptstrCopy;

	// open the clipboard, and empty it
	if( !OpenClipboard( NULL ) ) 
		return qfalse;

	EmptyClipboard();

	// this is actually ANSI, not utf-8
	size = strlen( cliptext );

	// allocate a global memory object for the text
	hglbCopy = GlobalAlloc( GMEM_MOVEABLE, (size + 1) * sizeof( *lptstrCopy ) ); 
	if( hglbCopy == NULL )
	{
		CloseClipboard(); 
		return qfalse; 
	} 

	// lock the handle and copy the text to the buffer
	lptstrCopy = GlobalLock( hglbCopy ); 

	// this is actually ANSI, not utf-8
	uFormat = CF_TEXT;
	memcpy( lptstrCopy, cliptext, size );
	lptstrCopy[size] = 0;

	GlobalUnlock( hglbCopy ); 

	// place the handle on the clipboard
	SetClipboardData( uFormat, hglbCopy );

	// close the clipboard
	CloseClipboard();

	return qtrue;
}

/*
================
Sys_FreeClipboardData
================
*/
void Sys_FreeClipboardData( char *data )
{
	Q_free( data );
}

/*
==============================================================================

WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate( void )
{
#ifndef DEDICATED_ONLY
	ShowWindow( cl_hwnd, SW_RESTORE );
	SetForegroundWindow( cl_hwnd );
#endif
}

//=======================================================================

/*
==================
ParseCommandLine
==================
*/
static void ParseCommandLine( LPSTR lpCmdLine )
{
	argc = 1;
	argv[0] = "exe";

	while( *lpCmdLine && ( argc < MAX_NUM_ARGVS ) )
	{
		while( *lpCmdLine && ( *lpCmdLine <= 32 || *lpCmdLine > 126 ) )
			lpCmdLine++;

		if( *lpCmdLine )
		{
			char quote = ( ( '"' == *lpCmdLine || '\'' == *lpCmdLine ) ? *lpCmdLine++ : 0 );

			argv[argc++] = lpCmdLine;
			if( quote )
			{
				while( *lpCmdLine && *lpCmdLine != quote && *lpCmdLine >= 32 && *lpCmdLine <= 126 )
					lpCmdLine++;
			}
			else
			{
				while( *lpCmdLine && *lpCmdLine > 32 && *lpCmdLine <= 126 )
					lpCmdLine++;
			}

			if( *lpCmdLine )
				*lpCmdLine++ = 0;
		}
	}
}

/*
==================
WinMain

==================
*/
HINSTANCE global_hInstance;
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	MSG msg;
	unsigned int oldtime, newtime, time;

	/* previous instances do not exist in Win32 */
	if( hPrevInstance )
		return 0;

	global_hInstance = hInstance;

	ParseCommandLine( lpCmdLine );

	Qcommon_Init( argc, argv );

	oldtime = Sys_Milliseconds();

	/* main window message loop */
	while( 1 )
	{
		// if at a full screen console, don't update unless needed
		if( Minimized || ( dedicated && dedicated->integer ) )
		{
			Sys_Sleep( 1 );
		}

		while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
		{
			if( !GetMessage( &msg, NULL, 0, 0 ) )
				Com_Quit();
			sys_msg_time = msg.time;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		if( sys_hwtimer->modified )
			Sys_HwTimerModified();

		if( sys_affinity->modified )
			Sys_AffinityModified();

		do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime; // no warp problem as unsigned
			if( time > 0 )
				break;
#ifdef PUTCPU2SLEEP
			Sys_Sleep( 0 );
#endif
		}
		while( 1 );
		//Com_Printf ("time:%5.2u - %5.2u = %5.2u\n", newtime, oldtime, time);
		oldtime = newtime;

		// do as q3 (use the default floating point precision)
		//	_controlfp( ~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM );
		//_controlfp( _PC_24, _MCW_PC );
		Qcommon_Frame( time );
	}

	// never gets here
	return TRUE;
}
