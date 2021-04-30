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
// console.c

#include "client.h"

console_t con;

cvar_t *con_notifytime;
cvar_t *con_drawNotify;
cvar_t *con_printText;

static qboolean con_initialized = qfalse;

#define		MAXCMDLINE  256
char key_lines[32][MAXCMDLINE];
unsigned int key_linepos;
static int edit_line = 0;
static int history_line = 0;
qboolean key_insert = qtrue;

/*
* Con_ClearTyping
*/
static void Con_ClearTyping( void )
{
	key_lines[edit_line][1] = 0; // clear any typing
	key_linepos = 1;
}

/*
* Con_Close
*/
void Con_Close( void )
{
	scr_con_current = 0;

	Con_ClearTyping();
	Con_ClearNotify();
	Key_ClearStates();
}

/*
* Con_ToggleConsole_f
*/
void Con_ToggleConsole_f( void )
{
	SCR_EndLoadingPlaque(); // get rid of loading plaque

	if( cls.state == CA_CONNECTING || cls.state == CA_CONNECTED )
		return;

	Con_ClearTyping();
	Con_ClearNotify();

	if( cls.key_dest == key_console )
	{                               // close console
		CL_SetKeyDest( cls.old_key_dest );
	}
	else
	{                               // open console
		CL_SetOldKeyDest( cls.key_dest );
		CL_SetKeyDest( key_console );
	}
}

/*
* Con_Clear_f
*/
void Con_Clear_f( void )
{
	memset( con.text, ' ', CON_TEXTSIZE );
	con.display = con.current;
}

/*
* Con_SkipEmptyLines
*/
static int Con_SkipEmptyLines( void )
{
	int l, x;
	const char *line;

	// skip empty lines
	for( l = con.current - con.totallines + 1; l <= con.current; l++ )
	{
		line = con.text + ( l%con.totallines )*con.linewidth;
		for( x = 0; x < con.linewidth; x++ )
			if( line[x] != ' ' )
				break;
		if( x != con.linewidth )
			break;
	}

	return l;
}

/*
* Con_BufferText
*
* Dumps console text into human-readable format
*/
static size_t Con_BufferText( char *buffer, const char *delim )
{
	int l, x;
	const char *line;
	size_t length, delim_len = strlen( delim );

	if( !con_initialized )
		return 0;

	// skip empty lines
	l = Con_SkipEmptyLines();

	// write the remaining lines
	length = 0;
	for( ; l <= con.current; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for( x = con.linewidth; x > 0; x-- )
		{
			if( line[x-1] != ' ' )
				break;
		}

		if( buffer )
		{
			memcpy( buffer + length, line, x );
			memcpy( buffer + length + x, delim, delim_len );
		}

		length += x + delim_len;
	}

	if( buffer )
		buffer[length] = '\0';

	return length;
}

/*
* Con_Dump_f
*/
static void Con_Dump_f( void )
{
	int file;
	size_t buffer_size;
	char *buffer;
	size_t name_size;
	char *name;
	const char *newline = "\r\n";

	if( !con_initialized )
		return;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "usage: condump <filename>\n" );
		return;
	}

	name_size = sizeof( char ) * ( strlen( Cmd_Argv( 1 ) ) + strlen( ".txt" ) + 1 );
	name = Mem_TempMalloc( name_size );

	Q_strncpyz( name, Cmd_Argv( 1 ), name_size );
	COM_DefaultExtension( name, ".txt", name_size );
	COM_SanitizeFilePath( name );

	if( !COM_ValidateRelativeFilename( name ) )
	{
		Com_Printf( "Invalid filename.\n" );
		Mem_TempFree( name );
		return;
	}

	if( FS_FOpenFile( name, &file, FS_WRITE ) == -1 )
	{
		Com_Printf( "Couldn't open: %s\n", name );
		Mem_TempFree( name );
		return;
	}

	buffer_size = Con_BufferText( NULL, newline ) + 1;
	buffer = Mem_TempMalloc( buffer_size );

	Con_BufferText( buffer, newline );

	FS_Write( buffer, buffer_size - 1, file );

	FS_FCloseFile( file );

	Mem_TempFree( buffer );

	Com_Printf( "Dumped console text: %s\n", name );
	Mem_TempFree( name );
}

/*
* Con_ClearNotify
*/
void Con_ClearNotify( void )
{
	int i;

	for( i = 0; i < NUM_CON_TIMES; i++ )
		con.times[i] = 0;
}

/*
* Con_MessageMode_f
*/
static void Con_MessageMode_f( void )
{
	chat_team = qfalse;
	if( cls.state == CA_ACTIVE )
		CL_SetKeyDest( key_message );
}

/*
* Con_MessageMode2_f
*/
static void Con_MessageMode2_f( void )
{
	chat_team = qtrue;
	if( cls.state == CA_ACTIVE )
		CL_SetKeyDest( key_message );
}

/*
* Con_CheckResize
*/
void Con_CheckResize( void )
{
	int i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char tbuf[CON_TEXTSIZE];

	width = viddef.width / SMALL_CHAR_WIDTH - 2;

	if( width == con.linewidth )
		return;

	if( width < 1 )     // video hasn't been initialized yet
	{
		width = 78;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		memset( con.text, ' ', CON_TEXTSIZE );
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if( con.totallines < numlines )
			numlines = con.totallines;

		numchars = oldwidth;

		if( con.linewidth < numchars )
			numchars = con.linewidth;

		memcpy( tbuf, con.text, CON_TEXTSIZE );
		memset( con.text, ' ', CON_TEXTSIZE );

		for( i = 0; i < numlines; i++ )
		{
			for( j = 0; j < numchars; j++ )
			{
				con.text[( con.totallines - 1 - i ) * con.linewidth + j] =
					tbuf[( ( con.current - i + oldtotallines ) %
					oldtotallines ) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}

/*
* Con_Init
*/
void Con_Init( void )
{
	int i;

	con.linewidth = -1;

	Con_CheckResize();
	Com_Printf( "Console initialized.\n" );

	//
	// register our commands
	//
	con_notifytime = Cvar_Get( "con_notifytime", "3", CVAR_ARCHIVE );
	con_drawNotify = Cvar_Get( "con_drawNotify", "1", CVAR_ARCHIVE );
	con_printText  = Cvar_Get( "con_printText", "1", CVAR_ARCHIVE );

	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f );
	Cmd_AddCommand( "messagemode", Con_MessageMode_f );
	Cmd_AddCommand( "messagemode2", Con_MessageMode2_f );
	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );
	con.initialized = qtrue;

	for( i = 0; i < 32; i++ )
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}

	key_linepos = 1;
	con_initialized = qtrue;
}

/*
* Con_Linefeed
*/
static void Con_Linefeed( void )
{
	con.x = 0;
	if( con.display /* == con.current*/ )
		con.display++;
	con.current++;
	memset( &con.text[( con.current%con.totallines )*con.linewidth], ' ', con.linewidth );
}

/*
* Con_Print
* 
* Handles cursor positioning, line wrapping, etc
* All console printing must go through this in order to be logged to disk
* If no console is visible, the text will appear at the top of the game window
*/
void Con_Print( char *txt )
{
	int y;
	int c, l;
	static int cr;
	int color;
	qboolean colorflag = qfalse;

	if( !con.initialized )
		return;

	if( con_printText->integer == 0 )
		return;

	color = ColorIndex( COLOR_WHITE );

	while( ( c = *txt ) )
	{
		// count word length
		for( l = 0; l < con.linewidth; l++ )
			if( txt[l] <= ' ' )
				break;

		// word wrap
		if( l != con.linewidth && ( con.x + l > con.linewidth ) )
			con.x = 0;

		if( cr )
		{
			con.current--;
			cr = qfalse;
		}

		if( !con.x )
		{
			Con_Linefeed();
			// mark time for transparent overlay
			if( con.current >= 0 )
				con.times[con.current % NUM_CON_TIMES] = cls.realtime;

			y = con.current % con.totallines;

			if( color != ColorIndex( COLOR_WHITE ) )
			{
				con.text[y*con.linewidth] = Q_COLOR_ESCAPE;
				con.text[y*con.linewidth+1] = '0' + color;
				con.x += 2;
			}
		}

		switch( c )
		{
		case '\n':
			color = ColorIndex( COLOR_WHITE );
			con.x = 0;
			break;

		case '\r':
			color = ColorIndex( COLOR_WHITE );
			con.x = 0;
			cr = 1;
			break;

		default: // display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = c;
			con.x++;
			if( con.x >= con.linewidth )
				con.x = 0;

			if( colorflag )
			{
				if( *txt != Q_COLOR_ESCAPE )
					color = ColorIndex( *txt );
				colorflag = qfalse;
			}
			else if( *txt == Q_COLOR_ESCAPE )
				colorflag = qtrue;

			//			if( Q_IsColorString( txt ) ) {
			//				color = ColorIndex( *(txt+1) );
			//			}
			break;
		}

		txt++;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/

/*
* Con_DrawConsoleLine
* 
* Console lines need special handling cause they aren't zero terminated
*/
static void Con_DrawConsoleLine( int x, int y, int align, const char *str, size_t len, struct mufont_s *font, vec4_t color )
{
	static char consoleString[MAX_STRING_CHARS];

	Q_strncpyz( consoleString, str, len + 1 );
	SCR_DrawString( x, y, align, consoleString, font, color );
}

/*
* Q_ColorCharCount
*/
int Q_ColorCharCount( const char *s, int byteofs )
{
	char c;
	const char *end = s + byteofs;
	int charcount = 0;

	while( s < end )
	{
		int gc = COM_GrabCharFromColorString( &s, &c, NULL );
		if( gc == GRABCHAR_CHAR )
			charcount++;
		else if( gc == GRABCHAR_COLOR )
			;
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return charcount;
}

/*
* Q_ColorCharOffset
*/
int Q_ColorCharOffset( const char *s, int charcount )
{
	const char *start = s;
	char c;

	while( *s && charcount )
	{
		int gc = COM_GrabCharFromColorString( &s, &c, NULL );
		if( gc == GRABCHAR_CHAR )
			charcount--;
		else if( gc == GRABCHAR_COLOR )
			;
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return s - start;
}

/*
* Q_ColorStrLastColor
*/
static int Q_ColorStrLastColor( const char *s, int byteofs )
{
	char c;
	const char *end = s + byteofs;
	int lastcolor = ColorIndex(COLOR_WHITE), colorindex;

	while( s < end )
	{
		int gc = COM_GrabCharFromColorString( &s, &c, &colorindex );
		if( gc == GRABCHAR_CHAR )
			;
		else if( gc == GRABCHAR_COLOR )
			lastcolor = colorindex;
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return lastcolor;
}

/*
* Con_DrawInput
*/
static void Con_DrawInput( void )
{
	char *text;
	int colorlinepos;
	int startcolor = ColorIndex( COLOR_WHITE );
	int byteofs;
	int smallCharHeight = SCR_strHeight( cls.fontSystemSmall );

	if( cls.key_dest != key_console )
		return;

	text = key_lines[edit_line];

	// convert byte offset to visible character count
	colorlinepos = Q_ColorCharCount( text, key_linepos );

	// prestep if horizontally scrolling
	if( colorlinepos >= con.linewidth + 1 )
	{
		byteofs = Q_ColorCharOffset( text, colorlinepos - con.linewidth );
		startcolor = Q_ColorStrLastColor( text, byteofs );
		text += byteofs;
		colorlinepos = con.linewidth;
	}

	// draw it
	SCR_DrawStringWidth ( 8, con.vislines-smallCharHeight-14, ALIGN_LEFT_TOP, text, viddef.width - 8 - 8, cls.fontSystemSmall, color_table[startcolor] );

	// add the cursor frame
	if( (int)( cls.realtime>>8 )&1 )
	{
		int widthoffset;
		if( key_insert )
		{
			char keychar;
			keychar = text[key_linepos];
			text[key_linepos] = 0;
			widthoffset = 8+SCR_strWidth( text, cls.fontSystemSmall, 0 );
			text[key_linepos] = keychar;
		}
		else
		{
			widthoffset = 8+SCR_strWidth( text, cls.fontSystemSmall, 0 );
		}
		SCR_DrawRawChar( widthoffset, con.vislines-smallCharHeight-14, '_', cls.fontSystemSmall, colorWhite );
	}
}

/*
* Con_DrawNotify
*/
void Con_DrawNotify( void )
{
	int v;
	char *text;
	int i;
	int time;
	char *s;
	int skip;
	unsigned int charbuffer_width;

	v = 0;
	if( con_drawNotify->integer )
	{
		for( i = con.current-NUM_CON_TIMES+1; i <= con.current; i++ )
		{
			if( i < 0 )
				continue;
			time = con.times[i % NUM_CON_TIMES];
			if( time == 0 )
				continue;
			time = cls.realtime - time;
			if( time > con_notifytime->value*1000 )
				continue;
			text = con.text + ( i % con.totallines )*con.linewidth;

			Con_DrawConsoleLine( 8, v, ALIGN_LEFT_TOP, text, con.linewidth, cls.fontSystemSmall, colorWhite );

			v += SCR_strHeight( cls.fontSystemSmall );
		}
	}

	if( cls.key_dest == key_message )
	{
		if( chat_team )
		{
			SCR_DrawString( 8, v, ALIGN_LEFT_TOP, "say_team:", cls.fontSystemSmall, colorWhite );
			skip = 8 + SCR_strWidth( "say_team: ", cls.fontSystemSmall, 0 );
		}
		else
		{
			SCR_DrawString( 8, v, ALIGN_LEFT_TOP, "say:", cls.fontSystemSmall, colorWhite );
			skip = 8 + SCR_strWidth( "say: ", cls.fontSystemSmall, 0 );
		}

		s = chat_buffer;
		charbuffer_width = SCR_strWidth( s, cls.fontSystemSmall, chat_bufferlen+1 );
		while( charbuffer_width > viddef.width - ( skip+72 ) )
		{                                              // 72 is an arbitrary offset for not overlapping the FPS and clock prints
			s++;
			charbuffer_width = SCR_strWidth( s, cls.fontSystemSmall, chat_bufferlen+1 );
		}

		SCR_DrawString( skip, v, ALIGN_LEFT_TOP, s, cls.fontSystemSmall, colorWhite );
		skip += SCR_strWidth( s, cls.fontSystemSmall, 0 );
		SCR_DrawRawChar( skip, v, ( ( cls.realtime>>8 )&1 ) ? '_' : ' ', cls.fontSystemSmall, colorWhite );

		v += SCR_strHeight( cls.fontSystemSmall );
	}
}

/*
* Con_DrawConsole
*/
void Con_DrawConsole( float frac )
{
	int i, x, y;
	int rows;
	char *text;
	int row;
	unsigned int lines;
	char version[256];
	time_t long_time;
	struct tm *newtime;
	int smallCharHeight = SCR_strHeight( cls.fontSystemSmall );

	lines = viddef.height * frac;
	if( lines <= 0 )
		return;

	if( lines > viddef.height )
		lines = viddef.height;

	// draw the background
	R_DrawStretchPic( 0, 0, viddef.width, lines, 0, 0, 1, 1, colorWhite, cls.consoleShader );
	SCR_DrawFillRect( 0, lines - 2, viddef.width, 2, colorRed );

	// get date from system
	time( &long_time );
	newtime = localtime( &long_time );

	Q_snprintfz( version, sizeof( version ), "%02d:%02d %s v%4.2f rev:%d", newtime->tm_hour, newtime->tm_min,
		APPLICATION, APP_VERSION, SVN_RevNumber() );
	SCR_DrawString( viddef.width-SCR_strWidth( version, cls.fontSystemSmall, 0 )-4, lines-20, ALIGN_LEFT_TOP, version,
		cls.fontSystemSmall, colorRed );

	// draw the text
	con.vislines = lines;

	rows = ( lines-smallCharHeight-14 ) / smallCharHeight;  // rows of text to draw
	y = lines - smallCharHeight-14-smallCharHeight;

	// draw from the bottom up
	if( con.display != con.current )
	{
		// draw arrows to show the buffer is backscrolled
		for( x = 0; x < con.linewidth; x += 4 )
			SCR_DrawRawChar( ( x+1 )*SCR_strWidth( "^", cls.fontSystemSmall, 0 ), y, '^', cls.fontSystemSmall, colorRed );

		y -= smallCharHeight;
		rows--;
	}

	row = con.display;
	for( i = 0; i < rows; i++, y -= smallCharHeight, row-- )
	{
		if( row < 0 )
			break;
		if( con.current - row >= con.totallines )
			break; // past scrollback wrap point

		text = con.text + ( row % con.totallines )*con.linewidth;

		Con_DrawConsoleLine( 8, y, ALIGN_LEFT_TOP, text, con.linewidth, cls.fontSystemSmall, colorWhite );
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();
}

/*
* Con_DisplayList
* 
* New function for tab-completion system
* Added by EvilTypeGuy
* MEGA Thanks to Taniwha
*/
static void Con_DisplayList( char **list )
{
	int i = 0;
	int pos = 0;
	int len = 0;
	int maxlen = 0;
	int width = ( con.linewidth - 4 );
	char **walk = list;

	while( *walk )
	{
		len = (int)strlen( *walk );
		if( len > maxlen )
			maxlen = len;
		walk++;
	}
	maxlen += 1;

	while( *list )
	{
		len = (int)strlen( *list );

		if( pos + maxlen >= width )
		{
			Com_Printf( "\n" );
			pos = 0;
		}

		Com_Printf( "%s", *list );
		for( i = 0; i < ( maxlen - len ); i++ )
			Com_Printf( " " );

		pos += maxlen;
		list++;
	}

	if( pos )
		Com_Printf( "\n\n" );
}

/*
* Con_CompleteCommandLine
* 
* New function for tab-completion system
* Added by EvilTypeGuy
* Thanks to Fett erich@heintz.com
* Thanks to taniwha
*/
void Con_CompleteCommandLine( void )
{
	char *cmd = "";
	char *s;
	int c, v, a, ca, i;
	int cmd_len;
	char **list[5] = { 0, 0, 0, 0, 0 };

	s = key_lines[edit_line] + 1;
	if( *s == '\\' || *s == '/' )
		s++;
	if( !*s )
		return;

	// Count number of possible matches
	c = Cmd_CompleteCountPossible( s );
	v = Cvar_CompleteCountPossible( s );
	a = Cmd_CompleteAliasCountPossible( s );
	ca = 0;

	if( !( c + v + a ) )
	{   
		// now see if there's any valid cmd in there, providing
		// a list of matching arguments
		list[4] = Cmd_CompleteBuildArgList( s );
		if( !list[4] )
		{
			// No possible matches, let the user know they're insane
			Com_Printf( "\nNo matching aliases, commands or cvars were found.\n\n" );
			return;
		}

		// count the number of matching arguments
		for( ca = 0; list[4][ca]; ca++ );

		if( !ca )
		{
			// the list is empty, although non-NULL list pointer suggests that the command
			// exists, so clean up and exit without printing anything
			Mem_TempFree( list[4] );
			return;
		}
	}

	if( c + v + a + ca == 1 )
	{
		// find the one match to rule them all
		if( c )
			list[0] = Cmd_CompleteBuildList( s );
		else if( v )
			list[0] = Cvar_CompleteBuildList( s );
		else if( a )
			list[0] = Cmd_CompleteAliasBuildList( s );
		else
			list[0] = list[4], list[4] = NULL;

		cmd = *list[0];
		cmd_len = (int)strlen( cmd );
	}
	else
	{
		int i_start = 0;

		if( c )
			cmd = *( list[0] = Cmd_CompleteBuildList( s ) );
		if( v )
			cmd = *( list[1] = Cvar_CompleteBuildList( s ) );
		if( a )
			cmd = *( list[2] = Cmd_CompleteAliasBuildList( s ) );
		if( ca )
			s = strstr( s, " " ) + 1, cmd = *( list[4] ), i_start = 4;

		cmd_len = (int)strlen( s );
		do
		{
			for( i = i_start; i < 5; i++ )
			{
				char ch = cmd[cmd_len];
				char **l = list[i];
				if( l )
				{
					while( *l && ( *l )[cmd_len] == ch )
						l++;
					if( *l )
						break;
				}
			}
			if( i == 5 )
				cmd_len++;
		}
		while( i == 5 );

		// Print Possible Commands
		if( c )
		{
			Com_Printf( S_COLOR_RED "%i possible command%s%s\n", c, ( c > 1 ) ? "s: " : ":", S_COLOR_WHITE );
			Con_DisplayList( list[0] );
		}

		if( v )
		{
			Com_Printf( S_COLOR_CYAN "%i possible variable%s%s\n", v, ( v > 1 ) ? "s: " : ":", S_COLOR_WHITE );
			Con_DisplayList( list[1] );
		}

		if( a )
		{
			Com_Printf( S_COLOR_MAGENTA "%i possible alias%s%s\n", a, ( a > 1 ) ? "es: " : ":", S_COLOR_WHITE );
			Con_DisplayList( list[2] );
		}

		if( ca )
		{
			Com_Printf( S_COLOR_YELLOW "%i possible argument%s%s\n", ca, ( ca > 1 ) ? "s: " : ":", S_COLOR_WHITE );
			Con_DisplayList( list[4] );
		}
	}

	if( cmd )
	{
		int skip = 1;
		char *cmd_temp = NULL, *p;

		if( ca )
		{
			size_t temp_size;

			temp_size = sizeof( key_lines[edit_line] );
			cmd_temp = Mem_TempMalloc( temp_size );

			Q_strncpyz( cmd_temp, key_lines[edit_line] + skip, temp_size );
			p = strstr( cmd_temp, " " );
			if( p )
				*(p+1) = '\0';

			cmd_len += strlen( cmd_temp );

			Q_strncatz( cmd_temp, cmd, temp_size );
			cmd = cmd_temp;
		}

		Q_strncpyz( key_lines[edit_line] + skip, cmd, sizeof( key_lines[edit_line] ) - ( 1 + skip ) );
		key_linepos = min( cmd_len + skip, sizeof( key_lines[edit_line] ) - 1 );

		if( c + v + a == 1 && key_linepos < sizeof( key_lines[edit_line] ) - 1 )
		{
			key_lines[edit_line][key_linepos] = ' ';
			key_linepos++;
		}
		key_lines[edit_line][key_linepos] = 0;

		if( cmd == cmd_temp )
			Mem_TempFree( cmd );
	}

	for( i = 0; i < 5; ++i )
	{
		if( list[i] )
			Mem_TempFree( list[i] );
	}
}

/*
==============================================================================

LINE TYPING INTO THE CONSOLE

==============================================================================
*/


/*
* Con_Key_Copy
* 
* Copies console text to clipboard
* Should be Con_Copy prolly
*/
static void Con_Key_Copy( void )
{
	size_t buffer_size;
	char *buffer;
	const char *newline = "\r\n";

	buffer_size = Con_BufferText( NULL, newline ) + 1;
	buffer = Mem_TempMalloc( buffer_size );

	Con_BufferText( buffer, newline );

	CL_SetClipboardData( buffer );

	Mem_TempFree( buffer );
}

/*
* Con_Key_Paste
* 
* Inserts stuff from clipboard to console
* Should be Con_Paste prolly
*/
static void Con_Key_Paste( qboolean primary )
{
	char *cbd;
	char *tok;

	cbd = CL_GetClipboardData( primary );
	if( cbd )
	{
		int i;

		tok = strtok( cbd, "\n\r\b" );

		while( tok != NULL )
		{
			i = (int)strlen( tok );
			if( i + key_linepos >= MAXCMDLINE )
				i = MAXCMDLINE - key_linepos;

			if( i > 0 )
			{
				Q_strncatz( key_lines[edit_line], tok, sizeof( key_lines[edit_line] ) );
				key_linepos += i;
			}

			tok = strtok( NULL, "\n\r\b" );

			if( tok != NULL )
			{
				if( key_lines[edit_line][1] == '\\' || key_lines[edit_line][1] == '/' )
					Cbuf_AddText( key_lines[edit_line] + 2 ); // skip the >
				else
					Cbuf_AddText( key_lines[edit_line] + 1 ); // valid command

				Cbuf_AddText( "\n" );
				Com_Printf( "%s\n", key_lines[edit_line] );
				edit_line = ( edit_line + 1 ) & 31;
				history_line = edit_line;
				key_lines[edit_line][0] = ']';
				key_lines[edit_line][1] = 0;
				key_linepos = 1;
				if( cls.state == CA_DISCONNECTED )
					SCR_UpdateScreen(); // force an update, because the command may take some time
			}
		}

		CL_FreeClipboardData( cbd );
	}
}

/*
* 
*/
void Con_CharEvent( int key )
{
	if( cls.state == CA_CONNECTING || cls.state == CA_CONNECTED )
		return;

	switch( key )
	{
	case 22: // CTRL - V : paste
		Con_Key_Paste( qfalse );
		return;

	case 12: // CTRL - L : clear
		Cbuf_AddText( "clear\n" );
		return;

	case 16: // CTRL+P : history prev
		do
		{
			history_line = ( history_line - 1 ) & 31;
		}
		while( history_line != edit_line && !key_lines[history_line][1] );

		if( history_line == edit_line )
			history_line = ( edit_line+1 )&31;

		Q_strncpyz( key_lines[edit_line], key_lines[history_line], sizeof( key_lines[edit_line] ) );
		key_linepos = (unsigned int)strlen( key_lines[edit_line] );
		return;

	case 14: // CTRL+N : history next
		if( history_line == edit_line ) 
			return;

		do
		{
			history_line = ( history_line + 1 ) & 31;
		}
		while( history_line != edit_line && !key_lines[history_line][1] );

		if( history_line == edit_line )
		{
			key_lines[edit_line][0] = ']';
			key_linepos = 1;
		}
		else
		{
			Q_strncpyz( key_lines[edit_line], key_lines[history_line], sizeof( key_lines[edit_line] ) );
			key_linepos = (unsigned int)strlen( key_lines[edit_line] );
		}

		return;

	case 3: // CTRL+C: copy text to clipboard
		Con_Key_Copy();
		return;

		// maybe add CTRL-A : HOME and CTRL-E : END
	}

	if( key < 32 || key > 126 )
		return; // non printable

	if( key_linepos < MAXCMDLINE-1 )
	{
		unsigned i;

		// check insert mode
		if( key_insert )
		{
			// can't do strcpy to move string to right
			key_lines[edit_line][MAXCMDLINE-1] = 0;
			for( i = MAXCMDLINE-3; i >= key_linepos; i-- )
				key_lines[edit_line][i + 1] = key_lines[edit_line][i];
		}

		// only null terminate if at the end
		i = key_lines[edit_line][key_linepos];
		key_lines[edit_line][key_linepos] = key;
		key_linepos++;
		if( !i )
			key_lines[edit_line][key_linepos] = 0;
	}
}

/*
* Con_KeyDown
*
* Interactive line editing and console scrollback except for ascii char
*/
void Con_KeyDown( int key )
{
	if( cls.state == CA_CONNECTING || cls.state == CA_CONNECTED )
		return;

	switch( key )
	{
	case KP_SLASH:
		key = '/';
		break;
	case KP_MINUS:
		key = '-';
		break;
	case KP_PLUS:
		key = '+';
		break;
	case KP_HOME:
		key = '7';
		break;
	case KP_UPARROW:
		key = '8';
		break;
	case KP_PGUP:
		key = '9';
		break;
	case KP_LEFTARROW:
		key = '4';
		break;
	case KP_5:
		key = '5';
		break;
	case KP_RIGHTARROW:
		key = '6';
		break;
	case KP_END:
		key = '1';
		break;
	case KP_DOWNARROW:
		key = '2';
		break;
	case KP_PGDN:
		key = '3';
		break;
	case KP_INS:
		key = '0';
		break;
	case KP_DEL:
		key = '.';
		break;
	}

	if( ( ( key == K_INS ) || ( key == KP_INS ) ) && Key_IsDown(K_SHIFT) )
	{
		Con_Key_Paste( qtrue );
		return;
	}

	if( key == K_ENTER || key == KP_ENTER )
	{ 
		// backslash text are commands, else chat
		if( key_lines[edit_line][1] == '\\' || key_lines[edit_line][1] == '/' )
			Cbuf_AddText( key_lines[edit_line]+2 ); // skip the >
		else
			Cbuf_AddText( key_lines[edit_line]+1 ); // valid command

		Cbuf_AddText( "\n" );
		Com_Printf( "%s\n", key_lines[edit_line] );
		edit_line = ( edit_line + 1 ) & 31;
		history_line = edit_line;
		key_lines[edit_line][0] = ']';
		key_lines[edit_line][1] = 0;
		key_linepos = 1;
		if( cls.state == CA_DISCONNECTED )
			SCR_UpdateScreen(); // force an update, because the command
		// may take some time
		return;
	}

	if( key == K_TAB )
	{ 
		// command completion
		Con_CompleteCommandLine();
		return;
	}

	if( ( key == K_LEFTARROW ) || ( key == KP_LEFTARROW ) )
	{
		int charcount;
		// jump over invisible color sequences
		charcount = Q_ColorCharCount( key_lines[edit_line], key_linepos );
		if( charcount > 1 )
			key_linepos = Q_ColorCharOffset( key_lines[edit_line], charcount - 1 );
		return;
	}

	if( ( key == K_BACKSPACE ) )
	{
		if( key_linepos > 1 )
		{
			while( 1 )
			{
				char c;
				const char *tmp = key_lines[edit_line] + key_linepos;
				if( COM_GrabCharFromColorString( &tmp, &c, NULL ) == GRABCHAR_COLOR )
					key_linepos = tmp - key_lines[edit_line]; // advance, try again
				else	// GRABCHAR_CHAR or GRABCHAR_END
					break;
			}

			Q_strncpyz( key_lines[edit_line] + key_linepos - 1, key_lines[edit_line] + key_linepos,
				sizeof( key_lines[edit_line] ) - ( key_linepos - 1 ) );
			key_linepos--;
		}

		return;
	}

	if( key == K_DEL )
	{
		if( key_linepos < strlen( key_lines[edit_line] ) )
			Q_strncpyz( key_lines[edit_line] + key_linepos, key_lines[edit_line] + key_linepos + 1,
			sizeof( key_lines[edit_line] ) - key_linepos );
		return;
	}

	if( key == K_INS )
	{ 
		// toggle insert mode
		key_insert = !key_insert;
		return;
	}

	if( key == K_RIGHTARROW )
	{
		if( strlen( key_lines[edit_line] ) == key_linepos )
		{
			if( strlen( key_lines[( edit_line + 31 ) & 31] ) <= key_linepos )
				return;

			key_lines[edit_line][key_linepos] = key_lines[( edit_line + 31 ) & 31][key_linepos];
			key_linepos++;
			key_lines[edit_line][key_linepos] = 0;
		}
		else
		{
			int charcount;
			// jump over invisible color sequences
			charcount = Q_ColorCharCount( key_lines[edit_line], key_linepos );
			key_linepos = Q_ColorCharOffset( key_lines[edit_line], charcount + 1 );
		}
		return;
	}

	if( ( key == K_UPARROW ) || ( key == KP_UPARROW ) )
	{
		do
		{
			history_line = ( history_line - 1 ) & 31;
		}
		while( history_line != edit_line
			&& !key_lines[history_line][1] );
		if( history_line == edit_line )
			history_line = ( edit_line+1 )&31;
		Q_strncpyz( key_lines[edit_line], key_lines[history_line], sizeof( key_lines[edit_line] ) );
		key_linepos = (unsigned int)strlen( key_lines[edit_line] );
		return;
	}

	if( ( key == K_DOWNARROW ) || ( key == KP_DOWNARROW ) )
	{
		if( history_line == edit_line ) return;
		do
		{
			history_line = ( history_line + 1 ) & 31;
		}
		while( history_line != edit_line
			&& !key_lines[history_line][1] );
		if( history_line == edit_line )
		{
			key_lines[edit_line][0] = ']';
			key_linepos = 1;
		}
		else
		{
			Q_strncpyz( key_lines[edit_line], key_lines[history_line], sizeof( key_lines[edit_line] ) );
			key_linepos = (unsigned int)strlen( key_lines[edit_line] );
		}
		return;
	}

	if( key == K_PGUP || key == KP_PGUP || key == K_MWHEELUP ) // support mwheel in console
	{
		con.display -= 2;
		return;
	}

	if( key == K_PGDN || key == KP_PGDN || key == K_MWHEELDOWN ) // support mwheel in console
	{
		con.display += 2;
		if( con.display > con.current )
			con.display = con.current;
		return;
	}

	if( key == K_HOME || key == KP_HOME )
	{
		if( Key_IsDown(K_CTRL) )
		{
			int smallCharHeight = SCR_strHeight( cls.fontSystemSmall );
			int rows = ( con.vislines-smallCharHeight-14 ) / smallCharHeight;  // rows of text to draw

			con.display = Con_SkipEmptyLines() + rows - 2;
		}
		else
			key_linepos = 1;

		return;
	}

	if( key == K_END || key == KP_END )
	{
		if( Key_IsDown(K_CTRL) )
			con.display = con.current;
		else
			key_linepos = (unsigned int)strlen( key_lines[edit_line] );
		return;
	}

	// key is a normal printable key normal which wil be HANDLE later in response to WM_CHAR event
}
