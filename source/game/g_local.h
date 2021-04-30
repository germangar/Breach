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
// g_local.h -- local definitions for game module
#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_comref.h"
#include "../gameshared/q_msg.h"
#include "../gameshared/q_collision.h"

#include "../gameshared/gs_public.h"
#include "g_public.h"
#include "g_syscalls.h"


typedef struct gclient_s gclient_t;
typedef struct gentity_s gentity_t;

#include "g_localsounds.h"
#include "g_spawnpoints.h"

//==================================================================

#define G_IsDead( ent )	      ( ent->health < 1.0f )

// view pitching times
#define DAMAGE_TIME	0.5
#define	FALL_TIME	0.3

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct
{
	gentity_t *entities;			// [maxentities]
	int numentities;

	gclient_t *clients;				// [maxclients]

	unsigned int snapNum;

	unsigned int framemsecs;		// in milliseconds
	unsigned int serverTime;		// actual time in the server
} game_locals_t;

#define GAMETYPE_PROJECT_EXTENSION ".gt"
#define GAMETYPE_SCRIPT_EXTENSION ".as"
#define GAMETYPE_CHAR_SEPARATOR ';'

typedef struct
{
	unsigned int spawnableItemsMask;

	// scripts
	int asEngineHandle;
	qboolean asEngineIsGeneric;
	int initFuncID;
	int spawnFuncID;
	int matchStateStartedFuncID;
	int matchStateFinishedFuncID;
	int thinkRulesFuncID;
	int playerRespawnFuncID;
	int scoreEventFuncID;
	int scoreboardMessageFuncID;
	int selectSpawnPointFuncID;
	int clientCommandFuncID;
	int botStatusFuncID;
} gametype_descriptor_t;

//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct
{
	unsigned int framenum;
	unsigned int time;              // time in milliseconds
	unsigned int mapTimeStamp;      // level.time when G_NewMap was called (map was restarted)
	unsigned int pausedTime;

	char mapname[MAX_QPATH];        // the server name
	char nextmap[MAX_QPATH];        // go here when match is finished
	char forcemap[MAX_QPATH];       // go here

	char *mapString;
	size_t mapStrlen;
	qboolean canSpawnEntities;				// security check. Don't init entities before map ones are ready

	int farplanedist;               // forced cull minimum distance
	int numLocalTargetNames;

	gametype_descriptor_t gametype;
} level_locals_t;


// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in gentity_t during gameplay
typedef struct
{
	// world vars
	float fov;
	char *nextmap;

	char *music;

	int lip;
	int distance;
	int height;
	float roll;
	float radius;
	float phase;
	float pausetime;
	char *item;
	char *gravity;

	float minyaw;
	float maxyaw;
	float minpitch;
	float maxpitch;

	char *team;

	char *shader;

	float anglehack;   // is only yaw, UP and DOWN
	float speed;
	float accel;
	float scale;

	int noents;
	int farplanedist;     // worldspawn only
	char *gametype;
} spawn_temp_t;


extern game_locals_t game;
extern level_locals_t level;
extern spawn_temp_t st;

extern int meansOfDeath;


#define	FOFS( x ) (size_t)&( ( (gentity_t *)0 )->x )
#define	STOFS( x ) (size_t)&( ( (spawn_temp_t *)0 )->x )
#define	LLOFS( x ) (size_t)&( ( (level_locals_t *)0 )->x )
#define	CLOFS( x ) (size_t)&( ( (gclient_t *)0 )->x )

extern cvar_t *password;
extern cvar_t *dedicated;
extern cvar_t *developer;

extern cvar_t *filterban;

extern cvar_t *g_minculldistance;

extern cvar_t *sv_cheats;

extern cvar_t *g_antilag_timenudge;
extern cvar_t *g_antilag_maxtimedelta;

extern cvar_t *g_teams_maxplayers;

void G_Teams_Join_Cmd( gentity_t *ent );
qboolean G_Teams_JoinTeam( gentity_t *ent, int team );
void G_Teams_SetTeam( gentity_t *ent, int team );

void Cmd_Say_f( gentity_t *ent, qboolean arg0, qboolean checkflood );
void G_Say_Team( gentity_t *who, char *msg, qboolean checkflood );

void G_Gametype_AdvanceMatchState( void );
void G_Gametype_Init( void );
qboolean G_Gametype_CanPickUpItem( gsitem_t *item );
qboolean G_Gametype_CanRespawnItem( gsitem_t *item );
qboolean G_Gametype_CanDropItem( gsitem_t *item );

void G_asShutdownGametypeScript( void );
qboolean G_asLoadGametypeScript( const char *gametypeName );
void G_asCallRunFrameScript( void );

//
// fields are needed for spawning from the entity string
//
#define FFL_SPAWNTEMP	    1

typedef enum
{
	F_INT,
	F_FLOAT,
	F_LSTRING,      // string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,      // string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_EDICT,        // index on disk, pointer in memory
	F_ITEM,         // index on disk, pointer in memory
	F_CLIENT,       // index on disk, pointer in memory
	F_FUNCTION,
	F_IGNORE
} fieldtype_t;

typedef struct
{
	const char *name;
	size_t ofs;
	fieldtype_t type;
	int flags;
} field_t;

extern const field_t fields[];
extern struct gentity_s *worldEntity;

//
// g_cmds.c
//
qboolean G_CheckFlood( gentity_t *ent );
void G_InitGameCommands( void );
void G_AddCommand( char *name, void *cmdfunc );
void G_ClientCommand( int clientNum );

//
// g_items.c
//
extern void G_PrecacheItem( int index );
extern void G_PrecacheItems( void );
extern void G_Item_EntityPickUp( gentity_t *ent, gentity_t *other );
extern gentity_t *G_Item_Drop( gentity_t *ent, gsitem_t *item, int count );
extern void G_Item_Spawn( gentity_t *ent, gsitem_t *item );
extern void G_Item_Use( gentity_t *ent, gsitem_t *item );
extern qboolean G_Item_AddToInventory( gentity_t *ent, gsitem_t *item, int count );

//
// g_utils.c
//
#define		G_LEVEL_DEFAULT_POOL_SIZE	128 * 1024

qboolean    KillBox( gentity_t *ent );
gentity_t *G_Find( gentity_t *from, size_t fieldofs, char *match );
gentity_t *findradius( gentity_t *from, vec3_t org, float rad );
gentity_t *G_FindBoxInRadius( gentity_t *from, vec3_t org, float rad );
gentity_t *G_PickTarget( char *targetname );
void	    G_ActivateTargets( gentity_t *ent, gentity_t *activator );
void	    G_SetMovedir( vec3_t angles, vec3_t movedir );
void	    G_DropSpawnpointToFloor( gentity_t *ent );

void	    G_InitEntity( gentity_t *e );
gentity_t *G_Spawn( void );
void	    G_FreeEntity( gentity_t *e );
extern void G_PredictedEvent( int entNum, int ev, int parm );
extern qboolean	G_IsTargetOfEnt( gentity_t *ent, gentity_t *check );
extern char *G_GenerateLocalTargetName( void );
extern void G_RotationFromAngles( vec3_t angles, vec3_t movedir );
extern void G_FireTouches( gentity_t *ent, touchlist_t *touchList );
extern void G_Door_SetAreaportalState( gentity_t *ent, qboolean open );
extern void G_InitMover( gentity_t *ent );


void	G_AddEvent( gentity_t *ent, int event, int parm, qboolean highPriority );
gentity_t *G_SpawnEvent( int event, int parm, vec3_t origin );
void	G_TurnEntityIntoEvent( gentity_t *ent, int event, int parm );

void	G_PrintMsg( gentity_t *ent, const char *format, ... );
void	G_ChatMsg( gentity_t *ent, const char *format, ... );
void	G_CenterPrintMsg( gentity_t *ent, const char *format, ... );
extern void G_Sound( gentity_t *ent, int channel, int soundindex, float attenuation );
extern void G_PositionedSound( vec3_t origin, int channel, int soundindex, float attenuation );
extern void G_AnnouncerSound( gentity_t *target, int soundindex, int team, qboolean queued, gentity_t *ignore );
extern void G_GlobalSound( int channel, int soundindex );
float vectoyaw( vec3_t vec );

//more warsow utils
char *G_ListNameForPosition( const char *namesList, int position, const char separator );
char *G_AllocCreateNamesList( const char *path, const char *extension, const char separator );
extern game_locals_t game;
#define ENTNUM( x ) ( ( ( x ) != NULL ) ? ( ( x ) - game.entities ) : -1 )
#define CLIENTNUM( x ) ( ( x ) - game.clients )
void	G_ReleasePlayerStateEvent( gclient_t *client );
void	G_AddPlayerStateEvent( gclient_t *client, int event, int parm );
void	G_ClearPlayerStateEvents( gclient_t *client );

extern int G_ConfigstringIndex( const char *newString, int initial, int max );
extern int G_PlayerObjectIndex( const char *name );
extern int G_WeaponObjectIndex( const char *name );

void	G_LevelInitPool( size_t size );
void	G_LevelFreePool( void );
void	*_G_LevelMalloc( size_t size, const char *filename, int fileline );
void	_G_LevelFree( void *data, const char *filename, int fileline );

//
// g_trigger.c
//
void G_Trigger_Spawn( gentity_t *ent );
void G_Trigger_WaterBlend_Spawn( gentity_t *ent );

//
// g_target.c
//
void SP_target_location( gentity_t *self );
void SP_target_position( gentity_t *self );

//
// g_clip.c
//

void	GClip_BackUpCollisionFrame( void );
int G_Clip_AreaForPoint( vec3_t origin );
int	GClip_PointCluster( vec3_t origin );
void	GClip_SetBrushModel( gentity_t *ent, char *name );
extern void GClip_SetEntityAreaInfo( gentity_t *ent );
void	GClip_LinkEntity( gentity_t *ent );
void	GClip_UnlinkEntity( gentity_t *ent );
void	GClip_TouchTriggers( gentity_t *ent );
entity_state_t *GClip_GetClipStateForDeltaTime( int entNum, int deltaTime );

//
// g_combat.c
//
void G_Damage_Init( void );
extern float G_KnockbackPushFrac( vec3_t pushdir, vec3_t pushorigin, vec3_t origin, vec3_t mins, vec3_t maxs, float pushradius );
void G_TakeDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, float damage, float knockback, int dflags, int mod );
void G_RadiusDamage( gentity_t *inflictor, gentity_t *attacker, vec3_t plane_normal, float radius, float maxdamage, float maxknockback, int ignore );
void G_TakeFallDamage( gentity_t *ent, int damage );

//
//	teams
//
void G_Teams_Init( void );


// damage flags
#define DAMAGE_RADIUS		0x00000001  // damage was indirect
#define DAMAGE_NO_PROTECTION	0x00000002  // unstopable damage

#define	GIB_HEALTH		-70


//
// g_misc.c
//
void SP_path_corner( gentity_t *self );
void SP_misc_model( gentity_t *ent );
void G_misc_portal_surface( gentity_t *ent );
void G_misc_portal_camera( gentity_t *ent );
void G_skyportal( gentity_t *ent );

//
// g_client.c
//
#include "g_client.h"

//
// g_svcmds.c
//
qboolean SV_FilterPacket( char *from );
void G_AddCommands( void );
void G_RemoveCommands( void );

//
// g_phys.c
//
void G_RunPhysicsTick( gentity_t *ent, vec3_t accel, unsigned int msecs );

//
// g_main.c
//

#define G_Malloc( size ) trap_MemAlloc( size, __FILE__, __LINE__ )
#define G_Free( data ) trap_MemFree( data, __FILE__, __LINE__ )

#define	G_LevelMalloc(size) _G_LevelMalloc((size),__FILE__,__LINE__)
#define	G_LevelFree(data) _G_LevelFree((data),__FILE__,__LINE__)

int		G_API( void );
void	G_Init( unsigned int seed, unsigned int maxclients, unsigned int framemsec, int protocol );
void	G_Shutdown( void );

qboolean    G_AllowDownload( int clientNum, const char *requestname, const char *uploadname );

//
// g_frame.c
//
void G_RunFrame( unsigned int msec, unsigned int serverTime );
void G_SnapClients( void );
void G_ClearSnap( void );
int G_SnapFrame( unsigned int snapNum );

// g_snapshot.c
extern void G_BuildSnapEntitiesList( int clientNum, snapshotEntityNumbers_t *entsList, qboolean cull );
extern entity_state_t *G_GetEntityState( int entNum );
extern player_state_t *G_GetPlayerState( int clientNum );
extern game_state_t *G_GetGameState( void );

// g_buildables.c
void G_Buildables_Init( void );

// g_playerclasses.c
void G_PlayerClass_RegisterClasses( void );

//
// g_spawn.c
//
qboolean G_CallSpawn( gentity_t *ent );
void G_InitLevel( const char *mapname, const char *entities, int len, unsigned int serverTime );

void G_info_player_start( gentity_t *ent );
void G_Spawnpoint_Spawn( gentity_t *ent );

//============================================================================

#define MOVER_FLAG_TOGGLE	1
#define MOVER_FLAG_REVERSE	2
#define MOVER_FLAG_CRUSHER	4
#define MOVER_FLAG_TAKEDAMAGE	8
#define MOVER_FLAG_DENY_TOUCH_ACTIVATION    16
#define MOVER_FLAG_ALLOW_SHOT_ACTIVATION    32

#define MOVER_STATE_ENDPOS	0
#define MOVER_STATE_STARTPOS	1
#define MOVER_STATE_GO_START	2
#define MOVER_STATE_GO_END	3

typedef struct
{
	// fixed data
	vec3_t start_origin;
	vec3_t start_angles;
	vec3_t end_origin;
	vec3_t end_angles;

	int sound;
	int event_endpos, event_startpos, event_startmoving, event_startreturning;
	vec3_t movedir;  // direction defined in the bsp

	float speed;

	float wait;

	float phase;        // pendulum only
	float accel;        // func_rotating only

	// state data
	int state;
	qboolean is_areaportal; // doors only
	qboolean rotating;

	void ( *endfunc )( gentity_t * );

	gentity_t *activator;

	vec3_t dest;
	vec3_t destangles;
} g_moverinfo_t;

//============================================================================

#define MAX_FLOOD_MESSAGES 32

#define MAX_CLIENT_EVENTS   16
#define MAX_CLIENT_EVENTS_MASK ( MAX_CLIENT_EVENTS - 1 )

#define G_MAX_TIME_DELTAS   8
#define G_MAX_TIME_DELTAS_MASK ( G_MAX_TIME_DELTAS - 1 )

typedef struct
{
	int buttons;
	int pmoveEvents;            // jumped, dashed, etc
	int waterShaderIndex;
} client_snap_t;

typedef struct
{
	unsigned int timeStamp;     // timemsec when it was respawned
	unsigned int jumppad_time;
	usercmd_t fakeClientUCmd;
	client_snap_t snap;         // cleared at opening a new snapshot and at spawning
} client_spawn_t;

typedef struct
{
	unsigned int timeStamp;
	unsigned int respawnCount;

	entity_state_t last_entity_state;

	int events[MAX_CLIENT_EVENTS];
	unsigned int eventsCurrent;
	unsigned int eventsHead;
} client_level_t;

struct gclient_s
{
	// known to server
	player_state_t ps;          // communicated by server to clients

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================

	// private to game
	client_spawn_t spawn;       // cleared each time the client is spawned
	client_level_t level;       // cleared when client enters the game. Which is each map load.

	unsigned int timeStamp;
	qboolean fakeClient;

	char netname[MAX_INFO_VALUE];
	int team;
	int playerClassIndex;


	int hand;
	int fov;
	int zoomfov;
	byte_vec4_t color;

	unsigned int ucmdTime; // last ucmd time fed to the gameshared thinking
	short cmd_angles[3];            // angles sent over in the last command
	int timeDelta;      // time offset to adjust for shots collision (antilag)
	int timeDeltas[G_MAX_TIME_DELTAS];
	int timeDeltasHead;

#ifdef UCMDTIMENUDGE
	int ucmdTimeNudge;
#endif
};

// gentity->netflags
#define	SVF_NOCLIENT	    0x00000001  // don't send entity to clients, even if it has effects
#define SVF_PORTAL	    0x00000002  // merge PVS at old_origin
#define SVF_BROADCAST	    0x00000004  // always transmit
#define SVF_CORPSE	    0x00000008  // treat as CONTENTS_CORPSE for collision
#define SVF_UNUSED01	    0x00000010  // sets ms.solid to SOLID_NOT for prediction
#define SVF_ONLYTEAM	    0x00000020  // this entity is only transmited to clients with the same ent->ms.team value

#define	MAX_ENT_CLUSTERS    16

// visibility information for snapshot culling
typedef struct
{
	int num_clusters;           // if -1, use headnode instead
	int clusternums[MAX_ENT_CLUSTERS];
	int headnode;               // unused if num_clusters != -1
	int areanum, areanum2;
} entity_vis_t;

typedef struct snap_edict_s
{
	// whether we have killed anyone this snap
	qboolean kill;
	qboolean teamkill;

	// ents can accumulate damage along the frame, so they spawn less events
	float damage_taken;
	float damage_given;
	float damage_teamgiven;

	float damage_fall;
} snap_edict_t;

struct gentity_s
{
	entity_state_t s;

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================

	snap_edict_t snap; // information that is cleared each frame snap
	entity_vis_t vis;   // visibility information for snapshot culling

	int spawnflags;
	int netflags;                   // SVF_NOCLIENT, etc

	gclient_t *client;

	moveenvironment_t env;

	const char *classname;

	char *target;
	char *targetname;
	char *killtarget;

	char *model;
	char *model2;

	char *message;

	unsigned int delay;             // delay before "use" makes effect
	float wait;

	int count;
	float health;
	int maxHealth;
	int damage;
	int kick;
	int splashRadius;

	//vec3_t color;
	int mass;

	unsigned int freetimestamp;         // time when the object was freed

	int numEvents;
	qboolean eventPriority[2];

	//
	// only used locally in game, not by server
	//
	unsigned int nextthink;

	void ( *think )( gentity_t *self );
	void ( *touch )( gentity_t *self, gentity_t *other, cplane_t *plane, int surfFlags );
	void ( *activate )( gentity_t *self, gentity_t *other, gentity_t *activator );
	void ( *pain )( gentity_t *self, gentity_t *other, float kick, int damage );
	void ( *blocked )( gentity_t *self, gentity_t *obstacle );
	void ( *die )( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage );
	void ( *closeSnap )( gentity_t *ent );
	void ( *clearSnap )( gentity_t *ent );

	vec3_t avelocity;

	int timeDelta;              // SVF_PROJECTILE only. Used for 4D collision detection
	unsigned int timestamp;
//	unsigned int deathtimestamp;
	unsigned int paintimestamp;	// last time a pain event was cast

	qboolean door_portal_state;			// fixme: only used for doors. Let's see what we can do with it.
	qboolean last_door_portal_state;

	gentity_t *owner;   // FIXME: used only for portal cameras

	g_moverinfo_t mover;

	// scripts
	int asRefCount, asFactored;
};

#include "g_projectiles.h"
