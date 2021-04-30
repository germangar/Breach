/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

static playerobject_t *cg_PModelObjects;

static bonepose_t blendpose[SKM_MAX_BONES];

//======================================================================
//						PlayerModel Registering
//======================================================================

/*
* CG_PlayerObject_ParseRotationBone
*/
static void CG_PlayerObject_ParseRotationBone( playerobject_t *pmodelObject, char *token, int pmpart )
{
	int boneNumber;

	boneNumber = CG_FindBoneNum( CG_SkeletonForModel( pmodelObject->model ), token );
	if( boneNumber < 0 )
	{
		GS_Printf( "CG_ParseRotationBone: No such bone name %s in model %s\n", token, pmodelObject->name );
		return;
	}
	//register it into the player model object
	pmodelObject->rotator[pmpart][pmodelObject->numRotators[pmpart]] = boneNumber;
	pmodelObject->numRotators[pmpart]++;
}

/*
* CG_PlayerObject_ParseAnimationScript
*/
static qboolean CG_PlayerObject_ParseAnimationScript( playerobject_t *pmodelObject, char *filename )
{
	qbyte *buf;
	char *ptr, *token;
	int rounder, counter, i;
	qboolean debug = qfalse;
	int anim_data[4][PMODEL_TOTAL_ANIMATIONS];     // data is: firstframe, lastframe, loopingframes
	int rootanims[PMODEL_PARTS];
	int filenum;
	int length;

	memset( rootanims, -1, sizeof( rootanims ) );
	pmodelObject->sex = GENDER_MALE;
	rounder = 0;
	counter = 1; // reserve 0 for 'no animation'

	// load the file
	length = trap_FS_FOpenFile( filename, &filenum, FS_READ );
	if( length == -1 )
	{
		GS_Printf( "Couldn't find animation script: %s\n", filename );
		return qfalse;
	}

	buf = ( qbyte * )CG_Malloc( length + 1 );
	length = trap_FS_Read( buf, length, filenum );
	trap_FS_FCloseFile( filenum );
	if( !length )
	{
		CG_Free( buf );
		GS_Printf( "Couldn't load animation script: %s\n", filename );
		return qfalse;
	}

	//proceed
	ptr = ( char * )buf;
	while( ptr )
	{
		token = COM_ParseExt( &ptr, qtrue );
		if( !token[0] )
			break;

		if( *token < '0' || *token > '9' )
		{
			if( !Q_stricmp( token, "sex" ) )
			{

				if( debug ) GS_Printf( "Script: %s:", token );

				token = COM_ParseExt( &ptr, qfalse );
				if( !token[0] )
					break;

				if( token[0] == 'm' || token[0] == 'M' )
				{
					pmodelObject->sex = GENDER_MALE;
					if( debug ) GS_Printf( " %s -Gender set to MALE\n", token );

				}
				else if( token[0] == 'f' || token[0] == 'F' )
				{
					pmodelObject->sex = GENDER_FEMALE;
					if( debug ) GS_Printf( " %s -Gender set to FEMALE\n", token );

				}
				else if( token[0] == 'n' || token[0] == 'N' )
				{
					pmodelObject->sex = GENDER_NEUTRAL;
					if( debug ) GS_Printf( " %s -Gender set to NEUTRAL\n", token );

				}
				else
				{
					if( debug )
					{
						if( token[0] )
							GS_Printf( " WARNING: unrecognized token: %s\n", token );
						else
							GS_Printf( " WARNING: no value after cmd sex: %s\n", token );
					}
					break; //Error
				}
			}
			else if( !Q_stricmp( token, "rotationbone" ) )
			{
				// Rotation bone
				token = COM_ParseExt( &ptr, qfalse );
				if( !token[0] ) break;

				if( !Q_stricmp( token, "upper" ) )
				{
					token = COM_ParseExt( &ptr, qfalse );
					if( !token[0] ) break;
					CG_PlayerObject_ParseRotationBone( pmodelObject, token, UPPER );
				}
				else if( !Q_stricmp( token, "head" ) )
				{
					token = COM_ParseExt( &ptr, qfalse );
					if( !token[0] ) break;
					CG_PlayerObject_ParseRotationBone( pmodelObject, token, HEAD );
				}
				else if( debug )
				{
					GS_Printf( "Script: ERROR: Unrecognized rotation pmodel part %s\n", token );
					GS_Printf( "Script: ERROR: Valid names are: 'upper', 'head'\n" );
				}
			}
			else if( !Q_stricmp( token, "rootanim" ) )
			{
				// Root animation bone
				token = COM_ParseExt( &ptr, qfalse );
				if( !token[0] ) break;

				if( !Q_stricmp( token, "upper" ) )
				{
					rootanims[UPPER] = CG_FindBoneNum( CG_SkeletonForModel( pmodelObject->model ), COM_ParseExt( &ptr, qfalse ) );
				}
				else if( !Q_stricmp( token, "head" ) )
				{
					rootanims[HEAD] = CG_FindBoneNum( CG_SkeletonForModel( pmodelObject->model ), COM_ParseExt( &ptr, qfalse ) );
				}
				else if( !Q_stricmp( token, "lower" ) )
				{
					rootanims[LOWER] = CG_FindBoneNum( CG_SkeletonForModel( pmodelObject->model ), COM_ParseExt( &ptr, qfalse ) );
					//we parse it so it makes no error, but we ignore it later on
					GS_Printf( "Script: WARNING: Ignored rootanim lower: Valid names are: 'upper', 'head' (lower is always skeleton root)\n" );
				}
				else if( debug )
				{
					GS_Printf( "Script: ERROR: Unrecognized root animation pmodel part %s\n", token );
					GS_Printf( "Script: ERROR: Valid names are: 'upper', 'head'\n" );
				}
			}
			else if( !Q_stricmp( token, "tagmask" ) )
			{
				// Tag bone (format is: tagmask "bone name" "tag name")
				int bonenum;

				token = COM_ParseExt( &ptr, qfalse );
				if( !token[0] )
					break; //Error

				bonenum =  CG_FindBoneNum( CG_SkeletonForModel( pmodelObject->model ), token );
				if( bonenum != -1 )
				{
					char maskname[MAX_QPATH];
					float forward, right, up, pitch, yaw, roll;

					token = COM_ParseExt( &ptr, qfalse );
					if( !token[0] )
					{
						GS_Printf( "Script: ERROR: missing maskname in tagmask for bone %i\n", bonenum );
						break;
					}
					Q_strncpyz( maskname, token, sizeof( maskname ) );
					forward = atof( COM_ParseExt( &ptr, qfalse ) );
					right = atof( COM_ParseExt( &ptr, qfalse ) );
					up = atof( COM_ParseExt( &ptr, qfalse ) );
					pitch = atof( COM_ParseExt( &ptr, qfalse ) );
					yaw = atof( COM_ParseExt( &ptr, qfalse ) );
					roll = atof( COM_ParseExt( &ptr, qfalse ) );
					if( !CG_AddTagMaskToSkeleton( pmodelObject->model, bonenum, maskname, forward, right, up, pitch, yaw, roll ) )
						GS_Printf( "Failed to add Tagmask: %s\n", maskname );
					else if( debug )
						GS_Printf( "Script: Tagmask: %s\n", maskname );
				}
				else if( debug )
				{
					GS_Printf( "Script: WARNING: Unknown bone name: %s\n", token );
				}

			}
			else if( token[0] && debug )
				GS_Printf( "Script: WARNING: unrecognized token: %s\n", token );

		}
		else
		{
			// frame & animation values
			i = (int)atoi( token );
			if( debug ) GS_Printf( "%i - ", i );
			anim_data[rounder][counter] = i;
			rounder++;
			if( rounder > 3 )
			{
				rounder = 0;
				if( debug ) GS_Printf( " anim: %i\n", counter );
				counter++;
				if( counter == PMODEL_TOTAL_ANIMATIONS )
					break;
			}
		}
	}

	CG_Free( buf );

	if( counter < PMODEL_TOTAL_ANIMATIONS )
	{
		GS_Printf( "PModel Error: Not enough animations(%i) at animations script: %s\n", counter, filename );
		return qfalse;
	}

	// animation ANIM_NONE (0) is always at frame 0, and it's never
	// received from the game, but just used on the client when none
	// animation was ever set for a model (head).

	anim_data[0][ANIM_NONE] = 0;
	anim_data[1][ANIM_NONE]	= 0;
	anim_data[2][ANIM_NONE]	= 1;
	anim_data[3][ANIM_NONE]	= 15;

	// reorganize to make my life easier
	for( i = 0; i < counter; i++ )
	{
		pmodelObject->animSet.firstframe[i] = anim_data[0][i];

		pmodelObject->animSet.lastframe[i] = anim_data[1][i];
		if( pmodelObject->animSet.lastframe[i] < pmodelObject->animSet.firstframe[i] )
			pmodelObject->animSet.lastframe[i] = pmodelObject->animSet.firstframe[i];

		pmodelObject->animSet.loopingframes[i] = anim_data[2][i];
		if( pmodelObject->animSet.loopingframes[i] > pmodelObject->animSet.lastframe[i] - pmodelObject->animSet.firstframe[i] + 1 )
			pmodelObject->animSet.loopingframes[i] = pmodelObject->animSet.lastframe[i] - pmodelObject->animSet.firstframe[i] + 1;

		pmodelObject->animSet.frametime[i] = 1000.0f/(float)( ( anim_data[3][i] < 10 ) ? 10 : anim_data[3][i] );
	}

	rootanims[LOWER] = -1;
	for( i = LOWER; i < PMODEL_PARTS; i++ )
		pmodelObject->rootanims[i] = rootanims[i];

	return qtrue;
}

/*
* CG_PlayerObject_Load
*/
static qboolean CG_PlayerObject_Load( playerobject_t *pmodelObject, const char *name )
{
	qboolean loaded_model = qfalse;
	char anim_filename[MAX_QPATH];
	char scratch[MAX_QPATH];

	Q_snprintfz( scratch, sizeof( scratch ), "%s%s/tris%s", GS_PlayerObjects_BasePath(), name, GS_PlayerObjects_Extension() );
	if( cgs.pure && !trap_FS_IsPureFile( scratch ) )
		return qfalse;

	pmodelObject->model = CG_RegisterModel( scratch );
	pmodelObject->skel = CG_SkeletonForModel( pmodelObject->model );
	if( !pmodelObject->skel )
	{
		// pmodels only accept skeletal models
		pmodelObject->model = NULL;
		return qfalse;
	}

	pmodelObject->name = GS_CopyString( name );

	// load animations script
	if( pmodelObject->model )
	{
		Q_snprintfz( anim_filename, sizeof( anim_filename ), "%s%s/animation.cfg", GS_PlayerObjects_BasePath(), name );
		if( !cgs.pure || trap_FS_IsPureFile( anim_filename ) )
			loaded_model = CG_PlayerObject_ParseAnimationScript( pmodelObject, anim_filename );
	}

	// clean up if failed
	if( !loaded_model )
	{
		pmodelObject->model = NULL;
		CG_Free( pmodelObject->name );
		return qfalse;
	}

	// load sexed sounds for this model
	CG_UpdateSexedSoundsRegistration( pmodelObject );
	return qtrue;
}

/*
* CG_RegisterPlayerObject
*/
struct playerobject_s *CG_RegisterPlayerObject( const char *cstring )
{
	playerobject_t *pmodelObject;
	char name[MAX_QPATH];

	if( !cstring || !cstring[0] )
		return NULL;

	if( cstring[0] != '#' )
	{
		GS_Printf( "WARNING: CG_RegisterPlayerModel: Bad player object name %s\n", cstring );
		return NULL;
	}

	Q_strncpyz( name, cstring + 1, sizeof( name ) );
	if( !name[0] )
		return NULL;

	COM_StripExtension( name );

	for( pmodelObject = cg_PModelObjects; pmodelObject; pmodelObject = pmodelObject->next )
	{
		if( !Q_stricmp( pmodelObject->name, name ) )
			return pmodelObject;
	}

	pmodelObject = ( playerobject_t * )CG_Malloc( sizeof( playerobject_t ) );
	if( !CG_PlayerObject_Load( pmodelObject, name ) )
	{
		CG_Free( pmodelObject );
		return NULL;
	}

	pmodelObject->next = cg_PModelObjects;
	cg_PModelObjects = pmodelObject;

	return pmodelObject;
}

//======================================================================
//							animations
//======================================================================

/*
* CG_PlayerObject_AddAnimation
*/
inline void CG_PlayerObject_AddAnimation( int entNum, int loweranim, int upperanim, int headanim, int channel )
{
	assert( entNum >= 0 && entNum < MAX_EDICTS && cg_entities[entNum].pmodelObject );
	GS_PlayerModel_AddAnimation( &cg_entities[entNum].animState, loweranim, upperanim, headanim, channel );
}

/*
* CG_AddPlayerObject
*/
entity_t *CG_AddPlayerObject( playerobject_t *pmodelObject, struct skinfile_s *skin,
                              gs_player_animationstate_t *animState, vec3_t origin, vec3_t axis[3],
                              vec3_t lightOrigin, vec3_t lookAngles[PMODEL_PARTS],
                              byte_vec4_t color, int renderfx, int weapon, unsigned int flash_time )
{
	static entity_t	ent;
	int i, j;
	vec3_t tmpangles;
	int rootanim;
	struct cgs_skeleton_s *skel;

	if( !pmodelObject )
		return NULL;

	skel = pmodelObject->skel;
	if( !skel ) GS_Error( "CG_AddPlayerObject: Player Object without a skeleton\n" );

	memset( &ent, 0, sizeof( ent ) );
	ent.rtype = RT_MODEL;
	ent.scale = 1.0f;
	ent.backlerp = 1.0f;
	ent.model = pmodelObject->model;
	ent.customSkin = skin;
	ent.customShader = NULL;
	ent.renderfx = renderfx;
	Vector4Copy( color, ent.shaderRGBA );
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.origin2 );
	VectorCopy( lightOrigin, ent.lightingOrigin );
	Matrix_Copy( axis, ent.axis );
	ent.boneposes = ent.oldboneposes = CG_RegisterTemporaryExternalBoneposes( skel );
	ent.frame = animState->frame[LOWER]; // set base frames to help frustum culling
	ent.oldframe = animState->oldframe[LOWER];

	// transform animation values into frames, and set up old-current poses pair
	GS_Player_AnimToFrame( cg.time, &pmodelObject->animSet, animState );

	// fill base pose with lower animation already interpolated
	CG_LerpSkeletonPoses( skel, animState->frame[LOWER], animState->oldframe[LOWER], ent.boneposes, animState->lerpFrac[LOWER] );

	// create an interpolated pose of the animation to be blent
	CG_LerpSkeletonPoses( skel, animState->frame[UPPER], animState->oldframe[UPPER], blendpose, animState->lerpFrac[UPPER] );

	// blend it into base pose
	rootanim = pmodelObject->rootanims[UPPER];
	CG_RecurseBlendSkeletalBone( blendpose, ent.boneposes, CG_BoneNodeFromNum( skel, rootanim ), 1.0f );

	// apply UPPER and HEAD angles to rotator bones
	if( lookAngles )
	{
		for( i = LOWER + 1; i < PMODEL_PARTS; i++ )
		{
			if( pmodelObject->numRotators[i] )
			{
				for( j = 0; j < 3; j++ )  // divide angles by the number of rotation bones
					tmpangles[j] = lookAngles[i][j] / pmodelObject->numRotators[i];

				for( j = 0; j < pmodelObject->numRotators[i]; j++ )
					CG_RotateBonePose( tmpangles, &ent.boneposes[pmodelObject->rotator[i][j]] );
			}
		}
	}

	// mount pose. Now it's the final skeleton just as it's drawn.
	CG_TransformBoneposes( skel, ent.boneposes, ent.boneposes );

	CG_AddEntityToScene( &ent );

	if( weapon )
	{
		static orientation_t tag_weapon;
		if( CG_GrabTag( &tag_weapon, &ent, "tag_weapon" ) )
		{
			static vec3_t origin, axis[3];
			VectorClear( origin );
			Matrix_Identity( axis );
			CG_MoveToTag( origin, axis, tag_weapon.origin, tag_weapon.axis, ent.origin, ent.axis );
			CG_AddWeaponObject( CG_WeaponObjectFromIndex( weapon ), origin, axis, ent.lightingOrigin, ent.renderfx, flash_time );
		}
	}

	return &ent; // return the entity for subsequent tagging
}
