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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

full screen console
put up loading plaque
blanked background with loading plaque
blanked background with menu
cinematics
full screen image for quit and victory

end of unit intermissions

*/

#include "client.h"

float scr_con_current;    // aproaches scr_conlines at scr_conspeed
float scr_conlines;       // 0.0 to 1.0 lines of console to display

qboolean scr_initialized;    // ready to draw

static cvar_t *scr_consize;
static cvar_t *scr_conspeed;
static cvar_t *scr_netgraph;
static cvar_t *scr_timegraph;
static cvar_t *scr_debuggraph;
static cvar_t *scr_graphheight;
static cvar_t *scr_graphscale;
static cvar_t *scr_graphshift;
static cvar_t *scr_forceclear;

/*
===============================================================================

MUFONT STRINGS

===============================================================================
*/


//
//	Variable width (proportional) fonts
//

//===============================================================================
//FONT LOADING
//===============================================================================

mempool_t *fonts_mempool;

#define Font_Alloc( size ) Mem_Alloc( fonts_mempool, size )
#define Font_Free( size ) Mem_Free( size )

typedef struct
{
	short x, y;
	qbyte width, height;
	float s1, t1, s2, t2;
} muchar_t;
typedef struct mufont_s
{
	char name[MAX_QPATH];
	muchar_t chars[256];
	int fontheight;
	float imagewidth, imageheight;
	struct shader_s	*shader;
	struct mufont_s	*next;
} mufont_t;

static mufont_t *SCR_LoadMUFont( const char *name )
{

	char filename[MAX_QPATH];
	qbyte *buf;
	char *ptr, *token;
	int filenum;
	int length;
	mufont_t *font;
	struct shader_s *shader;
	int numchar;

	if( !name || !name[0] )
		return NULL;

	// load the shader
	Q_snprintfz( filename, sizeof( filename ), "fonts/%s", name );
	COM_ReplaceExtension( filename, ".tga", sizeof( filename ) );
	shader = R_RegisterPic( filename );
	if( !shader )
		return NULL;

	// load the font description
	COM_ReplaceExtension( filename, ".wfd", sizeof( filename ) );

	// load the file
	length = FS_FOpenFile( filename, &filenum, FS_READ );
	if( length == -1 )
	{
		return NULL;
	}

	buf = Mem_TempMalloc( length + 1 );
	length = FS_Read( buf, length, filenum );
	FS_FCloseFile( filenum );
	if( !length )
	{
		Mem_TempFree( buf );
		return NULL;
	}

	// seems to be valid. Allocate it
	font = (mufont_t *)Font_Alloc( sizeof( mufont_t ) );
	font->shader = shader;
	Q_strncpyz( font->name, name, sizeof( font->name ) );

	//proceed
	ptr = ( char * )buf;

	// get texture width and height
	token = COM_Parse( &ptr );
	if( !token[0] )
	{
		Font_Free( font );
		Mem_TempFree( buf );
		return NULL;
	}
	font->imagewidth = atoi( token );
	token = COM_Parse( &ptr );
	if( !token[0] )
	{
		Font_Free( font );
		Mem_TempFree( buf );
		return NULL;
	}
	font->imageheight = atoi( token );

	//get the chars
	while( ptr )
	{
		// "<char>" "<x>" "<y>" "<width>" "<height>"
		token = COM_Parse( &ptr );
		if( !token[0] ) break;
		numchar = atoi( token );
		if( numchar <= 0 )
			break;
		if( numchar < 32 || numchar >= 256 )
			continue;

		font->chars[numchar].x = atoi( COM_Parse( &ptr ) );
		font->chars[numchar].y = atoi( COM_Parse( &ptr ) );
		font->chars[numchar].width = atoi( COM_Parse( &ptr ) );
		font->chars[numchar].height = atoi( COM_Parse( &ptr ) );
		// create the texture coordinates
		font->chars[numchar].s1 = ( (float)font->chars[numchar].x )/(float)font->imagewidth;
		font->chars[numchar].s2 = ( (float)( font->chars[numchar].x + font->chars[numchar].width ) )/(float)font->imagewidth;
		font->chars[numchar].t1 = ( (float)font->chars[numchar].y )/(float)font->imageheight;
		font->chars[numchar].t2 = ( (float)( font->chars[numchar].y + font->chars[numchar].height ) )/(float)font->imageheight;
	}

	// mudFont is not always giving a proper size to the space character
	font->chars[' '].width = font->chars['-'].width;

	// height is the same for every character
	font->fontheight = font->chars['a'].height;

	Mem_TempFree( buf );
	return font;
}
mufont_t *gs_muFonts;

//==================
// SCR_RegisterFont
//==================
struct mufont_s *SCR_RegisterFont( const char *name )
{
	mufont_t *font;
	const char	*extension;
	size_t		len;

	extension = COM_FileExtension( name );
	len = ( extension ? extension - name - 1 : strlen( name ) );

	for( font = gs_muFonts; font; font = font->next )
	{
		if( !Q_strnicmp( font->name, name, len ) )
			return font;
	}

	font = SCR_LoadMUFont( name );
	if( !font )
	{
		return NULL;
	}

	font->next = gs_muFonts;
	gs_muFonts = font;

	return font;
}

static void SCR_InitFonts( void )
{
	cvar_t *con_fontSystemSmall = Cvar_Get( "con_fontSystemSmall", DEFAULT_FONT_SMALL, CVAR_ARCHIVE );
	cvar_t *con_fontSystemMedium = Cvar_Get( "con_fontSystemMedium", DEFAULT_FONT_MEDIUM, CVAR_ARCHIVE );
	cvar_t *con_fontSystemBig = Cvar_Get( "con_fontSystemBig", DEFAULT_FONT_BIG, CVAR_ARCHIVE );

	fonts_mempool = Mem_AllocPool( NULL, "Fonts" );

	// register system fonts
	cls.fontSystemSmall = SCR_RegisterFont( con_fontSystemSmall->string );
	if( !cls.fontSystemSmall )
	{
		cls.fontSystemSmall = SCR_RegisterFont( DEFAULT_FONT_SMALL );
		if( !cls.fontSystemSmall )
			Com_Error( ERR_FATAL, "Couldn't load default font \"%s\"", DEFAULT_FONT_SMALL );
	}

	cls.fontSystemMedium = SCR_RegisterFont( con_fontSystemMedium->string );
	if( !cls.fontSystemMedium )
		cls.fontSystemMedium = SCR_RegisterFont( DEFAULT_FONT_MEDIUM );

	cls.fontSystemBig = SCR_RegisterFont( con_fontSystemBig->string );
	if( !cls.fontSystemBig )
		cls.fontSystemBig = SCR_RegisterFont( DEFAULT_FONT_BIG );
}

static void SCR_ShutdownFonts( void )
{
	Mem_FreePool( &fonts_mempool );
	gs_muFonts = NULL;
	cls.fontSystemSmall = NULL;
	cls.fontSystemMedium = NULL;
	cls.fontSystemBig = NULL;
}

//===============================================================================
//STRINGS HELPERS
//===============================================================================


static int SCR_HorizontalAlignForString( const int x, int align, int width )
{
	int nx = x;

	if( align % 3 == 0 )  // left
		nx = x;
	if( align % 3 == 1 )  // center
		nx = x - width / 2;
	if( align % 3 == 2 )  // right
		nx = x - width;

	return nx;
}

static int SCR_VerticalAlignForString( const int y, int align, int height )
{
	int ny = y;

	if( align / 3 == 0 )  // top
		ny = y;
	else if( align / 3 == 1 )  // middle
		ny = y - height / 2;
	else if( align / 3 == 2 )  // bottom
		ny = y - height;

	return ny;
}

//==================
// SCR_strHeight
// it's font height in fact, but for preserving simetry I call it str
//==================
size_t SCR_strHeight( struct mufont_s *font )
{
	if( !font )
		font = cls.fontSystemSmall;

	return font->fontheight;
}

//==================
// SCR_strWidth
// doesn't count invisible characters. Counts up to given length, if any.
//==================
size_t SCR_strWidth( const char *str, struct mufont_s *font, int maxlen )
{
	const char *s = str;
	size_t width = 0;
	char c;
	int num;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	while( *s && *s != '\n' )
	{
		if( maxlen && ( s - str ) >= maxlen )  // stop counting at desired len
			return width;

		switch( COM_GrabCharFromColorString( &s, &c, NULL ) )
		{
		case GRABCHAR_CHAR:
			num = ( unsigned char )c;
			if( num >= ' ' )
				width += font->chars[num].width;
			break;

		case GRABCHAR_COLOR:
			break;

		case GRABCHAR_END:
			return width;

		default:
			assert( 0 );
		}
	}

	return width;
}

//==================
// SCR_StrlenForWidth
// returns the len allowed for the string to fit inside a given width when using a given font.
//==================
size_t SCR_StrlenForWidth( const char *str, struct mufont_s *font, size_t maxwidth )
{
	const char *s = str;
	size_t width = 0;
	char c;
	int num;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	while( *s && *s != '\n' )
	{
		// FIXME: shouldn't we return last known good len if width > maxwidth? -- Tonik
		if( width >= maxwidth )
			return (unsigned)( s - str ); // this is real len (not in-screen len)

		switch( COM_GrabCharFromColorString( &s, &c, NULL ) )
		{
		case GRABCHAR_CHAR:
			num = ( unsigned char )c;
			if( num >= ' ' )
				width += font->chars[num].width;
			break;

		case GRABCHAR_COLOR:
			break;

		case GRABCHAR_END:
			return (unsigned)( s - str );

		default:
			assert( 0 );
		}
	}

	return (unsigned)( s - str );
}


//===============================================================================
//STRINGS DRAWING
//===============================================================================

//================
//SCR_DrawRawChar
//
//Draws one graphics character with 0 being transparent.
//It can be clipped to the top of the screen to allow the console to be
//smoothly scrolled off.
//================
void SCR_DrawRawChar( int x, int y, int num, struct mufont_s *font, vec4_t color )
{
	if( !font )
		font = cls.fontSystemSmall;

	num &= 255;
	if( num <= ' ' )
		return;

	if( y <= -font->fontheight )
		return; // totally off screen

	R_DrawStretchPic( x, y, font->chars[num].width, font->fontheight,
		font->chars[num].s1, font->chars[num].t1, font->chars[num].s2, font->chars[num].t2,
		color, font->shader );
}

static void SCR_DrawClampChar( int x, int y, int num, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color )
{
	float pixelsize;
	float s1, s2, t1, t2;
	int sx, sy, sw, sh;
	int offset;

	if( !font )
		font = cls.fontSystemSmall;

	num &= 255;
	if( num <= ' ' )
		return;

	// ignore if completely out of the drawing space
	if( y + font->fontheight <= ymin || y >= ymax ||
		x + font->chars[num].width <= xmin || x >= xmax )
		return;

	pixelsize = 1.0f / font->imageheight;

	s1 = font->chars[num].s1;
	t1 = font->chars[num].t1;
	s2 = font->chars[num].s2;
	t2 = font->chars[num].t2;

	sx = x;
	sy = y;
	sw = font->chars[num].width;
	sh = font->fontheight;

	// clamp xmin
	if( x < xmin && x + font->chars[num].width >= xmin )
	{
		offset = xmin - x;
		if( offset )
		{
			sx += offset;
			sw -= offset;
			s1 += ( pixelsize * offset );
		}
	}

	// clamp ymin
	if( y < ymin && y + font->chars[num].height >= ymin )
	{
		offset = ymin - y;
		if( offset )
		{
			sy += offset;
			sh -= offset;
			t1 += ( pixelsize * offset );
		}
	}

	// clamp xmax
	if( x < xmax && x + font->chars[num].width >= xmax )
	{
		offset = ( x + font->chars[num].width ) - xmax;
		if( offset != 0 )
		{
			sw -= offset;
			s2 -= ( pixelsize * offset );
		}
	}

	// clamp ymax
	if( y < ymax && y + font->chars[num].height >= ymax )
	{
		offset = ( y + font->chars[num].height ) - ymax;
		if( offset != 0 )
		{
			sh -= offset;
			t2 -= ( pixelsize * offset );
		}
	}

	R_DrawStretchPic( sx, sy, sw, sh, s1, t1, s2, t2, color, font->shader );
}

void SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color )
{
	int xoffset = 0, yoffset = 0;
	vec4_t scolor;
	int num, colorindex;
	char c;
	const char *s = str;
	int gc;

	if( !str )
		return;

	if( !font )
		font = cls.fontSystemSmall;

	// clamp mins and maxs to the screen space
	if( xmin < 0 )
		xmin = 0;
	if( xmax > (int)viddef.width )
		xmax = (int)viddef.width;
	if( ymin < 0 )
		ymin = 0;
	if( ymax > (int)viddef.height )
		ymax = (int)viddef.height;
	if( xmax <= xmin || ymax <= ymin || x > xmax || y > ymax )
		return;

	Vector4Copy( color, scolor );

	while( 1 )
	{
		gc = COM_GrabCharFromColorString( &s, &c, &colorindex );
		if( gc == GRABCHAR_CHAR )
		{
			if( c == '\n' )
				break;

			num = ( unsigned char )c;
			if( num < ' ' )
				continue;

			if( num != ' ' )
				SCR_DrawClampChar( x + xoffset, y + yoffset, num, xmin, ymin, xmax, ymax, font, scolor );

			xoffset += font->chars[num].width;
			if( x + xoffset > xmax )
				break;
		}
		else if( gc == GRABCHAR_COLOR )
		{
			assert( ( unsigned )colorindex < MAX_S_COLORS );
			VectorCopy( color_table[colorindex], scolor );
		}
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}
}

//===============
//SCR_DrawRawString - Doesn't care about aligning. Returns drawn len.
// It can stop when reaching maximum width when a value has been parsed.
//===============
static int SCR_DrawRawString( int x, int y, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	int xoffset = 0, yoffset = 0;
	vec4_t scolor;
	const char *s;
	char c;
	int num, gc, colorindex;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	if( maxwidth < 0 )
		maxwidth = 0;

	Vector4Copy( color, scolor );

	s = str;

	while( 1 )
	{
		gc = COM_GrabCharFromColorString( &s, &c, &colorindex );
		if( gc == GRABCHAR_CHAR )
		{
			if( c == '\n' )
				break;

			num = ( unsigned char )c;
			if( num < ' ' )
				continue;

			if( maxwidth && ( ( xoffset + font->chars[num].width ) > maxwidth ) )
				break;

			if( num != ' ' )
				R_DrawStretchPic( x+xoffset, y+yoffset, font->chars[num].width, font->chars[num].height,
				font->chars[num].s1, font->chars[num].t1, font->chars[num].s2, font->chars[num].t2,
				scolor, font->shader );

			xoffset += font->chars[num].width;
		}
		else if( gc == GRABCHAR_COLOR )
		{
			assert( ( unsigned )colorindex < MAX_S_COLORS );
			VectorCopy( color_table[colorindex], scolor );
		}
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return ( s - str );
}

//==================
// SCR_DrawString
//==================
void SCR_DrawString( int x, int y, int align, const char *str, struct mufont_s *font, vec4_t color )
{
	int width;

	if( !str )
		return;

	if( !font )
		font = cls.fontSystemSmall;

	width = SCR_strWidth( str, font, 0 );
	if( width )
	{
		x = SCR_HorizontalAlignForString( x, align, width );
		y = SCR_VerticalAlignForString( y, align, font->fontheight );

		if( y <= -font->fontheight || y >= (int)viddef.height )
			return; // totally off screen

		if( x <= -width  || x >= (int)viddef.width )
			return; // totally off screen

		SCR_DrawRawString( x, y, str, 0, font, color );
	}
}

//===============
//SCR_DrawStringWidth - clamp to width in pixels. Returns drawn len
//===============
int SCR_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	int width;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	if( maxwidth < 0 )
		maxwidth = 0;

	width = SCR_strWidth( str, font, 0 );
	if( width )
	{
		if( maxwidth && width > maxwidth )
			width = maxwidth;

		x = SCR_HorizontalAlignForString( x, align, width );
		y = SCR_VerticalAlignForString( y, align, font->fontheight );

		return SCR_DrawRawString( x, y, str, maxwidth, font, color );
	}

	return 0;
}

//=============
//SCR_DrawFillRect
//
//Fills a box of pixels with a single color
//=============
void SCR_DrawFillRect( int x, int y, int w, int h, vec4_t color )
{
	R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, color, cls.whiteShader );
}

/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

//==============
//CL_AddNetgraph
//
//A new packet was just parsed
//==============
void CL_AddNetgraph( void )
{
	int i;
	int ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if( scr_timegraph->integer )
		return;

	for( i = 0; i < cls.netchan.dropped; i++ )
		SCR_DebugGraph( 30.0f, 0.655f, 0.231f, 0.169f );

	for( i = 0; i < cl.suppressCount; i++ )
		SCR_DebugGraph( 30.0f, 0.0f, 1.0f, 0.0f );

	// see what the latency was on this packet
	ping = cls.realtime - cl.cmd_time[cls.ucmdAcknowledged & CMD_MASK];
	ping /= 30;
	if( ping > 30 )
		ping = 30;
	SCR_DebugGraph( ping, 1.0f, 0.75f, 0.06f );
}


typedef struct
{
	float value;
	vec4_t color;
} graphsamp_t;

static int current;
static graphsamp_t values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph( float value, float r, float g, float b )
{
	values[current].value = value;
	values[current].color[0] = r;
	values[current].color[1] = g;
	values[current].color[2] = b;
	values[current].color[3] = 1.0f;

	current++;
	current &= 1023;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
static void SCR_DrawDebugGraph( void )
{
	int a, x, y, w, i, h;
	float v;

	//
	// draw the graph
	//
	w = viddef.width;
	x = 0;
	y = 0+viddef.height;
	SCR_DrawFillRect( x, y-scr_graphheight->integer,
		w, scr_graphheight->integer, colorBlack );

	for( a = 0; a < w; a++ )
	{
		i = ( current-1-a+1024 ) & 1023;
		v = values[i].value;
		v = v*scr_graphscale->integer + scr_graphshift->integer;

		if( v < 0 )
			v += scr_graphheight->integer * ( 1+(int)( -v/scr_graphheight->integer ) );
		h = (int)v % scr_graphheight->integer;
		SCR_DrawFillRect( x+w-1-a, y - h, 1, h, values[i].color );
	}
}

//============================================================================

/*
==================
SCR_InitScreen
==================
*/
void SCR_InitScreen( void )
{
	scr_consize = Cvar_Get( "scr_consize", "0.5", CVAR_ARCHIVE );
	scr_conspeed = Cvar_Get( "scr_conspeed", "3", CVAR_ARCHIVE );
	scr_netgraph = Cvar_Get( "netgraph", "0", 0 );
	scr_timegraph = Cvar_Get( "timegraph", "0", 0 );
	scr_debuggraph = Cvar_Get( "debuggraph", "0", 0 );
	scr_graphheight = Cvar_Get( "graphheight", "32", 0 );
	scr_graphscale = Cvar_Get( "graphscale", "1", 0 );
	scr_graphshift = Cvar_Get( "graphshift", "0", 0 );
	scr_forceclear = Cvar_Get( "scr_forceclear", "0", CVAR_READONLY );

	SCR_InitCinematic ();

	scr_initialized = qtrue;
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void SCR_RunConsole( int msec )
{
	// decide on the height of the console
	if( cls.key_dest == key_console )
		scr_conlines = bound( 0.1f, scr_consize->value, 1.0f );
	else
		scr_conlines = 0;

	if( scr_conlines < scr_con_current )
	{
		scr_con_current -= scr_conspeed->value * msec * 0.001f;
		if( scr_conlines > scr_con_current )
			scr_con_current = scr_conlines;

	}
	else if( scr_conlines > scr_con_current )
	{
		scr_con_current += scr_conspeed->value * msec * 0.001f;
		if( scr_conlines < scr_con_current )
			scr_con_current = scr_conlines;
	}
}

/*
==================
SCR_DrawConsole
==================
*/
static void SCR_DrawConsole( void )
{
	Con_CheckResize();

	if( scr_con_current )
	{
		Con_DrawConsole( scr_con_current );
		return;
	}

	if( cls.state == CA_ACTIVE && ( cls.key_dest == key_game || cls.key_dest == key_message ) )
	{
		Con_DrawNotify(); // only draw notify in game
	}
}

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque( void )
{
	CL_SoundModule_StopAllSounds();

	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );

	scr_conlines = 0;       // none visible
	SCR_UpdateScreen();

	CL_ShutdownMedia();
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque( void )
{
	cls.disable_screen = 0;
	Con_ClearNotify();
	CL_InitMedia();
}


//=======================================================

//=================
//SCR_RegisterConsoleMedia
//=================
void SCR_RegisterConsoleMedia( void )
{
	cls.whiteShader = R_RegisterPic( "2d/white" );
	cls.consoleShader = R_RegisterPic( "2d/console" );
	SCR_InitFonts();
}

//=================
//SCR_ShutDownConsoleMedia
//=================
void SCR_ShutDownConsoleMedia( void )
{
	SCR_ShutdownFonts();
}

//============================================================================

/*
==================
SCR_RenderView

==================
*/
static void SCR_RenderView( float stereo_separation )
{
	if( cls.demo.playing )
	{
		if( cl_timedemo->integer )
		{
			if( !cl.timedemo.start )
				cl.timedemo.start = Sys_Milliseconds();
			cl.timedemo.frames++;
		}
	}

	// frame is not valid until we load the CM data
	//	if( CM_ClientLoad() )
	CL_GameModule_RenderView( stereo_separation );
}

//============================================================================

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void )
{
	int numframes;
	int i;
	float separation[2];

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if( cls.disable_screen )
	{
		if( Sys_Milliseconds() - cls.disable_screen > 120000 )
		{
			cls.disable_screen = 0;
			Com_Printf( "Loading plaque timed out.\n" );
		}
		return;
	}

	if( !scr_initialized || !con.initialized || !cls.mediaInitialized )
		return;     // not initialized yet

	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	if( cl_stereo_separation->value > 1.0 )
		Cvar_SetValue( "cl_stereo_separation", 1.0 );
	else if( cl_stereo_separation->value < 0 )
		Cvar_SetValue( "cl_stereo_separation", 0.0 );

	if( cl_stereo->integer )
	{
		numframes = 2;
		separation[0] = -cl_stereo_separation->value / 2;
		separation[1] =  cl_stereo_separation->value / 2;
	}
	else
	{
		separation[0] = 0;
		separation[1] = 0;
		numframes = 1;
	}

	for( i = 0; i < numframes; i++ )
	{
		R_BeginFrame( separation[i], scr_forceclear->integer ? qtrue : qfalse );

		// if a cinematic is supposed to be running, handle menus
		// and console specially
		if( SCR_GetCinematicTime() > 0 )
		{
			SCR_DrawCinematic();
		}
		else if( cls.state == CA_DISCONNECTED )
		{
			CL_UIModule_Refresh( qtrue );
			SCR_DrawConsole();
		}
		// connection process & loading plaque
		else if( cls.state == CA_CONNECTING || cls.state == CA_HANDSHAKE
			|| cls.state == CA_CONNECTED || cls.state == CA_LOADING )
		{
			// draw the opened console
			R_DrawStretchPic( 0, 0, viddef.width, viddef.height, 0, 0, 1, 1, colorBlack, cls.whiteShader );
			Con_DrawConsole( 0.8f );
			CL_UIModule_DrawConnectScreen();
			SCR_RenderView( separation[i] ); // cgame adds its drawing if loaded
		}
		else if( cls.state == CA_ACTIVE )
		{
			SCR_RenderView( separation[i] );

			CL_UIModule_Refresh( qfalse );

			if( scr_timegraph->integer )
				SCR_DebugGraph( cls.frametime*300, 1, 1, 1 );

			if( scr_debuggraph->integer || scr_timegraph->integer || scr_netgraph->integer )
				SCR_DrawDebugGraph();

			SCR_DrawConsole();
		}

		R_EndFrame();
	}
}
