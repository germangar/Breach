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

// g_public.h -- game dll information visible to server

#define	GAME_API_VERSION    3

// connection state of the client in the server
typedef enum
{
	CS_FREE,    // can be reused for a new connection
	CS_ZOMBIE,  // client has been disconnected, but don't reuse connection for a couple seconds
	CS_CONNECTING, // has send a "new" command, is awaiting for fetching configstrings
	CS_CONNECTED, // has been assigned to a client_t, but not in game yet
	CS_SPAWNED  // client is fully in game
} sv_client_state_t;

//===============================================================

//
// functions provided by the main engine
//
typedef struct
{
	// special messages
	void ( *Print )( const char *msg );

	// aborts server with a game error
	void ( *Error )( const char *msg );

	// server commands sent to clients
	void ( *ServerCmd )( int clientNum, char *cmd );

	// config strings hold all the index strings,
	// and misc data like audio track and gridsize.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void ( *ConfigString )( int num, const char *string );
	const char *( *GetConfigString )( int num );

	// general 2D layout
	void ( *Layout )( int clientNum, char *string );

	// stuffed into client's console buffer, should be \n terminated
	void ( *StuffCmd )( int clientNum, char *string );

	// the *index functions create configstrings and some internal server state
	int ( *ModelIndex )( const char *name );
	int ( *SoundIndex )( const char *name );
	int ( *ImageIndex )( const char *name );
	int ( *SkinIndex )( const char *name );

	void ( *PureSound )( const char *name );
	void ( *PureModel )( const char *name );

	unsigned int ( *Milliseconds )( void );

	// visibility for snap
	void ( *SetVIS )( vec3_t origin, qboolean merge );
	qboolean ( *PVSCullEntity )( int ent_headnode, int ent_num_clusters, int *ent_clusternums );
	qboolean ( *PHSCullEntity )( int ent_headnode, int ent_num_clusters, int *ent_clusternums );
	qboolean ( *AreaCullEntity )( int ent_areanum, int ent_areanum2 );

	qboolean ( *inPVS )( vec3_t p1, vec3_t p2 );

	int ( *CM_NumInlineModels )( void );
	struct cmodel_s	*( *CM_InlineModel )( int num );
	int ( *CM_TransformedPointContents )( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles );
	void ( *CM_TransformedBoxTrace )( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles );
	void ( *CM_InlineModelBounds )( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs );
	struct cmodel_s	*( *CM_ModelForBBox )( vec3_t mins, vec3_t maxs );
	void ( *CM_SetAreaPortalState )( int area, int otherarea, qboolean open );
	qboolean ( *CM_AreasConnected )( int area1, int area2 );
	int ( *CM_BoxLeafnums )( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode );
	int ( *CM_LeafCluster )( int leafnum );
	int ( *CM_LeafArea )( int leafnum );

	// managed memory allocation
	void *( *Mem_Alloc )( size_t size, const char *filename, int fileline );
	void ( *Mem_Free )( void *data, const char *filename, int fileline );

	// console variable interaction
	cvar_t *( *Cvar_Get )( const char *name, const char *value, int flags );
	cvar_t *( *Cvar_Set )( const char *name, const char *value );
	void ( *Cvar_SetValue )( const char *name, float value );
	cvar_t *( *Cvar_ForceSet )( const char *name, const char *value );  // will return 0 0 if not found
	float ( *Cvar_Value )( const char *name );
	const char *( *Cvar_String )( const char *name );

	// ClientCommand and ServerCommand parameter access
	int ( *Cmd_Argc )( void );
	char *( *Cmd_Argv )( int arg );
	char *( *Cmd_Args )( void );        // concatenation of all argv >= 1

	void ( *Cmd_AddCommand )( const char *name, void ( *cmd )( void ) );
	void ( *Cmd_RemoveCommand )( const char *cmd_name );

	// files will be memory mapped read only
	// the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int ( *FS_FOpenFile )( const char *filename, int *filenum, int mode );
	int ( *FS_Read )( void *buffer, size_t len, int file );
	int ( *FS_Write )( const void *buffer, size_t len, int file );
	int ( *FS_Print )( int file, const char *msg );
	int ( *FS_Tell )( int file );
	int ( *FS_Seek )( int file, int offset, int whence );
	int ( *FS_Eof )( int file );
	int ( *FS_Flush )( int file );
	void ( *FS_FCloseFile )( int file );
	qboolean ( *FS_RemoveFile )( const char *filename );
	int ( *FS_GetFileList )( const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end );
	const char *( *FS_FirstExtension )( const char *filename, const char *extensions[], int num_extensions );

	// map list
	qboolean ( *ML_Update )( void );
	size_t ( *ML_GetMapByNum )( int num, char *out, size_t size );
	qboolean ( *ML_FilenameExists )( const char *filename );

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void ( *Cmd_ExecuteText )( int exec_when, const char *text );
	void ( *Cbuf_Execute )( void );
	void ( *AddCommandString )( const char *text );

	// a fake client connection, ClientConnect is called afterwords
	// with fakeClient set to true
	int ( *FakeClientConnect )( char *fakeUserinfo, char *fakeIP );
	void ( *DropClient )( int clientNum, int type, char *message );
	int ( *GetClientState )( int numClient );
	void ( *ExecuteClientThinks )( int clientNum );

	struct angelwrap_api_s *( *asGetAngelExport )( void );
} game_import_t;

//
// functions exported by the game subsystem
//
typedef struct
{
	// if API is different, the dll cannot be used
	int ( *API )( void );

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void ( *Init )( unsigned int seed, unsigned int maxclients, unsigned int snapFrameTime, int protocol );
	void ( *Shutdown )( void );

	// each new level entered will cause a call to SpawnEntities
	void ( *InitLevel )( const char *mapname, const char *entstring, int entstrlen, unsigned int serverTime );

	qboolean ( *ClientConnect )( int clientNum, char *userinfo, qboolean fakeClient );
	void ( *ClientBegin )( int clientNum );
	void ( *ClientUserinfoChanged )( int clientNum, char *userinfo );
	void ( *ClientDisconnect )( int clientNum );
	void ( *ClientCommand )( int clientNum );
	void ( *ClientThink )( int clientNum, usercmd_t *cmd, int timeDelta );

	void ( *RunFrame )( unsigned int msec, unsigned int serverTime );
	int ( *SnapFrame )( unsigned int snapNum );
	void ( *ClearSnap )( void );

	void ( *BuildSnapEntitiesList )( int clientNum, snapshotEntityNumbers_t *entsList, qboolean cull );
	entity_state_t *( *GetEntityState )( int entNum );
	player_state_t *( *GetPlayerState )( int clientNum );
	game_state_t *( *GetGameState )( void );

	qboolean ( *AllowDownload )( int clientNum, const char *requestname, const char *uploadname );
} game_export_t;
