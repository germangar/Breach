/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

//==================================================
// ENTITY_T TOOLS
//==================================================

//=================
//CG_MoveToTag - "moving" tag must have an axis and origin set up. Use vec3_origin and axis_identity for "nothing"
//=================
void CG_MoveToTag( vec3_t move_origin, vec3_t move_axis[3],
                   const vec3_t tag_origin, const vec3_t tag_axis[3],
				   const vec3_t space_origin, const vec3_t space_axis[3] )
{
	int i;
	vec3_t tmpAxis[3];

	VectorCopy( space_origin, move_origin );

	for( i = 0; i < 3; i++ )
		VectorMA( move_origin, tag_origin[i], space_axis[i], move_origin );

	Matrix_Multiply( move_axis, tag_axis, tmpAxis );
	Matrix_Multiply( tmpAxis, space_axis, move_axis );
}

//=================
//CG_PutEntityAtTag
//=================
void CG_PutEntityAtTag( entity_t *ent, orientation_t *tag,
                        const vec3_t ref_origin, vec3_t ref_axis[3], const vec3_t ref_lightingorigin )
{
	int i;

	VectorCopy( ref_origin, ent->origin );
	VectorCopy( ( ref_lightingorigin ? ref_lightingorigin : ref_origin ), ent->lightingOrigin );

	for( i = 0; i < 3; i++ )
		VectorMA( ent->origin, tag->origin[i] * ent->scale, ref_axis[i], ent->origin );

	VectorCopy( ent->origin, ent->origin2 );
	Matrix_Multiply( tag->axis, ref_axis, ent->axis );
}

//=================
//CG_PutRotatedEntityAtTag
//=================
void CG_PutRotatedEntityAtTag( entity_t *ent, orientation_t *tag,
                               const vec3_t ref_origin, vec3_t ref_axis[3], const vec3_t ref_lightingorigin )
{
	int i;
	vec3_t tmpAxis[3];

	VectorCopy( ref_origin, ent->origin );
	VectorCopy( ( ref_lightingorigin ? ref_lightingorigin : ref_origin ), ent->lightingOrigin );

	for( i = 0; i < 3; i++ )
		VectorMA( ent->origin, tag->origin[i] * ent->scale, ref_axis[i], ent->origin );

	VectorCopy( ent->origin, ent->origin2 );
	Matrix_Multiply( ent->axis, tag->axis, tmpAxis );
	Matrix_Multiply( tmpAxis, ref_axis, ent->axis );
}

//=================
//CG_DrawEntityBox
// draw the bounding box (in brush models case the box containing the model)
//=================
static void CG_DrawEntityBox( centity_t *cent )
{
	struct cmodel_s *cmodel;
	vec3_t mins, maxs;
	vec3_t origin, angles;
	int i;

	if( cent->ent.renderfx & RF_VIEWERMODEL )
		return;

	cmodel = GS_CModelForEntity( &cent->current );
	if( cmodel )
	{
		trap_CM_InlineModelBounds( cmodel, mins, maxs );
		VectorLerp( cent->prev.ms.origin, cg.lerpfrac, cent->current.ms.origin, origin );
		if( cent->current.cmodeltype == CMODEL_BRUSH ||
			cent->current.cmodeltype == CMODEL_BBOX_ROTATED )
		{
			for( i = 0; i < 3; i++ )
				angles[i] = LerpAngle( cent->prev.ms.angles[i], cent->current.ms.angles[i], cg.lerpfrac );

			if( angles[0] || angles[1] || angles[2] )
				CG_DrawBox( origin, cent->current.local.boundmins, cent->current.local.boundmaxs, vec3_origin, colorGreen );
		}
		else
		{
			VectorCopy( vec3_origin, angles );
		}

		CG_DrawBox( origin, mins, maxs, angles, colorWhite );
	}
}

//=================
//CG_CheckCrossHairActivation
//=================
void CG_CheckCrossHairActivation( void )
{
	int targetNum;

	targetNum = GS_FindActivateTargetInFront( &cg.predictedEntityState, &cg.predictedPlayerState, 0 );
	if( targetNum != ENTITY_INVALID )
		CG_DrawEntityBox( &cg_entities[targetNum] );
}

//==================================================
// ET_SOUNDEVENT
//==================================================

void CG_SoundEntityNewState( centity_t *cent )
{
	int channel, soundindex, owner;
	float attenuation;

	soundindex = cent->current.sound;
	owner = cent->current.modelindex2;
	channel = cent->current.modelindex1;
	attenuation = (float)cent->current.skinindex / 16.0f;

	if( attenuation == ATTN_GLOBAL )
		CG_StartGlobalIndexedSound( soundindex, channel, 1.0 );
	else if( owner && cg_entities[owner].snapNum == cg.frame.snapNum )
		CG_StartRelativeIndexedSound( soundindex, owner, channel, 1.0, attenuation );
	else
		CG_StartFixedIndexedSound( soundindex, cent->current.ms.origin, channel, 1.0, attenuation );
}

//==================================================
// ET_PORTALSURFACE
//==================================================

//=================
//CG_PortalSurfaceEntityAddToScene
//=================
static void CG_PortalSurfaceEntityAddToScene( centity_t *cent )
{
	// when origin and origin2 are the same, it's drawn as a mirror, otherwise as portal
	if( !VectorCompare( cent->ent.origin, cent->ent.origin2 ) )
	{
		if( cent->current.modelindex2 )
		{
			float phase = ( (float)cent->current.effects ) / 256.0f;
			float speed = (float)cent->current.modelindex2;

			Matrix_Identity( cent->ent.axis );
			Matrix_Rotate( cent->ent.axis, 5 * sin( ( phase + cg.time * 0.001 * speed * 0.01 ) * M_TWOPI ), 1, 0, 0 );
		}
	}

	CG_AddEntityToScene( &cent->ent );
}

//=================
//CG_PortalSurfaceEntityNewState
//=================
static void CG_PortalSurfaceEntityNewState( centity_t *cent )
{
	memset( &cent->ent, 0, sizeof( cent->ent ) );

	cent->ent.rtype = RT_PORTALSURFACE;
	Matrix_Identity( cent->ent.axis );
	VectorCopy( cent->current.ms.origin, cent->ent.origin );
	VectorCopy( cent->current.origin2, cent->ent.origin2 );

	// when origin and origin2 are the same, it's drawn as a mirror, otherwise as portal
	if( !VectorCompare( cent->ent.origin, cent->ent.origin2 ) )
	{
		cg.portalInView = qtrue;
		cent->ent.frame = cent->current.skinindex;
	}

	if( cent->current.effects & EF_NOPORTALENTS )
		cent->ent.renderfx |= RF_NOPORTALENTS;
}

//==================================================
// ET_DECAL
//==================================================

/*
* CG_DecalEntityAddToScene
*/
static void CG_DecalEntityAddToScene( centity_t *cent )
{
	// if set to invisible, skip
	if( !cent->current.modelindex1 )
		return;

	CG_AddFragmentedDecal( cent->ent.origin, cent->ent.origin2, 
		cent->ent.rotation, cent->ent.radius, 
		cent->ent.shaderRGBA[0]*(1.0/255.0), cent->ent.shaderRGBA[1]*(1.0/255.0), cent->ent.shaderRGBA[2]*(1.0/255.0), 
		cent->ent.shaderRGBA[3]*(1.0/255.0), cent->ent.customShader );
}

/*
* CG_DecalEntityInterpolateState
*/
static void CG_DecalEntityInterpolateState( centity_t *cent )
{
	int i;
	float a1, a2;

	// interpolate origin
	for( i = 0; i < 3; i++ )
		cent->ent.origin[i] = cent->prev.ms.origin[i] + cg.lerpfrac * ( cent->current.ms.origin[i] - cent->prev.ms.origin[i] );

	cent->ent.radius = cent->prev.skinindex + cg.lerpfrac * ( cent->current.skinindex - cent->prev.skinindex );

	a1 = cent->prev.modelindex2 / 255.0 * 360;
	a2 = cent->current.modelindex2 / 255.0 * 360;
	cent->ent.rotation = LerpAngle( a1, a2, cg.lerpfrac );
}

/*
* CG_DecalEntityNewState
*/
static void CG_DecalEntityNewState( centity_t *cent )
{
	int rgbacolor = CG_ForcedTeamColor( cent->current.team );

	memset( &cent->ent, 0, sizeof( cent->ent ) );
	cent->ent.model = NULL;
	cent->ent.scale = 1.0f;
	cent->ent.customShader = cgm.indexedShaders[ cent->current.modelindex1 ];
	cent->ent.renderfx = cent->renderfx;
	cent->ent.radius = cent->prev.skinindex;
	cent->ent.rotation = cent->prev.modelindex2 / 255.0 * 360;
	Vector4Set( cent->ent.shaderRGBA, COLOR_R( rgbacolor ), COLOR_G( rgbacolor ), COLOR_B( rgbacolor ), COLOR_A( rgbacolor ) );
}

//==================================================
// ET_ITEM
//==================================================

//=================
//CG_ItemEntityAddToScene
//=================
static void CG_ItemEntityAddToScene( centity_t *cent )
{
	gsitem_t *item;

	if( cent->ent.scale )
	{
		if( ( item = GS_FindItemByIndex( cent->current.modelindex1 ) ) == NULL )
			return;

		if( item->objectIndex )
		{
			if( item->type & IT_WEAPON )
			{
				CG_AddWeaponObject( CG_WeaponObjectFromIndex( item->objectIndex ), cent->ent.origin, cent->ent.axis, cent->ent.lightingOrigin, cent->ent.renderfx, 0 );
			}
			else
			{ // set up the model
				cgs_skeleton_t *skel;
				cent->ent.rtype = RT_MODEL;
				cent->ent.model = cgm.indexedModels[item->objectIndex];
				skel = CG_SkeletonForModel( cent->ent.model );

				// add to refresh list
				if( skel )
				{
					//get space in cache, lerp, transform, link
					cent->ent.boneposes = cent->ent.oldboneposes = CG_RegisterTemporaryExternalBoneposes( skel );
					CG_LerpSkeletonPoses( skel, cent->ent.frame, cent->ent.oldframe, cent->ent.boneposes, 1.0 - cent->ent.backlerp );
					CG_TransformBoneposes( skel, cent->ent.boneposes, cent->ent.boneposes );
				}
				CG_AddEntityToScene( &cent->ent );
			}
		}

#ifdef CGAMEGETLIGHTORIGIN
		// add shadows for items (do it before offseting for weapons)
		if( !( cent->ent.renderfx & RF_NOSHADOW ) )
			CG_AllocShadeBox( cent->current.number, cent->ent.origin, cent->current.local.mins, cent->current.local.maxs, NULL );
#endif

		// modelindex2 disabled by now
	}

	// add loop sound
	if( cent->current.sound )
	{
		if( ISVIEWERENTITY( cent->current.number ) )
			trap_S_AddLoopSound( cgm.indexedSounds[cent->current.sound], cent->current.number, 1.0, ATTN_GLOBAL );
		else
			trap_S_AddLoopSound( cgm.indexedSounds[cent->current.sound], cent->current.number, 1.0, ATTN_STATIC );
	}
}

//=================
//CG_ItemEntityNewState
//=================
static void CG_ItemEntityNewState( centity_t *cent )
{
	gsitem_t *item;
	int rgbacolor = CG_ForcedTeamColor( cent->current.team );

	if( ( item = GS_FindItemByIndex( cent->current.modelindex1 ) ) == NULL )
		return;

	memset( &cent->ent, 0, sizeof( cent->ent ) );
	cent->ent.scale = 1.0f;
	cent->ent.renderfx = cent->renderfx;
	Vector4Set( cent->ent.shaderRGBA, COLOR_R( rgbacolor ), COLOR_G( rgbacolor ), COLOR_B( rgbacolor ), COLOR_A( rgbacolor ) );

	if( cent->current.cmodeltype != CMODEL_BBOX )
		GS_Error( "CG_ItemEntityNewState: ET_ITEM without BBOX cmodeltype\n" );
}

//==================================================
// ET_GENERIC
//==================================================

//=================
//CG_GenericEntityAddToScene
//=================
static void CG_GenericEntityAddToScene( centity_t *cent )
{
	if( cent->ent.scale )
	{
		cent->ent.renderfx = cent->renderfx;
		if( ISVIEWERENTITY( cent->current.number ) && !cg.view.thirdperson )
		{
			cent->ent.renderfx |= RF_VIEWERMODEL;
		}

		// add to refresh list
		if( cent->skel )
		{
			// get space in cache, interpolate, transform, link
			cent->ent.boneposes = cent->ent.oldboneposes = CG_RegisterTemporaryExternalBoneposes( cent->skel );
			CG_LerpSkeletonPoses( cent->skel, cent->ent.frame, cent->ent.oldframe, cent->ent.boneposes, 1.0 - cent->ent.backlerp );
			CG_TransformBoneposes( cent->skel, cent->ent.boneposes, cent->ent.boneposes );
		}
		CG_AddEntityToScene( &cent->ent );

#ifdef CGAMEGETLIGHTORIGIN
		// add shadows for items (do it before offseting for weapons)
		if( !( cent->ent.renderfx & RF_NOSHADOW ) )
			CG_AllocShadeBox( cent->current.number, cent->ent.origin, cent->current.local.mins, cent->current.local.maxs, NULL );
#endif

		// add modelindex2 if needed
		if( cent->current.modelindex2 )
		{
			struct model_s *model1 = cent->ent.model;
			cent->ent.model = cgm.indexedModels[cent->current.modelindex2];
			CG_AddEntityToScene( &cent->ent );
			cent->ent.model = model1;
		}
	}

	// add loop sound
	if( cent->current.sound )
	{
		if( ISVIEWERENTITY( cent->current.number ) )
			trap_S_AddLoopSound( cgm.indexedSounds[cent->current.sound], cent->current.number, 1.0, ATTN_GLOBAL );
		else
			trap_S_AddLoopSound( cgm.indexedSounds[cent->current.sound], cent->current.number, 1.0, ATTN_STATIC );
	}
}

//=================
//CG_GenericEntityInterpolateState
//=================
static void CG_GenericEntityInterpolateState( centity_t *cent )
{
	vec3_t ent_angles = { 0, 0, 0 };
	int i;

	// thirdperson prediction
	if( cg.predictedPlayerState.POVnum == cent->current.number )
	{
		VectorCopy( cg.predictedEntityState.ms.origin, cent->ent.origin );
		VectorCopy( cg.predictedEntityState.ms.angles, ent_angles );
	}
	// linear projectile extrapolation
	else if( cent->current.ms.linearProjectileTimeStamp )
	{
		unsigned int serverTime;

		if( GS_Paused() )
			serverTime = cg.frame.timeStamp;
		else
		{
			serverTime = cg.serverTime;
			// to do. Check that this projectile doesn't belong to our own client before adding this offset
			if( !cgs.demoPlaying /*&& !ISVIEWERENTITY( cent->current.ownerNum )*/ )
				serverTime += cent->current.weapon; // add antilag visual compensation
		}

		GS_Move_LinearProjectile( &cent->current, serverTime, cent->ent.origin, ENTITY_INVALID, 0, 0 );

		for( i = 0; i < 3; i++ )
			ent_angles[i] = LerpAngle( cent->prev.ms.angles[i], cent->current.ms.angles[i], cg.lerpfrac );
	}
	else if( cent->stopped )
	{
		VectorCopy( cent->current.ms.origin, cent->ent.origin );

		for( i = 0; i < 3; i++ )
			ent_angles[i] = LerpAngle( cent->prev.ms.angles[i], cent->current.ms.angles[i], cg.lerpfrac );

	}
	// extrapolation
	else if( cent->canExtrapolate && cg.extrapolationTime )
	{
		vec3_t origin, xorigin1, xorigin2;

		// extrapolation with half-snapshot smoothing
		if( cg.xerpTime >= 0 || !cent->canExtrapolatePrev )
		{
			VectorMA( cent->current.ms.origin, cg.xerpTime, cent->current.ms.velocity, xorigin1 );
		}
		else
		{
			VectorMA( cent->current.ms.origin, cg.xerpTime, cent->current.ms.velocity, xorigin1 );
			if( cent->canExtrapolatePrev )
			{
				vec3_t oldPosition;

				VectorMA( cent->prev.ms.origin, cg.oldXerpTime, cent->prev.ms.velocity, oldPosition );
				VectorLerp( oldPosition, cg.xerpSmoothFrac, xorigin1, xorigin1 );
			}
		}


		// extrapolation with full-snapshot smoothing
		VectorMA( cent->current.ms.origin, cg.xerpTime, cent->current.ms.velocity, xorigin2 );
		if( cent->canExtrapolatePrev )
		{
			vec3_t oldPosition;

			VectorMA( cent->prev.ms.origin, cg.oldXerpTime, cent->prev.ms.velocity, oldPosition );
			VectorLerp( oldPosition, cg.lerpfrac, xorigin2, xorigin2 );
		}

		VectorLerp( xorigin1, 0.5f, xorigin2, origin );

#if 1
		VectorCopy( origin, cent->ent.origin );
#else
		if( cent->microSmooth == 2 )
		{
			vec3_t oldsmoothorigin;

			VectorLerp( cent->microSmoothOrigin2, 0.65f, cent->microSmoothOrigin, oldsmoothorigin );
			VectorLerp( origin, 0.5f, oldsmoothorigin, cent->ent.origin );
		}
		else if( cent->microSmooth == 1 )
			VectorLerp( origin, 0.5f, cent->microSmoothOrigin, cent->ent.origin );
		else
			VectorCopy( origin, cent->ent.origin );

		if( cent->microSmooth )
			VectorCopy( cent->microSmoothOrigin, cent->microSmoothOrigin2 );

		VectorCopy( origin, cent->microSmoothOrigin );
		cent->microSmooth++;
		clamp_high( cent->microSmooth, 2 );
#endif

		for( i = 0; i < 3; i++ )
			ent_angles[i] = LerpAngle( cent->prev.ms.angles[i], cent->current.ms.angles[i], cg.lerpfrac );
	}
	// linear interpolation
	else
	{
		for( i = 0; i < 3; i++ )
		{
			ent_angles[i] = LerpAngle( cent->prev.ms.angles[i], cent->current.ms.angles[i], cg.lerpfrac );
			cent->ent.origin[i] = cent->prev.ms.origin[i] + cg.lerpfrac * ( cent->current.ms.origin[i] - cent->prev.ms.origin[i] );
		}
	}

	cent->ent.backlerp = 1.0f - cg.lerpfrac;
	VectorCopy( cent->ent.origin, cent->ent.origin2 ); // backlerp is only used for animations

	if( ent_angles[0] || ent_angles[1] || ent_angles[2] )
		AnglesToAxis( ent_angles, cent->ent.axis );
	else
		Matrix_Copy( axis_identity, cent->ent.axis );

	// put lighting origin in the center of the box if possible
	GS_CenterOfEntity( cent->ent.origin, &cent->current, cent->ent.lightingOrigin );
}

//=================
//CG_GenericEntityNewState
//=================
static void CG_GenericEntityNewState( centity_t *cent )
{
	int rgbacolor = CG_ForcedTeamColor( cent->current.team );

	memset( &cent->ent, 0, sizeof( cent->ent ) );
	cent->ent.scale = 1.0f;
	cent->ent.renderfx = cent->renderfx;
	Vector4Set( cent->ent.shaderRGBA, COLOR_R( rgbacolor ), COLOR_G( rgbacolor ), COLOR_B( rgbacolor ), COLOR_A( rgbacolor ) );

	if( GS_IsBrushModel( cent->current.modelindex1 ) )
		cent->ent.renderfx |= RF_NOSHADOW;

	// set up the model
	cent->ent.rtype = RT_MODEL;

#ifdef QUAKE2_JUNK
	if( !cent->current.ms.linearProjectileTimeStamp )
		cent->ent.skinNum = cent->current.skinindex;
#endif
	cent->ent.model = cgm.indexedModels[cent->current.modelindex1];

	cent->skel = CG_SkeletonForModel( cent->ent.model );
}

//==================================================
// MANAGERS
//==================================================

//=================
//CG_AddEntitiesToScene
//=================
void CG_AddEntitiesToScene( void )
{
	centity_t *cent;
	int i;

	for( i = 0; i < cg.frame.ents.numSnapshotEntities; i++ )
	{
		cent = &cg_entities[cg.frame.ents.snapshotEntities[i]];

		// extrapolated projectiles can be present in the snapshot before they were fired
		if( cent->current.ms.linearProjectileTimeStamp )
		{
			if( cg.serverTime < cent->current.ms.linearProjectileTimeStamp )
				continue;
		}

		switch( cent->type )
		{
		case ET_NODRAW:
		case ET_TRIGGER:
			break;
		case ET_MODEL:
			CG_GenericEntityAddToScene( cent );
			break;
		case ET_ITEM:
			CG_ItemEntityAddToScene( cent );
			break;
		case ET_PLAYER:
			CG_PlayerModelEntityAddToScene( cent );
			break;

		case ET_EVENT:
			break;

		case ET_PORTALSURFACE:
			CG_PortalSurfaceEntityAddToScene( cent );
			break;

		case ET_DECAL:
			CG_DecalEntityAddToScene( cent );
			break;

		case ET_SOUNDEVENT:
			break;

		default:
			GS_Error( "CG_AddEntitiesToScene: unknown entity type" );
			break;
		}

		if( cg_drawEntityBoxes->integer && cent->current.solid )
		{
			CG_DrawEntityBox( cent );
		}
	}
}

//=================
//CG_InterpolateEntities
//=================
void CG_InterpolateEntities( void )
{
	centity_t *cent;
	int i;

	for( i = 0; i < cg.frame.ents.numSnapshotEntities; i++ )
	{
		cent = &cg_entities[cg.frame.ents.snapshotEntities[i]];

		switch( cent->type )
		{
		case ET_NODRAW:
		case ET_TRIGGER:
			break;

		case ET_MODEL:
		case ET_ITEM:
		case ET_PLAYER:
			CG_GenericEntityInterpolateState( cent );
			break;

		case ET_PORTALSURFACE:
			break;

		case ET_DECAL:
			CG_DecalEntityInterpolateState( cent );
			break;

		case ET_EVENT:
			break;

		case ET_SOUNDEVENT:
			break;

		default:
			GS_Error( "CG_InterpolateEntities: unknown entity type" );
			break;
		}
	}
}

//=================
//CG_UpdateEntitiesState
//=================
void CG_UpdateEntitiesState( void )
{
	centity_t *cent;
	int i;

	for( i = 0; i < cg.frame.ents.numSnapshotEntities; i++ )
	{
		cent = &cg_entities[ cg.frame.ents.snapshotEntities[i] ];

		switch( cent->current.type )
		{
		case ET_NODRAW:
		case ET_TRIGGER:
			break;
		case ET_MODEL:
			cent->renderfx = ( RF_MINLIGHT|RF_PLANARSHADOW );
			CG_GenericEntityNewState( cent );
			break;
		case ET_ITEM:
			cent->renderfx = ( RF_MINLIGHT|RF_PLANARSHADOW );
			CG_ItemEntityNewState( cent );
			break;

		case ET_PLAYER:
			cent->renderfx = ( RF_MINLIGHT|RF_OCCLUSIONTEST );
			CG_PlayerModelEntityNewState( cent );
			break;

		case ET_PORTALSURFACE:
			CG_PortalSurfaceEntityNewState( cent );
			break;

		case ET_DECAL:
			CG_DecalEntityNewState( cent );
			break;

		case ET_EVENT:
		case ET_SOUNDEVENT:
			break;

		default:
			GS_Error( "CG_NewPacketEntityState: unknown entity type %i", cent->current.type );
			break;
		}
	}
}
