/*
Copyright (C) 2007 German Garcia
*/

#include "cg_local.h"

/*
* CG_BulletImpactEvent
*/
void CG_BulletImpactEvent( vec3_t frstart, trace_t *trace, qboolean water, qboolean transition )
{
	if( trace->ent != ENTITY_INVALID )
	{
		if( transition )
		{
			CG_TestImpact( trace->endpos, trace->plane.normal );
			if( !water )
				trap_S_StartFixedSound( CG_LocalSound( SOUND_WATERENTER ), trace->endpos, CHAN_AUTO, 1.0f, ATTN_STATIC );
			else
				trap_S_StartFixedSound( CG_LocalSound( SOUND_WATEREXIT ), trace->endpos, CHAN_AUTO, 1.0f, ATTN_STATIC );
		}
		else if( !( trace->surfFlags & SURF_NOIMPACT ) )
		{
			CG_TestImpact( trace->endpos, trace->plane.normal );
		}
	}

	if( water )
	{
		//CG_BubbleTrail( frstart, trace->endpos, 32 );
	}
}

/*
* CG_FireBullet
*/
void CG_FireBullet( vec3_t origin, vec3_t dir, int seed, int spread, int ownerNum )
{
	trace_t	trace;
	int range = 8192;

	GS_SeedFireBullet( &trace, origin, dir, range, spread, spread, &seed, ownerNum, 0, CG_BulletImpactEvent );
}

/*
* CG_EntityEvent
*/
void CG_EntityEvent( entity_state_t *state, int ev, int parm, qboolean predicted )
{
	weaponobject_t *weaponObject;
	qboolean viewer;
	vec3_t dir;

	viewer = ISVIEWERENTITY( state->number );
	if( viewer && ( ev < PREDICTABLE_EVENTS_MAX ) && ( predicted != cg.view.playerPrediction ) )
		return;

	switch( ev )
	{
	case EV_NONE:
	default:
		break;

		// predictable events

	case EV_WEAPONACTIVATE:
		CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_WEAP_UP, ANIM_NONE, EVENT_CHANNEL );
		if( viewer )
		{
			cg.predictedEntityState.weapon = parm;
			trap_S_StartGlobalSound( CG_LocalSound( SOUND_WEAPONSWITCH ), CHAN_WEAPON, cg_volume_effects->value );

			cg.predictedNewWeapon = 0; // cut out the predicted weapon
		}
		else
		{
			trap_S_StartRelativeSound( CG_LocalSound( SOUND_WEAPONSWITCH ), state->number, CHAN_WEAPON, cg_volume_effects->value, ATTN_NORMAL );
		}

		break;

	case EV_WEAPON_MODEUP:
		if( viewer )
		{
			CG_ViewWeapon_StartAnimationEvent( WEAPMODEL_ALTFIRE_UP );
		}
		break;

	case EV_WEAPON_MODEDOWN:
		if( viewer )
		{
			CG_ViewWeapon_StartAnimationEvent( WEAPMODEL_ALTFIRE_DOWN );
		}
		break;

	case EV_RELOADING:
		//CG_PlayerModel_AddAnimation( state->number, ANIM_NONE, TORSO_ATTACK1, ANIM_NONE, EVENT_CHANNEL );
		//if( viewer ) {
		//	CG_ViewWeapon_AddAnimation( WEAPMODEL_ATTACK_STRONG );
		//}

		weaponObject = CG_WeaponObjectFromIndex( state->weapon );
		if( weaponObject && weaponObject->sound_reload )
		{
			if( viewer )
				trap_S_StartGlobalSound( weaponObject->sound_reload, CHAN_WEAPON, cg_volume_effects->value );
			else
				trap_S_StartRelativeSound( weaponObject->sound_reload, state->number, CHAN_WEAPON, cg_volume_effects->value, ATTN_NORMAL );
		}

		break;

	case EV_NOAMMOCLICK:
		if( viewer )
			trap_S_StartGlobalSound( CG_LocalSound( SOUND_NOAMMOCLICK ), CHAN_WEAPON, cg_volume_effects->value );
		else
			trap_S_StartRelativeSound( CG_LocalSound( SOUND_NOAMMOCLICK ), state->number, CHAN_WEAPON, cg_volume_effects->value, ATTN_NORMAL );
		break;

	case EV_FIREWEAPON:
		CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_ATTACK1, ANIM_NONE, EVENT_CHANNEL );
		if( viewer )
		{
			CG_ViewWeapon_StartAnimationEvent( WEAPMODEL_ATTACK_STRONG );
		}

		weaponObject = CG_WeaponObjectFromIndex( state->weapon );
		if( weaponObject )
		{
			cg_entities[state->number].flash_time = cg.time + (unsigned int)weaponObject->flashTime;
			if( weaponObject->num_fire_sounds )
			{
				struct sfx_s *sound_fire = weaponObject->sound_fire[(int)brandom( 0, weaponObject->num_fire_sounds )];
				if( sound_fire )
				{
					if( viewer )
						trap_S_StartGlobalSound( sound_fire, CHAN_WEAPON, cg_volume_effects->value );
					else
						trap_S_StartRelativeSound( sound_fire, state->number, CHAN_WEAPON, cg_volume_effects->value, ATTN_NORMAL );
				}
			}
		}

		// EV_FIREBULLET is predictable
		if( predicted && cg_predict_gun->integer )
		{
			if( state->weapon == WEAP_MACHINEGUN )
			{
				int seed = cg.predictedEventTimes[EV_FIREWEAPON] & 255;

				vec3_t vieworg; // cg.view is not set up until prediction is over
				VectorCopy( cg.predictedEntityState.ms.origin, vieworg );
				vieworg[2] += cg.predictedPlayerState.viewHeight - ( 1.0f - cg.lerpfrac ) * cg.predictionError[2];
				AngleVectors( cg.predictedEntityState.ms.angles, dir, NULL, NULL );

				// FIXME: Spread hardcoded
				if( cg.predictedPlayerState.stats[STAT_WEAPON_MODE] )
					CG_FireBullet( vieworg, dir, seed, 8, cg.predictedEntityState.number );
				else
					CG_FireBullet( vieworg, dir, seed, 32, cg.predictedEntityState.number );
			}

			if( state->weapon == WEAP_SNIPERRIFLE )
			{
				int seed = cg.predictedEventTimes[EV_FIREWEAPON] & 255;

				vec3_t vieworg; // cg.view is not set up until prediction is over
				VectorCopy( cg.predictedEntityState.ms.origin, vieworg );
				vieworg[2] += cg.predictedPlayerState.viewHeight - ( 1.0f - cg.lerpfrac ) * cg.predictionError[2];
				AngleVectors( cg.predictedEntityState.ms.angles, dir, NULL, NULL );

				// FIXME: Spread hardcoded
				if( cg.predictedPlayerState.stats[STAT_WEAPON_MODE] )
					CG_FireBullet( vieworg, dir, seed, 0, cg.predictedEntityState.number );
				else
					CG_FireBullet( vieworg, dir, seed, 32, cg.predictedEntityState.number );
			}
		}

		break;

	case EV_SMOOTHREFIREWEAPON:
		CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_ATTACK1, ANIM_NONE, EVENT_CHANNEL );
		if( viewer )
		{
			CG_ViewWeapon_StartAnimationEvent( WEAPMODEL_ATTACK_STRONG );
		}

		weaponObject = CG_WeaponObjectFromIndex( state->weapon );
		if( weaponObject )
		{
			cg_entities[state->number].flash_time = cg.time + (unsigned int)weaponObject->flashTime;
			// todo: add a continuous fire looping sound

		}
		break;

	case EV_FIRE_BULLET:
		// is predictable but client is identified inside modelindex1
		if( cg_predict_gun->integer && ISVIEWERENTITY( state->modelindex1 ) && ( ev < PREDICTABLE_EVENTS_MAX ) && ( predicted != cg.view.playerPrediction ) )
			return;
		CG_FireBullet( state->ms.origin, state->origin2, parm, state->weapon, state->modelindex1 );
		break;

	case EV_ACTIVATE:
		// does nothing client side
		break;

	case EV_JUMP:
		CG_PlayerObject_AddAnimation( state->number, LEGS_JUMP, 0, 0, EVENT_CHANNEL );
		CG_SexedSound( state->number, CHAN_SELF, "*jump_1", cg_volume_players->value );
		break;

	case EV_WATERSPLASH:
		trap_S_StartFixedSound( CG_LocalSound( SOUND_WATERSPLASH ), state->ms.origin, CHAN_AUTO, 1.0f, ATTN_STATIC );
		break;

	case EV_WATERENTER:
		trap_S_StartFixedSound( CG_LocalSound( SOUND_WATERENTER ), state->ms.origin, CHAN_AUTO, 1.0f, ATTN_STATIC );
		break;

	case EV_WATEREXIT:
		trap_S_StartFixedSound( CG_LocalSound( SOUND_WATEREXIT ), state->ms.origin, CHAN_AUTO, 1.0f, ATTN_STATIC );
		break;

	case EV_FALLIMPACT:
		if( state->type == ET_PLAYER )
		{
			cg_entities[state->number].footstep_time = cg.time + 400; // extend the steps time
			if( parm < 15 )
			{
				CG_SexedSound( state->number, CHAN_FOOTSTEPS, "*fall_0", cg_volume_players->value );
			}
			else if( parm < 25 )
			{
				CG_SexedSound( state->number, CHAN_FOOTSTEPS, "*fall_1", cg_volume_players->value );
				CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_PAIN2, ANIM_NONE, EVENT_CHANNEL );
			}
			else
			{
				CG_SexedSound( state->number, CHAN_FOOTSTEPS, "*fall_2", cg_volume_players->value );
				CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_PAIN1, ANIM_NONE, EVENT_CHANNEL );
			}
		}
		else
		{ // play some generic impact sound?
		}

		break;

		// not predictable events

	case EV_WEAPONDROP:
		CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_WEAP_DOWN, ANIM_NONE, EVENT_CHANNEL );
		break;

		// func movers
	case EV_PLAT_START:
		trap_S_StartRelativeSound( CG_LocalSound( SOUND_PLAT_START ), state->number, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		break;
	case EV_PLAT_STOP:
		trap_S_StartRelativeSound( CG_LocalSound( SOUND_PLAT_STOP ), state->number, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		break;
	case EV_DOOR_HIT_TOP:
	case EV_DOOR_HIT_BOTTOM:
	case EV_DOOR_START_MOVING:
		break;
	case EV_BUTTON_START:
		trap_S_StartRelativeSound( CG_LocalSound( SOUND_BUTTON_ACTIVATE ), state->number, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		break;
	case EV_TRAIN_STOP:
	case EV_TRAIN_START:
		break;

	case EV_PAIN_SOFT:
		if( state->type == ET_PLAYER )
		{
			int toggle = (int)(rand()&1);
			CG_SexedSound( state->number, CHAN_VOICE, va( "*pain_soft_%i", 1 + toggle ), cg_volume_players->value );
			CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_PAIN1, ANIM_NONE, EVENT_CHANNEL );
		}
		break;
	case EV_PAIN_MEDIUM:
		if( state->type == ET_PLAYER )
		{
			CG_SexedSound( state->number, CHAN_VOICE, "*pain_medium", cg_volume_players->value );
			CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_PAIN2, ANIM_NONE, EVENT_CHANNEL );
		}
		break;
	case EV_PAIN_STRONG:
		if( state->type == ET_PLAYER )
		{
			CG_SexedSound( state->number, CHAN_VOICE, "*pain_strong", cg_volume_players->value );
			CG_PlayerObject_AddAnimation( state->number, ANIM_NONE, TORSO_PAIN3, ANIM_NONE, EVENT_CHANNEL );
		}
		break;

	case EV_PICKUP_WEAPON:
		if( viewer )
			trap_S_StartGlobalSound( CG_LocalSound( SOUND_PICKUP_WEAPON ), CHAN_AUTO, cg_volume_effects->value );
		else
			trap_S_StartRelativeSound( CG_LocalSound( SOUND_PICKUP_WEAPON ), state->number, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		break;

	case EV_PICKUP_AMMO:
		if( viewer )
			trap_S_StartGlobalSound( CG_LocalSound( SOUND_PICKUP_AMMO ), CHAN_AUTO, cg_volume_effects->value );
		else
			trap_S_StartRelativeSound( CG_LocalSound( SOUND_PICKUP_AMMO ), state->number, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		break;

	case EV_PICKUP_HEALTH:
		if( viewer )
			trap_S_StartGlobalSound( CG_LocalSound( SOUND_PICKUP_HEALTH ), CHAN_AUTO, cg_volume_effects->value );
		else
			trap_S_StartRelativeSound( CG_LocalSound( SOUND_PICKUP_HEALTH ), state->number, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		break;

	case EV_TESTIMPACT:
		ByteToDir( parm, dir );
		CG_TestImpact( state->ms.origin, dir );
		break;

	case EV_EXPLOSION_ONE:
		ByteToDir( parm, dir );
		CG_Event_ExplosionOne( state->ms.origin, dir );
		break;
	}
}

/*
* CG_FireEntityEvents
*/
static void CG_FireEntityEvents( void )
{
	int i, j;
	const entity_state_t *state;
	centity_t *cent;

	for( i = 0; i < cg.frame.ents.numSnapshotEntities; i++ )
	{
		state = trap_GetEntityFromSnapsBackup( cg.frame.snapNum, cg.frame.ents.snapshotEntities[i] );
		if( !state )
			continue;

		// FIXME: Too much fwd&back. Change events code to create a events list
		cent = &cg_entities[state->number];

		if( state->type == ET_SOUNDEVENT )
		{
			CG_SoundEntityNewState( cent );
			continue;
		}

		for( j = 0; j < 2; j++ )
		{
			CG_EntityEvent( &cent->current, cent->current.events[j], cent->current.eventParms[j], qfalse );
		}
	}
}

/*
* CG_FirePlayerStateEvents
*/
static void CG_FirePlayerStateEvents( void )
{
	unsigned int event, parm, count;

	if( !cg.predictedPlayerState.event )
		return;

	if( !ISVIEWERENTITY( cg.predictedPlayerState.POVnum ) )
		return;

	for( count = 0; count < 2; count++ )
	{
		event = cg.predictedPlayerState.event[count] & 127;
		parm = cg.predictedPlayerState.eventParm[count] & 0xFF;

		switch( event )
		{
		case PSEV_HIT:
			break;

		case PSEV_PICKUP:
			break;

		case PSEV_DAMAGED:
			break;

		case PSEV_INDEXEDSOUND:
			if( cgm.indexedSounds[parm] )
				trap_S_StartGlobalSound( cgm.indexedSounds[parm], CHAN_AUTO, cg_volume_effects->value );
			break;

		case PSEV_ANNOUNCER:
			CG_AddAnnouncerEvent( cgm.indexedSounds[parm], qfalse );
			break;

		case PSEV_ANNOUNCER_QUEUED:
			CG_AddAnnouncerEvent( cgm.indexedSounds[parm], qfalse );
			break;

		default:
			break;
		}
	}
}

/*
* CG_FireEvents
*/
void CG_FireEvents( void )
{
	CG_FireEntityEvents();
	CG_FirePlayerStateEvents();

	cg.snap.newEntityEvents = qfalse;
}
