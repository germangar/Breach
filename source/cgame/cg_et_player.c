

#include "cg_local.h"

//=================
//CG_AddHeadIcon
//=================
static void CG_AddHeadIcon( entity_t *ent, struct shader_s *iconShader )
{
	entity_t balloon;
	float radius = 10, upoffset = 2;
	orientation_t tag_head;

	// add the current active icon
	if( iconShader != NULL && CG_GrabTag( &tag_head, ent, "tag_head" ) )
	{
		memset( &balloon, 0, sizeof( entity_t ) );
		balloon.rtype = RT_SPRITE;
		balloon.model = NULL;
		balloon.renderfx = RF_NOSHADOW;
		balloon.renderfx |= ( ent->renderfx & RF_VIEWERMODEL );
		balloon.radius = radius;
		balloon.customShader = iconShader;
		balloon.scale = 1.0f;
		Matrix_Identity( balloon.axis );
		balloon.origin[0] = tag_head.origin[0];
		balloon.origin[1] = tag_head.origin[1];
		balloon.origin[2] = tag_head.origin[2] + balloon.radius + upoffset;
		VectorCopy( balloon.origin, balloon.origin2 );
		CG_PutEntityAtTag( &balloon, &tag_head, ent->origin, ent->axis, ent->lightingOrigin );
		trap_R_AddEntityToScene( &balloon );
	}
}

//=================
//CG_AddFootSteps
//=================
static void CG_AddFootSteps( centity_t *cent )
{
	struct sfx_s *sound;
	float *velocity, frequency, scale;

	if( !cent->footstep_type )
		return;

	if( cg.time < cent->footstep_time )
		return;

	if( ISVIEWERENTITY( cent->current.number ) )
		velocity = cg.predictedEntityState.ms.velocity;
	else
		velocity = cent->current.ms.velocity;

	frequency = SQRTFAST( ( velocity[0] * velocity[0] ) + ( velocity[1] * velocity[1] ) );
	scale = ( 1000 - frequency ) * 0.3f;
	clamp( scale, 0, 1000 );

	cent->footstep_time = cg.time + 200 + scale;

	if( cent->footstep_type == 2 ) // there is no surfaceflag for water
	{
		sound = CG_LocalSound( SOUND_FOOTSTEPS_WATER );
	}
	else
	{
		vec3_t point;
		trace_t	trace;
		float testDist;

		testDist = ( 2 * STEPSIZE ) + abs( cent->current.local.mins[2] );

		VectorMA( cent->ent.origin, testDist, gs.environment.gravityDir, point );
		GS_Trace( &trace, cent->ent.origin, vec3_origin, vec3_origin, point, cent->current.number, MASK_PLAYERSOLID, 0 );
		if( trace.ent == ENTITY_INVALID )
			return;

		if( trace.surfFlags & SURF_DUST )
		{
			sound = CG_LocalSound( SOUND_FOOTSTEPS_DUST );
		}
		else if( trace.surfFlags & SURF_METALSTEPS )
		{
			sound = CG_LocalSound( SOUND_FOOTSTEPS_METAL );
		}
		else if( trace.surfFlags & SURF_SNOW )
		{
			sound = CG_LocalSound( SOUND_FOOTSTEPS_DUST ); // fixme
		}
		else if( trace.surfFlags & SURF_GRASS )
		{
			sound = CG_LocalSound( SOUND_FOOTSTEPS_DUST ); // and me
		}
		else
		{
			sound = CG_LocalSound( SOUND_FOOTSTEPS );
		}
	}

	if( ISVIEWERENTITY( cent->current.number ) )
	{
		trap_S_StartGlobalSound( sound, CHAN_FOOTSTEPS, cg_volume_effects->value );
	}
	else
	{
		trap_S_StartRelativeSound( sound, cent->current.number, CHAN_FOOTSTEPS, cg_volume_effects->value, ATTN_FOOTSTEPS );
	}
}

//=================
//CG_PlayerModelEntitySplitAngles - use YAW on lower, PITCH on upper and head.
//=================
static void CG_PlayerModelEntitySplitAngles( const vec3_t angles, const vec3_t velocity, const vec3_t avelocity, vec3_t playerAngles[PMODEL_PARTS] )
{
	//lower has horizontal direction, and zeros vertical
	playerAngles[LOWER][PITCH] = 0;
	playerAngles[LOWER][YAW] = angles[YAW];
	playerAngles[LOWER][ROLL] = 0;

	//upper marks vertical direction (total angle, so it fits aim)
	if( angles[PITCH] > 180 )
		playerAngles[UPPER][PITCH] = ( -360 + angles[PITCH] );
	else
		playerAngles[UPPER][PITCH] = angles[PITCH];
	playerAngles[UPPER][YAW] = 0;
	playerAngles[UPPER][ROLL] = 0;

	//head adds a fraction of vertical angle again
	if( angles[PITCH] > 180 )
		playerAngles[HEAD][PITCH] = ( -360 + angles[PITCH] )/3;
	else
		playerAngles[HEAD][PITCH] = angles[PITCH]/3;
	playerAngles[HEAD][YAW] = 0;
	playerAngles[HEAD][ROLL] = 0;

	if( velocity )
	{
#define MAX_LEANING_PITCH 5
#define MAX_LEANING_YAW 30
#define MAX_LEANING_ROLL 20
#define MIN_LEANING_SPEED 10
		vec3_t hvelocity, axis[3];
		float speed, front, side, aside, scale;
		vec3_t leanAngles[PMODEL_PARTS];
		int i, j;

		memset( leanAngles, 0, sizeof( leanAngles ) );

		hvelocity[0] = velocity[0];
		hvelocity[1] = velocity[1];
		hvelocity[2] = 0;

		scale = 0.04f;

		if( ( speed = VectorLengthFast( hvelocity ) ) * scale > 1.0f )
		{
			AnglesToAxis( tv( 0, angles[YAW], 0 ), axis );

			front = scale * DotProduct( hvelocity, axis[FORWARD] );
			if( front < -0.1 || front > 0.1 )
			{
				leanAngles[LOWER][PITCH] += front;
				leanAngles[UPPER][PITCH] -= front * 0.25;
				leanAngles[HEAD][PITCH] -= front * 0.5;
			}

			if( avelocity )
			{
				aside = ( front * 0.001f ) * avelocity[YAW];

				if( aside )
				{
					float asidescale = 75;
					leanAngles[LOWER][ROLL] -= aside * 0.5 * asidescale;
					leanAngles[UPPER][ROLL] += aside * 1.75 * asidescale;
					leanAngles[HEAD][ROLL] -= aside * 0.35 * asidescale;
				}
			}

			side = scale * DotProduct( hvelocity, axis[RIGHT] );

			if( side < -1 || side > 1 )
			{
				leanAngles[LOWER][ROLL] -= side * 0.5;
				leanAngles[UPPER][ROLL] += side * 0.5;
				leanAngles[HEAD][ROLL] += side * 0.25;
			}

			clamp( leanAngles[LOWER][PITCH], -MAX_LEANING_PITCH, MAX_LEANING_PITCH );
			clamp( leanAngles[LOWER][ROLL], -MAX_LEANING_ROLL, MAX_LEANING_ROLL );

/*			clamp( leanAngles[UPPER][PITCH], -45, 45 );
			clamp( leanAngles[UPPER][ROLL], -20, 20 );

			clamp( leanAngles[HEAD][PITCH], -45, 45 );
			clamp( leanAngles[HEAD][ROLL], -20, 20 );
*/
			for( j = LOWER; j < PMODEL_PARTS; j++ )
			{
				for( i = 0; i < 3; i++ )
					playerAngles[i][j] = AngleNormalize180( playerAngles[i][j] + leanAngles[i][j] );
			}
		}

#undef MIN_LEANING_SPEED
#undef MAX_LEANING_PITCH
#undef MAX_LEANING_YAW
#undef MAX_LEANING_ROLL
	}
}

//=================
//CG_PlayerModelEntityAddToScene
//=================
void CG_PlayerModelEntityAddToScene( centity_t *cent )
{
	int i;
	int weapon;
	vec3_t playerAngles[PMODEL_PARTS];
	entity_t *playerEnt;
	vec3_t angles, velocity;

	cent->ent.renderfx = cent->renderfx;
	if( ISVIEWERENTITY( cent->current.number ) && !cg.view.thirdperson )
	{
		cent->ent.renderfx |= RF_VIEWERMODEL;
	}

	// if viewer move the entity backwards a few units so the shadow doesn't look out of spot
	if( !( cent->ent.renderfx & RF_NOSHADOW ) && ( cent->ent.renderfx & RF_VIEWERMODEL ) )
	{
		vec3_t forward;
		VectorCopy( cg.view.axis[FORWARD], forward );
		forward[2] = 0;
		VectorNormalizeFast( forward );
		VectorMA( cent->ent.origin, -24, forward, cent->ent.origin );
		VectorCopy( cent->ent.origin, cent->ent.origin2 );
	}

	if( cg.view.playerPrediction && ISVIEWERENTITY( cent->current.number ) )
	{
		for( i = 0; i < 3; i++ )
			cent->ent.origin[i] = cg.predictedEntityState.ms.origin[i] - ( ( 1.0f - cg.lerpfrac ) * cg.predictionError[i] );

		VectorCopy( cg.predictedEntityState.ms.angles, angles );
		VectorCopy( cg.predictedEntityState.ms.velocity, velocity );
		CG_PredictSmoothOriginForSteps( cent->ent.origin );

		// recalculate light origin to the center of the box
		GS_CenterOfEntity( cent->ent.origin, &cent->current, cent->ent.lightingOrigin );
	}
	else
	{
		for( i = 0; i < 3; i++ )
			angles[i] = LerpAngle( cent->prev.ms.angles[i], cent->current.ms.angles[i], cg.lerpfrac );

		VectorCopy( cent->current.ms.velocity, velocity );
	}

	weapon = ISVIEWERENTITY( cent->current.number ) ? cg.predictedEntityState.weapon : cent->current.weapon;

	// apply UPPER and HEAD angles to rotator bones unless dead
	if( cent->current.solid == SOLID_CORPSE )
	{
		playerEnt = CG_AddPlayerObject( cent->pmodelObject, cent->skin, &cent->animState,
		                                cent->ent.origin, cent->ent.axis, cent->ent.lightingOrigin, NULL,
		                                cent->ent.shaderRGBA, cent->ent.renderfx, weapon, cent->flash_time );
	}
	else
	{
		// fixme: avelocity not yet used
		CG_PlayerModelEntitySplitAngles( angles, velocity, NULL, playerAngles );
		AnglesToAxis( playerAngles[LOWER], cent->ent.axis );
		playerEnt = CG_AddPlayerObject( cent->pmodelObject, cent->skin, &cent->animState,
		                                cent->ent.origin, cent->ent.axis, cent->ent.lightingOrigin, playerAngles,
		                                cent->ent.shaderRGBA, cent->ent.renderfx, weapon, cent->flash_time );
	}

	if( !playerEnt )
		return;

	if( cent->effects & EF_BUSYICON )
		CG_AddHeadIcon( playerEnt, CG_LocalShader( SHADER_CHAT ) );

	CG_AddFootSteps( cent );

#ifdef CGAMEGETLIGHTORIGIN
	if( !( cent->ent.renderfx & RF_NOSHADOW ) )
	{
		gsplayerclass_t *playerClass = GS_PlayerClassByIndex( cent->current.playerclass );
		CG_AllocShadeBox( cent->current.number, cent->ent.origin, playerClass->mins, playerClass->maxs, NULL );
	}
#endif
}

//=================
//CG_PlayerModelEntityNewState
//=================
void CG_PlayerModelEntityNewState( centity_t *cent )
{
	int frameanims;
	int newanim[PMODEL_PARTS];
	int rgbacolor = CG_ForcedTeamColor( cent->current.team );

	memset( &cent->ent, 0, sizeof( cent->ent ) );
	cent->ent.scale = 1.0f;
	cent->ent.rtype = RT_MODEL;
	cent->ent.customShader = NULL;
	cent->ent.renderfx = cent->renderfx;
	cent->ent.renderfx |= RF_MINLIGHT;
	Vector4Set( cent->ent.shaderRGBA, COLOR_R( rgbacolor ), COLOR_G( rgbacolor ), COLOR_B( rgbacolor ), COLOR_A( rgbacolor ) );

	cent->pmodelObject = CG_PModelForCentity( cent );
	cent->skin = CG_SkinForCentity( cent );

	if( !cent->pmodelObject )
	{
		cent->ent.model = NULL;
		cent->ent.customSkin = NULL;
		cent->pmodelObject = NULL;
		cent->skin = NULL;
	}
	else
	{
		cent->ent.model = cent->pmodelObject->model;
		if( !cent->pmodelObject->skel )
			GS_Error( "CG_PlayerModelEntityNewState: ET_PLAYER without a skeleton\n" );
	}

	if( ISVIEWERENTITY( cent->current.number ) )
		frameanims = GS_UpdateBaseAnims( &cent->current, cg.predictedEntityState.ms.velocity, &cent->footstep_type );
	else
		frameanims = GS_UpdateBaseAnims( &cent->current, cent->current.ms.velocity, &cent->footstep_type );

	// in special cases stop interpolation and force the restart of animations
	if( cent->pmodelObject && ( cent->noOldState || ( cent->current.flags & SFL_TELEPORTED ) ) )
	{
		memset( &cent->animState, 0, sizeof( cent->animState ) );
	}

	// filter repeated animations
	newanim[LOWER] = ( frameanims&0x3F ) * ( ( frameanims &0x3F ) != ( cent->animState.oldframeanims &0x3F ) );
	newanim[UPPER] = ( frameanims>>6 &0x3F ) * ( ( frameanims>>6 &0x3F ) != ( cent->animState.oldframeanims>>6 &0x3F ) );
	newanim[HEAD] = ( frameanims>>12 &0xF ) * ( ( frameanims>>12 &0xF ) != ( cent->animState.oldframeanims>>12 &0xF ) );
	CG_PlayerObject_AddAnimation( cent->current.number, newanim[LOWER], newanim[UPPER], newanim[HEAD], BASE_CHANNEL );

	cent->animState.oldframeanims = frameanims;
}
