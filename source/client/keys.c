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
#include "client.h"

/*

key up events are sent even if in console mode

*/

#define SEMICOLON_BINDNAME	"SEMICOLON"

#define		MAXCMDLINE  256

int anykeydown;
int key_waiting;
char *keybindings[256];
qboolean consolekeys[256];   // if qtrue, can't be rebound while in console
qboolean menubound[256];     // if qtrue, can't be rebound while in menu
int key_repeats[256];   // if > 1, it is autorepeating
qboolean keydown[256];

static qboolean	key_initialized = qfalse;

typedef struct
{
	const char *name;
	int keynum;
} keyname_t;

const keyname_t keynames[] =
{
	{ "TAB", K_TAB },
	{ "ENTER", K_ENTER },
	{ "ESCAPE", K_ESCAPE },
	{ "SPACE", K_SPACE },
	{ "CAPSLOCK", K_CAPSLOCK },
	{ "SCROLLLOCK", K_SCROLLLOCK },
	{ "NUMLOCK", K_NUMLOCK },
	{ "KP_NUMLOCK", K_NUMLOCK },
	{ "BACKSPACE", K_BACKSPACE },
	{ "UPARROW", K_UPARROW },
	{ "DOWNARROW", K_DOWNARROW },
	{ "LEFTARROW", K_LEFTARROW },
	{ "RIGHTARROW", K_RIGHTARROW },

	{ "ALT", K_ALT },
	{ "CTRL", K_CTRL },
	{ "SHIFT", K_SHIFT },

	{ "F1", K_F1 },
	{ "F2", K_F2 },
	{ "F3", K_F3 },
	{ "F4", K_F4 },
	{ "F5", K_F5 },
	{ "F6", K_F6 },
	{ "F7", K_F7 },
	{ "F8", K_F8 },
	{ "F9", K_F9 },
	{ "F10", K_F10 },
	{ "F11", K_F11 },
	{ "F12", K_F12 },

	{ "INS", K_INS },
	{ "DEL", K_DEL },
	{ "PGDN", K_PGDN },
	{ "PGUP", K_PGUP },
	{ "HOME", K_HOME },
	{ "END", K_END },

	{ "WIN", K_WIN },
	{ "POPUPMENU", K_MENU },

#if defined ( __APPLE__ ) || defined ( MACOSX )
	{ "F13", K_F13 },
	{ "F14", K_F14 },
	{ "F15", K_F15 },
	{ "COMMAND", K_COMMAND },
#endif /* __APPLE__ || MACOSX */

	{ "MOUSE1", K_MOUSE1 },
	{ "MOUSE2", K_MOUSE2 },
	{ "MOUSE3", K_MOUSE3 },
	{ "MOUSE4", K_MOUSE4 },
	{ "MOUSE5", K_MOUSE5 },
#if !defined ( __APPLE__ ) && !defined ( MACOSX )
	{ "MOUSE6", K_MOUSE6 },
	{ "MOUSE7", K_MOUSE7 },
	{ "MOUSE8", K_MOUSE8 },
#endif /* !__APPLE__ &&?!MACOSX */

	{ "JOY1", K_JOY1 },
	{ "JOY2", K_JOY2 },
	{ "JOY3", K_JOY3 },
	{ "JOY4", K_JOY4 },

	{ "AUX1", K_AUX1 },
	{ "AUX2", K_AUX2 },
	{ "AUX3", K_AUX3 },
	{ "AUX4", K_AUX4 },
	{ "AUX5", K_AUX5 },
	{ "AUX6", K_AUX6 },
	{ "AUX7", K_AUX7 },
	{ "AUX8", K_AUX8 },
	{ "AUX9", K_AUX9 },
	{ "AUX10", K_AUX10 },
	{ "AUX11", K_AUX11 },
	{ "AUX12", K_AUX12 },
	{ "AUX13", K_AUX13 },
	{ "AUX14", K_AUX14 },
	{ "AUX15", K_AUX15 },
	{ "AUX16", K_AUX16 },
	{ "AUX17", K_AUX17 },
	{ "AUX18", K_AUX18 },
	{ "AUX19", K_AUX19 },
	{ "AUX20", K_AUX20 },
	{ "AUX21", K_AUX21 },
	{ "AUX22", K_AUX22 },
	{ "AUX23", K_AUX23 },
	{ "AUX24", K_AUX24 },
	{ "AUX25", K_AUX25 },
	{ "AUX26", K_AUX26 },
	{ "AUX27", K_AUX27 },
	{ "AUX28", K_AUX28 },
	{ "AUX29", K_AUX29 },
	{ "AUX30", K_AUX30 },
	{ "AUX31", K_AUX31 },
	{ "AUX32", K_AUX32 },

	{ "KP_HOME", KP_HOME },
	{ "KP_UPARROW",	KP_UPARROW },
	{ "KP_PGUP", KP_PGUP },
	{ "KP_LEFTARROW", KP_LEFTARROW },
	{ "KP_5", KP_5 },
	{ "KP_RIGHTARROW", KP_RIGHTARROW },
	{ "KP_END", KP_END },
	{ "KP_DOWNARROW", KP_DOWNARROW },
	{ "KP_PGDN", KP_PGDN },
	{ "KP_ENTER", KP_ENTER },
	{ "KP_INS", KP_INS },
	{ "KP_DEL", KP_DEL },
	{ "KP_STAR", KP_STAR },
	{ "KP_SLASH", KP_SLASH },
	{ "KP_MINUS", KP_MINUS },
	{ "KP_PLUS", KP_PLUS },

#if defined ( __APPLE__ ) || ( MACOSX )
	{ "KP_MULT", KP_MULT },
	{ "KP_EQUAL", KP_EQUAL },
#endif /* __APPLE__ || MACOSX */

	{ "MWHEELUP", K_MWHEELUP },
	{ "MWHEELDOWN", K_MWHEELDOWN },

	{ "PAUSE", K_PAUSE },

	{ "SEMICOLON", ';' }, // because a raw semicolon separates commands

	{ NULL, 0 }
};

static void Key_DelegateCallKeyDel( int key );
static void Key_DelegateCallCharDel( int key );

/*
==============================================================================

LINE TYPING INTO THE MESSAGE INPUT

==============================================================================
*/

qboolean chat_team;
char chat_buffer[MAXCMDLINE];
int chat_bufferlen = 0;

static void Key_CharMessage( int key )
{
	if( key == 12 ) // CTRL - L : clear
	{
		chat_bufferlen = 0;
		memset( chat_buffer, 0, MAXCMDLINE );
		return;
	}

	if( key < 32 || key > 126 )
		return; // non printable

	if( chat_bufferlen == sizeof( chat_buffer )-1 )
		return; // all full

	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}

static void Key_Message( int key )
{
	if( key == K_ENTER || key == KP_ENTER )
	{
		if( chat_bufferlen > 0 )
		{
			if( chat_team )
				Cbuf_AddText( "say_team \"" );
			else
				Cbuf_AddText( "say \"" );
			Cbuf_AddText( chat_buffer );
			Cbuf_AddText( "\"\n" );

			chat_bufferlen = 0;
			chat_buffer[0] = 0;
		}

		CL_SetKeyDest( key_game );
		return;
	}

	if( key == K_BACKSPACE )
	{
		if( chat_bufferlen )
		{
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if( key == K_ESCAPE )
	{
		CL_SetKeyDest( key_game );
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}
}

//============================================================================


/*
* Key_StringToKeynum
*
* Returns a key number to be used to index keybindings[] by looking at
* the given string.  Single ascii characters return themselves, while
* the K_* names are matched up.
*/
int Key_StringToKeynum( char *str )
{
	const keyname_t *kn;

	if( !str || !str[0] )
		return -1;
	if( !str[1] )
		return (int)(unsigned char)str[0];

	for( kn = keynames; kn->name; kn++ )
	{
		if( !Q_stricmp( str, kn->name ) )
			return kn->keynum;
	}
	return -1;
}

/*
* Key_KeynumToString
*
* Returns a string (either a single ascii char, or a K_* name) for the
* given keynum.
* FIXME: handle quote special (general escape sequence?)
*/
const char *Key_KeynumToString( int keynum )
{
	const keyname_t *kn;
	static char tinystr[2];

	if( keynum == -1 )
		return "<KEY NOT FOUND>";
	if( keynum > 32 && keynum < 127 )
	{ // printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	for( kn = keynames; kn->name; kn++ )
		if( keynum == kn->keynum )
			return kn->name;

	return "<UNKNOWN KEYNUM>";
}


/*
* Key_SetBinding
*/
void Key_SetBinding( int keynum, char *binding )
{
	if( keynum == -1 )
		return;

	// free old bindings
	if( keybindings[keynum] )
	{
		Mem_ZoneFree( keybindings[keynum] );
		keybindings[keynum] = NULL;
	}

	if( !binding )
		return;

	// allocate memory for new binding
	keybindings[keynum] = ZoneCopyString( binding );
}

/*
* Key_Unbind_f
*/
static void Key_Unbind_f( void )
{
	int b;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "unbind <key> : remove commands from a key\n" );
		return;
	}

	b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if( b == -1 )
	{
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	Key_SetBinding( b, NULL );
}

static void Key_Unbindall_f( void )
{
	int i;

	for( i = 0; i < 256; i++ )
		if( keybindings[i] )
			Key_SetBinding( i, NULL );
}


/*
* Key_Bind_f
*/
static void Key_Bind_f( void )
{
	int i, c, b;
	char cmd[1024];

	c = Cmd_Argc();
	if( c < 2 )
	{
		Com_Printf( "bind <key> [command] : attach a command to a key\n" );
		return;
	}

	b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if( b == -1 )
	{
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	if( c == 2 )
	{
		if( keybindings[b] )
			Com_Printf( "\"%s\" = \"%s\"\n", Cmd_Argv( 1 ), keybindings[b] );
		else
			Com_Printf( "\"%s\" is not bound\n", Cmd_Argv( 1 ) );
		return;
	}

	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string
	for( i = 2; i < c; i++ )
	{
		Q_strncatz( cmd, Cmd_Argv( i ), sizeof( cmd ) );
		if( i != ( c-1 ) )
			Q_strncatz( cmd, " ", sizeof( cmd ) );
	}

	Key_SetBinding( b, cmd );
}

/*
* Key_WriteBindings
*
* Writes lines containing "bind key value"
*/
void Key_WriteBindings( int file )
{
	int i;

	FS_Printf( file, "unbindall\n" );

	for( i = 0; i < 256; i++ )
		if( keybindings[i] && keybindings[i][0] )
			FS_Printf( file, "bind %s \"%s\"\r\n", (i == ';' ? SEMICOLON_BINDNAME : Key_KeynumToString( i )), keybindings[i] );
}


/*
* Key_Bindlist_f
*/
static void Key_Bindlist_f( void )
{
	int i;

	for( i = 0; i < 256; i++ )
		if( keybindings[i] && keybindings[i][0] )
			Com_Printf( "%s \"%s\"\n", Key_KeynumToString( i ), keybindings[i] );
}


//#include "../win32/winquake.h"

/*
* Key_Init
*/
void Key_Init( void )
{
	int i;

	assert( !key_initialized );

	//
	// init ascii characters in console mode
	//
	for( i = 32; i < 128; i++ )
		consolekeys[i] = qtrue;
	consolekeys[K_ENTER] = qtrue;
	consolekeys[KP_ENTER] = qtrue;
	consolekeys[K_TAB] = qtrue;
	consolekeys[K_LEFTARROW] = qtrue;
	consolekeys[KP_LEFTARROW] = qtrue;
	consolekeys[K_RIGHTARROW] = qtrue;
	consolekeys[KP_RIGHTARROW] = qtrue;
	consolekeys[K_UPARROW] = qtrue;
	consolekeys[KP_UPARROW] = qtrue;
	consolekeys[K_DOWNARROW] = qtrue;
	consolekeys[KP_DOWNARROW] = qtrue;
	consolekeys[K_BACKSPACE] = qtrue;
	consolekeys[K_HOME] = qtrue;
	consolekeys[KP_HOME] = qtrue;
	consolekeys[K_END] = qtrue;
	consolekeys[KP_END] = qtrue;
	consolekeys[K_PGUP] = qtrue;
	consolekeys[KP_PGUP] = qtrue;
	consolekeys[K_PGDN] = qtrue;
	consolekeys[KP_PGDN] = qtrue;
	consolekeys[K_SHIFT] = qtrue;
	consolekeys[K_INS] = qtrue;
	consolekeys[K_DEL] = qtrue;
	consolekeys[KP_INS] = qtrue;
	consolekeys[KP_DEL] = qtrue;
	consolekeys[KP_SLASH] = qtrue;
	consolekeys[KP_PLUS] = qtrue;
	consolekeys[KP_MINUS] = qtrue;
	consolekeys[KP_5] = qtrue;

	consolekeys[K_CTRL] = qtrue; // ctrl in console for ctrl-v
	consolekeys[K_ALT] = qtrue;

	consolekeys['`'] = qfalse;
	consolekeys['~'] = qfalse;

	// support mwheel in console
	consolekeys[K_MWHEELDOWN] = qtrue;
	consolekeys[K_MWHEELUP] = qtrue;

	menubound[K_ESCAPE] = qtrue;

	//
	// register our functions
	//
	Cmd_AddCommand( "bind", Key_Bind_f );
	Cmd_AddCommand( "unbind", Key_Unbind_f );
	Cmd_AddCommand( "unbindall", Key_Unbindall_f );
	Cmd_AddCommand( "bindlist", Key_Bindlist_f );

	key_initialized = qtrue;
}

void Key_Shutdown( void )
{
	if( !key_initialized )
		return;

	Cmd_RemoveCommand( "bind" );
	Cmd_RemoveCommand( "unbind" );
	Cmd_RemoveCommand( "unbindall" );
	Cmd_RemoveCommand( "bindlist" );

	Key_Unbindall_f();
}

/*
* Key_CharEvent
*
* Called by the system between frames for key down events for standard characters
* Should NOT be called during an interrupt!
*/
void Key_CharEvent( int key )
{
	// the console key should never be used as a char
	if( key == '`' || key == '~' )
	{
		return;
	}

	switch( cls.key_dest )
	{
	case key_message:
		Key_CharMessage( key );
		break;
	case key_menu:
		CL_UIModule_CharEvent( key );
		break;
	case key_game:
	case key_console:
		Con_CharEvent( key );
		break;
	case key_delegate:
		Key_DelegateCallCharDel( key );
		break;
	default:
		Com_Error( ERR_FATAL, "Bad cls.key_dest" );
	}
}

/*
* Key_Event
*
* Called by the system between frames for both key up and key down events
* Should NOT be called during an interrupt!
*/
void Key_Event( int key, qboolean down, unsigned time )
{
	char *kb;
	char cmd[1024];
	qboolean handled = qfalse;

	// hack for modal presses
	if( key_waiting == -1 )
	{
		if( down )
			key_waiting = key;
		return;
	}

	// update auto-repeat status
	if( down )
	{
		key_repeats[key]++;
		if( key_repeats[key] > 1 )
		{
			if( ( key != K_BACKSPACE && key != K_DEL
				&& key != K_LEFTARROW && key != K_RIGHTARROW
				&& key != K_UPARROW && key != K_DOWNARROW
				&& key != K_PGUP && key != K_PGDN && ( key < 32 || key > 126 || key == '`' ) )
				|| cls.key_dest == key_game )
				return;
		}
	}
	else
	{
		key_repeats[key] = 0;
	}

#ifndef WIN32
	// switch between fullscreen/windowed when ALT+ENTER is pressed
	if( key == K_ENTER && down && keydown[K_ALT] )
		Cbuf_ExecuteText( EXEC_APPEND, "toggle vid_fullscreen; vid_restart\n" );
#endif

	// console key is hardcoded, so the user can never unbind it
	if( key == '`' || key == '~' )
	{
		if( !down )
			return;
		Con_ToggleConsole_f();
		return;
	}

	// menu key is hardcoded, so the user can never unbind it
	if( key == K_ESCAPE )
	{
		if( !down )
			return;

		if( cls.state != CA_ACTIVE )
		{
			if( cls.key_dest == key_game || cls.key_dest == key_menu )
			{
				if( cls.state != CA_DISCONNECTED )
					Cbuf_AddText( "disconnect\n" );
				else if( cls.key_dest == key_menu )
					CL_UIModule_KeyEvent( key, down );
				return;
			}
		}

		switch( cls.key_dest )
		{
		case key_message:
			Key_Message( key );
			break;
		case key_menu:
			CL_UIModule_KeyEvent( key, down );
			break;
		case key_game:
			CL_GameModule_EscapeKey();
			break;
		case key_console:
			Con_ToggleConsole_f();
			break;
		case key_delegate:
			Key_DelegateCallKeyDel( key );
			break;
		default:
			Com_Error( ERR_FATAL, "Bad cls.key_dest" );
		}
		return;
	}

	//
	// if not a console key, send to the interpreter no matter what mode is
	//
	if( ( cls.key_dest == key_menu && menubound[key] )
		|| ( cls.key_dest == key_console && !consolekeys[key] )
		|| ( cls.key_dest == key_game && ( cls.state == CA_ACTIVE || !consolekeys[key] ) ) )
	{
		kb = keybindings[key];

		if( kb )
		{
			if( kb[0] == '+' )
			{ // button commands add keynum and time as a parm
				if( down )
				{
					Q_snprintfz( cmd, sizeof( cmd ), "%s %i %u\n", kb, key, time );
					Cbuf_AddText( cmd );
				}
				else if( keydown[key] )
				{
					Q_snprintfz( cmd, sizeof( cmd ), "-%s %i %u\n", kb+1, key, time );
					Cbuf_AddText( cmd );
				}
			}
			else if( down )
			{
				Cbuf_AddText( kb );
				Cbuf_AddText( "\n" );
			}
		}
		handled = qtrue; // can't return here, because we want to track keydown & repeats
	}

	// track if any key is down for BUTTON_ANY
	keydown[key] = down;
	if( down )
	{
		if( key_repeats[key] == 1 )
			anykeydown++;
	}
	else
	{
		anykeydown--;
		if( anykeydown < 0 )
			anykeydown = 0;
	}

	if( handled )
		return; // other systems only care about key down events

	switch( cls.key_dest )
	{
	case key_message:
		if( down )
			Key_Message( key );
		break;
	case key_menu:
		CL_UIModule_KeyEvent( key, down );
		break;
	case key_game:
	case key_console:
		if( down )
			Con_KeyDown( key );
		break;
	case key_delegate:
		if( down )
			Key_DelegateCallKeyDel( key );
		break;
	default:
		Com_Error( ERR_FATAL, "Bad cls.key_dest" );
	}
}

/*
* Key_ClearStates
*/
void Key_ClearStates( void )
{
	int i;

	anykeydown = qfalse;

	for( i = 0; i < 256; i++ )
	{
		if( keydown[i] || key_repeats[i] )
			Key_Event( i, qfalse, 0 );
		keydown[i] = 0;
		key_repeats[i] = 0;
	}
}


/*
* Key_GetKey
*/
int Key_GetKey( void )
{
	key_waiting = -1;

	while( key_waiting == -1 )
		Sys_SendKeyEvents();

	return key_waiting;
}

/*
* Key_GetBindingBuf
*/
const char *Key_GetBindingBuf( int binding )
{
	return keybindings[binding];
}

/*
* Key_GetKey
*/
qboolean Key_IsDown( int keynum )
{
	if( keynum < 0 || keynum > 255 )
		return qfalse;
	return keydown[keynum];
}

typedef struct
{
	key_delegate_f key_del;
	key_char_delegate_f char_del;
} key_delegates_t;

static key_delegates_t key_delegate_stack[32];
static int key_delegate_stack_index = 0;

/*
* Key_DelegatePush
*/
keydest_t Key_DelegatePush( key_delegate_f key_del, key_char_delegate_f char_del )
{
	assert( key_delegate_stack_index < sizeof( key_delegate_stack ) / sizeof( key_delegate_f ) );
	key_delegate_stack[key_delegate_stack_index].key_del = key_del;
	key_delegate_stack[key_delegate_stack_index].char_del = char_del;
	++key_delegate_stack_index;
	if( key_delegate_stack_index == 1 )
	{
		CL_SetOldKeyDest( cls.key_dest );
		CL_SetKeyDest( key_delegate );
		return cls.old_key_dest;
	}
	else
		return key_delegate;
}

/*
* Key_DelegatePop
*/
void Key_DelegatePop( keydest_t next_dest )
{
	assert( key_delegate_stack_index > 0 );
	--key_delegate_stack_index;
	CL_SetKeyDest( next_dest );
}

/*
* Key_DelegateCallKeyDel
*/
static void Key_DelegateCallKeyDel( int key )
{
	assert( key_delegate_stack_index > 0 );
	key_delegate_stack[key_delegate_stack_index - 1].key_del( key, keydown );
}

/*
* Key_DelegateCallCharDel
*/
static void Key_DelegateCallCharDel( int key )
{
	assert( key_delegate_stack_index > 0 );
	key_delegate_stack[key_delegate_stack_index - 1].char_del( key );
}
