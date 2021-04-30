
#include "q_msg.h"
#include "q_shared.h" // needed for sys_Error

//============================================================
//	MESSAGE IO FUNCTIONS
//	Handles byte ordering and avoids alignment errors
//============================================================

#define MAX_MSG_STRING_CHARS	2048

void MSG_Init( msg_t *msg, qbyte *data, size_t length )
{
	memset( msg, 0, sizeof( *msg ) );
	msg->data = data;
	msg->maxsize = length;
	msg->cursize = 0;
	msg->compressed = qfalse;
}

void MSG_Clear( msg_t *msg )
{
	msg->cursize = 0;
	msg->compressed = qfalse;
}

void *MSG_GetSpace( msg_t *msg, size_t length )
{
	void *ptr;

	assert( msg->cursize + length <= msg->maxsize );
	if( msg->cursize + length > msg->maxsize )
		Sys_Error( "MSG_GetSpace: overflowed" );
	//Com_Error( ERR_FATAL, "MSG_GetSpace: overflowed" );

	ptr = msg->data + msg->cursize;
	msg->cursize += length;
	return ptr;
}

//==================================================
// WRITE FUNCTIONS
//==================================================

void MSG_WriteData( msg_t *msg, const void *data, size_t length )
{
#if 0
	unsigned int i;
	for( i = 0; i < length; i++ )
		MSG_WriteByte( msg, ( (qbyte *)data )[i] );
#else
	MSG_CopyData( msg, data, length );
#endif
}

void MSG_CopyData( msg_t *buf, const void *data, size_t length )
{
	memcpy( MSG_GetSpace( buf, length ), data, length );
}

void MSG_WriteChar( msg_t *msg, int c )
{
	qbyte *buf = (qbyte *)MSG_GetSpace( msg, 1 );
	buf[0] = ( char )c;
}

void MSG_WriteByte( msg_t *msg, int c )
{
	qbyte *buf = (qbyte *)MSG_GetSpace( msg, 1 );
	buf[0] = ( qbyte )( c&0xff );
}

void MSG_WriteShort( msg_t *msg, int c )
{
	unsigned short *sp = (unsigned short *)MSG_GetSpace( msg, 2 );
	*sp = LittleShort( c );
}

void MSG_WriteInt3( msg_t *msg, int c )
{
	qbyte *buf = (qbyte *)MSG_GetSpace( msg, 3 );
	buf[0] = ( qbyte )( c&0xff );
	buf[1] = ( qbyte )( ( c>>8 )&0xff );
	buf[2] = ( qbyte )( ( c>>16 )&0xff );
}

void MSG_WriteLong( msg_t *msg, int c )
{
	unsigned int *ip = (unsigned int *)MSG_GetSpace( msg, 4 );
	*ip = LittleLong( c );
}

void MSG_WriteFloat( msg_t *msg, float f )
{
	union {
		float f;
		int l;
	} dat;

	dat.f = f;
	MSG_WriteLong( msg, dat.l );
}

void MSG_WriteDir( msg_t *msg, vec3_t dir )
{
	MSG_WriteByte( msg, dir ? DirToByte( dir ) : 0 );
}

void MSG_WriteString( msg_t *msg, const char *s )
{
	if( !s )
	{
		MSG_WriteData( msg, "", 1 );
	}
	else
	{
		int l = strlen( s );
		if( l >= MAX_MSG_STRING_CHARS )
		{
			Com_Printf( "MSG_WriteString: MAX_MSG_STRING_CHARS overflow" );
			MSG_WriteData( msg, "", 1 );
			return;
		}
		MSG_WriteData( msg, s, l+1 );
	}
}

//==================================================
// READ FUNCTIONS
//==================================================

void MSG_BeginReading( msg_t *msg )
{
	msg->readcount = 0;
}

int MSG_ReadChar( msg_t *msg )
{
	int i = (signed char)msg->data[msg->readcount++];
	if( msg->readcount > msg->cursize )
		i = -1;
	return i;
}

int MSG_ReadByte( msg_t *msg )
{
	int i = (unsigned char)msg->data[msg->readcount++];
	if( msg->readcount > msg->cursize )
		i = -1;
	return i;
}

int MSG_ReadShort( msg_t *msg )
{
	int i;
	short *sp = (short *)&msg->data[msg->readcount];
	i = LittleShort( *sp );
	msg->readcount += 2;
	if( msg->readcount > msg->cursize )
		i = -1;
	return i;
}

int MSG_ReadInt3( msg_t *msg )
{
	int i = msg->data[msg->readcount]
	        | ( msg->data[msg->readcount+1]<<8 )
	        | ( msg->data[msg->readcount+2]<<16 )
	        | ( ( msg->data[msg->readcount+2] & 0x80 ) ? ~0xFFFFFF : 0 );
	msg->readcount += 3;
	if( msg->readcount > msg->cursize )
		i = -1;
	return i;
}

int MSG_ReadLong( msg_t *msg )
{
	int i;
	unsigned int *ip = (unsigned int *)&msg->data[msg->readcount];
	i = LittleLong( *ip );
	msg->readcount += 4;
	if( msg->readcount > msg->cursize )
		i = -1;
	return i;
}

float MSG_ReadFloat( msg_t *msg )
{
	union {
		float f;
		int l;
	} dat;

	dat.l = MSG_ReadLong( msg );
	if( msg->readcount > msg->cursize )
		dat.f = -1;
	return dat.f;
}

void MSG_ReadDir( msg_t *msg, vec3_t dir )
{
	ByteToDir( MSG_ReadByte( msg ), dir );
}

void MSG_ReadData( msg_t *msg, void *data, size_t length )
{
	unsigned int i;

	for( i = 0; i < length; i++ )
		( (qbyte *)data )[i] = MSG_ReadByte( msg );
}

int MSG_SkipData( msg_t *msg, size_t length )
{
	if( msg->readcount + length <= msg->cursize )
	{
		msg->readcount += length;
		return 1;
	}
	return 0;
}

static char *MSG_ReadString2( msg_t *msg, qboolean linebreak )
{
	int l, c;
	static char string[MAX_MSG_STRING_CHARS];

	l = 0;
	do
	{
		c = MSG_ReadByte( msg );
		if( c == 255 ) continue;
		if( c == -1 || c == 0 || ( linebreak && c == '\n' ) ) break;

		string[l] = c;
		l++;
	}
	while( l < sizeof( string )-1 );

	string[l] = 0;

	return string;
}

char *MSG_ReadString( msg_t *msg )
{
	return MSG_ReadString2( msg, qfalse );
}

char *MSG_ReadStringLine( msg_t *msg )
{
	return MSG_ReadString2( msg, qtrue );
}
