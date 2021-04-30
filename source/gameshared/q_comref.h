
#ifndef __Q_COMREF_H__
#define __Q_COMREF_H__

#include "q_arch.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// per-level limits
//
#define	MAX_CLIENTS	    256
#define	MAX_EDICTS	    1024
#define	MAX_LIGHTSTYLES	256
#define	MAX_MODELS	    256
#define	MAX_SOUNDS	    256
#define	MAX_IMAGES	    256
#define MAX_SKINFILES	256
#define	MAX_ITEMS	    256

//==========================================================
//
//  ELEMENTS COMMUNICATED ACROSS THE NET
//
//==========================================================

// masterservers cvar is shared by client and server. This ensures both have the same default string
#define	DEFAULT_MASTER_SERVERS_IPS  "dpmaster.deathmask.net ghdigital.com excalibur.nvg.ntnu.no"
#define SERVER_PINGING_TIMEOUT 500

#ifdef UCMDTIMENUDGE
#define MAX_UCMD_TIMENUDGE 50
#endif

#define MAX_GAMESTRINGS	    2048 // this makes 8 blocks of 256 strings

//
// button bits
//
#define	BUTTON_ATTACK		1
#define	BUTTON_SPEED		2
#define	BUTTON_ACTIVATE		4
#define	BUTTON_ZOOM			8
#define	BUTTON_SPECIAL		16
#define	BUTTON_MODE			32
#define	BUTTON_BUSYICON		64

#define	BUTTON_ANY			128     // any key whatsoever

// user command communications
#define	CMD_BACKUP			64  // allow a lot of command backups for very fast systems
#define CMD_MASK			( CMD_BACKUP-1 )

#define UCMD_PUSHFRAC_SNAPSIZE 127.0f //32767.0f //send as char or short

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
	qbyte msec;
	qbyte buttons;
	short angles[3];
	float forwardfrac, sidefrac, upfrac;
	short forwardmove, sidemove, upmove;
	unsigned int serverTimeStamp;
} usercmd_t;

#define	ANGLE2SHORT( x )  ( (int)( ( x )*65536/360 ) & 65535 )
#define	SHORT2ANGLE( x )  ( ( x )*( 360.0/65536 ) )

#define	ANGLE2BYTE( x )	  ( (int)( ( x )*256/360 ) & 255 )
#define	BYTE2ANGLE( x )	  ( ( x )*( 360.0/256 ) )

//
// Each config string can be at most MAX_CONFIGSTRING_CHARS characters.
//

#define	CS_MESSAGE			0
#define	CS_MAPNAME			1
#define	CS_AUDIOTRACK		2
#define CS_HOSTNAME			3
#define CS_GRAVITY			4
#define CS_SKYBOX			5
#define CS_MODMANIFEST		6

#define CS_WORLDMODEL		30
#define	CS_MAPCHECKSUM		31

// precache stuff begins here
#define	CS_MODELS			32
#define	CS_SOUNDS			( CS_MODELS+MAX_MODELS )
#define	CS_IMAGES			( CS_SOUNDS+MAX_SOUNDS )
#define	CS_SKINFILES		( CS_IMAGES+MAX_IMAGES )
#define	CS_LIGHTS			( CS_SKINFILES+MAX_SKINFILES )

// space reserved for game specific strings (no auto-download for these)
#define CS_GAMESTRINGS	    ( CS_LIGHTS+MAX_LIGHTSTYLES )
#define	MAX_CONFIGSTRINGS   ( CS_GAMESTRINGS+MAX_GAMESTRINGS )


//==============================================
//	SNAPSHOT
//==============================================

#define PM_VECTOR_SNAP		16
#define ET_INVERSE			128
#define EV_INVERSE			128

#define	SNAPS_BACKUP_SIZE   32  // copies of entity_state_t to keep buffered
#define	SNAPS_BACKUP_MASK	( SNAPS_BACKUP_SIZE-1 )

#define	MAX_SNAPSHOT_ENTITIES	MAX_EDICTS

#define SNAPFLAG_NODELTA    ( 1<<0 )
#define SNAPFLAG_FRAMETIME  ( 1<<1 )

typedef struct
{
	int numSnapshotEntities;
	unsigned short snapshotEntities[MAX_SNAPSHOT_ENTITIES];
} snapshotEntityNumbers_t;

#define MAX_CM_AREAS	0x100

typedef struct
{
	qboolean valid;
	unsigned int snapNum;
	qbyte snapFlags;
	unsigned int deltaSnapNum;
	unsigned int timeStamp;
	unsigned int ucmdExecuted;
	int areabytes;
	qbyte areabits[MAX_CM_AREAS/8];             // portalarea visibility bits
	snapshotEntityNumbers_t ents;
} snapshot_t;

//==============================================
//	GAME STATE
//==============================================

#define	MAX_GAME_STATS	16
#define MAX_GAME_LONGSTATS 8

typedef struct
{
	short stats[MAX_GAME_STATS];
	unsigned int longstats[MAX_GAME_LONGSTATS];
} game_state_t;

//==============================================
//	ENTITY STATE
//==============================================

#define MAX_TEAMS   8
#define MAX_PLAYERCLASSES 32
#define MAX_WEAPONS 64

#define EVENT_ENTITIES_START	96 // entity types above this index will get event treatment
#define ISEVENTENTITY( x ) ( qboolean )( ((entity_state_t *)x)->type >= EVENT_ENTITIES_START )

// this struct is game-shared, but not transmitted.
// It is retrieved for encoded data in the transmission, or shared data
typedef struct gs_entity_s
{
	qboolean inuse;
	vec3_t mins, maxs, boundmins, boundmaxs, absmins, absmaxs;
	struct entity_state_s *nextLink;
	struct entity_state_s *prevLink;
} gs_entity_t;

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.

typedef struct
{
	int type;

	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	unsigned int linearProjectileTimeStamp;
} move_state_t;

// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way

#define SFL_TELEPORTED	1
#define SFL_TAKEDAMAGE	2
#define SFL_ACTIVABLE	4
#define SFL_ONLYTEAM	8   // for multiview snapshots

typedef struct entity_state_s
{
	int number;                 // entity index

	int type;                   // ET_GENERIC, ET_PLAYER, etc

	int flags;

	int team;                   // team in the game
	int playerclass;

	int solid;                  // solid, trigger, player, etc
	int cmodeltype;             // bbox, bbox_rotated or brush
	int bbox;                   // for client side prediction, 8*(bits 0-4) is x/y radius
	// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up

	move_state_t ms;            // position and velocity of entity

	int modelindex1;
	int modelindex2;
	int skinindex;

	int weapon;                 // WEAP_ for players

	unsigned int effects;

	int sound;                  // for looping sounds, to guarantee shutoff

	int events[2], eventParms[2];              // impulse events -- muzzle flashes, footsteps, etc

	vec3_t origin2;             // extra origin coordinates. The whole vector is sent each time
	// one of the parameters is changed. Use carefully (only static stuff)

	gs_entity_t local;
} entity_state_t;

//==============================================
//	CLIENT STATE
//==============================================

#define	PS_MAX_STATS	32
#define INV_MAX_SLOTS 16    // short
#define MAX_USERINPUT_STATS 8

typedef struct
{
	qbyte viewType;
	short delta_angles[3];          // add to command angles to get view direction. changed by spawns, rotating objects, and teleporters
	short controlTimers[MAX_USERINPUT_STATS];       // for user movement prediction
	qbyte weaponState;
	int event[2], eventParm[2];
	int POVnum;
	float viewHeight;
	float fov;
	qbyte inventory[INV_MAX_SLOTS][2];
	short stats[PS_MAX_STATS];
} player_state_t;

#ifdef __cplusplus
};
#endif

#endif // __Q_COMREF_H__
