/*
Copyright (C) 2007 German Garcia
*/

#include "cg_local.h"

//========================================================================
//
//				SKELETONS
//
//========================================================================

cgs_skeleton_t *skel_headnode;

/*
* CG_FindBoneNum
*/
int CG_FindBoneNum( cgs_skeleton_t *skel, char *bonename )
{
	int j;

	if( !skel || !bonename )
		return -1;

	for( j = 0; j < skel->numBones; j++ )
	{
		if( !Q_stricmp( skel->bones[j].name, bonename ) )
			return j;
	}

	return -1;
}

/*
* CG_AddTagMaskToSkeleton
*/
qboolean CG_AddTagMaskToSkeleton( struct model_s *model, int bonenum, char *name, float forward, float right, float up, float pitch, float yaw, float roll )
{
	cg_tagmask_t *tagmask;
	cgs_skeleton_t *skel;

	if( !name || !name[0] )
		return qfalse;

	skel = CG_SkeletonForModel( model );
	if( !skel || skel->numBones <= bonenum )
		return qfalse;

	// fixme: check the name isn't already in use, or it isn't the same as a bone name

	// store it
	tagmask = ( cg_tagmask_t * )CG_Malloc( sizeof( cg_tagmask_t ) );
	Q_snprintfz( tagmask->tagname, sizeof( tagmask->tagname ), name );
	Q_snprintfz( tagmask->bonename, sizeof( tagmask->bonename ), skel->bones[bonenum].name );
	tagmask->bonenum = bonenum;
	tagmask->offset[FORWARD] = forward;
	tagmask->offset[RIGHT] = right;
	tagmask->offset[UP] = up;
	tagmask->offset[PITCH] = pitch;
	tagmask->offset[YAW] = yaw;
	tagmask->offset[ROLL] = roll;
	tagmask->next = skel->tagmasks;
	skel->tagmasks = tagmask;

	return qtrue;
}

//#define SKEL_PRINTBONETREE
#ifdef SKEL_PRINTBONETREE
static void CG_PrintBoneTree( cgs_skeleton_t *skel, bonenode_t *node, int level )
{
	int i;

	if( node->bonenum != -1 )
	{
		for( i = 0; i < level; i++ )
		{
			GS_Printf( "  " );
		}
		GS_Printf( "%i %s\n", skel->bones[node->bonenum].parent, skel->bones[node->bonenum].name );
	}

	level++;
	// find childs of this bone
	for( i = 0; i < node->numbonechildren; i++ )
	{
		if( node->bonechildren[i] )
			CG_PrintBoneTree( skel, node->bonechildren[i], level );
	}
}
#endif

/*
* CG_CreateBonesTreeNode
* Find out the original tree
*/
static bonenode_t *CG_CreateBonesTreeNode( cgs_skeleton_t *skel, int bone )
{
	int i, count;
	int children[SKM_MAX_BONES];
	bonenode_t *bonenode;

	bonenode = ( bonenode_t * )CG_Malloc( sizeof( bonenode_t ) );
	bonenode->bonenum = bone;
	if( bone != -1 )
		skel->bones[bone].node = bonenode; // store a pointer in the linear array for fast first access.

	// find children of this bone
	count = 0;
	for( i = 0; i < skel->numBones; i++ )
	{
		if( skel->bones[i].parent == bone )
		{
			children[count] = i;
			count++;
		}
	}

	bonenode->numbonechildren = count;
	if( bonenode->numbonechildren )
	{
		bonenode->bonechildren = ( struct bonenode_s ** )CG_Malloc( sizeof( bonenode_t * ) * bonenode->numbonechildren );
		for( i = 0; i < bonenode->numbonechildren; i++ )
		{
			bonenode->bonechildren[i] = CG_CreateBonesTreeNode( skel, children[i] );
		}
	}

	return bonenode;
}

/*
* CG_SkeletonForModel
*/
cgs_skeleton_t *CG_SkeletonForModel( struct model_s *model )
{
	int i, j;
	cgs_skeleton_t *skel;
	qbyte *buffer;
	cgs_bone_t *bone;
	bonepose_t *bonePose;
	int numBones, numFrames;

	if( !model )
		return NULL;

	numBones = trap_R_SkeletalGetNumBones( model, &numFrames );
	if( !numBones || !numFrames )
		return NULL; // no bones or frames

	for( skel = skel_headnode; skel; skel = skel->next )
	{
		if( skel->model == model )
			return skel;
	}

	// allocate one huge array to hold our data
	buffer = ( qbyte * )CG_Malloc( sizeof( cgs_skeleton_t ) + numBones * sizeof( cgs_bone_t ) +
		numFrames * ( sizeof( bonepose_t * ) + numBones * sizeof( bonepose_t ) ) );

	skel = ( cgs_skeleton_t * )buffer; buffer += sizeof( cgs_skeleton_t );
	skel->bones = ( cgs_bone_t * )buffer; buffer += numBones * sizeof( cgs_bone_t );
	skel->numBones = numBones;
	skel->bonePoses = ( bonepose_t ** )buffer; buffer += numFrames * sizeof( bonepose_t * );
	skel->numFrames = numFrames;
	// register bones
	for( i = 0, bone = skel->bones; i < numBones; i++, bone++ )
		bone->parent = trap_R_SkeletalGetBoneInfo( model, i, bone->name, sizeof( bone->name ), &bone->flags );

	// register poses for all frames for all bones
	for( i = 0; i < numFrames; i++ )
	{
		skel->bonePoses[i] = ( bonepose_t * )buffer; buffer += numBones * sizeof( bonepose_t );
		for( j = 0, bonePose = skel->bonePoses[i]; j < numBones; j++, bonePose++ )
			trap_R_SkeletalGetBonePose( model, j, i, bonePose );
	}

	skel->next = skel_headnode;
	skel_headnode = skel;

	skel->model = model;

	// create a bones tree that can be run from parent to children
	skel->bonetree = CG_CreateBonesTreeNode( skel, -1 );
#ifdef SKEL_PRINTBONETREE
	CG_PrintBoneTree( skel, skel->bonetree, 1 );
#endif
	return skel;
}

/*
* CG_RegisterModel
*/
struct model_s *CG_RegisterModel( char *name )
{
	struct model_s *model;

	model = trap_R_RegisterModel( name );

	// precache bones
	if( trap_R_SkeletalGetNumBones( model, NULL ) )
		CG_SkeletonForModel( model );

	return model;
}

//========================================================================
//
//				BONEPOSES
//
//========================================================================

/*
* CG_BoneNodeFromNum
*/
bonenode_t *CG_BoneNodeFromNum( cgs_skeleton_t *skel, int bonenum )
{
	if( bonenum < 0 || bonenum >= skel->numBones )
		return skel->bonetree;
	return skel->bones[bonenum].node;
}

/*
* CG_RecurseBlendSkeletalBone
* Combine 2 different poses in one from a given root bone
*/
void CG_RecurseBlendSkeletalBone( bonepose_t *blendpose, bonepose_t *basepose, bonenode_t *bonenode, float frac )
{
	int i;
	bonepose_t *inbone, *outbone;

	if( bonenode->bonenum != -1 )
	{
		inbone = blendpose + bonenode->bonenum;
		outbone = basepose + bonenode->bonenum;
		if( frac == 1.0f )
		{
			memcpy( outbone, inbone, sizeof( bonepose_t ) );
		}
		else
		{
			// blend current node pose
			Quat_Lerp( inbone->quat, outbone->quat, frac, outbone->quat );
			VectorLerp( outbone->origin, frac, inbone->origin, outbone->origin );
		}
	}

	for( i = 0; i < bonenode->numbonechildren; i++ )
	{
		if( bonenode->bonechildren[i] )
			CG_RecurseBlendSkeletalBone( blendpose, basepose, bonenode->bonechildren[i], frac );
	}
}

/*
* CG_TransformBoneposes
* Transform boneposes to parent bone space (mount the skeleton)
*/
void CG_TransformBoneposes( cgs_skeleton_t *skel, bonepose_t *outboneposes, bonepose_t *sourceboneposes )
{
	int j;
	bonepose_t temppose;

	for( j = 0; j < (int)skel->numBones; j++ )
	{
		if( skel->bones[j].parent >= 0 )
		{
			memcpy( &temppose, &sourceboneposes[j], sizeof( bonepose_t ) );
			Quat_ConcatTransforms( outboneposes[skel->bones[j].parent].quat,
				outboneposes[skel->bones[j].parent].origin,
				temppose.quat,
				temppose.origin,
				outboneposes[j].quat,
				outboneposes[j].origin );
		}
		else
			memcpy( &outboneposes[j], &sourceboneposes[j], sizeof( bonepose_t ) );
	}
}

/*
* CG_LerpBoneposes
* Interpolate between 2 poses. It doesn't matter where they come
* from nor if they are previously transformed or not
*/
qboolean CG_LerpBoneposes( cgs_skeleton_t *skel, bonepose_t *curboneposes, bonepose_t *oldboneposes, bonepose_t *outboneposes, float frontlerp )
{
	int i;

	assert( curboneposes && oldboneposes && outboneposes );
	assert( skel && skel->numBones && skel->numFrames );

	if( frontlerp == 1.0f )
	{
		memcpy( outboneposes, curboneposes, sizeof( bonepose_t ) * skel->numBones );
	}
	else if( frontlerp == 0.0f )
	{
		memcpy(  outboneposes, oldboneposes, sizeof( bonepose_t ) * skel->numBones );
	}
	else
	{
		// run all bones
		for( i = 0; i < (int)skel->numBones; i++, curboneposes++, oldboneposes++, outboneposes++ )
		{
			Quat_Lerp( oldboneposes->quat, curboneposes->quat, frontlerp, outboneposes->quat );
			outboneposes->origin[0] = oldboneposes->origin[0] + ( curboneposes->origin[0] - oldboneposes->origin[0] ) * frontlerp;
			outboneposes->origin[1] = oldboneposes->origin[1] + ( curboneposes->origin[1] - oldboneposes->origin[1] ) * frontlerp;
			outboneposes->origin[2] = oldboneposes->origin[2] + ( curboneposes->origin[2] - oldboneposes->origin[2] ) * frontlerp;
		}
	}

	return qtrue;
}

/*
* CG_LerpSkeletonPoses
* Interpolate between 2 frame poses in a skeleton
*/
qboolean CG_LerpSkeletonPoses( cgs_skeleton_t *skel, int curframe, int oldframe, bonepose_t *outboneposes, float frontlerp )
{
	if( !skel )
		return qfalse;

	if( curframe >= skel->numFrames || curframe < 0 )
	{
		GS_Printf( S_COLOR_YELLOW "CG_LerpSkeletonPoses: out of bounds frame: %i [%i]\n", curframe, skel->numFrames );
		curframe = 0;
	}

	if( oldframe >= skel->numFrames || oldframe < 0 )
	{
		GS_Printf( S_COLOR_YELLOW "CG_LerpSkeletonPoses: out of bounds oldframe: %i [%i]\n", oldframe, skel->numFrames );
		oldframe = 0;
	}

	if( curframe == oldframe )
	{
		memcpy( outboneposes, skel->bonePoses[curframe], sizeof( bonepose_t ) * skel->numBones );
		return qtrue;
	}

	return CG_LerpBoneposes( skel, skel->bonePoses[curframe], skel->bonePoses[oldframe], outboneposes, frontlerp );
}

/*
* CG_RotateBonePose
*/
void CG_RotateBonePose( vec3_t angles, bonepose_t *bonepose )
{
	vec3_t axis_rotator[3];
	quat_t quat_rotator;
	bonepose_t temppose;
	vec3_t tempangles;

	tempangles[0] = -angles[YAW];
	tempangles[1] = -angles[PITCH];
	tempangles[2] = -angles[ROLL];
	AnglesToAxis( tempangles, axis_rotator );
	Matrix_Quat( axis_rotator, quat_rotator );

	memcpy( &temppose, bonepose, sizeof( bonepose_t ) );

	Quat_ConcatTransforms( quat_rotator, vec3_origin, temppose.quat, temppose.origin, bonepose->quat, bonepose->origin );
}

/*
* CG_TagMask
* Use alternative names for tag bones
*/
static cg_tagmask_t *CG_TagMask( const char *maskname, cgs_skeleton_t *skel )
{
	cg_tagmask_t *tagmask;

	if( !skel )
		return NULL;

	for( tagmask = skel->tagmasks; tagmask; tagmask = tagmask->next )
	{
		if( !Q_stricmp( tagmask->tagname, maskname ) )
			return tagmask;
	}

	return NULL;
}

/*
* CG_SkeletalPoseGetAttachment
* Get the tag from the interpolated and transformed pose
*/
static qboolean CG_SkeletalPoseGetAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, const char *bonename )
{
	int i;
	quat_t quat;
	cgs_bone_t *bone;
	bonepose_t *bonepose;
	cg_tagmask_t *tagmask;

	if( !boneposes || !skel )
	{
		GS_Printf( "CG_SkeletalPoseLerpAttachment: Wrong model or boneposes %s\n", bonename );
		return qfalse;
	}

	tagmask = CG_TagMask( bonename, skel );
	// find the appropriate attachment bone
	if( tagmask )
	{
		bone = skel->bones;
		for( i = 0; i < skel->numBones; i++, bone++ )
		{
			if( !Q_stricmp( bone->name, tagmask->bonename ) )
				break;
		}
	}
	else
	{
		bone = skel->bones;
		for( i = 0; i < skel->numBones; i++, bone++ )
		{
			if( !Q_stricmp( bone->name, bonename ) )
				break;
		}
	}

	if( i == skel->numBones )
	{
		GS_Printf( "CG_SkeletalPoseLerpAttachment: no such bone %s\n", bonename );
		return qfalse;
	}

	// get the desired bone
	bonepose = boneposes + i;

	// copy the inverted bone into the tag
	Quat_Inverse( bonepose->quat, quat ); //inverse the tag direction
	Quat_Matrix( quat, orient->axis );
	orient->origin[0] = bonepose->origin[0];
	orient->origin[1] = bonepose->origin[1];
	orient->origin[2] = bonepose->origin[2];
	// normalize each axis
	for( i = 0; i < 3; i++ )
		VectorNormalizeFast( orient->axis[i] );

	// do the offsetting if having a tagmask
	if( tagmask )
	{
		// we want to place a rotated model over this tag, not to rotate the tag, 
		// because all rotations would move. So we create a new orientation for the
		// model and we position the new orientation in tag space
		if( tagmask->rotate[YAW] || tagmask->rotate[PITCH] || tagmask->rotate[ROLL] )
		{
			orientation_t modOrient, newOrient;

			VectorCopy( tagmask->offset, modOrient.origin );
			AnglesToAxis( tagmask->rotate, modOrient.axis );

			VectorCopy( vec3_origin, newOrient.origin );
			Matrix_Identity( newOrient.axis );

			CG_MoveToTag( newOrient.origin, newOrient.axis,
				modOrient.origin, modOrient.axis,
				orient->origin, orient->axis
				 );

			Matrix_Copy( newOrient.axis, orient->axis );
			VectorCopy( newOrient.origin, orient->origin );
		}
		else
		{
			// offset
			for( i = 0; i < 3; i++ )
			{
				if( tagmask->offset[i] )
					VectorMA( orient->origin, tagmask->offset[i], orient->axis[i], orient->origin );
			}
		}
	}

	return qtrue;
}

/*
* CG_GrabTag - In the case of skeletal models, boneposes must be transformed prior to calling this function
*/
qboolean CG_GrabTag( orientation_t *tag, entity_t *ent, const char *tagname )
{
	cgs_skeleton_t *skel;

	if( !ent->model )
		return qfalse;

	skel = CG_SkeletonForModel( ent->model );
	if( skel )
		return CG_SkeletalPoseGetAttachment( tag, skel, ent->boneposes, tagname );

	return trap_R_LerpTag( tag, ent->model, ent->frame, ent->oldframe, ent->backlerp, tagname );
}

//==================================================
// TMP BONEPOSES
//==================================================

#define TBC_BLOCK_SIZE	    1024
static int TBC_Size;

bonepose_t *TBC; // Temporary Boneposes Cache
static int TBC_Count;

/*
* CG_InitTemporaryBoneposesCache
* allocate space for temporary boneposes
*/
void CG_InitTemporaryBoneposesCache( void )
{
	TBC_Size = TBC_BLOCK_SIZE;
	TBC = ( bonepose_t * )CG_Malloc( sizeof( bonepose_t ) * TBC_Size );
	TBC_Count = 0;
}

/*
* CG_ExpandTemporaryBoneposesCache
*/
static void CG_ExpandTemporaryBoneposesCache( void )
{
	bonepose_t *temp;

	temp = TBC;

	TBC = ( bonepose_t * )CG_Malloc( sizeof( bonepose_t ) * ( TBC_Size + TBC_BLOCK_SIZE ) );
	memcpy( TBC, temp, sizeof( bonepose_t ) * TBC_Size );
	TBC_Size += TBC_BLOCK_SIZE;

	CG_Free( temp );
}

/*
* CG_ResetTemporaryBoneposesCache
*/
void CG_ResetTemporaryBoneposesCache( void )
{
	TBC_Count = 0;
}

/*
* CG_RegisterTemporaryExternalBoneposes
* These boneposes are RESET after drawing EACH FRAME
*/
bonepose_t *CG_RegisterTemporaryExternalBoneposes( cgs_skeleton_t *skel )
{
	bonepose_t *boneposes;
	if( ( TBC_Count + skel->numBones ) > TBC_Size )
		CG_ExpandTemporaryBoneposesCache();

	boneposes = &TBC[TBC_Count];
	TBC_Count += skel->numBones;

	return boneposes;
}

/*
* CG_SetBoneposesForTemporaryEntity
* Sets up skeleton with inline boneposes based on frame/oldframe values
* These boneposes will be RESET after drawing EACH FRAME.
*/
cgs_skeleton_t *CG_SetBoneposesForTemporaryEntity( entity_t *ent )
{
	cgs_skeleton_t *skel;

	skel = CG_SkeletonForModel( ent->model );
	if( skel )
	{
		// get space in cache, interpolate, transform, link
		ent->boneposes = CG_RegisterTemporaryExternalBoneposes( skel );
		CG_LerpSkeletonPoses( skel, ent->frame, ent->oldframe, ent->boneposes, 1.0 - ent->backlerp );
		CG_TransformBoneposes( skel, ent->boneposes, ent->boneposes );
		ent->oldboneposes = ent->boneposes;
	}

	return skel;
}
