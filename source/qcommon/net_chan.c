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

#include "qcommon.h"

#if defined ( __APPLE__ ) || defined ( MACOSX )
#include <arpa/inet.h>
#endif /* __APPLE__ || MACOSX */

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16	qport

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher than the last reliable sequence, but without the correct even/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted. It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

If the sequence number is -1, the packet should be handled without a netcon.

The reliable message can be added to at any time by doing
MSG_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are always placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection. This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.


The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs. The IP port should be updated
to the new value before sending out any replies.


If there is no information that needs to be transfered on a given frame,
such as during the connection stage while waiting for the client to load,
then a packet only needs to be delivered if there is something in the
unacknowledged reliable
*/

cvar_t *showpackets;
cvar_t *showdrop;
cvar_t *qport;
cvar_t *net_showfragments; // & 1 = client & 2 = server

netadr_t net_from;

/*
* Netchan_Init
*/
void Netchan_Init( void )
{
	int port;

	// pick a port value that should be nice and random
	port = Sys_Milliseconds() & 0xffff;

	showpackets = Cvar_Get( "showpackets", "0", 0 );
	showdrop = Cvar_Get( "showdrop", "0", 0 );
	qport = Cvar_Get( "qport", va( "%i", port ), CVAR_NOSET );
	net_showfragments = Cvar_Get( "net_showfragments", "0", 0 );
}

/*
* Netchan_OutOfBand
*
* Sends an out-of-band datagram
*/
void Netchan_OutOfBand( int net_socket, netadr_t adr, size_t length, qbyte *data )
{
	msg_t send;
	qbyte send_buf[MAX_PACKETLEN];

	// write the packet header
	MSG_Init( &send, send_buf, sizeof( send_buf ) );

	MSG_WriteLong( &send, -1 ); // -1 sequence means out of band
	MSG_WriteData( &send, data, length );

	// send the datagram
	NET_SendPacket( net_socket, send.cursize, send.data, adr );
}

/*
* Netchan_OutOfBandPrint
*
* Sends a text message in an out-of-band datagram
*/
void Netchan_OutOfBandPrint( int net_socket, netadr_t adr, const char *format, ... )
{
	va_list	argptr;
	static char string[MAX_PACKETLEN - 4];

	va_start( argptr, format );
	Q_vsnprintfz( string, sizeof( string ), format, argptr );
	va_end( argptr );

	Netchan_OutOfBand( net_socket, adr, (int)strlen( string ), (qbyte *)string );
}

/*
* Netchan_Setup
*
* called to open a channel to a remote system
*/
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport )
{
	memset( chan, 0, sizeof( *chan ) );

	chan->sock = sock;
	chan->remoteAddress = adr;
	chan->qport = qport;
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;
}


static qbyte msg_process_data[MAX_MSGLEN];

static char socknames[3][16] = { "Client", "Server", "Unknown" };

static char *SockString( int sock )
{

	if( sock == NS_CLIENT )
		return socknames[0];
	else if( sock == NS_SERVER )
		return socknames[1];
	else
		return socknames[2];
}

//=============================================================
// Zlib compression
//=============================================================

#include "zlib.h"

/*
http://www.zlib.net/manual.html#compress2
int compress2 (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
Compresses the source buffer into the destination buffer. The level parameter has the same meaning
as in deflateInit. sourceLen is the byte length of the source buffer. Upon entry, destLen is the
total size of the destination buffer, which must be at least 0.1% larger than sourceLen plus 12 bytes.
Upon exit, destLen is the actual size of the compressed buffer.

compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough memory, Z_BUF_ERROR if there
was not enough room in the output buffer, Z_STREAM_ERROR if the level parameter is invalid.
*/
static int Netchan_ZLibCompressChunk( const qbyte *source, unsigned long sourceLen, qbyte *dest, unsigned long destLen,
									 int level, int wbits )
{
	int result, zlerror;

	zlerror = compress2( dest, &destLen, source, sourceLen, level );
	switch( zlerror )
	{
	case Z_OK:
		result = destLen; // returns the new length into destLen
		break;
	case Z_MEM_ERROR:
		Com_DPrintf( "ZLib data error! Z_MEM_ERROR on compress.\n" );
		result = -1;
		break;
	case Z_BUF_ERROR:
		Com_DPrintf( "ZLib data error! Z_BUF_ERROR on compress.\n" );
		result = -1;
		break;
	case Z_STREAM_ERROR:
		Com_DPrintf( "ZLib data error! Z_STREAM_ERROR on compress.\n" );
		result = -1;
		break;
	default:
		Com_DPrintf( "ZLib data error! Error code %i on compress.\n", zlerror );
		result = -1;
		break;
	}

	return result;
}
/*
http://www.zlib.net/manual.html#uncompress
int uncompress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
Decompresses the source buffer into the destination buffer. sourceLen is the byte length
of the source buffer. Upon entry, destLen is the total size of the destination buffer,
which must be large enough to hold the entire uncompressed data. (The size of the uncompressed
data must have been saved previously by the compressor and transmitted to the decompressor
by some mechanism outside the scope of this compression library.) Upon exit, destLen is the
actual size of the compressed buffer.

This function can be used to decompress a whole file at once if the input file is mmap'ed.

uncompress returns Z_OK if success, Z_MEM_ERROR if there was not enough memory, Z_BUF_ERROR if
there was not enough room in the output buffer, or Z_DATA_ERROR if the input data was corrupted.
*/
static int Netchan_ZLibDecompressChunk( const qbyte *source, unsigned long sourceLen, qbyte *dest, unsigned long destLen,
									   int wbits )
{
	int result, zlerror;

	zlerror = uncompress( dest, &destLen, source, sourceLen );
	switch( zlerror )
	{
	case Z_OK:
		result = destLen; // returns the new length into destLen
		break;
	case Z_MEM_ERROR:
		Com_DPrintf( "ZLib data error! Z_MEM_ERROR on decompress.\n" );
		result = -1;
		break;
	case Z_BUF_ERROR:
		Com_DPrintf( "ZLib data error! Z_BUF_ERROR on decompress.\n" );
		result = -1;
		break;
	case Z_DATA_ERROR:
		Com_DPrintf( "ZLib data error! Z_DATA_ERROR on decompress.\n" );
		result = -1;
		break;
	default:
		Com_DPrintf( "ZLib data error! Error code %i on decompress.\n", zlerror );
		result = -1;
		break;
	}

	return result;
}

/*
* Netchan_CompressMessage
*/
int Netchan_CompressMessage( msg_t *msg )
{
	int length;

	if( msg == NULL || !msg->data )
		return 0;

	// zero-fill our buffer
	length = 0;
	memset( msg_process_data, 0, sizeof( msg_process_data ) );

	//compress the message
	length = Netchan_ZLibCompressChunk( msg->data, msg->cursize, msg_process_data, sizeof( msg_process_data ), Z_DEFAULT_COMPRESSION, -MAX_WBITS );
	if( length < 0 )  // failed to compress, return the error
		return length;

	if( (size_t)length >= msg->cursize || length >= MAX_MSGLEN )
	{
		return 0; // compressed was bigger. Send uncompressed
	}

	//write it back into the original container
	MSG_Clear( msg );
	MSG_CopyData( msg, msg_process_data, length );
	msg->compressed = qtrue;

	return length; // return the new size
}

/*
* Netchan_DecompressMessage
*/
int Netchan_DecompressMessage( msg_t *msg )
{

	int length;

	if( msg == NULL || !msg->data )
		return 0;

	if( msg->compressed == qfalse )
		return 0;

	length = Netchan_ZLibDecompressChunk( msg->data + msg->readcount, msg->cursize - msg->readcount, msg_process_data, ( sizeof( msg_process_data ) - msg->readcount ), -MAX_WBITS );
	if( length < 0 )
		return length;

	if( ( msg->readcount + length ) >= msg->maxsize )
	{
		Com_Printf( "Netchan_DecompressMessage: Packet too big\n" );
		return -1;
	}

	//write it back into the original container
	msg->cursize = msg->readcount;
	MSG_CopyData( msg, msg_process_data, length );
	msg->compressed = qfalse;

	return length;
}

/*
* Netchan_TransmitNextFragment
*
* Send one fragment of the current message
*/
void Netchan_TransmitNextFragment( netchan_t *chan )
{
	msg_t send;
	qbyte send_buf[MAX_PACKETLEN];
	int fragmentLength;
	qboolean last;

	// write the packet header
	MSG_Init( &send, send_buf, sizeof( send_buf ) );
	MSG_Clear( &send );
	if( net_showfragments->integer & 1 && chan->sock == NS_CLIENT )
	{
		Com_Printf( "Transmit fragment (%s) (id:%i)\n", SockString( chan->sock ), chan->outgoingSequence );
	}
	if( net_showfragments->integer & 2 && chan->sock == NS_SERVER )
	{
		Com_Printf( "Transmit fragment (%s) (id:%i)\n", SockString( chan->sock ), chan->outgoingSequence );
	}

	MSG_WriteLong( &send, chan->outgoingSequence | FRAGMENT_BIT );
	// by now our header sends incoming ack too, but we should clean it
	// also add compressed bit if it's compressed
	if( chan->unsentIsCompressed )
		MSG_WriteLong( &send, chan->incomingSequence | FRAGMENT_BIT );
	else
		MSG_WriteLong( &send, chan->incomingSequence );

	// send the qport if we are a client
	if( chan->sock == NS_CLIENT )
	{
		MSG_WriteShort( &send, qport->integer );
	}

	// copy the reliable message to the packet first
	if( chan->unsentFragmentStart + FRAGMENT_SIZE > chan->unsentLength )
	{
		fragmentLength = chan->unsentLength - chan->unsentFragmentStart;
		last = qtrue;
	}
	else
	{
		fragmentLength = ceil( ( chan->unsentLength - chan->unsentFragmentStart )*1.0 / ceil( ( chan->unsentLength - chan->unsentFragmentStart )*1.0 / FRAGMENT_SIZE ) );
		last = qfalse;
	}

	MSG_WriteShort( &send, chan->unsentFragmentStart );
	MSG_WriteShort( &send, ( last ? ( fragmentLength | FRAGMENT_LAST ) : fragmentLength ) );
	MSG_CopyData( &send, chan->unsentBuffer + chan->unsentFragmentStart, fragmentLength );

	// send the datagram
	NET_SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );

	if( showpackets->integer )
	{
		Com_Printf( "%s send %4i : s=%i fragment=%i,%i\n"
			, SockString( chan->sock )
			, send.cursize
			, chan->outgoingSequence
			, chan->unsentFragmentStart, fragmentLength );
	}

	chan->unsentFragmentStart += fragmentLength;

	// this exit condition is a little tricky, because a packet
	// that is exactly the fragment length still needs to send
	// a second packet of zero length so that the other side
	// can tell there aren't more to follow
	if( chan->unsentFragmentStart == chan->unsentLength && fragmentLength != FRAGMENT_SIZE )
	{
		chan->outgoingSequence++;
		chan->unsentFragments = qfalse;
	}
}

/*
* Netchan_PushAllFragments
*
* Send all remaining fragments at once
*/
void Netchan_PushAllFragments( netchan_t *chan )
{
	while( chan->unsentFragments )
		Netchan_TransmitNextFragment( chan );
}

/*
* Netchan_Transmit
*
* Sends a message to a connection, fragmenting if necessary
* A 0 length will still generate a packet.
*/
void Netchan_Transmit( netchan_t *chan, msg_t *msg )
{
	msg_t send;
	qbyte send_buf[MAX_PACKETLEN];

	if( !msg || !msg->data )
		return;

	if( msg->cursize > MAX_MSGLEN )
	{
		Com_Error( ERR_DROP, "Netchan_Transmit: Excessive length = %i", msg->cursize );
	}
	chan->unsentFragmentStart = 0;
	chan->unsentIsCompressed = qfalse;

	// fragment large reliable messages
	if( msg->cursize >= FRAGMENT_SIZE )
	{
		chan->unsentFragments = qtrue;
		chan->unsentLength = msg->cursize;
		chan->unsentIsCompressed = msg->compressed;
		memcpy( chan->unsentBuffer, msg->data, msg->cursize );

		// only send the first fragment now
		Netchan_TransmitNextFragment( chan );
		return;
	}

	// write the packet header
	MSG_Init( &send, send_buf, sizeof( send_buf ) );
	MSG_Clear( &send );

	MSG_WriteLong( &send, chan->outgoingSequence );
	// by now our header sends incoming ack too, but we should clean it
	// also add compressed information if it's compressed
	if( msg->compressed )
		MSG_WriteLong( &send, chan->incomingSequence | FRAGMENT_BIT );
	else
		MSG_WriteLong( &send, chan->incomingSequence );

	chan->outgoingSequence++;

	// send the qport if we are a client
	if( chan->sock == NS_CLIENT )
	{
		MSG_WriteShort( &send, qport->integer );
	}

	MSG_CopyData( &send, msg->data, msg->cursize );

	// send the datagram
	NET_SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );

	if( showpackets->integer )
	{
		Com_Printf( "%s send %4i : s=%i ack=%i\n"
			, SockString( chan->sock )
			, send.cursize
			, chan->outgoingSequence - 1
			, chan->incomingSequence );
	}
}

/*
* Netchan_Process
*
* Returns qfalse if the message should not be processed due to being
* out of order or a fragment.
*
* Msg must be large enough to hold MAX_MSGLEN, because if this is the
* final fragment of a multi-part message, the entire thing will be
* copied out.
*/
qboolean Netchan_Process( netchan_t *chan, msg_t *msg )
{
	int sequence, sequence_ack;
	int qport = -1;
	int fragmentStart, fragmentLength;
	qboolean fragmented = qfalse;
	int headerlength;
	qboolean compressed = qfalse;
	qboolean lastfragment = qfalse;


	// get sequence numbers
	MSG_BeginReading( msg );
	sequence = MSG_ReadLong( msg );
	sequence_ack = MSG_ReadLong( msg ); // by now our header sends incoming ack too

	// check for fragment information
	if( sequence & FRAGMENT_BIT )
	{
		sequence &= ~FRAGMENT_BIT;
		fragmented = qtrue;

		if( net_showfragments->integer & 1 && chan->sock == NS_CLIENT )
		{
			Com_Printf( "Process fragmented packet (%s) (id:%i)\n", SockString( chan->sock ), sequence );
		}
		if( net_showfragments->integer & 2 && chan->sock == NS_SERVER )
		{
			Com_Printf( "Process fragmented packet (%s) (id:%i)\n", SockString( chan->sock ), sequence );
		}
	}
	else
	{
		fragmented = qfalse;
	}

	// check for compressed information
	if( sequence_ack & FRAGMENT_BIT )
	{
		sequence_ack &= ~FRAGMENT_BIT;
		compressed = qtrue;
		if( !fragmented )
			msg->compressed = qtrue;
	}

	// read the qport if we are a server
	if( chan->sock == NS_SERVER )
	{
		qport = MSG_ReadShort( msg );
	}

	// read the fragment information
	if( fragmented )
	{
		fragmentStart = MSG_ReadShort( msg );
		fragmentLength = MSG_ReadShort( msg );
		if( fragmentLength & FRAGMENT_LAST )
		{
			lastfragment = qtrue;
			fragmentLength &= ~FRAGMENT_LAST;
		}
	}
	else
	{
		fragmentStart = 0; // stop warning message
		fragmentLength = 0;
	}

	if( showpackets->integer )
	{
		if( fragmented )
		{
			Com_Printf( "%s recv %4i : s=%i fragment=%i,%i\n"
				, SockString( chan->sock )
				, msg->cursize
				, sequence
				, fragmentStart, fragmentLength );
		}
		else
		{
			Com_Printf( "%s recv %4i : s=%i\n"
				, SockString( chan->sock )
				, msg->cursize
				, sequence );
		}
	}

	//
	// discard out of order or duplicated packets
	//
	if( sequence <= chan->incomingSequence )
	{
		if( showdrop->integer || showpackets->integer )
		{
			Com_Printf( "%s:Out of order packet %i at %i\n"
				, NET_AddressToString( &chan->remoteAddress )
				, sequence
				, chan->incomingSequence );
		}
		return qfalse;
	}

	//
	// dropped packets don't keep the message from being used
	//
	chan->dropped = sequence - ( chan->incomingSequence+1 );
	if( chan->dropped > 0 )
	{
		if( showdrop->integer || showpackets->integer )
		{
			Com_Printf( "%s:Dropped %i packets at %i\n"
				, NET_AddressToString( &chan->remoteAddress )
				, chan->dropped
				, sequence );
		}
	}

	//
	// if this is the final framgent of a reliable message,
	// bump incoming_reliable_sequence
	//
	if( fragmented )
	{
		// TTimo
		// make sure we add the fragments in correct order
		// either a packet was dropped, or we received this one too soon
		// we don't reconstruct the fragments. we will wait till this fragment gets to us again
		// (NOTE: we could probably try to rebuild by out of order chunks if needed)
		if( sequence != chan->fragmentSequence )
		{
			chan->fragmentSequence = sequence;
			chan->fragmentLength = 0;
		}

		// if we missed a fragment, dump the message
		if( fragmentStart != (int) chan->fragmentLength )
		{
			if( showdrop->integer || showpackets->integer )
			{
				Com_Printf( "%s:Dropped a message fragment\n"
					, NET_AddressToString( &chan->remoteAddress )
					, sequence );
			}
			// we can still keep the part that we have so far,
			// so we don't need to clear chan->fragmentLength
			return qfalse;
		}

		// copy the fragment to the fragment buffer
		if( fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
			chan->fragmentLength + fragmentLength > sizeof( chan->fragmentBuffer ) )
		{
			if( showdrop->integer || showpackets->integer )
			{
				Com_Printf( "%s:illegal fragment length\n"
					, NET_AddressToString( &chan->remoteAddress ) );
			}
			return qfalse;
		}

		memcpy( chan->fragmentBuffer + chan->fragmentLength,
			msg->data + msg->readcount, fragmentLength );

		chan->fragmentLength += fragmentLength;

		// if this wasn't the last fragment, don't process anything
		if( !lastfragment )
		{
			return qfalse;
		}

		if( chan->fragmentLength > msg->maxsize )
		{
			Com_Printf( "%s:fragmentLength %i > msg->maxsize\n"
				, NET_AddressToString( &chan->remoteAddress ),
				chan->fragmentLength );
			return qfalse;
		}

		// reconstruct the message

		MSG_Clear( msg );
		MSG_WriteLong( msg, sequence );
		MSG_WriteLong( msg, sequence_ack );
		if( chan->sock == NS_SERVER )
			MSG_WriteShort( msg, qport );

		msg->compressed = compressed;

		headerlength = msg->cursize;

		MSG_CopyData( msg, chan->fragmentBuffer, chan->fragmentLength );
		msg->readcount = headerlength; // put read pointer after header again
		chan->fragmentLength = 0;

		//let it be finished as standard packets
	}

	// the message can now be read from the current message pointer
	chan->incomingSequence = sequence;

	// get the ack from the very first fragment
	chan->incoming_acknowledged = sequence_ack;

	return qtrue;
}

//==============================================================================

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

#define	MAX_LOOPBACK	4

typedef struct
{
	qbyte data[MAX_MSGLEN];
	int datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t msgs[MAX_LOOPBACK];
	int get, send;
} loopback_t;

loopback_t loopbacks[2];

static qboolean NET_GetLoopPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message )
{
	int i;
	loopback_t *loop;

	loop = &loopbacks[sock];

	if( loop->send - loop->get > ( MAX_LOOPBACK - 1 ) )  // from q2pro
		loop->get = loop->send - MAX_LOOPBACK + 1; // from q2pro

	if( loop->get >= loop->send )
		return qfalse;

	i = loop->get & ( MAX_LOOPBACK-1 );
	loop->get++;

	memcpy( net_message->data, loop->msgs[i].data, loop->msgs[i].datalen );
	net_message->cursize = loop->msgs[i].datalen;
	memset( net_from, 0, sizeof( *net_from ) );
	net_from->type = NA_LOOPBACK;
	return qtrue;

}

static void NET_SendLoopPacket( netsrc_t sock, size_t length, void *data )
{
	int i;
	loopback_t *loop;

	loop = &loopbacks[sock^1];

	i = loop->send & ( MAX_LOOPBACK-1 );
	loop->send++;

	memcpy( loop->msgs[i].data, data, length );
	loop->msgs[i].datalen = length;
}

/*
* NET_GetPacket
*
* Traps "localhost" for loopback, passes everything else to system
*/
qboolean NET_GetPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message )
{
	if( NET_GetLoopPacket( sock, net_from, net_message ) )
		return qtrue;

	return Sys_GetPacket( sock, net_from, net_message );

}

/*
* NET_SendPacket
*
* Traps "localhost" for loopback, passes everything else to system
*/
void NET_SendPacket( netsrc_t sock, size_t length, /*const*/ void *data, netadr_t to )
{
	if( to.type < NA_LOOPBACK )
		return;

	if( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket( sock, length, data );
		return;
	}

	Sys_SendPacket( sock, length, data, to );
}

/*
* NET_AdrToString
*/
char *NET_AddressToString( netadr_t *a )
{
	static char s[64];

	if( a->type == NA_LOOPBACK )
		Q_snprintfz( s, sizeof( s ), "loopback" );
	else if( a->type == NA_IP || a->type == NA_BROADCAST )
		Q_snprintfz( s, sizeof( s ), "%i.%i.%i.%i:%hu", a->ip[0], a->ip[1], a->ip[2], a->ip[3], BigShort( a->port ) );

	return s;
}

/*
* NET_CompareBaseAdr
*
* Compares without the port
*/
qboolean NET_CompareBaseAdr( netadr_t *a, netadr_t *b )
{
	if( a->type != b->type )
		return qfalse;

	if( a->type == NA_LOOPBACK )
		return qtrue;

	if( a->type == NA_IP )
	{
		if( a->ip[0] == b->ip[0] && a->ip[1] == b->ip[1] && a->ip[2] == b->ip[2] && a->ip[3] == b->ip[3] )
			return qtrue;
		return qfalse;
	}

	Com_Printf( "NET_CompareBaseAdr: bad address type\n" );
	return qfalse;
}

/*
* NET_CompareAdr
*
* Compares with the port
*/
qboolean NET_CompareAdr( netadr_t *a, netadr_t *b )
{
	if( a->type != b->type )
		return qfalse;

	if( a->type == NA_LOOPBACK )
		return qtrue;

	if( a->type == NA_IP )
	{
		if( a->ip[0] == b->ip[0] && a->ip[1] == b->ip[1] && a->ip[2] == b->ip[2] && a->ip[3] == b->ip[3] && a->port == b->port )
			return qtrue;
		return qfalse;
	}

	Com_Printf( "NET_CompareAdr: bad address type\n" );
	return qfalse;
}

/*
* NET_IsLocalAddress
*/
qboolean NET_IsLocalAddress( netadr_t *adr )
{
	return ( adr->type == NA_LOOPBACK );
}

/*
* NET_StringToAddress
*
* Traps "loopback" for loopback, passes everything else to system
*/
qboolean NET_StringToAddress( const char *s, netadr_t *a )
{
	qboolean ret;

	// trap localhost
	if( !strcmp( s, "loopback" ) )
	{
		memset( a, 0, sizeof( *a ) );
		a->type = NA_LOOPBACK;
		return qtrue;
	}

	// colon filtering not implemented by now

	ret = Sys_StringToAdr( s, a ); // call system function

	if( !ret )
	{
		a->type = NA_NOTRANSMIT;
		return qfalse;
	}

	// inet_addr returns this if out of range
	if( a->ip[0] == 255 && a->ip[1] == 255 && a->ip[2] == 255 && a->ip[3] == 255 )
	{
		a->type = NA_NOTRANSMIT;
		return qfalse;
	}

	//port replacing not implemented by now

	return ret;
}
