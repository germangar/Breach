/*
   Copyright (C) 2007 German Garcia
 */


typedef struct bonenode_s
{
	int bonenum;
	int numbonechildren;
	struct bonenode_s **bonechildren;
} bonenode_t;

typedef struct cg_tagmask_s
{
	char tagname[64];
	char bonename[64];
	int bonenum;
	struct cg_tagmask_s *next;
	vec3_t offset;
	vec3_t rotate;
} cg_tagmask_t;

typedef struct
{
	char name[MAX_QPATH];
	int flags;
	int parent;
	struct bonenode_s *node;
} cgs_bone_t;

typedef struct cgs_skeleton_s
{
	struct model_s *model;

	int numBones;
	cgs_bone_t *bones;

	int numFrames;
	bonepose_t **bonePoses;

	struct cgs_skeleton_s *next;

	struct cg_tagmask_s *tagmasks; // tagmasks are only used by player models
	struct bonenode_s *bonetree;
} cgs_skeleton_t;

extern qboolean	    CG_GrabTag( orientation_t *tag, entity_t *ent, const char *tagname );

extern struct model_s *CG_RegisterModel( char *name );
extern cgs_skeleton_t *CG_SkeletonForModel( struct model_s *model );
extern int CG_FindBoneNum( cgs_skeleton_t *skel, char *bonename );
extern qboolean	    CG_AddTagMaskToSkeleton( struct model_s *model, int bonenum, char *name, float forward, float right, float up, float pitch, float yaw, float roll );

extern bonepose_t *CG_RegisterTemporaryExternalBoneposes( cgs_skeleton_t *skel );
extern cgs_skeleton_t *CG_SetBoneposesForTemporaryEntity( entity_t *ent );

extern void	CG_InitTemporaryBoneposesCache( void );
extern void	CG_ResetTemporaryBoneposesCache( void );
extern bonenode_t *CG_BoneNodeFromNum( cgs_skeleton_t *skel, int bonenum );
extern void	CG_RecurseBlendSkeletalBone( bonepose_t *blendpose, bonepose_t *basepose, bonenode_t *bonenode, float frac );
extern qboolean	CG_LerpBoneposes( cgs_skeleton_t *skel, bonepose_t *curboneposes, bonepose_t *oldboneposes, bonepose_t *lerpboneposes, float frontlerp );
extern qboolean CG_LerpSkeletonPoses( cgs_skeleton_t *skel, int curframe, int oldframe, bonepose_t *lerpboneposes, float frontlerp );
extern void	CG_TransformBoneposes( cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *sourceboneposes );
extern void	CG_RotateBonePose( vec3_t angles, bonepose_t *bonepose );
