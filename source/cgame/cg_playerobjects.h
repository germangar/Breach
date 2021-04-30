/*
Copyright (C) 2007 German Garcia
*/

#define SKM_MAX_BONES 256

// playerobject_t is the player model structure as originally read
// consider it static 'read-only', cause it is shared by different players
typedef struct playerobject_s
{
	char *name;
	int sex;

	struct	model_s	*model;
	struct cgs_skeleton_s *skel;
	struct cg_sexedSfx_s *sexedSfx;

	int numRotators[PMODEL_PARTS];
	int rotator[PMODEL_PARTS][16];
	int rootanims[PMODEL_PARTS];

	gs_pmodel_animationset_t animSet;

	struct playerobject_s *next;
} playerobject_t;

extern struct playerobject_s *CG_RegisterPlayerObject( const char *filename );
extern void CG_LoadClientPmodel( int cenum, char *model_name, char *skin_name );
extern inline void CG_PlayerObject_AddAnimation( int entNum, int loweranim, int upperanim, int headanim, int channel );
extern entity_t *CG_AddPlayerObject( playerobject_t *pmodelObject, struct skinfile_s *skin,
									gs_player_animationstate_t *animState, vec3_t origin, vec3_t axis[3],
									vec3_t lightOrigin, vec3_t lookAngles[PMODEL_PARTS],
									byte_vec4_t color, int renderfx, int weapon, unsigned int flash_time );

