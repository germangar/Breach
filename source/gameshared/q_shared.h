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

#ifndef __Q_SHARED_H__
#define __Q_SHARED_H__

#include "q_arch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	MAX_QPATH	    64      // max length of a quake game pathname
#define	MAX_STRING_CHARS    1024    // max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS   256     // max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS	    1024    // max length of an individual token
#define MAX_CONFIGSTRING_CHARS 128    // max length of a configstring string
#define MAX_NAME_CHARS	    15      // max visible characters name (color tokens not counted)

//=============================================

char *COM_SanitizeFilePath( char *filename );
qboolean COM_ValidateFilename( const char *filename );
qboolean COM_ValidateRelativeFilename( const char *filename );
void COM_StripExtension( char *filename );
const char *COM_FileExtension( const char *in );
void COM_DefaultExtension( char *path, const char *extension, size_t size );
void COM_ReplaceExtension( char *path, const char *extension, size_t size );
const char *COM_FileBase( const char *in );
void COM_StripFilename( char *filename );
int COM_FilePathLength( const char *in );

// data is an in/out parm, returns a parsed out token
int COM_Compress( char *data_p );
char *COM_ParseExt2( const char **data_p, qboolean nl, qboolean sq );
#define COM_ParseExt( data_p, nl ) COM_ParseExt2( (const char **)data_p, nl, qtrue )
#define COM_Parse( data_p )   COM_ParseExt( data_p, qtrue )
const char *COM_RemoveJunkChars( const char *in );
int COM_ReadColorRGBString( const char *in );
qboolean COM_ValidateConfigstring( const char *string );

// color string functions ("^1text" etc)
#define GRABCHAR_END	0
#define GRABCHAR_CHAR	1
#define GRABCHAR_COLOR	2
int COM_GrabCharFromColorString( const char **pstr, char *c, int *colorindex );
char *Q_ColorStringTerminator( const char *str, int finalcolor );
const char *COM_RemoveColorTokensExt( const char *str, qboolean draw );
#define COM_RemoveColorTokens( in ) COM_RemoveColorTokensExt( in,qfalse )
int COM_SanitizeColorString( const char *str, char *buf, int bufsize, int maxprintablechars, int startcolor );

//=============================================

extern const char *SOUND_EXTENSIONS[];
extern const size_t NUM_SOUND_EXTENSIONS;

extern const char *IMAGE_EXTENSIONS[];
extern const size_t NUM_IMAGE_EXTENSIONS;

// AL_INVERSE_DISTANCE_CLAMPED
#define S_DEFAULT_ATTENUATION_MODEL	    "3"

#define S_DEFAULT_ATTENUATION_MAXDISTANCE   "8000"
#define S_DEFAULT_ATTENUATION_REFDISTANCE   "180"

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) always override a playing sound on that channel
#define	CHAN_AUTO			0
#define	CHAN_SELF			1
#define	CHAN_VOICE			2
#define	CHAN_WEAPON			3
#define	CHAN_FOOTSTEPS		4
#define	CHAN_FIVE			5
#define	CHAN_SIX			6
#define	CHAN_SEVEN			7

//=============================================

#define Q_COLOR_ESCAPE	'^'
#define S_COLOR_ESCAPE	"^"

#define COLOR_BLACK	'0'
#define COLOR_RED	'1'
#define COLOR_GREEN	'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE	'4'
#define COLOR_CYAN	'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE	'7'
#define COLOR_ORANGE	'8'
#define COLOR_GREY	'9'
#define ColorIndex( c )	  ( ( ( ( c )-'0' ) < MAX_S_COLORS ) && ( ( ( c )-'0' ) >= 0 ) ? ( ( c ) - '0' ) : 7 )

#define	COLOR_R( rgba )	      ( ( rgba ) & 0xFF )
#define	COLOR_G( rgba )	      ( ( ( rgba ) >> 8 ) & 0xFF )
#define	COLOR_B( rgba )	      ( ( ( rgba ) >> 16 ) & 0xFF )
#define	COLOR_A( rgba )	      ( ( ( rgba ) >> 24 ) & 0xFF )
#define COLOR_RGB( r, g, b )	( ( ( r ) << 0 )|( ( g ) << 8 )|( ( b ) << 16 ) )
#define COLOR_RGBA( r, g, b, a ) ( ( ( r ) << 0 )|( ( g ) << 8 )|( ( b ) << 16 )|( ( a ) << 24 ) )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED	"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"
#define S_COLOR_ORANGE	"^8"
#define S_COLOR_GREY	"^9"

//=============================================

void Q_strncpyz( char *dest, const char *src, size_t size );
void Q_strncatz( char *dest, const char *src, size_t size );
int Q_vsnprintfz( char *dest, size_t size, const char *format, va_list argptr );
void Q_snprintfz( char *dest, size_t size, const char *format, ... );
char *Q_strupr( char *s );
char *Q_strlwr( char *s );
const char *Q_strlocate( const char *s, const char *substr, int skip );
size_t Q_strcount( const char *s, const char *substr );
const char *Q_strrstr( const char *s, const char *substr );
qboolean Q_WildCmp( const char *pattern, const char *text );
qboolean Q_isdigit( const char *str );
char *Q_trim( char *s );
char *Q_chrreplace( char *s, const char subj, const char repl );

float *tv( float x, float y, float z );
char *vtos( float v[3] );
char *va( const char *format, ... );

//
// key / value info strings
//
#define	MAX_INFO_KEY	    64
#define	MAX_INFO_VALUE	    64
#define	MAX_INFO_STRING	    512

char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
qboolean Info_SetValueForKey( char *s, const char *key, const char *value );
qboolean Info_Validate( const char *s );

//=============================================
#if !defined ( ENDIAN_LITTLE ) && !defined ( ENDIAN_BIG )
#if defined ( __i386__ ) || defined ( __ia64__ ) || defined ( WIN32 ) || ( defined ( __alpha__ ) || defined ( __alpha ) ) || defined ( __arm__ ) || ( defined ( __mips__ ) && defined ( __MIPSEL__ ) ) || defined ( __LITTLE_ENDIAN__ ) || defined ( __x86_64__ )
#define ENDIAN_LITTLE
#else
#define ENDIAN_BIG
#endif
#endif

short ShortSwap( short l );
int LongSwap( int l );
float FloatSwap( float f );

#ifdef ENDIAN_LITTLE
// little endian
#define BigShort( l ) ShortSwap( l )
#define LittleShort( l ) ( l )
#define BigLong( l ) LongSwap( l )
#define LittleLong( l ) ( l )
#define BigFloat( l ) FloatSwap( l )
#define LittleFloat( l ) ( l )
#elif defined ( ENDIAN_BIG )
// big endian
#define BigShort( l ) ( l )
#define LittleShort( l ) ShortSwap( l )
#define BigLong( l ) ( l )
#define LittleLong( l ) LongSwap( l )
#define BigFloat( l ) ( l )
#define LittleFloat( l ) FloatSwap( l )
#else
// figure it out at runtime
extern short ( *BigShort )(short l);
extern short ( *LittleShort )(short l);
extern int ( *BigLong )(int l);
extern int ( *LittleLong )(int l);
extern float ( *BigFloat )(float l);
extern float ( *LittleFloat )(float l);
#endif

void	Swap_Init( void );

//==============================================================
//
//SYSTEM SPECIFIC
//
//==============================================================


// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *error, ... );
void Com_Printf( const char *msg, ... );


//==============================================================
//
//FILESYSTEM
//
//==============================================================

#define FS_READ				0
#define FS_WRITE			1
#define FS_APPEND			2

#define FS_SEEK_CUR			0
#define FS_SEEK_SET			1
#define FS_SEEK_END			2

//==============================================================

#define DEFAULT_FONT_SMALL "tahoma_8"
#define DEFAULT_FONT_MEDIUM "bitstream_12"
#define DEFAULT_FONT_BIG "virtue_16"

#define ALIGN_LEFT_TOP	    0
#define ALIGN_CENTER_TOP    1
#define ALIGN_RIGHT_TOP	    2
#define ALIGN_LEFT_MIDDLE   3
#define ALIGN_CENTER_MIDDLE 4
#define ALIGN_RIGHT_MIDDLE  5
#define ALIGN_LEFT_BOTTOM   6
#define ALIGN_CENTER_BOTTOM 7
#define ALIGN_RIGHT_BOTTOM  8


// command line execution flags
#define	EXEC_NOW    0               // don't return until completed
#define	EXEC_INSERT 1               // insert at current position, but don't run yet
#define	EXEC_APPEND 2               // add to end of the command buffer

typedef enum
{
	key_game,
	key_console,
	key_message,
	key_menu,
	key_delegate
} keydest_t;

typedef enum
{
	CA_UNINITIALIZED,
	CA_DISCONNECTED, // not talking to a server
	CA_CONNECTING,  // sending request packets to the server
	CA_HANDSHAKE,   // netchan_t established, waiting for svc_serverdata
	CA_CONNECTED,   // connection established, game module not loaded
	CA_LOADING,     // loading game module
	CA_ACTIVE       // game views should be displayed
} connstate_t;

enum
{
	DROP_TYPE_GENERAL,
	DROP_TYPE_PASSWORD,
	DROP_TYPE_NORECONNECT,
	DROP_TYPE_TOTAL
};

#define DROP_FLAG_AUTORECONNECT 1 // it's okay try reconnectting automatically

#ifdef __cplusplus
};
#endif

#endif // __Q_SHARED_H__


