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

//
// console
//

#define	NUM_CON_TIMES 4

// global definition to activate case-sensitivity of console (1 == activated)
#define	    CON_CASE_SENSITIVE 0

#define	    CON_TEXTSIZE    65536
typedef struct
{
	qboolean initialized;

	char text[CON_TEXTSIZE];
	int current;        // line where next message will be printed
	int x;              // offset in current line for next print
	int display;        // bottom of console displays this line

	int linewidth;      // characters across screen
	int totallines;     // total lines in console scrollback

	float cursorspeed;

	int vislines;

	float times[NUM_CON_TIMES]; // cls.realtime time the line was generated
	// for transparent notify lines
} console_t;

extern console_t con;

void Con_CheckResize( void );
void Con_Init( void );
void Con_DrawConsole( float frac );
void Con_Print( char *txt );
void Con_CenteredPrint( char *text );
void Con_Clear_f( void );
void Con_DrawNotify( void );
void Con_ClearNotify( void );
void Con_ToggleConsole_f( void );
void Con_Paste( void );
void Con_CompleteCommandLine( void );
void Con_Close( void );

void Con_KeyDown( int key );
void Con_CharEvent( int key );

int Q_ColorCharCount( const char *s, int byteofs );
int Q_ColorCharOffset( const char *s, int charcount );
