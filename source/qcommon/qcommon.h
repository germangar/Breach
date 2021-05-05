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

// qcommon.h -- definitions common between client and server, but not game.dll

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_comref.h"
#include "../gameshared/q_msg.h"
#include "../gameshared/q_collision.h"

#include "qfiles.h"
#include "cmodel.h"
#include "version.h"

#define	DEFAULT_BASEGAME    "base"

//============================================================================

int COM_Argc( void );
char *COM_Argv( int arg );  // range and null checked
void COM_ClearArgv( int arg );
int COM_CheckParm( char *parm );
void COM_AddParm( char *parm );

void COM_Init( void );
void COM_InitArgv( int argc, char **argv );

char *_ZoneCopyString( const char *in, const char *filename, int fileline );
char *_TempCopyString( const char *in, const char *filename, int fileline );

#define ZoneCopyString( in ) _ZoneCopyString( in, __FILE__, __LINE__ )
#define TempCopyString( in ) _TempCopyString( in, __FILE__, __LINE__ )

int Com_GlobMatch( const char *pattern, const char *text, const qboolean casecmp );

void Info_Print( char *s );

//============================================================================

/* crc.h */
void CRC_Init( unsigned short *crcvalue );
void CRC_ProcessByte( unsigned short *crcvalue, qbyte data );
unsigned short CRC_Value( unsigned short crcvalue );
unsigned short CRC_Block( qbyte *start, int count );

/* patch.h */
void Patch_GetFlatness( float maxflat, const float *points, int comp, const int *patch_cp, int *flat );
void Patch_Evaluate( const vec_t *p, int *numcp, const int *tess, vec_t *dest, int comp );

/*
==============================================================

BSP FORMATS

==============================================================
*/

typedef void ( *modelLoader_t )( void *param0, void *param1, void *param2, void *param3 );

#define BSP_NONE		0
#define BSP_RAVEN		1
#define BSP_NOAREAS		2

typedef struct
{
	const char * const header;
	const int * const versions;
	const int lightmapWidth;
	const int lightmapHeight;
	const int flags;
	const int entityLumpNum;
} bspFormatDesc_t;

typedef struct
{
	const char * const header;
	const int headerLen;
	const bspFormatDesc_t * const bspFormats;
	const int maxLods;
	const modelLoader_t loader;
} modelFormatDescr_t;

extern const bspFormatDesc_t q1BSPFormats[];
extern const bspFormatDesc_t q2BSPFormats[];
extern const bspFormatDesc_t q3BSPFormats[];

const bspFormatDesc_t *Com_FindBSPFormat( const bspFormatDesc_t *formats, const char *header, int version );
const modelFormatDescr_t *Com_FindFormatDescriptor( const modelFormatDescr_t *formats, const qbyte *buf, const bspFormatDesc_t **bspFormat );


/*
   ==============================================================

   PROTOCOL

   ==============================================================
 */

// protocol.h -- communications protocols

//=========================================

#define	PORT_MASTER     27950
#define	PORT_CLIENT     45000+( rand()%5000 )
#define	PORT_SERVER     44400


//==================
// the svc_strings[] array in cl_parse.c should mirror this
//==================

//
// server to client
//
enum svc_ops_e
{
	svc_bad,

	// the rest are private to the client and server
	svc_nop,
	svc_servercmd,      // [string] string
	svc_serverdata,     // [int] protocol ...
	svc_spawnbaseline,
	svc_download,       // [short] size [size bytes]
	svc_playerinfo,     // variable
	svc_packetentities, // [...]
	svc_clcack,
	svc_gameinfo,
	svc_frame,
	svc_demoinfo,
	svc_extension			// for future expansion
};

//==============================================

//
// client to server
//
enum clc_ops_e
{
	clc_bad,
	clc_nop,
	clc_move,       // [[usercmd_t]
	clc_svcack,
	clc_clientcommand,   // [string] message
	clc_extension
};

//==============================================

// serverdata flags
#define SV_BITFLAGS_PURE	( 1<<0 )

//==============================================

// ms and light always sent, the others are optional
#define	CM_ANGLE1   ( 1<<0 )
#define	CM_ANGLE2   ( 1<<1 )
#define	CM_ANGLE3   ( 1<<2 )
#define	CM_FORWARD  ( 1<<3 )
#define	CM_SIDE     ( 1<<4 )
#define	CM_UP       ( 1<<5 )
#define	CM_BUTTONS  ( 1<<6 )

//==============================================

#include "com_snapshot.h"

/*
   ==============================================================

   Library

   Dynamic library loading

   ==============================================================
 */


#ifdef __cplusplus
#define EXTERN_API_FUNC    extern "C"
#else
#define EXTERN_API_FUNC    extern
#endif

// qcommon/library.c
typedef struct { char *name; void **funcPointer; } dllfunc_t;

void Com_UnloadLibrary( void **lib );
void *Com_LoadLibrary( const char *name, dllfunc_t *funcs ); // NULL-terminated array of functions

void *Com_LoadGameLibrary( const char *basename, const char *apifuncname, void **handle, void *parms,
						  void *( *builtinAPIfunc )(void *), qboolean pure, char *manifest );
void Com_UnloadGameLibrary( void **handle );

/*
   ==============================================================

   CMD

   Command text buffering and command execution

   ==============================================================
 */

/*

   Any number of commands can be added in a frame, from several different sources.
   Most commands come from either keybindings or console line input, but remote
   servers can also send across commands and entire text files can be execed.

   The + command line options are also added to the command buffer.

   The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute ();

 */

void        Cbuf_Init( void );
void	    Cbuf_Shutdown( void );
void        Cbuf_AddText( const char *text );
void        Cbuf_InsertText( const char *text );
void        Cbuf_ExecuteText( int exec_when, const char *text );
void        Cbuf_AddEarlyCommands( qboolean clear );
qboolean    Cbuf_AddLateCommands( void );
void        Cbuf_Execute( void );

//===========================================================================

/*

   Command execution takes a null terminated string, breaks it into tokens,
   then searches for a command or variable that matches the first token.

 */

typedef void ( *xcommand_t )( void );

void        Cmd_PreInit( void );
void        Cmd_Init( void );
void		Cmd_Shutdown( void );
void        Cmd_AddCommand( const char *cmd_name, xcommand_t function );
void        Cmd_RemoveCommand( const char *cmd_name );
qboolean    Cmd_Exists( const char *cmd_name );
char *Cmd_CompleteCommand( const char *partial );
void        Cmd_WriteAliases( int file );
int	    Cmd_CompleteAliasCountPossible( const char *partial );
char **Cmd_CompleteAliasBuildList( const char *partial );
int	    Cmd_CompleteCountPossible( const char *partial );
char **Cmd_CompleteBuildList( const char *partial );
char *Cmd_CompleteAlias( const char *partial );
int	    Cmd_Argc( void );
char *Cmd_Argv( int arg );
char *Cmd_Args( void );
void        Cmd_TokenizeString( const char *text );
void        Cmd_ExecuteString( const char *text );
void        Cmd_ForwardToServer( void );

/*
   ==============================================================

   CVAR

   ==============================================================
 */

#include "cvar.h"

/*
   ==============================================================

   SVN

   ==============================================================
 */

int SVN_RevNumber( void );
const char *SVN_RevString( void );

/*
   ==============================================================

   NET

   ==============================================================
 */

// net.h -- quake's interface to the networking layer

#define	PORT_ANY    -1

#define	PACKET_HEADER	10          // two ints, and a short

#define	MAX_RELIABLE_COMMANDS	64          // max string commands buffered for restransmit
#define	MAX_PACKETLEN	    1400        // max size of a network packet
#define	MAX_MSGLEN	16384       // max length of a message, which may be fragmented into multiple packets
#define	FRAGMENT_SIZE	    ( MAX_PACKETLEN - 96 )
#define	FRAGMENT_LAST	    ( 1<<14 )
#define	FRAGMENT_BIT	    ( 1<<31 )

typedef enum
{
	NA_NOTRANSMIT,  // fakeclients
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP
} netadrtype_t;

typedef enum { NS_CLIENT, NS_SERVER } netsrc_t;

typedef struct netadr_s
{
	netadrtype_t type;
	qbyte ip[4];
	unsigned short port;
} netadr_t;

char *NET_ErrorString( void );
void        NET_Init( void );
void        NET_Shutdown( void );

void        NET_Config( qboolean multiplayer );

qboolean    NET_GetPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message );
qboolean    Sys_GetPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message );
void        NET_SendPacket( netsrc_t sock, size_t length, /*const*/ void *data, netadr_t to );
void        Sys_SendPacket( netsrc_t sock, size_t length, /*const*/ void *data, netadr_t to );
qboolean    NET_StringToAddress( const char *s, netadr_t *a );
qboolean    Sys_StringToAdr( const char *s, netadr_t *a );
char *NET_AddressToString( netadr_t *a );
void        NET_Sleep( int msec );
qboolean    Sys_IsLANAddress( netadr_t adr );
void        Sys_ShowIP( void );

//
//	Shared system. Inside net_chan.c
//
qboolean    NET_CompareAdr( netadr_t *a, netadr_t *b );
qboolean    NET_CompareBaseAdr( netadr_t *a, netadr_t *b );
qboolean    NET_IsLocalAddress( netadr_t *adr );

//============================================================================

typedef struct
{
	netsrc_t sock;

	int dropped;            // between last packet and previous

	netadr_t remoteAddress;
	int qport;              // qport value to write when transmitting

	// sequencing variables
	int incomingSequence;
	int incoming_acknowledged;
	int outgoingSequence;

	// incoming fragment assembly buffer
	int fragmentSequence;
	size_t fragmentLength;
	qbyte fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	qboolean unsentFragments;
	size_t unsentFragmentStart;
	size_t unsentLength;
	qbyte unsentBuffer[MAX_MSGLEN];
	qboolean unsentIsCompressed;

	qboolean fatal_error;
} netchan_t;

extern netadr_t	net_from;


void Netchan_Init( void );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport );
qboolean Netchan_Process( netchan_t *chan, msg_t *msg );
void Netchan_Transmit( netchan_t *chan, msg_t *msg );
void Netchan_PushAllFragments( netchan_t *chan );
void Netchan_TransmitNextFragment( netchan_t *chan );
int Netchan_CompressMessage( msg_t *msg );
int Netchan_DecompressMessage( msg_t *msg );
void Netchan_OutOfBand( int net_socket, netadr_t adr, size_t length, qbyte *data );
void Netchan_OutOfBandPrint( int net_socket, netadr_t adr, const char *format, ... );

/*
==============================================================

FILESYSTEM

==============================================================
*/

#define FS_NOTIFT_NEWPAKS	0x01

void FS_Init( void );
int FS_Rescan( void );
void FS_Frame( void );
void FS_Shutdown( void );

const char *FS_GameDirectory( void );
const char *FS_BaseGameDirectory( void );
qboolean    FS_SetGameDirectory( const char *dir, qboolean force );
int			FS_GetGameDirectoryList( char *buf, size_t bufsize );
int			FS_GetExplicitPurePakList( char ***paknames );

// handling of absolute filenames
// only to be used if necessary (library not supporting custom file handling functions etc.)
const char *FS_WriteDirectory( void );
void        FS_CreateAbsolutePath( const char *path );
const char *FS_AbsoluteNameForFile( const char *filename );
const char *FS_AbsoluteNameForBaseFile( const char *filename );

// // game and base files
// file streaming
int     FS_FOpenFile( const char *filename, int *filenum, int mode );
int     FS_FOpenBaseFile( const char *filename, int *filenum, int mode );
int		FS_FOpenAbsoluteFile( const char *filename, int *filenum, int mode );
void    FS_FCloseFile( int file );
void    FS_FCloseBaseFile( int file );

int     FS_Read( void *buffer, size_t len, int file );
int		FS_Print( int file, const char *msg );
int     FS_Printf( int file, const char *format, ... );
int     FS_Write( const void *buffer, size_t len, int file );
int     FS_Tell( int file );
int     FS_Seek( int file, int offset, int whence );
int     FS_Eof( int file );
int     FS_Flush( int file );

// file loading
int	    FS_LoadFileExt( const char *path, void **buffer, void *stack, size_t stackSize, const char *filename, int fileline );
int	    FS_LoadBaseFileExt( const char *path, void **buffer, void *stack, size_t stackSize, const char *filename, int fileline );
void	FS_FreeFile( void *buffer );
void	FS_FreeBaseFile( void *buffer );
#define FS_LoadFile( path,buffer,stack,stacksize ) FS_LoadFileExt( path,buffer,stack,stacksize,__FILE__,__LINE__ )
#define FS_LoadBaseFile( path,buffer,stack,stacksize ) FS_LoadBaseFileExt( path,buffer,stack,stacksize,__FILE__,__LINE__ )

int		FS_GetNotifications( void );
int		FS_RemoveNotifications( int bitmask );

// util functions
qboolean    FS_CopyFile( const char *src, const char *dst );
qboolean    FS_CopyBaseFile( const char *src, const char *dst );
qboolean    FS_MoveFile( const char *src, const char *dst );
qboolean    FS_MoveBaseFile( const char *src, const char *dst );
qboolean    FS_RemoveFile( const char *filename );
qboolean    FS_RemoveBaseFile( const char *filename );
qboolean    FS_RemoveAbsoluteFile( const char *filename );
qboolean    FS_RemoveDirectory( const char *dirname );
qboolean    FS_RemoveBaseDirectory( const char *dirname );
qboolean    FS_RemoveAbsoluteDirectory( const char *dirname );
unsigned    FS_ChecksumAbsoluteFile( const char *filename );
unsigned    FS_ChecksumBaseFile( const char *filename );
qboolean	FS_CheckPakExtension( const char *filename );

// // only for game files
const char *FS_FirstExtension( const char *filename, const char *extensions[], int num_extensions );
const char *FS_PakNameForFile( const char *filename );
qboolean    FS_IsPureFile( const char *pakname );
const char *FS_FileManifest( const char *filename );
const char *FS_BaseNameForFile( const char *filename );

int     FS_GetFileList( const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end );
int     FS_GetFileListExt( const char *dir, const char *extension, char *buf, size_t *bufsize, int start, int end );

// // only for base files
qboolean    FS_IsPakValid( const char *filename, unsigned *checksum );
qboolean    FS_AddPurePak( unsigned checksum );
void        FS_RemovePurePaks( void );

/*
   ==============================================================

   MISC

   ==============================================================
 */


#define	ERR_FATAL   0       // exit the entire game with a popup window
#define	ERR_DROP    1       // print to console and disconnect from game
#define	ERR_DISCONNECT	2       // don't kill server

#define MAX_PRINTMSG	3072

void        Com_BeginRedirect( int target, char *buffer, int buffersize, void ( *flush ) );
void        Com_EndRedirect( void );
void        Com_Printf( const char *format, ... );
void        Com_DPrintf( const char *format, ... );
void        Com_Error( int code, const char *format, ... );
void        Com_Quit( void );

int     Com_ClientState( void );        // this should have just been a cvar...
void        Com_SetClientState( int state );

qboolean    Com_DemoPlaying( void );
void        Com_SetDemoPlaying( qboolean state );

int     Com_ServerState( void );        // this should have just been a cvar...
void        Com_SetServerState( int state );

unsigned    Com_BlockChecksum( void *buffer, size_t length );
qbyte       COM_BlockSequenceCRCByte( qbyte *base, size_t length, int sequence );

void        Com_PageInMemory( qbyte *buffer, int size );

unsigned int Com_HashKey( const char *name, int hashsize );

unsigned int Com_DaysSince1900( void );

extern cvar_t *developer;
extern cvar_t *dedicated;
extern cvar_t *host_speeds;
extern cvar_t *log_stats;
extern cvar_t *versioncvar;
extern cvar_t *revisioncvar;

extern int log_stats_file;

// host_speeds times
extern unsigned int time_before_game;
extern unsigned int time_after_game;
extern unsigned int time_before_ref;
extern unsigned int time_after_ref;


//#define MEMCLUMPING
//#define MEMTRASH

#define POOLNAMESIZE 128

#ifdef MEMCLUMPING

// give malloc padding so we can't waste most of a page at the end
#define MEMCLUMPSIZE ( 65536 - 1536 )

// smallest unit we care about is this many bytes
#define MEMUNIT 8
#define MEMBITS ( MEMCLUMPSIZE / MEMUNIT )
#define MEMBITINTS ( MEMBITS / 32 )
#define MEMCLUMP_SENTINEL 0xABADCAFE

#endif

#define MEMHEADER_SENTINEL1 0xDEADF00D
#define MEMHEADER_SENTINEL2 0xDF

typedef struct memheader_s
{
	// next and previous memheaders in chain belonging to pool
	struct memheader_s *next;
	struct memheader_s *prev;

	// pool this memheader belongs to
	struct mempool_s *pool;

#ifdef MEMCLUMPING
	// clump this memheader lives in, NULL if not in a clump
	struct memclump_s *clump;
#endif

	// size of the memory after the header (excluding header and sentinel2)
	size_t size;

	// file name and line where Mem_Alloc was called
	const char *filename;
	int fileline;

	// should always be MEMHEADER_SENTINEL1
	unsigned int sentinel1;
	// immediately followed by data, which is followed by a MEMHEADER_SENTINEL2 byte
} memheader_t;

#ifdef MEMCLUMPING

typedef struct memclump_s
{
	// contents of the clump
	qbyte block[MEMCLUMPSIZE];

	// should always be MEMCLUMP_SENTINEL
	unsigned int sentinel1;

	// if a bit is on, it means that the MEMUNIT bytes it represents are
	// allocated, otherwise free
	int bits[MEMBITINTS];

	// should always be MEMCLUMP_SENTINEL
	unsigned int sentinel2;

	// if this drops to 0, the clump is freed
	int blocksinuse;

	// largest block of memory available (this is reset to an optimistic
	// number when anything is freed, and updated when alloc fails the clump)
	int largestavailable;

	// next clump in the chain
	struct memclump_s *chain;
} memclump_t;

#endif

#define MEMPOOL_TEMPORARY		1
#define MEMPOOL_GAMEPROGS		2
#define MEMPOOL_USERINTERFACE	4
#define MEMPOOL_CLIENTGAME		8
#define MEMPOOL_SOUND			16
#define MEMPOOL_ANGELSCRIPT		64

typedef struct mempool_s
{
	// should always be MEMHEADER_SENTINEL1
	unsigned int sentinel1;

	// chain of individual memory allocations
	struct memheader_s *chain;

#ifdef MEMCLUMPING
	// chain of clumps (if any)
	struct memclump_s *clumpchain;
#endif

	// temporary, etc
	int flags;

	// total memory allocated in this pool (inside memheaders)
	int totalsize;

	// total memory allocated in this pool (actual malloc total)
	int realsize;

	// updated each time the pool is displayed by memlist, shows change from previous time (unless pool was freed)
	int lastchecksize;

	// name of the pool
	char name[POOLNAMESIZE];

	// linked into global mempool list or parent's children list
	struct mempool_s *next;

	struct mempool_s *parent;
	struct mempool_s *child;

	// file name and line where Mem_AllocPool was called
	const char *filename;

	int fileline;

	// should always be MEMHEADER_SENTINEL1
	unsigned int sentinel2;
} mempool_t;

void Memory_Init( void );
void Memory_InitCommands( void );
void Memory_Shutdown( void );
void Memory_ShutdownCommands( void );

void *_Mem_AllocExt( mempool_t *pool, size_t size, int z, int musthave, int canthave, const char *filename, int fileline );
void *_Mem_Alloc( mempool_t *pool, size_t size, int musthave, int canthave, const char *filename, int fileline );
void *_Mem_Realloc( void *data, size_t size, const char *filename, int fileline );
void _Mem_Free( void *data, int musthave, int canthave, const char *filename, int fileline );
mempool_t *_Mem_AllocPool( mempool_t *parent, const char *name, int flags, const char *filename, int fileline );
mempool_t *_Mem_AllocTempPool( const char *name, const char *filename, int fileline );
void _Mem_FreePool( mempool_t **pool, int musthave, int canthave, const char *filename, int fileline );
void _Mem_EmptyPool( mempool_t *pool, int musthave, int canthave, const char *filename, int fileline );

void _Mem_CheckSentinels( void *data, const char *filename, int fileline );
void _Mem_CheckSentinelsGlobal( const char *filename, int fileline );

#define Mem_AllocExt( pool, size, z ) _Mem_AllocExt( pool, size, z, 0, 0, __FILE__, __LINE__ )
#define Mem_Alloc( pool, size ) _Mem_Alloc( pool, size, 0, 0, __FILE__, __LINE__ )
#define Mem_Realloc( data, size ) _Mem_Realloc( data, size, __FILE__, __LINE__ )
#define Mem_Free( mem ) _Mem_Free( mem, 0, 0, __FILE__, __LINE__ )
#define Mem_AllocPool( parent, name ) _Mem_AllocPool( parent, name, 0, __FILE__, __LINE__ )
#define Mem_AllocTempPool( name ) _Mem_AllocTempPool( name, __FILE__, __LINE__ )
#define Mem_FreePool( pool ) _Mem_FreePool( pool, 0, 0, __FILE__, __LINE__ )
#define Mem_EmptyPool( pool ) _Mem_EmptyPool( pool, 0, 0, __FILE__, __LINE__ )

#define Mem_CheckSentinels( data ) _Mem_CheckSentinels( data, __FILE__, __LINE__ )
#define Mem_CheckSentinelsGlobal() _Mem_CheckSentinelsGlobal( __FILE__, __LINE__ )

// used for temporary allocations
extern mempool_t *tempMemPool;
extern mempool_t *zoneMemPool;

#define Mem_ZoneMallocExt( size, z ) Mem_AllocExt( zoneMemPool, size, z )
#define Mem_ZoneMalloc( size ) Mem_Alloc( zoneMemPool, size )
#define Mem_ZoneFree( data ) Mem_Free( data )

#define Mem_TempMallocExt( size, z ) Mem_AllocExt( tempMemPool, size, z )
#define Mem_TempMalloc( size ) Mem_Alloc( tempMemPool, size )
#define Mem_TempFree( data ) Mem_Free( data )

void *Q_malloc( size_t size );
void *Q_realloc( void *buf, size_t newsize );
void Q_free( void *buf );

void Qcommon_Init( int argc, char **argv );
void Qcommon_Frame( unsigned int realmsec );
void Qcommon_Shutdown( void );

/*
   ==============================================================

   NON-PORTABLE SYSTEM SERVICES

   ==============================================================
 */

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

void	Sys_Init( void );
void	Sys_AppActivate( void );

unsigned int	Sys_Milliseconds( void );
quint64	    Sys_Microseconds( void );
void	    Sys_Sleep( unsigned int millis );

char *Sys_ConsoleInput( void );
void	Sys_ConsoleOutput( char *string );
void	Sys_SendKeyEvents( void );
void	Sys_Error( const char *error, ... );
void	Sys_Quit( void );
char	*Sys_GetClipboardData( qboolean primary );
qboolean Sys_SetClipboardData( char *data );
void	Sys_FreeClipboardData( char *data );

// get symbol address in executable
#ifdef SYS_SYMBOL
void *Sys_GetSymbol( const char *moduleName, const char *symbolName );
#endif

/*
==============================================================

CPU FEATURES

==============================================================
*/

#define QCPU_HAS_RDTSC		0x00000001
#define QCPU_HAS_MMX		0x00000002
#define QCPU_HAS_MMXEXT		0x00000004
#define QCPU_HAS_3DNOW		0x00000010
#define QCPU_HAS_3DNOWEXT	0x00000020
#define QCPU_HAS_SSE		0x00000040
#define QCPU_HAS_SSE2		0x00000080

unsigned int COM_CPUFeatures( void );

/*
   ==============================================================

   CLIENT / SERVER SYSTEMS

   ==============================================================
 */

void CL_Init( void );
void CL_Disconnect( const char *message );
void CL_Shutdown( void );
void CL_Frame( int realmsec, int gamemsec );
void Con_Print( char *text );
void SCR_BeginLoadingPlaque( void );

void SV_Init( void );
void SV_Shutdown( char *finalmsg, qboolean reconnect );
void SV_Frame( int realmsec, int gamemsec );

/*
==============================================================

PURE SUBSYSTEM

==============================================================
*/
typedef struct purelist_s
{
	char *filename;
	unsigned checksum;
	struct purelist_s *next;
} purelist_t;

void Com_AddPakToPureList( purelist_t **purelist, const char *pakname, const unsigned checksum, struct mempool_s *mempool );
unsigned Com_CountPureListFiles( purelist_t *purelist );
purelist_t *Com_FindPakInPureList( purelist_t *purelist, const char *pakname );
void Com_FreePureList( purelist_t **purelist );

/*
==============================================================

ANGELSCRIPT SUBSYSTEM

==============================================================
*/

void Com_ScriptModule_Init( void );
void Com_ScriptModule_Shutdown( void );
struct angelwrap_api_s *Com_asGetAngelExport( void );

/*
==============================================================

MAPLIST SUBSYSTEM

==============================================================
*/
void ML_Init( void );
void ML_Shutdown( void );
void ML_Restart( qboolean forcemaps );
qboolean ML_Update( void );

const char *ML_GetFilenameExt( const char *fullname, qboolean recursive );
const char *ML_GetFilename( const char *fullname );
const char *ML_GetFullname( const char *filename );
size_t ML_GetMapByNum( int num, char *out, size_t size );

qboolean ML_FilenameExists( const char *filename );

qboolean ML_ValidateFilename( const char *filename );
qboolean ML_ValidateFullname( const char *fullname );

