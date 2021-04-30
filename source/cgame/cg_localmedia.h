/*
Copyright (C) 2007 German Garcia
*/

//==================================================
// Direct pointers to some media.
//==================================================

typedef struct
{
	qboolean precacheDone;

	// shaders
	struct shader_s	*shaderLevelshot;

	// fonts
	struct mufont_s	*fontSystemSmall;
	struct mufont_s	*fontSystemMedium;
	struct mufont_s	*fontSystemBig;

	// indexed arrays from server
	struct model_s *indexedModels[MAX_MODELS];
	struct sfx_s *indexedSounds[MAX_SOUNDS];
	struct shader_s	*indexedShaders[MAX_IMAGES];
	struct skinfile_s *indexedSkins[MAX_SKINFILES];

	struct playerobject_s *indexedPlayerObjects[MAX_PLAYEROBJECTS];
	struct weaponobject_s *indexedWeaponObjects[MAX_WEAPONOBJECTS];

	skyportal_t skyportal;
} cgame_media_t;

extern cgame_media_t cgm;

//==================================================
// local media indexes.
//==================================================

enum
{
	MODEL_TESTIMPACT
	, LM_TOTAL_MODELS
};

enum
{
	SOUND_CHAT
	, SOUND_WEAPONSWITCH
	, SOUND_NOAMMOCLICK
	, SOUND_WATERSPLASH
	, SOUND_WATERENTER
	, SOUND_WATEREXIT

	, SOUND_PICKUP_WEAPON
	, SOUND_PICKUP_AMMO
	, SOUND_PICKUP_HEALTH

	// footsteps
	, SOUND_FOOTSTEPS
	, SOUND_FOOTSTEPS_METAL
	, SOUND_FOOTSTEPS_WATER
	, SOUND_FOOTSTEPS_DUST

	, SOUND_BUTTON_ACTIVATE
	, SOUND_PLAT_START
	, SOUND_PLAT_STOP

	// announcer
	, SOUND_COUNTDOWN_01
	, SOUND_COUNTDOWN_02
	, SOUND_COUNTDOWN_03

	, LM_TOTAL_SOUNDS
};

enum
{
	SHADER_WHITE, 
	SHADER_CHAT,
	SHADER_PLAYERSHADOW,
	SHADER_LINEBEAM,
	SHADER_CURSOR,
	SHADER_CROSSHAIR,
	SHADER_EXPLOSION_ONE,

	LM_TOTAL_SHADERS
};

extern void CG_PrecacheLocalModels( void );
extern struct model_s *CG_LocalModel( int index );
extern void CG_PrecacheLocalSounds( void );
extern struct sfx_s *CG_LocalSound( int index );
extern void CG_PrecacheLocalShaders( void );
extern struct shader_s *CG_LocalShader( int index );
extern void CG_RegisterFonts( void );
