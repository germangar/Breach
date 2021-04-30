
#ifndef __GS_PUBLIC_H__
#define __GS_PUBLIC_H__

//#ifdef __cplusplus
//extern "C" {
//#endif

#ifndef __cplusplus
struct cmodel_s *shut_up_cmodels_warning; // jalfixme
#endif

#define ENTITY_WORLD	( MAX_EDICTS - 1 )
#define ENTITY_INVALID	( -1 )

#define DEFAULT_PLAYEROBJECT	"#padpork"

//==================================================
// GAME LOCAL CONFISTRINGS
// They can never be superior to MAX_CONFIGSTRINGS,
// and they are not part of auto-download process
//==================================================
#define MAX_GAMECOMMANDS    64
#define MAX_PLAYEROBJECTS   64
#define MAX_WEAPONOBJECTS   MAX_WEAPONS
#define MAX_BUILDABLES		16

#define CS_GAMECOMMANDS			( CS_GAMESTRINGS )
#define CS_PLAYEROBJECTS		( CS_GAMECOMMANDS+MAX_GAMECOMMANDS )
#define CS_WEAPONOBJECTS		( CS_PLAYEROBJECTS+MAX_PLAYEROBJECTS )
#define	CS_PLAYERINFOS			( CS_WEAPONOBJECTS+MAX_WEAPONOBJECTS )
#define	CS_ITEMS				( CS_PLAYERINFOS+MAX_CLIENTS )
#define CS_BUILDABLES			( CS_ITEMS+MAX_ITEMS )
#define CS_PLAYERCLASSES		( CS_BUILDABLES+MAX_BUILDABLES ) // MAX_PLAYERCLASSES is limited by the netcode
#define CS_PLAYERCLASSESDATA   ( CS_PLAYERCLASSES+MAX_PLAYERCLASSES )


//==================================================
// module callbacks
//==================================================

typedef enum
{
	GS_MODULE_GAME = 1
	, GS_MODULE_CGAME
	, GS_MODULE_TOTAL
}moduletype_t;

typedef struct 
{
	moduletype_t type;

	void *( *Malloc )( size_t size, const char *filename, const int fileline );
	void ( *Free )( void *data, const char *filename, const int fileline );
	void ( *trap_Print )( const char *msg );
	void ( *trap_Error )( const char *msg );

	// file system
	int ( *trap_FS_FOpenFile )( const char *filename, int *filenum, int mode );
	int ( *trap_FS_Read )( void *buffer, size_t len, int file );
	void ( *trap_FS_FCloseFile )( int file );

	// collision traps
	int ( *trap_CM_NumInlineModels )( void );
	int ( *trap_CM_TransformedPointContents )( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles );
	struct cmodel_s *( *trap_CM_InlineModel )( int num );
	struct cmodel_s *( *trap_CM_ModelForBBox )( vec3_t mins, vec3_t maxs );
	void ( *trap_CM_InlineModelBounds )( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs );
	void ( *trap_CM_TransformedBoxTrace )( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles );
	int ( *trap_CM_BoxLeafnums )( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode );
	int ( *trap_CM_LeafCluster )( int leafnum );

	entity_state_t *( *GetClipStateForDeltaTime )( int entNum, int deltaTime );

	void ( *predictedEvent )( int entNum, int ev, int parm );
	void ( *DrawBox )( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t angles, vec4_t color );
} gs_moduleapi_t;

// world environment
typedef struct
{
	float gravity;
	vec3_t gravityDir;
	vec3_t inverseGravityDir;
} gameshared_environment_t;

typedef struct
{
	int maxclients;
	int numBuildables;
	int numPlayerClasses;
	unsigned int snapFrameTime; 
	int protocol;

	qboolean timeout;
	game_state_t gameState;

	gameshared_environment_t environment;
} gameshared_locals_t;

extern gameshared_locals_t gs;

//==================================================
// gs_main.c
//==================================================

extern char *_GS_CopyString( const char *in, const char *filename, const int fileline );
#define GS_CopyString(in) _GS_CopyString(in,__FILE__,__LINE__)
extern void GS_Printf( const char *format, ... );
extern void GS_Error( const char *format, ... );
extern void GS_Init( gs_moduleapi_t *moduleapi, unsigned int maxclients, unsigned int snapFrameTime, int protocol );

//==================================================
//
//==================================================

#include "gs_trace.h"
#include "gs_scripts.h"
#include "gs_utils.h"

//==================================================
//	ENTITY STATE
//==================================================

// entity_state_t->solid

enum
{
	SOLID_NOT = 0,
	SOLID_SOLID,
	SOLID_PLAYER,
	SOLID_CORPSE,
	SOLID_PROJECTILE,
	SOLID_TRIGGER,
	MAX_SOLID_TYPES = 8 // netcode uses 3 bits for solid type.
};

// entity_state_t->cmodeltype

enum
{
	CMODEL_NOT = 0,
	CMODEL_BRUSH,
	CMODEL_BBOX,
	CMODEL_BBOX_ROTATED,
	MAX_CMODEL_TYPES = 8 // netcode uses 3 bits for collision model type.
};

// ET_PLAYER only
#define EF_PLAYER_CROUCHED  1
#define EF_PLAYER_PRONED  2
#define EF_BUSYICON	    32

// ET_PORTAL only
#define EF_NOPORTALENTS EF_PLAYER_CROUCHED

// entity_state_t->type
enum
{
	ET_NODRAW,
	ET_TRIGGER,

	ET_MODEL,
	ET_ITEM,
	ET_PLAYER,

	ET_DECAL,
	ET_PORTALSURFACE,
	ET_SKYPORTAL,

	// eventual entities: types below this will get event treatment
	ET_EVENT = EVENT_ENTITIES_START,
	ET_SOUNDEVENT,

	ET_TOTAL_TYPES, // current count
	MAX_ENTITY_TYPES = 128
};

#define GS_IsGhostState( x ) ( ( ( x )->type == ET_NODRAW ) && ( ( x )->solid == SOLID_NOT ) )

//	entity_state_t->event

#define PREDICTABLE_EVENTS_MAX 32
typedef enum
{
	EV_NONE,

	// predictable events
	EV_WEAPONACTIVATE,
	EV_WEAPON_MODEUP,
	EV_WEAPON_MODEDOWN,
	EV_RELOADING,
	EV_NOAMMOCLICK,
	EV_FIREWEAPON,
	EV_SMOOTHREFIREWEAPON,
	EV_FIRE_BULLET,
	EV_ACTIVATE,

	EV_JUMP,
	EV_WATERSPLASH,
	EV_WATERENTER,
	EV_WATEREXIT,
	EV_FALLIMPACT,

	// non predictable events
	EV_WEAPONDROP = PREDICTABLE_EVENTS_MAX,

	EV_PLAT_STOP,
	EV_PLAT_START,

	EV_DOOR_HIT_TOP,
	EV_DOOR_HIT_BOTTOM,
	EV_DOOR_START_MOVING,

	EV_BUTTON_START,

	EV_TRAIN_STOP,
	EV_TRAIN_START,

	EV_PAIN_SOFT,
	EV_PAIN_MEDIUM,
	EV_PAIN_STRONG,

	EV_PICKUP_WEAPON,
	EV_PICKUP_AMMO,
	EV_PICKUP_HEALTH,

	EV_TESTIMPACT,
	EV_EXPLOSION_ONE,

	MAX_EVENTS = 128
} entity_event_t;

//==================================================
// SOUND ATTENUATION TYPES
//==================================================

// // S_DEFAULT_ATTENUATION_MODEL 1
#define	ATTN_GLOBAL			0		// full volume the entire level
#define	ATTN_DISTANT		0.5f		// distant sound (most likely explosions)
#define	ATTN_NORMAL			1.0f		// players, weapons, etc
#define	ATTN_STATIC			4		// diminish very rapidly with distance
#define	ATTN_FOOTSTEPS		10		// must be very close to hear it

#if 0
// S_DEFAULT_ATTENUATION_MODEL 3
#define	ATTN_GLOBAL				0	// full volume the entire level
#define	ATTN_DISTANT			2	// distant sound (most likely explosions)
#define	ATTN_NORM				5	// players, weapons, etc
#define	ATTN_ROOM				8	// stuff around you
#define	ATTN_STATIC				10	// diminish very rapidly with distance
#define	ATTN_FOOTSTEPS			21	// must be very close to hear it
#endif

//==================================================
// PHYSICS
//==================================================

// player movement features
#define PMOVEFEAT_CROUCH 1
#define PMOVEFEAT_JUMP 2
#define PMOVEFEAT_SPRINT 4
#define PMOVEFEAT_PRONE 8

#define MAX_SLIDEMOVE_CLIP_PLANES 16
/*
typedef struct movespecificts_s
{
	const float frictionGround;
	const float frictionAir;
	const float frictionWater;
	const float bounceFrac;
} movespecificts_t;
*/

typedef struct
{
	float frictionGround;
	float frictionAir;
	float frictionWater;
	float bounceFrac;
}movespecificts_t;

extern movespecificts_t defaultPlayerMoveSpec;

typedef struct
{
	// local stuff
	vec3_t mins, maxs;
	float frameTime;
	float remainingTime;

	// for slide clipping
	int numClipPlanes;
	vec3_t clipPlaneNormals[MAX_SLIDEMOVE_CLIP_PLANES];
} movelocal_t;

// output from move calls
typedef struct
{
	touchlist_t touchList;
	float step;
} moveoutput_t;

typedef struct
{
	int groundentity;
	int groundsurfFlags;
	cplane_t groundplane;
	int groundcontents;
	int waterlevel;
	int watertype;
	float frictionFrac;
} moveenvironment_t;

typedef struct
{
	move_state_t *ms;       // pointer to the state to move. It will return modified

	int entNum;             // entity we are moving. Ignore for traces, entity casting events
	int contentmask;        // clip against what contents

	const movespecificts_t *specifics;
	float bounceFrac;
	vec3_t accel;

	// cleared at execution
	movelocal_t local;
	moveenvironment_t env;
	moveoutput_t output;
} move_t;

// -1.0 pure flat ground #### 0.0 pure flat wall ### inclination floors
#define IsGroundPlane( normal, gravityDir ) ( DotProduct( normal, gravityDir ) < -0.75f )
#define STEPSIZE 16.5f

#define WATERLEVEL_FLOAT    2
#define WATERLEVEL_FEETS    1

typedef enum
{
	MOVE_TYPE_NONE, // no acceleration or turning
	MOVE_TYPE_STEP,
	MOVE_TYPE_OBJECT,
	MOVE_TYPE_FREEFLY,
	MOVE_TYPE_LINEAR,
	MOVE_TYPE_LINEAR_PROJECTILE,
	MOVE_TYPE_PUSHER,

	MOVE_TYPE_TOTAL
} gs_movetype_t;

extern void GS_Move( move_t *move, unsigned int msecs, vec3_t mins, vec3_t maxs );
extern touchlist_t *GS_Move_LinearProjectile( entity_state_t *state, unsigned int curtime, vec3_t neworigin, int passent, int clipmask, int timeDelta );
extern void GS_Move_EnvironmentForBox( moveenvironment_t *env, vec3_t origin, vec3_t velocity, vec3_t mins, vec3_t maxs, int passent, int contentmask, const movespecificts_t *customSpecifics );

// FIXME: This is bullshit. Make each entity have a clip type or smthing
#define GS_ContentMaskForState( x ) ( (x)->type == ET_PLAYER ? ( (x)->solid == SOLID_NOT ? 0 : MASK_PLAYERSOLID ) : ( (x)->solid == SOLID_NOT ? 0 : MASK_SOLID ) )

// gs_client.c
#define PlayerViewHeightFromBox( mins, maxs ) ( maxs[2] - 12 < mins[2] + 8 ? mins[2] + 8 : maxs[2] - 12 )
extern void GS_Client_ApplyUserInput( vec3_t newaccel, entity_state_t *state, player_state_t *playerState, usercmd_t *usercmd, float fov, float zoomfov, int contentMask, int timeDelta );

//==================================================
//	PLAYER OBJECTS - gs_player - shared player definitions
//==================================================

#include "gs_player.h"

//==================================================
//	ITEM OBJECTS - gs_items - shared items definitions
//==================================================

#include "gs_items.h"

//==================================================
//	WEAPON OBJECTS - gs_weapons - shared weapons definitions
//==================================================

#define SPREAD_CALCULATION_RANGE	8192

enum
{
	WEAPON,
	BARREL,
	FLASH,
	HAND,

	WEAPMODEL_PARTS
};

enum
{
	WEAPON_STATE_READY,
	WEAPON_STATE_ACTIVATING,
	WEAPON_STATE_DROPPING,
	WEAPON_STATE_POWERING,
	WEAPON_STATE_FIRING,
	WEAPON_STATE_RELOADING,
	WEAPON_STATE_NOAMMOCLICK,
	WEAPON_STATE_CHANGING_MODE,
	WEAPON_STATE_REFIRE
};

typedef struct
{
	const char *name;
	item_tag_t ammoItem;
	int numModes;
	int smoothRefire;
	int dropTime;
	int activationTime;
	int refireTime;
	int clipTime;
	int noAmmoClickTime;
	
} gs_weapon_definition_t;

extern gs_weapon_definition_t *GS_GetWeaponDef( int weapon );
extern int GS_SelectBestWeapon( player_state_t *playerState );

//==================================================
//	MATCH_STATES - gs_gametypes - shared match state definitions
//==================================================

enum
{
	MATCH_STATE_NONE = 0,
	MATCH_STATE_WARMUP,
	MATCH_STATE_COUNTDOWN,
	MATCH_STATE_PLAYTIME,
	MATCH_STATE_POSTMATCH,
	MATCH_STATE_WAITEXIT
};

//==================================================
//	TEAMS - gs_gameteams - shared team definitions
//==================================================

enum
{
	TEAM_NOTEAM,
	TEAM_ALPHA,
	TEAM_BETA,
	TEAM_SPECTATOR,
	GS_NUMTEAMS
};

// teams
extern const char *GS_TeamName( int team );
extern int GS_TeamColor( int team );
extern int GS_Teams_TeamFromName( const char *teamname );
extern qboolean GS_IsTeamDamage( entity_state_t *targ, entity_state_t *attacker );

//==================================================
//	GAMESTATE STATS
//==================================================

// longs
enum 
{
	GAMELONG_MATCHSTART,
	GAMELONG_MATCHDURATION
};

// shorts
enum 
{
	GAMESTAT_MATCHSTATE,
	GAMESTAT_FLAGS
};

// GAMESTAT_FLAGS bits

#define GAMESTAT_FLAG_PAUSED ( 1<<0 )
#define GAMESTAT_FLAG_COUNTDOWN ( 1<<1 )
#define GAMESTAT_FLAG_TEAMBASED ( 1<<2 )

#define GS_GamestatSetFlag( flag, b ) ( b ? ( gs.gameState.stats[GAMESTAT_FLAGS] |= flag ) : ( gs.gameState.stats[GAMESTAT_FLAGS] &= ~flag ) )

#define GS_Paused() ( ( gs.gameState.stats[GAMESTAT_FLAGS] & GAMESTAT_FLAG_PAUSED ) ? qtrue : qfalse )
#define GS_Countdown() ( ( gs.gameState.stats[GAMESTAT_FLAGS] & GAMESTAT_FLAG_COUNTDOWN ) ? qtrue : qfalse )
#define GS_TeamBased() ( ( gs.gameState.stats[GAMESTAT_FLAGS] & GAMESTAT_FLAG_TEAMBASED ) ? qtrue : qfalse )
#define GS_MatchState() ( gs.gameState.stats[GAMESTAT_MATCHSTATE] )

#define GS_MatchDuration() ( gs.gameState.longstats[GAMELONG_MATCHDURATION] )
#define GS_MatchStartTime() ( gs.gameState.longstats[GAMELONG_MATCHSTART] )
#define GS_MatchEndTime() ( gs.gameState.longstats[GAMELONG_MATCHDURATION] ? gs.gameState.longstats[GAMELONG_MATCHSTART] + gs.gameState.longstats[GAMELONG_MATCHDURATION] : 0 )

//==================================================
//	PLAYERSTATE STATS
//==================================================

// STAT_FLAGS bits meanings
#define	STAT_FLAGS_NOINPUT		0x0001

#define	STAT_FLAGS_PRONED	    0x0004

enum
{
	STAT_FLAGS,
	STAT_HEALTH,
	STAT_PENDING_WEAPON,
	STAT_WEAPON_MODE,
	STAT_WATER_BLEND,
	STAT_NEXT_RESPAWN,

	MAX_STATS = PS_MAX_STATS
};

enum
{
	USERINPUT_STAT_NOUSERCONTROL,
	USERINPUT_STAT_WEAPONTIME,
	USERINPUT_STAT_DUCKTIME,
	USERINPUT_STAT_ZOOMTIME,

	USERINPUT_STAT_TOTAL = MAX_USERINPUT_STATS
};

// view types
enum
{
	VIEWDEF_FREE,
	VIEWDEF_PLAYERVIEW,

	VIEWDEF_MAXTYPES
};

// player_state_t->event
typedef enum
{
	PSEV_NONE = 0,
	PSEV_HIT,
	PSEV_PICKUP,
	PSEV_DAMAGED,
	PSEV_INDEXEDSOUND,
	PSEV_ANNOUNCER,
	PSEV_ANNOUNCER_QUEUED,

	PSEV_MAX_EVENTS = 0xFF
} playerstate_event_t;

//==================================================
//	MISC
//==================================================

// means of death
typedef enum
{

	MOD_UNKNOWN = 0,

	// World damage
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH, // moving item blocked by player
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,

} mod_damage_t;

// move me
extern const char *GS_WeaponObjects_BasePath( void );
extern const char *GS_WeaponObjects_Extension( void );
extern const char *GS_PlayerObjects_BasePath( void );
extern const char *GS_PlayerObjects_Extension( void );
extern const char *GS_ItemObjects_BasePath( void );
extern const char *GS_ItemObjects_Extension( void );

// BUILDABLES : fixme: move me
typedef struct gsbuildableobject_s
{
	char *name;
	int modelIndex;
	int cmodelType;
	vec3_t mins, maxs;

	// locally derived information
	int index;
	struct gsbuildableobject_s *next;
}
gsbuildableobject_t;

extern gsbuildableobject_t *GS_BuildableByIndex( int index );
extern gsbuildableobject_t *GS_BuildableByName( const char *name );
extern gsbuildableobject_t *GS_Buildables_Register( int index, const char *dataString );
extern void GS_Buildables_Free( void );

//#ifdef __cplusplus
//};
//#endif

#endif // __GS_PUBLIC_H__
