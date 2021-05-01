/*
   Copyright (C) 2002-2003 Victor Luchits

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
// cg_local.h -- local definitions for client game module

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_comref.h"
#include "../gameshared/q_collision.h"

#include "../gameshared/gs_public.h"
#include "ref.h"
#include "../gui/ui_ref.h"

#include "cg_public.h"
#include "cg_syscalls.h"
#include "cg_view.h"

#define MAX_PREDICTED_EDICTS 32

#define ITEM_RESPAWN_TIME   1000

#define FLAG_TRAIL_DROP_DELAY 300
#define HEADICON_TIMEOUT 8000

typedef struct
{
	entity_state_t current;
	entity_state_t prev;        // will always be valid, but might just be a copy of current

	unsigned int snapNum;       // if not current, this ent isn't in the frame
	qboolean noOldState;
	qboolean stopped;
	qboolean canExtrapolate;
	qboolean canExtrapolatePrev;

	entity_t ent;                   // interpolated, to be added to render list
	unsigned int type;
	unsigned int renderfx;
	unsigned int effects;

	struct cgs_skeleton_s *skel;
	struct skinfile_s *skin;

	vec3_t teleportedFrom;

	// player representation only
	struct playerobject_s *pmodelObject;
	gs_player_animationstate_t animState;   // animation state
	unsigned int flash_time;
	int footstep_type;
	unsigned int footstep_time;

	vec3_t avelocity;
} centity_t;

#include "cg_ents.h"
#include "cg_boneposes.h"
#include "cg_weaponobjects.h"
#include "cg_playerobjects.h"
#include "cg_polys.h"
#include "cg_localmedia.h"
#include "cg_screen.h"
#include "cg_menu.h"
#include "cg_sound.h"
#include "cg_snapshot.h"
#include "cg_clip.h"
#include "cg_lents.h"

#define CG_MAX_ANNOUNCER_EVENTS	32
#define CG_MAX_ANNOUNCER_EVENTS_MASK ( CG_MAX_ANNOUNCER_EVENTS - 1 )

typedef struct
{
	struct sfx_s *sounds[CG_MAX_ANNOUNCER_EVENTS];
	int eventsCurrent;
	int eventsHead;
	int eventsDelay;
} cg_announcer_t;

typedef struct cg_sexedSfx_s
{
	char *name;
	struct sfx_s *sfx;
	struct cg_sexedSfx_s *next;
} cg_sexedSfx_t;

typedef struct
{
	char name[MAX_QPATH];
	int hand;
	float fov, zoomfov;
	byte_vec4_t color;
} cg_clientInfo_t;

// this is not exactly "static" but still...
typedef struct
{
	int playerNum;

	int vidWidth, vidHeight;

	qboolean demoPlaying;
	qboolean pure;
	int gameProtocol;
	unsigned int snapFrameTime;

	cg_clientInfo_t	clientInfo[MAX_CLIENTS];
} cg_static_t;

typedef struct
{
	unsigned int time;
	unsigned int serverTime;
	unsigned int extrapolationTime;

	unsigned int realTime;
	float frametime;
	unsigned int frameCount;

	snapshot_t frame, oldFrame;
	float lerpfrac;
	float xerpTime;
	float oldXerpTime;
	float xerpSmoothFrac;

	qboolean frameSequenceRunning;
	qboolean oldAreabits;
	qboolean portalInView;
	qboolean skyportalInView;
	cg_snapshot_t snap;

	float predictedOrigins[CMD_BACKUP][3];              // for debug comparing against server

	moveenvironment_t env;
	float predictedStep;                // for stair up smoothing
	unsigned int predictedStepTime;
	unsigned int predictedEventTimes[PREDICTABLE_EVENTS_MAX];
	unsigned int topPredictTimeStamp;    // for predicted events

	entity_state_t predictedEntityState;
	player_state_t predictedPlayerState;
	int predictedNewWeapon;
	vec3_t predictionError;

	// prediction optimization (don't run all ucmds in not needed)
	int predictFrom;
	entity_state_t predictFromEntityState;
	player_state_t predictFromPlayerState;

	cg_viewdef_t view;
	cg_viewweapon_t	weapon;

	loadingscreen_t	loadingScreen;  // what is drawn while loading

	// statusbar program
	struct gs_scriptnode_s *HUDprogram;

	int effects;

	// bobbing effects
	float xyspeed;
	float oldBobTime;
	int bobCycle;
	float bobFracSin;

	// announcer sounds
	cg_announcer_t announcer;

} cg_state_t;

extern cg_static_t cgs;
extern cg_state_t cg;

extern centity_t cg_entities[MAX_EDICTS];

extern cvar_t *developer;

#define ISVIEWERENTITY( entNum ) ( qboolean )( ( cg.view.type == VIEWDEF_PLAYERVIEW ) && ( cg.predictedPlayerState.POVnum == entNum ) )

//
// cg_ents.c
//

void CG_UpdateEntities( void );
void CG_LerpEntities( void );

//
// cg_configstrings.c
//
void CG_RegisterConfigStrings( void );
void CG_ConfigStringUpdate( int i );

//
// cg_players.c
//

extern cvar_t *model;
extern cvar_t *skin;
extern cvar_t *hand;

void CG_LoadClientInfo( cg_clientInfo_t *ci, const char *cstring, int client );
void CG_UpdateSexedSoundsRegistration( playerobject_t *pmodelObject );
void CG_SexedSound( int entnum, int entchannel, char *name, float fvol );
struct sfx_s *CG_RegisterSexedSound( int entnum, char *name );

//
// cg_predict.c
//
extern cvar_t *cg_predict;
extern cvar_t *cg_predict_optimize;
extern cvar_t *cg_predict_gun;
extern cvar_t *cg_predict_thirdperson;
extern cvar_t *cg_predict_debug;

void CG_PredictedEvent( int entNum, int ev, int parm );
void CG_PredictSmoothOriginForSteps( vec3_t origin );
void CG_Predict( void );
void CG_CheckPredictionError( void );
void CG_Predict_ChangeWeapon( int new_weapon );


//
// cg_hud.c
//
void Cmd_CG_PrintHudHelp_f( void );
qboolean CG_LoadHUDScript( char *s );
void CG_ExecuteHUDScript( void );


//
// cg_main.c
//

extern cvar_t *cg_fov;
extern cvar_t *cg_zoomSens;

#define CG_Malloc( size ) trap_MemAlloc( size, __FILE__, __LINE__ )
#define CG_Free( mem ) trap_MemFree( mem, __FILE__, __LINE__ )

int CG_API( void );
void CG_Init( int playerNum, int vidWidth, int vidHeight, qboolean demoplaying, qboolean pure,
			 int maxclients, unsigned int snapFrameTime, int protocol );
void CG_Shutdown( void );

//
// cg_svcmds.c
//
void CG_ServerCommand( void );
extern void CG_Cmd_RegisterCommand( const char *cmdname, void ( *func )(void) );
void CG_InitCommands( void );
void CG_ShutdownCommands( void );

//
// cg_teams.c
//
int CG_ForcedTeamColor( int team );
void CG_SetSceneTeamColors( void );
playerobject_t *CG_PModelForCentity( centity_t *cent );
struct skinfile_s *CG_SkinForCentity( centity_t *cent );

//
// cg_view.c
//
extern void CG_AddEntityToScene( entity_t *ent );
float CG_SetSensitivityScale( const float sens );
void CG_InitView( void );
void CG_RenderView( float frameTime, int realTime, unsigned int serverTime, unsigned int extrapolationTime, float stereo_separation );

//
// cg_decals.c
//
extern cvar_t *cg_addDecals;

void CG_ClearDecals( void );
void CG_SpawnDecal( vec3_t origin, vec3_t dir, float orient, float radius,
                    float r, float g, float b, float a, float die, float fadetime, qboolean fadealpha, struct shader_s *shader );
void CG_AddDecals( void );

//
// cg_effects.c
//
extern void CG_ClearDlights( void );
extern void CG_ClearLightStyles( void );
extern void CG_AddLightToScene( vec3_t org, float intensity, float r, float g, float b, struct shader_s *shader );
extern void CG_AddDlights( void );
extern void CG_RunLightStyles( void );
extern void CG_SetLightStyle( int i, const char *cstring );
extern void CG_AddLightStyles( void );
#ifdef CGAMEGETLIGHTORIGIN
extern cvar_t *cg_shadows;
extern void CG_ClearShadeBoxes( void );
extern void CG_AllocShadeBox( int entNum, const vec3_t origin, const vec3_t mins, const vec3_t maxs, struct shader_s *shader );
extern void CG_AddShadeBoxes( void );
#endif
void CG_ClearFragmentedDecals( void );
void CG_AddFragmentedDecal( vec3_t origin, vec3_t dir, float orient, float radius, float r, float g, float b, float a, struct shader_s *shader );

//
//	cg_vweap.c - client weapon
//


//
// cg_events.c
//

void CG_FireEvents( void );
void CG_EntityEvent( entity_state_t *ent, int ev, int parm, qboolean predicted );
