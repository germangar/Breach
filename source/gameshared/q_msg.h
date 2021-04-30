/*
 */

#include "q_arch.h"
#include "q_math.h"

// entity_state_t header bits
// these are used by the engine and can't be changed from the game code

#define	U_HEADER_REMOVE	    ( 1<<6 )      // REMOVE this entity, don't add it
#define	U_HEADER_MOREBITS1  ( 1<<7 )      // read one additional byte
// second byte
#define	U_HEADER_NUMBER16   ( 1<<8 )      // NUMBER8 is implicit if not set
#define	U_HEADER_MOREBITS2  ( 1<<15 )     // read one additional byte
#define	U_HEADER_MOREBITS3  ( 1<<23 )     // read one additional byte


typedef struct
{
	qbyte *data;
	size_t maxsize;
	size_t cursize;
	size_t readcount;
	qboolean compressed;
} msg_t;

void MSG_Init( msg_t *buf, qbyte *data, size_t length );
void MSG_Clear( msg_t *buf );
void *MSG_GetSpace( msg_t *buf, size_t length );
void MSG_WriteData( msg_t *msg, const void *data, size_t length );
void MSG_CopyData( msg_t *buf, const void *data, size_t length );

void MSG_WriteChar( msg_t *sb, int c );
void MSG_WriteByte( msg_t *sb, int c );
void MSG_WriteShort( msg_t *sb, int c );
void MSG_WriteInt3( msg_t *sb, int c );
void MSG_WriteLong( msg_t *sb, int c );
void MSG_WriteFloat( msg_t *sb, float f );
void MSG_WriteString( msg_t *sb, const char *s );
#define MSG_WriteCoord( sb, f ) ( MSG_WriteInt3( ( sb ), Q_rint( ( f*PM_VECTOR_SNAP ) ) ) )
#define MSG_WritePos( sb, pos ) ( MSG_WriteCoord( ( sb ), ( pos )[0] ), MSG_WriteCoord( sb, ( pos )[1] ), MSG_WriteCoord( sb, ( pos )[2] ) )
#define MSG_WriteAngle( sb, f ) ( MSG_WriteByte( ( sb ), ANGLE2BYTE( ( f ) ) ) )
#define MSG_WriteAngle16( sb, f ) ( MSG_WriteShort( ( sb ), ANGLE2SHORT( ( f ) ) ) )
void MSG_WriteDir( msg_t *sb, vec3_t vector );


void MSG_BeginReading( msg_t *sb );

int MSG_ReadChar( msg_t *msg );
int MSG_ReadByte( msg_t *msg );
int MSG_ReadShort( msg_t *sb );
int MSG_ReadInt3( msg_t *sb );
int MSG_ReadLong( msg_t *sb );
float MSG_ReadFloat( msg_t *sb );
char *MSG_ReadString( msg_t *sb );
char *MSG_ReadStringLine( msg_t *sb );
#define MSG_ReadCoord( sb ) ( (float)MSG_ReadInt3( ( sb ) )*( 1.0/PM_VECTOR_SNAP ) )
#define MSG_ReadPos( sb, pos ) ( ( pos )[0] = MSG_ReadCoord( ( sb ) ), ( pos )[1] = MSG_ReadCoord( ( sb ) ), ( pos )[2] = MSG_ReadCoord( ( sb ) ) )
#define MSG_ReadAngle( sb ) ( BYTE2ANGLE( MSG_ReadByte( ( sb ) ) ) )
#define MSG_ReadAngle16( sb ) ( SHORT2ANGLE( MSG_ReadShort( ( sb ) ) ) )

void MSG_ReadDir( msg_t *sb, vec3_t vector );
void MSG_ReadData( msg_t *sb, void *buffer, size_t length );
int MSG_SkipData( msg_t *sb, size_t length );
