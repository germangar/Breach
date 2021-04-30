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
// server.h

#include "../qcommon/qcommon.h"
#include "../game/g_public.h"

extern cvar_t *sv_maxclients;
extern cvar_t *sv_compresspackets;
extern cvar_t *sv_public;         // should heartbeats be sent
extern cvar_t *sv_snap_nocull;
extern cvar_t *sv_debug_serverCmd;
extern cvar_t *sv_uploads;
extern cvar_t *sv_uploads_from_server;
extern cvar_t *sv_uploads_baseurl;
extern cvar_t *sv_pure;
extern cvar_t *sv_pure_forcemodulepk3;
extern cvar_t *sv_lastAutoUpdate;
extern cvar_t *sv_defaultmap;

//=============================================================================

#define	MAX_MASTERS 8               // max recipients for heartbeat packets
#define	HEARTBEAT_SECONDS   300

typedef enum
{
	ss_dead,        // no map loaded
	ss_loading,     // spawning level edicts
	ss_game         // actively running
} server_state_t;

typedef struct
{
	entity_state_t entityStateBackups[SNAPS_BACKUP_SIZE][MAX_EDICTS];
	player_state_t playerStateBackups[SNAPS_BACKUP_SIZE][MAX_CLIENTS];
	game_state_t gameStateBackups[SNAPS_BACKUP_SIZE];
	entity_state_t baselines[MAX_EDICTS];
} sv_snapsdata_t;

// some commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

typedef struct
{
	server_state_t state;       // precache commands are only valid during load

	unsigned nextSnapTime;              // always sv.framenum * svc.snapFrameTime msec
	unsigned framenum;

	char mapname[MAX_QPATH];

	char configstrings[MAX_CONFIGSTRINGS][MAX_CONFIGSTRING_CHARS];
	sv_snapsdata_t snapsData;
} server_t;

typedef struct
{
	char *name;
	qbyte *data;            // file being downloaded
	int size;               // total bytes (can't use EOF because of paks)
	unsigned int timeout;   // so we can free the file being downloaded
	                        // if client omits sending success or failure message
} client_download_t;

typedef struct
{
	unsigned int framenum;
	char command[MAX_STRING_CHARS];
} game_command_t;

#define	LATENCY_COUNTS	16

typedef struct client_s
{
	sv_client_state_t state;
	qboolean fakeClient;

	char userinfo[MAX_INFO_STRING];             // name, etc

	char reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	unsigned int reliableSequence;      // last added reliable message, not necesarily sent or acknowledged yet
	unsigned int reliableAcknowledge;   // last acknowledged reliable message
	unsigned int reliableSent;          // last sent reliable message, not necesarily acknowledged yet

	unsigned int clientCommandExecuted; // last client-command we received

	unsigned int UcmdTime;
	unsigned int UcmdExecuted;          // last client-command we executed
	unsigned int UcmdReceived;          // last client-command we received
	usercmd_t ucmds[CMD_BACKUP];        // each message will send several old cmds

	unsigned int lastPacketSentTime;    // time when we sent the last message to this client
	unsigned int lastPacketReceivedTime; // time when we received the last message from this client
	unsigned lastconnect;

	unsigned int snapAcknowledged;  // for delta compression
	qboolean noDeltaSnap;

	char name[MAX_INFO_VALUE];          // extracted from userinfo, high bits masked

	snapshot_t snaps[SNAPS_BACKUP_SIZE];

	client_download_t download;

	int challenge;                  // challenge of this user, randomly generated

	netchan_t netchan;
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

typedef struct
{
	netadr_t adr;
	int challenge;
	int time;
} challenge_t;


typedef struct
{
	qboolean initialized;               // sv_init has completed
	unsigned realtime;                  // real world time - always increasing, no clamping, etc
	unsigned gametime;                  // game world time - always increasing, no clamping, etc

	char mapcmd[MAX_TOKEN_CHARS];       // ie: *intro.cin+base

	int spawncount;                     // incremented each server start
	                                    // used to check late spawns

	client_t *clients;                  // [maxclients->integer];

	challenge_t challenges[MAX_CHALLENGES]; // to prevent invalid IPs from connecting

	// pure file support
	purelist_t *purelist;				// pure file support

	cmodel_state_t *cms;                // passed to CM-functions

} server_static_t;

typedef struct
{
	int last_heartbeat;
	unsigned int last_activity;
	unsigned int snapFrameTime;     // msecs between server packets
	unsigned int gameFrameTime;     // msecs between game code executions
	qboolean autostarted;
} server_constant_t;

//=============================================================================

// shared message buffer to be used for occasional messages
extern msg_t tmpMessage;
extern qbyte tmpMessageData[MAX_MSGLEN];

extern netadr_t	net_from;

extern mempool_t *sv_mempool;

extern server_constant_t svc;              // constant server info (trully persistant since sv_init)
extern server_static_t svs;                // persistant server info
extern server_t	sv;                 // local server

#define CLIENTNUM( c )	  ( ( c )-svs.clients )

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage( char *message, qboolean reconnect );

int SV_ModelIndex( const char *name );
int SV_SoundIndex( const char *name );
int SV_ImageIndex( const char *name );
int SV_SkinIndex( const char *name );

void SV_WriteClientdataToMessage( client_t *client, msg_t *msg );

void SV_AutoUpdateFromWeb( qboolean checkOnly );
void SV_InitOperatorCommands( void );

void SV_SendServerinfo( client_t *client );
void SV_UserinfoChanged( client_t *cl );

void SV_MasterHeartbeat( void );

void SVC_MasterInfoResponse( void );
int SVC_FakeConnect( char *fakeUserinfo, char *fakeIP );
void SV_UpdateActivity( void );

//
// sv_oob.c
//
void SV_ConnectionlessPacket( msg_t *msg );
void SV_InitMaster( void );

//
// sv_init.c
//
void SV_InitGame( void );
void SV_Map( const char *level, qboolean devmap );
void SV_AddPureFile( const char *filename );
void SV_PureList_f( void );

//
// sv_phys.c
//
void SV_PrepWorldFrame( void );

//
// sv_send.c
//
void SV_AddServerCommand( client_t *client, const char *cmd );
void SV_SendServerCommand( client_t *cl, const char *format, ... );
void SV_AddReliableCommandsToMessage( client_t *client, msg_t *msg );
qboolean SV_SendClientsFragments( void );
void SV_InitClientMessage( client_t *client, msg_t *msg, qbyte *data, size_t size );
void SV_SendMessageToClient( client_t *client, msg_t *msg );

typedef enum { RD_NONE, RD_PACKET } redirect_t;

#define	SV_OUTPUTBUF_LENGTH ( MAX_MSGLEN - 16 )

extern char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect( int sv_redirected, char *outputbuf );
void SV_SendClientMessages( void );
void SV_ClientPrintf( client_t *cl, const char *format, ... );
void SV_ClientChatf( client_t *cl, const char *format, ... );
void SV_BroadcastCommand( const char *format, ... );

//
// sv_client.c
//
void SV_ParseClientMessage( client_t *client, msg_t *msg );
qboolean SV_ClientConnect( client_t *client, char *userinfo, int qport, int challenge, qboolean fakeClient );
void SV_DropClient( client_t *drop, int type, const char *format, ... );
void SV_ExecuteClientThinks( int clientNum );

//
// sv_ccmds.c
//
void SV_Status_f( void );

//
// sv_ents.c
//
extern void SV_CreateBaselines( void );
extern void SV_BackUpSnapshotData( unsigned int snapNum, int numEntities );
extern void SV_WriteDeltaEntity( msg_t *msg, unsigned int entNum, unsigned int deltaSnapNum, unsigned int snapNum, qboolean force );
extern void SV_WriteFrameSnapToClient( client_t *client, msg_t *msg );
extern void SV_BuildClientFrameSnap( client_t *client );
extern void SV_SetVIS( vec3_t origin, qboolean merge );
extern qboolean SV_PVSCullEntity( int ent_headnode, int ent_num_clusters, int *ent_clusternums );
extern qboolean SV_PHSCullEntity( int ent_headnode, int ent_num_clusters, int *ent_clusternums );
extern qboolean SV_AreaCullEntity( int ent_areanum, int ent_areanum2 );


void SV_Error( char *error, ... );

//
// sv_game.c
//
extern game_export_t *ge;

void SV_InitGameProgs( void );
void SV_ShutdownGameProgs( void );
