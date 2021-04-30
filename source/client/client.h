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
// client.h -- primary header for client

#include "../qcommon/qcommon.h"
#include "../ref_gl/r_public.h"
#include "../cgame/cg_public.h"
#include "snd_public.h"
#include "../gui/ui_ref.h"

#include "vid.h"
#include "input.h"
#include "keys.h"
#include "console.h"

//=============================================================================

#ifdef SMOOTHSERVERTIME
#define MAX_TIMEDELTAS_BACKUP 8
#define MASK_TIMEDELTAS_BACKUP ( MAX_TIMEDELTAS_BACKUP - 1 )
#endif

//
// the client_state_t structure is wiped completely at every
// server map change
//
typedef struct
{
	entity_state_t entityStateBackups[SNAPS_BACKUP_SIZE][MAX_EDICTS];
	player_state_t playerStateBackups[SNAPS_BACKUP_SIZE];
	game_state_t gameStateBackups[SNAPS_BACKUP_SIZE];
	entity_state_t baselines[MAX_EDICTS];
} cl_snapsdata_t;

typedef struct
{
	int frames;
	unsigned int start;
	int counts[100];
} cl_timedemo_t;

typedef struct
{
	int timeoutcount;

	cl_timedemo_t timedemo;

	int cmdNum;                 // current cmd
	usercmd_t cmds[CMD_BACKUP]; // each mesage will send several old cmds
	int cmd_time[CMD_BACKUP];       // time sent, for calculating pings

	qboolean inputRefreshed;	// do not refresh input twice the same frame

	int suppressCount;          // number of messages rate suppressed

	unsigned int previousSnapNum;
	unsigned int currentSnapNum;
	unsigned int pendingSnapNum;
	unsigned int receivedSnapNum;
	snapshot_t snapShots[SNAPS_BACKUP_SIZE];
	cl_snapsdata_t snapsData;

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t viewangles;

#ifdef SMOOTHSERVERTIME
	int serverTimeDeltas[MAX_TIMEDELTAS_BACKUP];
#endif
	int newServerTimeDelta;         // the time difference with the server time, or at least our best guess about it
	int serverTimeDelta;         // the time difference with the server time, or at least our best guess about it
	unsigned int serverTime;    // the best match we can guess about current time in the server
	unsigned int snapFrameTime;
	unsigned int extrapolationTime;

	//
	// non-gameserver information
	void *cin;

	//
	// server state information
	//
	int servercount;        // server identification for prespawns
	int playernum;

	cmodel_state_t *cms;

	char servermessage[MAX_QPATH];
	char configstrings[MAX_CONFIGSTRINGS][MAX_CONFIGSTRING_CHARS];

} client_state_t;

extern client_state_t cl;

/*
   ==================================================================

   the client_static_t structure is persistant through an arbitrary number
   of server connections

   ==================================================================
 */

typedef struct download_list_s download_list_t;

struct download_list_s
{
	char *filename;
	download_list_t	*next;
};

typedef struct
{
	// for request
	char *requestname;              // file we requested from the server (NULL if none requested)
	qboolean requestnext;           // whether to request next download after this, for precaching
	qboolean requestpak;            // whether to only allow .pk3 or only allow normal file
	unsigned int timeout;
	unsigned int timestart;

	// both downloads
	char *name;                     // name of the file in download
	char *tempname;                 // temporary location
	size_t size;
	unsigned checksum;

	double percent;
	int successCount;               // so we know to restart media
	download_list_t	*list;          // list of all tried downloads, so we don't request same pk3 twice

	// server download
	int filenum;
	size_t offset;
	int retries;
	size_t baseoffset;					// for download speed calculation when resuming downloads

	// web download
	qboolean web;
	qboolean disconnect;            // set when user tries to disconnect, to allow cleaning up webdownload
} download_t;

typedef struct
{
	qboolean recording;
	qboolean waiting;		// don't record until a non-delta message is received
	qboolean playing;
	//qboolean paused;		// A boolean to test if demo is paused

	int file;
	char *filename;

	unsigned int duration, basetime;

	qboolean play_jump;
	qboolean play_ignore_next_frametime;

	qboolean avi;
	qboolean avi_video, avi_audio;
	qboolean pending_avi;
	int avi_frame;
} cl_demo_t;

typedef struct
{
	connstate_t state;          // only set through CL_SetClientState
	keydest_t key_dest;
	keydest_t old_key_dest;

	int framecount;
	unsigned int realtime;          // always increasing, no clamping, etc
	unsigned int gametime;          // always increasing, no clamping, etc
	float trueframetime;
	float frametime;                // seconds since last frame

	// screen rendering information
	qboolean cgameActive;
	qboolean mediaInitialized;

	unsigned int disable_screen;    // showing loading plaque between levels
	                                // or changing rendering dlls
	                                // if time gets > 30 seconds ahead, break it
	int disable_servercount;        // when we receive a frame and cl.servercount
	                                // > cls.disable_servercount, clear disable_screen

	// connection information
	char *servername;               // name of server from original connect
	netadr_t serveraddress;         // address of that server
	int connect_time;               // for connection retransmits
	int connect_count;

	netadr_t rconaddress;       // address where we are sending rcon messages, to ignore other print packets

	qboolean rejected;          // these are used when the server rejects our connection
	int rejecttype;
	char rejectmessage[80];

	int quakePort;              // a 16 bit value that allows quake servers
	                            // to work around address translating routers
	netchan_t netchan;

	char *statusbar;

	int challenge;              // from the server to use for connecting

	download_t download;

	cl_demo_t demo; // demo recording info must be here, so it isn't cleared on level change

	// these shaders have nothing to do with media
	struct shader_s *whiteShader;
	struct shader_s *consoleShader;

	// system fonts
	struct mufont_s *fontSystemSmall;
	struct mufont_s *fontSystemMedium;
	struct mufont_s *fontSystemBig;

	// these are our reliable messages that go to the server
	unsigned int reliableSequence;          // the last one we put in the list to be sent
	unsigned int reliableSent;              // the last one we sent to the server
	unsigned int reliableAcknowledge;       // the last one the server has executed
	char reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// reliable messages received from server
	unsigned int lastExecutedServerCommand; // last server command grabbed or executed with CL_GetServerCommand

	// ucmds buffer
	unsigned int ucmdAcknowledged;
	unsigned int ucmdHead;
	unsigned int ucmdSent;

	// times when we got/sent last valid packets from/to server
	unsigned int lastPacketSentTime;
	unsigned int lastPacketReceivedTime;

	int sv_maxclients; // max clients value in current server

	// pure list
	qboolean sv_pure;
	purelist_t *purelist;
} client_static_t;

extern client_static_t cls;

//=============================================================================

//
// cvars
//
extern cvar_t *cl_stereo_separation;
extern cvar_t *cl_stereo;

extern cvar_t *cl_yawspeed;
extern cvar_t *cl_pitchspeed;

extern cvar_t *cl_anglespeedkey;

extern cvar_t *cl_extrapolate;
extern cvar_t *cl_compresspackets;
extern cvar_t *cl_shownet;

extern cvar_t *m_pitch;
extern cvar_t *m_yaw;

extern cvar_t *cl_timedemo;
extern cvar_t *cl_demoavi_video;
extern cvar_t *cl_demoavi_audio;
extern cvar_t *cl_demoavi_fps;
extern cvar_t *cl_demoavi_scissor;

extern cvar_t *cl_debug_serverCmd;
extern cvar_t *cl_debug_timeDelta;

extern cvar_t *cl_downloads;
extern cvar_t *cl_downloads_from_web;
extern cvar_t *cl_downloads_from_web_timeout;
extern cvar_t *cl_download_allow_modules;

//=============================================================================


//
// cl_cin.c
//
void SCR_InitCinematic( void );
unsigned int SCR_GetCinematicTime( void );
qboolean SCR_DrawCinematic( void );
void SCR_RunCinematic( void );
void SCR_StopCinematic( void );
void SCR_FinishCinematic( void );
void CL_PlayCinematic_f( void );

//
// cl_main.c
//
void CL_Init( void );
void CL_Quit( void );

void CL_UpdateClientCommandsToServer( msg_t *msg );
void CL_AddReliableCommand( /*const*/ char *cmd );
void CL_Netchan_Transmit( msg_t *msg );
void CL_SendMessagesToServer( qboolean sendNow );

void CL_AdjustServerTime( unsigned int gamemsec );

qboolean CL_DownloadRequest( const char *filename, qboolean requestpak );
void CL_DownloadStatus_f( void );
void CL_DownloadCancel_f( void );
void CL_DownloadDone( void );
void CL_RequestNextDownload( void );
void CL_StopServerDownload( void );
void CL_CheckDownloadTimeout( void );

char *CL_GetClipboardData( qboolean primary );
qboolean CL_SetClipboardData( char *data );
void CL_FreeClipboardData( char *data );
void CL_SetKeyDest( int key_dest );
void CL_SetOldKeyDest( int key_dest );
void CL_ResetServerCount( void );
void CL_SetClientState( int state );
void CL_ClearState( void );
void CL_ReadPackets( void );
void CL_Disconnect_f( void );
void CL_S_Restart( qboolean noVideo );

void CL_Reconnect_f( void );
void CL_ServerReconnect_f( void );
void CL_Changing_f( void );
void CL_Precache_f( void );
void CL_ForwardToServer_f( void );
void CL_ServerDisconnect_f( void );

//
// cl_ents.c
//
void CL_ParseFrame( msg_t *msg );


//
// cl_game.c
//
qboolean CL_GameModule_Initialized( void );
void CL_GameModule_Init( void );
void CL_GameModule_Shutdown( void );
void CL_GameModule_ServerCommand( void );
void CL_GameModule_ConfigStringUpdate( int number );
void CL_GameModule_EscapeKey( void );
float CL_GameModule_SetSensitivityScale( const float sens );
void CL_GameModule_RenderView( float stereo_separation );
void CL_GameModule_GetEntitySpatilization( int entnum, vec3_t origin, vec3_t velocity );
qboolean CL_GameModule_NewSnapshot( unsigned int snapNum, unsigned int serverTime );

//
// cl_sound.c
//
void CL_SoundModule_Init( qboolean verbose );
void CL_SoundModule_Shutdown( qboolean verbose );
void CL_SoundModule_SoundsInMemory( void );
void CL_SoundModule_FreeSounds( void );
void CL_SoundModule_StopAllSounds( void );
void CL_SoundModule_Clear( void );
void CL_SoundModule_Update( const vec3_t origin, const vec3_t velocity, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up, qboolean avidump );
void CL_SoundModule_Activate( qboolean activate );
struct sfx_s *CL_SoundModule_RegisterSound( const char *sample );
void CL_SoundModule_FreeSound( struct sfx_s *sfx );
void CL_SoundModule_StartFixedSound( struct sfx_s *sfx, const vec3_t origin, int channel, float fvol, float attenuation );
void CL_SoundModule_StartRelativeSound( struct sfx_s *sfx, int entnum, int channel, float fvol, float attenuation );
void CL_SoundModule_StartGlobalSound( struct sfx_s *sfx, int channel, float fvol );
void CL_SoundModule_StartLocalSound( const char *s );
void CL_SoundModule_AddLoopSound( struct sfx_s *sfx, int entnum, float fvol, float attenuation );
void CL_SoundModule_RawSamples( int samples, int rate, int width, int channels, const qbyte *data, qboolean music );
void CL_SoundModule_StartBackgroundTrack( const char *intro, const char *loop );
void CL_SoundModule_StopBackgroundTrack( void );
void CL_SoundModule_BeginAviDemo( void );
void CL_SoundModule_StopAviDemo( void );

//
// cl_ui.c
//
void CL_UIModule_Init( void );
void CL_UIModule_Shutdown( void );
void CL_UIModule_KeyEvent( int key, qboolean down );
void CL_UIModule_CharEvent( int key );
void CL_UIModule_Refresh( qboolean backGround );
void CL_UIModule_DrawConnectScreen( void );
void CL_UIModule_DeactivateUI( void );
void CL_UIModule_AddToServerList( char *adr, char *info );
void CL_UIModule_MouseMove( int dx, int dy );

void CL_UI_InitWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic );
void CL_UI_OpenWindow( const char *name );
void CL_UI_CloseWindow( const char *name );
void CL_UI_ApplyWindowChanges( const char *name );
struct menuitem_public_s *CL_UI_InitItemWindowDragger( const char *windowname, const char *itemname, int x, int y, int align );
struct menuitem_public_s *CL_UI_InitItemStatic( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font );
struct menuitem_public_s *CL_UI_InitItemButton( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString );
struct menuitem_public_s *CL_UI_InitItemCheckBox( const char *windowname, const char *itemname, int x, int y, int align, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString );
struct menuitem_public_s *CL_UI_InitItemSpinner( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *spinnerNames, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString );
struct menuitem_public_s *CL_UI_InitItemSlider( const char *windowname, const char *itemname, int x, int y, int align, int min, int max, float stepsize, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString );
struct menuitem_public_s *CL_UI_InitItemListbox( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *namesList, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, void *Action, char *targetString );

//
// cl_serverlist.c
//
void CL_ParseGetInfoResponse( msg_t *msg );
void CL_ParseGetStatusResponse( msg_t *msg );
void CL_QueryGetInfoMessage_f( void );
void CL_QueryGetStatusMessage_f( void );
void CL_ParseStatusMessage( msg_t *msg );
void CL_ParseGetServersResponse( msg_t *msg );
void CL_GetServers_f( void );
void CL_PingServer_f( void );

//
// cl_input.c
//
typedef struct
{
	int down[2];            // key nums holding it down
	unsigned downtime;      // msec timestamp
	unsigned msec;          // msec down this frame
	int state;
} kbutton_t;

extern kbutton_t in_klook;
extern kbutton_t in_strafe;
extern kbutton_t in_speed;

void CL_InitInput( void );
void CL_ShutdownInput( void );
void CL_UserInputFrame( void );
void CL_NewUserCommand( int msec );
void CL_WriteUcmdsToMessage( msg_t *msg );
void CL_MouseMove( usercmd_t *cmd, int mx, int my );
void CL_UpdateCommandInput( void );
void IN_CenterView( void );



//
// cl_demo.c
//
void CL_WriteDemoMessage( msg_t *msg );
void CL_DemoCompleted( void );
void CL_PlayDemo_f( void );
void CL_PlayDemoToAvi_f( void );
void CL_ReadDemoPackets( void );
void CL_Stop_f( void );
void CL_Record_f( void );
void CL_DemoJump_f( void );
void CL_BeginDemoAviDump( void );
#define CL_WriteAvi() ( cls.demo.avi && cls.state == CA_ACTIVE && cls.demo.playing && !cls.demo.play_jump )

//
// cl_parse.c
//
extern char *svc_strings[256];
void CL_ParseServerMessage( msg_t *msg );
void SHOWNET( msg_t *msg, char *s );

void CL_FreeDownloadList( void );
qboolean CL_CheckOrDownloadFile( const char *filename );

//
// cl_screen.c
//
#define SMALL_CHAR_WIDTH    8
#define SMALL_CHAR_HEIGHT   16

#define BIG_CHAR_WIDTH	    16
#define BIG_CHAR_HEIGHT	    16

#define GIANT_CHAR_WIDTH    32
#define GIANT_CHAR_HEIGHT   48

void SCR_InitScreen( void );
void SCR_UpdateScreen( void );
void SCR_BeginLoadingPlaque( void );
void SCR_EndLoadingPlaque( void );
void SCR_DebugGraph( float value, float r, float g, float b );
void SCR_RunConsole( int msec );
void SCR_RegisterConsoleMedia( void );
void SCR_ShutDownConsoleMedia( void );
struct mufont_s *SCR_RegisterFont( const char *name );
size_t SCR_strHeight( struct mufont_s *font );
size_t SCR_strWidth( const char *str, struct mufont_s *font, int maxlen );
size_t SCR_StrlenForWidth( const char *str, struct mufont_s *font, size_t maxwidth );
void SCR_DrawString( int x, int y, int align, const char *str, struct mufont_s *font, vec4_t color );
int SCR_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color );
void SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color );
void SCR_DrawRawChar( int x, int y, int num, struct mufont_s *font, vec4_t color );
void SCR_DrawFillRect( int x, int y, int w, int h, vec4_t color );

void CL_InitMedia( void );
void CL_ShutdownMedia( void );
void CL_RestartMedia( void );

void CL_AddNetgraph( void );

extern float scr_con_current;
extern float scr_conlines;       // lines of console to display

//
// cl_vid.c
//
void VID_NewWindow( int width, int height );
qboolean VID_GetModeInfo( int *width, int *height, qboolean *wideScreen, int mode );
