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

#include "q_math.h" // needed for MAX_S_COLORS
#include "q_shared.h"

//============================================================================

const char *SOUND_EXTENSIONS[] = { ".ogg", ".wav" };
const size_t NUM_SOUND_EXTENSIONS = 2;

const char *IMAGE_EXTENSIONS[] = { ".jpg", ".tga", ".pcx", ".wal" };
const size_t NUM_IMAGE_EXTENSIONS = 3;

//============================================================================

/*
================
COM_SanitizeFilePath

Changes \ character to / characters in the string
Does NOT validate the string at all
Must be used before other functions are applied to the string (or those functions might function improperly)
================
*/
char *COM_SanitizeFilePath( char *path )
{
	char *p;

	assert( path );

	p = path;
	while( *p && ( p = strchr( p, '\\' ) ) )
	{
		*p = '/';
		p++;
	}

	return path;
}

/*
================
COM_ValidateFilename
================
*/
qboolean COM_ValidateFilename( const char *filename )
{
	assert( filename );

	if( !filename || !filename[0] )
		return qfalse;

	// we don't allow \ in filenames, all user inputted \ characters are converted to /
	if( strchr( filename, '\\' ) )
		return qfalse;

	return qtrue;
}

/*
================
COM_ValidateRelativeFilename
================
*/
qboolean COM_ValidateRelativeFilename( const char *filename )
{
	if( !COM_ValidateFilename( filename ) )
		return qfalse;

	if( strstr( filename, ".." ) || strstr( filename, "//" ) )
		return qfalse;

	if( *filename == '/' || *filename == '.' )
		return qfalse;

	return qtrue;
}

//============
//COM_StripExtension
//============
void COM_StripExtension( char *filename )
{
	char *src, *last = NULL;

	last = strrchr( filename, '/' );
	src = strrchr( last ? last : filename, '.' );
	if( src && *( src+1 ) )
		*src = 0;
}

//============
//COM_FileExtension
//============
const char *COM_FileExtension( const char *filename )
{
	const char *src, *last;

	if( !*filename )
		return filename;

	last = strrchr( filename, '/' );
	src = strrchr( last ? last : filename, '.' );
	if( src && *( src+1 ) )
		return src;

	return NULL;
}

//==================
//COM_DefaultExtension
//If path doesn't have extension, appends one to it
//If there is no room for it overwrites the end of the path
//==================
void COM_DefaultExtension( char *path, const char *extension, size_t size )
{
	const char *src, *last;
	size_t extlen;

	assert( extension && extension[0] && strlen( extension ) < size );

	extlen = strlen( extension );

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	last = strrchr( path, '/' );
	src = strrchr( last ? last : path, '.' );
	if( src && *( src+1 ) )
		return;     // it has an extension

	if( strlen( path )+extlen >= size )
		path[size-extlen-1] = 0;
	Q_strncatz( path, extension, size );
}

//==================
//COM_ReplaceExtension
//Replaces current extension, if there is none appends one
//If there is no room for it overwrites the end of the path
//==================
void COM_ReplaceExtension( char *path, const char *extension, size_t size )
{
	COM_StripExtension( path );
	COM_DefaultExtension( path, extension, size );
}

//============
//COM_FileBase
//============
const char *COM_FileBase( const char *in )
{
	const char *s;

	s = strrchr( in, '/' );
	if( s )
		return s+1;

	return in;
}

//============
//COM_StripFilename
//
//Cuts the string of, at the last / or erases the whole string if not found
//============
void COM_StripFilename( char *filename )
{
	char *p;

	p = strrchr( filename, '/' );
	if( !p )
		p = filename;

	*p = 0;
}

//============
//COM_FilePathLength
//
//Returns the length from start of string to the character before last /
//============
int COM_FilePathLength( const char *in )
{
	const char *s;

	s = strrchr( in, '/' );
	if( !s )
		s = in;

	return s - in;
}

//============================================================================
//
//					BYTE ORDER FUNCTIONS
//
//============================================================================

#if !defined ( ENDIAN_LITTLE ) && !defined ( ENDIAN_BIG )
short ( *BigShort )(short l);
short ( *LittleShort )(short l);
int ( *BigLong )(int l);
int ( *LittleLong )(int l);
float ( *BigFloat )(float l);
float ( *LittleFloat )(float l);
#endif

short   ShortSwap( short l )
{
	qbyte b1, b2;

	b1 = l&255;
	b2 = ( l>>8 )&255;

	return ( b1<<8 ) + b2;
}

#if !defined ( ENDIAN_LITTLE ) && !defined ( ENDIAN_BIG )
static short   ShortNoSwap( short l )
{
	return l;
}
#endif

int    LongSwap( int l )
{
	qbyte b1, b2, b3, b4;

	b1 = l&255;
	b2 = ( l>>8 )&255;
	b3 = ( l>>16 )&255;
	b4 = ( l>>24 )&255;

	return ( (int)b1<<24 ) + ( (int)b2<<16 ) + ( (int)b3<<8 ) + b4;
}

#if !defined ( ENDIAN_LITTLE ) && !defined ( ENDIAN_BIG )
static int     LongNoSwap( int l )
{
	return l;
}
#endif

float FloatSwap( float f )
{
	union
	{
		float f;
		qbyte b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

#if !defined ( ENDIAN_LITTLE ) && !defined ( ENDIAN_BIG )
static float FloatNoSwap( float f )
{
	return f;
}
#endif


//================
//Swap_Init
//================
void Swap_Init( void )
{
#if !defined ( ENDIAN_LITTLE ) && !defined ( ENDIAN_BIG )
	qbyte swaptest[2] = { 1, 0 };

	// set the byte swapping variables in a portable manner
	if( *(short *)swaptest == 1 )
	{
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
#endif
}


//=============
//TempVector
//
//This is just a convenience function
//for making temporary vectors for function calls
//=============
float *tv( float x, float y, float z )
{
	static int index;
	static float vecs[8][3];
	float *v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = ( index + 1 )&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

//=============
//VectorToString
//
//This is just a convenience function for printing vectors
//=============
char *vtos( float v[3] )
{
	static int index;
	static char str[8][32];
	char *s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = ( index + 1 )&7;

	Q_snprintfz( s, 32, "(%+6.3f %+6.3f %+6.3f)", v[0], v[1], v[2] );

	return s;
}

//============
//va
//
//does a varargs printf into a temp buffer, so I don't need to have
//varargs versions of all text functions.
//============
char *va( const char *format, ... )
{
	va_list	argptr;
	static int str_index;
	static char string[2][2048];

	str_index = !str_index;
	va_start( argptr, format );
	Q_vsnprintfz( string[str_index], sizeof( string[str_index] ), format, argptr );
	va_end( argptr );

	return string[str_index];
}

//==============
//COM_Compress
//
//Parse a token out of a string
//==============
int COM_Compress( char *data_p )
{
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	in = out = data_p;
	if( in )
	{
		while( ( c = *in ) != 0 )
		{
			// skip double slash comments
			if( c == '/' && in[1] == '/' )
			{
				while( *in && *in != '\n' )
					in++;
			}
			// skip /* */ comments
			else if( c == '/' && in[1] == '*' )
			{
				while( *in && ( *in != '*' || in[1] != '/' ) )
					in++;
				if( *in )
					in += 2;
			}
			// record when we hit a newline
			else if( c == '\n' || c == '\r' )
			{
				newline = qtrue;
				in++;
			}
			// record when we hit whitespace
			else if( c == ' ' || c == '\t' )
			{
				whitespace = qtrue;
				in++;
			}
			// an actual token
			else
			{
				// if we have a pending newline, emit it (and it counts as whitespace)
				if( newline )
				{
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				}
				if( whitespace )
				{
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if( c == '"' )
				{
					*out++ = c;
					in++;
					while( 1 )
					{
						c = *in;
						if( c && c != '"' )
						{
							*out++ = c;
							in++;
						}
						else
						{
							break;
						}
					}
					if( c == '"' )
					{
						*out++ = c;
						in++;
					}
				}
				else
				{
					*out = c;
					out++;
					in++;
				}
			}
		}
	}

	*out = 0;
	return out - data_p;
}

static char com_token[MAX_TOKEN_CHARS];

//==============
//COM_ParseExt
//
//Parse a token out of a string
//==============
char *COM_ParseExt2( const char **data_p, qboolean nl, qboolean sq )
{
	int c;
	int len;
	const char *data;
	qboolean newlines = qfalse;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if( !data )
	{
		*data_p = NULL;
		return "";
	}

	// skip whitespace
skipwhite:
	while( (unsigned char)( c = *data ) <= ' ' )
	{
		if( c == 0 )
		{
			*data_p = NULL;
			return "";
		}
		if( c == '\n' )
			newlines = qtrue;
		data++;
	}

	if( newlines && !nl )
	{
		*data_p = data;
		return com_token;
	}

	// skip // comments
	if( c == '/' && data[1] == '/' )
	{
		data += 2;

		while( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

	// skip /* */ comments
	if( c == '/' && data[1] == '*' )
	{
		data += 2;

		while( 1 )
		{
			if( !*data )
				break;
			if( *data != '*' || *( data+1 ) != '/' )
				data++;
			else
			{
				data += 2;
				break;
			}
		}
		goto skipwhite;
	}

	// handle quoted strings specially
	if( c == '\"' )
	{
		if( sq )
			data++;
		while( 1 )
		{
			c = *data++;
			if( c == '\"' || !c )
			{
				if( !c )
					data--;

				if( ( len < MAX_TOKEN_CHARS ) && ( !sq ) )
				{
					com_token[len] = '\"';
					len++;
					//					data++;
				}

				if( len == MAX_TOKEN_CHARS )
				{
					//					Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
					len = 0;
				}
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if( len < MAX_TOKEN_CHARS )
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if( len < MAX_TOKEN_CHARS )
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	}
	while( (unsigned char)c > 32 );

	if( len == MAX_TOKEN_CHARS )
	{
		//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

/*
=========================
COM_GrabCharFromColorString

Parses a char or color escape sequence and advances (*pstr)
"c" receives the character
"colorindex", if not NULL, receives color indexes (0..10)
Return values:
GRABCHAR_END - end of string reached; *c will be '\0';  *colorindex is undefined
GRABCHAR_CHAR - printable char parsed and saved to *c;  *colorindex is undefined
GRABCHAR_COLOR - color escape parsed and saved to *colorindex;  *c is undefined
=========================
*/
int COM_GrabCharFromColorString( const char **pstr, char *c, int *colorindex )
{
	switch( **pstr )
	{
	case '\0':
		*c = '\0';
		return GRABCHAR_END;

	case Q_COLOR_ESCAPE:
		if( ( *pstr )[1] >= '0' && ( *pstr )[1] < '0' + MAX_S_COLORS )
		{
			if( colorindex )
				*colorindex = ColorIndex( ( *pstr )[1] );
			( *pstr ) += 2;	// skip the ^7
			return GRABCHAR_COLOR;
		}
		else if( ( *pstr )[1] == Q_COLOR_ESCAPE )
		{
			*c = Q_COLOR_ESCAPE;
			( *pstr ) += 2;	// skip the ^^
			return GRABCHAR_CHAR;
		}
		/* fall through */

	default:
		// normal char
		*c = **pstr;
		( *pstr )++;
		return GRABCHAR_CHAR;
	}
}

/*
=======================
Q_ColorStringTerminator

Returns a color sequence to append to input string so that subsequent
characters have desired color (we can't just append ^7 because the string
may end in a ^, that would make the ^7 printable chars and color would stay)
Initial color is assumed to be white
Possible return values (assuming finalcolor is 7):
"" if no color needs to be appended,
"^7" or
"^^7" if the string ends in an unterminated ^
=======================
*/
char *Q_ColorStringTerminator( const char *str, int finalcolor )
{
	char c;
	int lastcolor = ColorIndex(COLOR_WHITE), colorindex;
	const char *s = str;

	// see what color the string ends in
	while( 1 )
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

	if( lastcolor == finalcolor )
		return "";
	else
	{
		int escapecount = 0;
		static char buf[4];
		char *p = buf;

		// count up trailing ^'s
		while( --s >= str )
			if( *s == Q_COLOR_ESCAPE )
				escapecount++;
			else
				break;

		if( escapecount & 1 )
			*p++ = Q_COLOR_ESCAPE;
		*p++ = Q_COLOR_ESCAPE;
		*p++ = '0' + finalcolor;
		*p++ = '\0';

		return buf;
	}
}

//==============
//COM_RemoveColorTokensExt
//
//Remove color tokens from a string
//==============
const char *COM_RemoveColorTokensExt( const char *str, qboolean draw )
{
	static char cleanString[MAX_STRING_CHARS];
	char *out = cleanString, *end = cleanString + sizeof( cleanString );
	const char *in = str;
	char c;
	int gc;

	while( out + 1 < end )
	{
		gc = COM_GrabCharFromColorString( &in, &c, NULL );
		if( gc == GRABCHAR_CHAR )
		{
			if( c == Q_COLOR_ESCAPE && draw )
			{
				// write two tokens so ^^1 doesn't turn into ^1 which is a color code
				if( out + 2 == end )
					break;
				*out++ = Q_COLOR_ESCAPE;
				*out++ = Q_COLOR_ESCAPE;
			}
			else
				*out++ = c;
		}
		else if( gc == GRABCHAR_COLOR )
			;
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	*out = '\0';
	return cleanString;
}

/*
=======================
COM_SanitizeColorString

Redundant color codes are removed: "^1^2text" ==> "^2text", "a^7" --> "a"
Color codes preceding whitespace are moved to before the first non-whitespace
char: "^1  a" ==> "  ^1a" (makes trimming spaces from the resulting string easier)
Removes incomplete trailing color codes: "a^" --> "a"
Removes redundant trailing color codes: "a^7" --> "a"
Non-redundant ^letter sequences are converted to ^7:  "^zx" ==> "^7x"
----------------------------
"bufsize" is size of output buffer including trailing zero, so strlen() of output
string will be bufsize-1 at max
"maxprintablechars" is how many printable (non-color-escape) characters
to write at most. Use -1 for no limit.
"startcolor" is the assumed color of the string if there are no color escapes
E.g. if startcolor is 7, leading ^7 sequences will be dropped;
if "startcolor" is -1, initial color is undefined, and any non-redundant
color escapes will be emitted, e.g. "^7player" will be written as is.
-----------------------------
Returns number of printable chars written
=======================
*/
int COM_SanitizeColorString( const char *str, char *buf, int bufsize, int maxprintablechars, int startcolor )
{
	char *out = buf, *end = buf + bufsize;
	const char *in = str;
	int oldcolor = startcolor, newcolor = startcolor;
	char c;
	int gc, colorindex;
	int c_printable = 0;

	if( maxprintablechars == -1 )
		maxprintablechars = INT_MAX;

	while( out + 1 < end && c_printable < maxprintablechars )
	{
		gc = COM_GrabCharFromColorString( &in, &c, &colorindex );

		if( gc == GRABCHAR_CHAR )
		{
			qboolean emitcolor = (qboolean)( newcolor != oldcolor && c != ' ' );
			int numbytes = ( c == Q_COLOR_ESCAPE ) ? 2 : 1; // ^ will be duplicated
			if( emitcolor )
				numbytes += 2;

			if( !( out + numbytes < end ) )
				break;	// no space to fit everything, so drop all

			// emit the color escape if necessary
			if( emitcolor )
			{
				*out++ = Q_COLOR_ESCAPE;
				*out++ = newcolor + '0';
				oldcolor = newcolor;
			}

			// emit the printable char
			*out++ = c;
			if( c == Q_COLOR_ESCAPE )
				*out++ = Q_COLOR_ESCAPE;
			c_printable++;

		}
		else if( gc == GRABCHAR_COLOR )
			newcolor = colorindex;
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}
	*out = '\0';

	return c_printable;
}

//==============
//COM_RemoveJunkChars
//
//Remove junk chars from a string (created for autoaction filenames)
//==============
const char *COM_RemoveJunkChars( const char *in )
{
	static char cleanString[MAX_STRING_CHARS];
	char *out = cleanString, *end = cleanString + sizeof( cleanString ) - 1;

	if( in )
	{
		while( *in && (out < end) )
		{
			if( isalpha( *in ) || isdigit( *in ) )
			{
				// keep it
				*out = *in;
				in++;
				out++;
			}
			else if( *in == '<' || *in == '[' || *in == '{' )
			{
				*out = '(';
				in++;
				out++;
			}
			else if( *in == '>' || *in == ']' || *in == '}' )
			{
				*out = ')';
				in++;
				out++;
			}
			else if( *in == '.' || *in == '/' || *in == '_' )
			{
				*out = '_';
				in++;
				out++;
			}
			else
			{
				// another char
				// skip it
				in++;
			}
		}
	}

	*out = '\0';
	return cleanString;
}

//==============
//COM_ReadColorRGBString
//==============
int COM_ReadColorRGBString( const char *in )
{
	static int playerColor[3];
	if( in && strlen( in ) )
	{
		if( sscanf( in, "%i %i %i", &playerColor[0], &playerColor[1], &playerColor[2] ) == 3 )
		{
			return COLOR_RGB( playerColor[0], playerColor[1], playerColor[2] );
		}
	}
	return -1;
}


//============================================================================
//
//					LIBRARY REPLACEMENT FUNCTIONS
//
//============================================================================

//==============
//Q_strncpyz
//==============
void Q_strncpyz( char *dest, const char *src, size_t size )
{
#ifdef HAVE_STRLCPY
	strlcpy( dest, src, size );
#else
	if( size )
	{
		while( --size && ( *dest++ = *src++ ) ) ;
		*dest = '\0';
	}
#endif
}

//==============
//Q_strncatz
//==============
void Q_strncatz( char *dest, const char *src, size_t size )
{
#ifdef HAVE_STRLCAT
	strlcat( dest, src, size );
#else
	if( size )
	{
		while( --size && *dest++ ) ;
		if( size )
		{
			dest--; size++;
			while( --size && ( *dest++ = *src++ ) ) ;
		}
		*dest = '\0';
	}
#endif
}

//==============
//Q_vsnprintfz
//==============
int Q_vsnprintfz( char *dest, size_t size, const char *format, va_list argptr )
{
	int len;

	assert( dest );
	assert( size );

	len = vsnprintf( dest, size, format, argptr );
	dest[size-1] = 0;

	return len;
}

//==============
//Q_snprintfz
//==============
void Q_snprintfz( char *dest, size_t size, const char *format, ... )
{
	va_list	argptr;

	va_start( argptr, format );
	Q_vsnprintfz( dest, size, format, argptr );
	va_end( argptr );
}

//==============
//Q_strupr
//==============
char *Q_strupr( char *s )
{
	char *p;

	if( s )
	{
		for( p = s; *s; s++ )
			*s = toupper( *s );
		return p;
	}

	return NULL;
}

//==============
//Q_strlwr
//==============
char *Q_strlwr( char *s )
{
	char *p;

	if( s )
	{
		for( p = s; *s; s++ )
			*s = tolower( *s );
		return p;
	}

	return NULL;
}

//==============
//Q_strlocate
//==============
const char *Q_strlocate( const char *s, const char *substr, int skip )
{
	int i;
	const char *p = NULL;
	size_t substr_len;

	if( !s || !*s )
		return NULL;

	if( !substr || !*substr )
		return NULL;

	substr_len = strlen( substr );

	for( i = 0; i <= skip; i++, s = p + substr_len )
	{
		if( !(p = strstr( s, substr )) )
			return NULL;
	}
	return p;
}

//==============
//Q_strcount
//==============
size_t Q_strcount( const char *s, const char *substr )
{
	size_t cnt;
	size_t substr_len;

	if( !s || !*s )
		return 0;

	if( !substr || !*substr )
		return 0;

	substr_len = strlen( substr );

	cnt = 0;
	while( (s = strstr( s, substr )) != NULL )
	{
		cnt++;
		s += substr_len;
	}

	return cnt;
}

//==============
//Q_strrstr
//==============
const char *Q_strrstr( const char *s, const char *substr )
{
	const char *p;

	s = p = strstr( s, substr );
	while( s != NULL )
	{
		p = s;
		s = strstr( s + 1, substr );
	}

	return p;
}

/*
==============
Q_isdigit
==============
*/
qboolean Q_isdigit( const char *str )
{
	if( str && *str )
	{
		while( isdigit( *str ) ) str++;
		if( !*str )
			return qtrue;
	}
	return qfalse;
}

//==============
//Q_trim
//==============
#define IS_TRIMMED_CHAR(s) ((s) == ' ' || (s) == '\t' || (s) == '\r' || (s) == '\n')
char *Q_trim( char *s )
{
	char *t = s;
	size_t len;

	// remove leading whitespace
	while( IS_TRIMMED_CHAR( *t ) ) t++;
	len = strlen( s ) - (t - s);
	if( s != t )
		memmove( s, t, len + 1 );

	// remove trailing whitespace
	while( len && IS_TRIMMED_CHAR( s[len-1] ) )
		s[--len] = '\0';

	return s;
}
#undef IS_TRIMMED_CHAR

/*
==============
Q_chrreplace
==============
*/
char *Q_chrreplace( char *s, const char subj, const char repl )
{
	char *t = s;
	while( ( t = strchr( t, subj ) ) != NULL )
		*t++ = repl;
	return s;
}

//============================================================================
//
//					WILDCARD COMPARES FUNCTIONS
//
//============================================================================

//==============
//Q_WildCmpAfterStar
//==============
static qboolean Q_WildCmpAfterStar( const char *pattern, const char *text )
{
	char c, c1;
	const char *p = pattern, *t = text;

	while( ( c = *p++ ) == '?' || c == '*' )
	{
		if( c == '?' && *t++ == '\0' )
			return qfalse;
	}

	if( c == '\0' )
		return qtrue;

	for( c1 = ( ( c == '\\' ) ? *p : c );; )
	{
		if( tolower( *t ) == c1 && Q_WildCmp( p - 1, t ) )
			return qtrue;
		if( *t++ == '\0' )
			return qfalse;
	}
}

//==============
//Q_WildCmp
//==============
qboolean Q_WildCmp( const char *pattern, const char *text )
{
	char c;

	while( ( c = *pattern++ ) != '\0' )
	{
		switch( c )
		{
		case '?':
			if( *text++ == '\0' )
				return qfalse;
			break;
		case '\\':
			if( tolower( *pattern++ ) != tolower( *text++ ) )
				return qfalse;
			break;
		case '*':
			return Q_WildCmpAfterStar( pattern, text );
		default:
			if( tolower( c ) != tolower( *text++ ) )
				return qfalse;
		}
	}

	return (qboolean)( *text == '\0' );
}

/*
==================
COM_ValidateConfigstring
==================
*/
qboolean COM_ValidateConfigstring( const char *string )
{
	const char *p;
	qboolean opened = qfalse;
	int parity = 0;

	if( !string )
		return qfalse;

	p = string;
	while( *p )
	{
		if( *p == '\"' )
		{
			if( opened )
			{
				parity--;
				opened = qfalse;
			}
			else
			{
				parity++;
				opened = qtrue;
			}
		}
		p++;
	}

	if( parity != 0 )
		return qfalse;

	return qtrue;
}

//=====================================================================
//
//  INFO STRINGS
//
//=====================================================================

//==================
//Info_ValidateValue
//==================
static qboolean Info_ValidateValue( const char *value )
{
	assert( value );

	if( !value )
		return qfalse;

	if( strlen( value ) >= MAX_INFO_VALUE )
		return qfalse;

	if( strchr( value, '\\' ) )
		return qfalse;

	if( strchr( value, ';' ) )
		return qfalse;

	if( strchr( value, '"' ) )
		return qfalse;

	return qtrue;
}

//==================
//Info_ValidateKey
//==================
static qboolean Info_ValidateKey( const char *key )
{
	assert( key );

	if( !key )
		return qfalse;

	if( !key[0] )
		return qfalse;

	if( strlen( key ) >= MAX_INFO_KEY )
		return qfalse;

	if( strchr( key, '\\' ) )
		return qfalse;

	if( strchr( key, ';' ) )
		return qfalse;

	if( strchr( key, '"' ) )
		return qfalse;

	return qtrue;
}

//==================
//Info_Validate
//
//Some characters are illegal in info strings because they
//can mess up the server's parsing
//==================
qboolean Info_Validate( const char *info )
{
	const char *p, *start;

	assert( info );

	if( !info )
		return qfalse;

	if( strlen( info ) >= MAX_INFO_STRING )
		return qfalse;

	if( strchr( info, '\"' ) )
		return qfalse;

	if( strchr( info, ';' ) )
		return qfalse;

	if( strchr( info, '"' ) )
		return qfalse;

	p = info;

	while( p && *p )
	{
		if( *p++ != '\\' )
			return qfalse;

		start = p;
		p = strchr( start, '\\' );
		if( !p )  // missing key
			return qfalse;
		if( p - start >= MAX_INFO_KEY )  // too long
			return qfalse;

		p++; // skip the \ char

		start = p;
		p = strchr( start, '\\' );
		if( ( p && p - start >= MAX_INFO_KEY ) || ( !p && strlen( start ) >= MAX_INFO_KEY ) )  // too long
			return qfalse;
	}

	return qtrue;
}

//==================
//Info_FindKey
//
//Returns the pointer to the \ character if key is found
//Otherwise returns NULL
//==================
static char *Info_FindKey( const char *info, const char *key )
{
	const char *p, *start;

	assert( Info_Validate( info ) );
	assert( Info_ValidateKey( key ) );

	if( !Info_Validate( info ) || !Info_ValidateKey( key ) )
		return NULL;

	p = info;

	while( p && *p )
	{
		start = p;

		p++; // skip the \ char
		if( !strncmp( key, p, strlen( key ) ) && p[strlen( key )] == '\\' )
			return (char *)start;

		p = strchr( p, '\\' );
		if( !p )
			return NULL;

		p++; // skip the \ char
		p = strchr( p, '\\' );
	}

	return NULL;
}

//===============
//Info_ValueForKey
//
//Searches the string for the given
//key and returns the associated value, or NULL
//===============
char *Info_ValueForKey( const char *info, const char *key )
{
	static char value[2][MAX_INFO_VALUE]; // use two buffers so compares work without stomping on each other
	static int valueindex;
	const char *p, *start;
	size_t len;

	assert( info && Info_Validate( info ) );
	assert( key && Info_ValidateKey( key ) );

	if( !Info_Validate( info ) || !Info_ValidateKey( key ) )
		return NULL;

	valueindex ^= 1;

	p = Info_FindKey( info, key );
	if( !p )
		return NULL;

	p++; // skip the \ char
	p = strchr( p, '\\' );
	if( !p )
		return NULL;

	p++; // skip the \ char
	start = p;
	p = strchr( p, '\\' );
	if( !p )
	{
		len = strlen( start );
	}
	else
	{
		len = p - start;
	}

	if( len >= MAX_INFO_VALUE )
	{
		assert( qfalse );
		return NULL;
	}
	strncpy( value[valueindex], start, len );
	value[valueindex][len] = 0;

	return value[valueindex];
}

//==================
//Info_RemoveKey
//==================
void Info_RemoveKey( char *info, const char *key )
{
	char *start, *p;

	assert( info && Info_Validate( info ) );
	assert( key && Info_ValidateKey( key ) );

	if( !Info_Validate( info ) || !Info_ValidateKey( key ) )
		return;

	p = Info_FindKey( info, key );
	if( !p )
		return;

	start = p;

	p++; // skip the \ char
	p = strchr( p, '\\' );
	if( p )
	{
		p++; // skip the \ char
		p = strchr( p, '\\' );
	}

	if( !p )
	{
		*start = 0;
	}
	else
	{
		// aiwa : fixed possible source and destination overlap with strcpy()
		memmove( start, p, strlen( p ) + 1 );
	}
}

//==================
//Info_SetValueForKey
//==================
qboolean Info_SetValueForKey( char *info, const char *key, const char *value )
{
	char pair[MAX_INFO_KEY + MAX_INFO_VALUE + 1];

	assert( info && Info_Validate( info ) );
	assert( key && Info_ValidateKey( key ) );
	assert( value && Info_ValidateValue( value ) );

	if( !Info_Validate( info ) || !Info_ValidateKey( key ) || !Info_ValidateValue( value ) )
		return qfalse;

	Info_RemoveKey( info, key );

	Q_snprintfz( pair, sizeof( pair ), "\\%s\\%s", key, value );

	if( strlen( pair ) + strlen( info ) > MAX_INFO_STRING )
		return qfalse;

	Q_strncatz( info, pair, MAX_INFO_STRING );

	return qtrue;
}