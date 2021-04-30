/*
   Copyright (C) 2007 German Garcia
 */

// definition of a weapon model (actually contains more than one model)

enum
{
	WEAPMODEL_NOANIM,
	WEAPMODEL_STANDBY,
	WEAPMODEL_HOLD_ALTFIRE,
	WEAPMODEL_ATTACK_WEAK,
	WEAPMODEL_ATTACK_STRONG,
	WEAPMODEL_WEAPONUP,
	WEAPMODEL_WEAPDOWN,
	WEAPMODEL_ALTFIRE_UP,
	WEAPMODEL_ALTFIRE_DOWN,

	WEAPMODEL_MAXANIMS
};

#define WEAPONOBJECT_MAX_FIRE_SOUNDS 4

typedef struct weaponobject_s
{
	char name[MAX_QPATH];

	struct	model_s	*model[WEAPMODEL_PARTS]; // one weapon consists of several models
	struct cgs_skeleton_s *skel[WEAPMODEL_PARTS];

	orientation_t tag_projectionsource;
	vec3_t handpositionOrigin;
	vec3_t handpositionAngles;
	vec3_t mins, maxs;

	struct weaponobject_s *next;

	// animation script
	float rotationscale;
	int firstframe[WEAPMODEL_MAXANIMS];
	int lastframe[WEAPMODEL_MAXANIMS];
	int loopingframes[WEAPMODEL_MAXANIMS];
	float frametime[WEAPMODEL_MAXANIMS];

	// sfx
	int num_fire_sounds;
	struct sfx_s *sound_fire[WEAPONOBJECT_MAX_FIRE_SOUNDS];
	struct sfx_s *sound_reload;

	vec3_t flashColor;
	float flashTime;
	qboolean flashFade;
	float flashRadius;
} weaponobject_t;

typedef struct
{
	entity_t ent;

	int POVnum;
	int lastWeapon;

	// animation
	int baseAnim;
	unsigned int baseAnimStartTime;
	int eventAnim;
	unsigned int eventAnimStartTime;
} cg_viewweapon_t;

extern void CG_InitWeapons( void );
extern weaponobject_t *CG_RegisterWeaponObject( const char *cstring );
extern struct weaponobject_s *CG_WeaponObjectFromIndex( int weapon );
extern void CG_AddWeaponObject( weaponobject_t *weaponObject, vec3_t ref_origin, vec3_t ref_axis[3], vec3_t ref_lightorigin, int renderfx, unsigned int flash_time );
extern void CG_CalcViewWeapon( cg_viewweapon_t *viewweapon );
extern void CG_AddViewWeapon( cg_viewweapon_t *viewweapon );
extern void CG_ViewWeapon_StartAnimationEvent( int newAnim );
