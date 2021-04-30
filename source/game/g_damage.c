/*
   Copyright (C) 2007 German Garcia
 */

#include "g_local.h"

static cvar_t *g_damage_fall;
static cvar_t *g_damage_self;
static cvar_t *g_damage_team;

/*
* G_Damage_Init
*/
void G_Damage_Init( void )
{
	g_damage_fall = trap_Cvar_Get( "g_damage_fall", "0", CVAR_ARCHIVE );
	g_damage_self = trap_Cvar_Get( "g_damage_self", "1", CVAR_ARCHIVE );
	g_damage_team = trap_Cvar_Get( "g_damage_team", "1", CVAR_ARCHIVE );
}

/*
* G_CanSplashDamage
*/
static qboolean G_CanSplashDamage( entity_state_t *targ, entity_state_t *inflictor, vec3_t normal, int timeDelta )
{
	vec3_t dest, origin, targ_center;
	trace_t	trace;
	int solidmask = MASK_SOLID;
	float radius;

	if( !targ )
		return qfalse;

	GS_CenterOfEntity( targ->ms.origin, targ, targ_center );
	VectorMA( inflictor->ms.origin, 3, normal, origin );
	
	// check to center
	VectorCopy( targ_center, dest );
	GS_Trace( &trace, origin, vec3_origin, vec3_origin, dest, inflictor->number, solidmask, timeDelta );
	if( trace.ent == ENTITY_INVALID || trace.ent == targ->number )
		return qtrue;

	// try with some offsets, but make sure it's inside the target box
	radius = RadiusFromBounds( targ->local.mins, targ->local.maxs );
	clamp_high( radius, 15 );
	if( radius < 8.0f )
		return qfalse;

	VectorCopy( targ_center, dest );
	dest[0] += radius;
	dest[1] += radius;
	GS_Trace( &trace, origin, vec3_origin, vec3_origin, dest, inflictor->number, solidmask, timeDelta );
	if( trace.ent == ENTITY_INVALID || trace.ent == targ->number )
		return qtrue;

	VectorCopy( targ_center, dest );
	dest[0] += radius;
	dest[1] -= radius;
	GS_Trace( &trace, origin, vec3_origin, vec3_origin, dest, inflictor->number, solidmask, timeDelta );
	if( trace.ent == ENTITY_INVALID || trace.ent == targ->number )
		return qtrue;

	VectorCopy( targ_center, dest );
	dest[0] -= radius;
	dest[1] += radius;
	GS_Trace( &trace, origin, vec3_origin, vec3_origin, dest, inflictor->number, solidmask, timeDelta );
	if( trace.ent == ENTITY_INVALID || trace.ent == targ->number )
		return qtrue;

	VectorCopy( targ_center, dest );
	dest[0] -= radius;
	dest[1] -= radius;
	GS_Trace( &trace, origin, vec3_origin, vec3_origin, dest, inflictor->number, solidmask, timeDelta );
	if( trace.ent == ENTITY_INVALID || trace.ent == targ->number )
		return qtrue;

	return qfalse;
}

/*
* G_KnockbackPushFrac
*/
float G_KnockbackPushFrac( vec3_t pushdir, vec3_t pushorigin, vec3_t origin, vec3_t mins, vec3_t maxs, float pushradius )
{
#define VERTICALBIAS 3.5f
	vec3_t boxcenter = { 0, 0, 0 };
	float distance;
	int i;
	float innerradius;
	float outerradius;
	float pushFrac;

	if( !pushradius )
		return 0;

	innerradius = ( maxs[0] + maxs[1] - mins[0] - mins[1] ) * 0.25;
	outerradius = ( sqrt( maxs[0]*maxs[0] + maxs[1]*maxs[1] ) + sqrt( mins[0]*mins[0] + mins[1]*mins[1] ) ) * 0.5;

	// find center of the box
	for( i = 0; i < 3; i++ )
		boxcenter[i] = origin[i] + ( 0.5f * ( maxs[i] + mins[i] ) );

	// find box radius to explosion origin direction
	VectorSubtract( boxcenter, pushorigin, pushdir );
	distance = VectorNormalize( pushdir );

	if( pushorigin[2] < boxcenter[2] ) // vertical bias
	{
		pushdir[2] += ( 0.01f * VERTICALBIAS );
		pushdir[2] *= VERTICALBIAS;
		VectorNormalizeFast( pushdir );
	}

	distance -= ( ( innerradius + outerradius ) * 0.5f );

	pushFrac = 1.0 - ( distance / pushradius );
	clamp( pushFrac, 0.0f, 1.0f );
	return pushFrac;
#undef VERTICALBIAS
}

/*
* G_TakeDamage
*/
void G_TakeDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
                   vec3_t dir, float damage, float knockback, int dflags, int mod )
{
	if( GS_Paused() )
		return;

	if( !( targ->s.flags & SFL_TAKEDAMAGE ) )
		return;

	if( !inflictor )
		inflictor = worldEntity;

	if( !attacker )
		attacker = worldEntity;

	if( knockback > 0 && dir )
	{
		// add the knockback push
		if( targ->s.ms.type != MOVE_TYPE_NONE && targ->s.ms.type != MOVE_TYPE_PUSHER )
		{
			float mass, push;
			mass = targ->mass;
			clamp( mass, 50, 1000 );
			push = 1000.0f * ( knockback / mass );
			VectorNormalizeFast( dir );
			VectorMA( targ->s.ms.velocity, push, dir, targ->s.ms.velocity );
		}
	}

	if( damage > 0 )
	{
		if( !g_damage_team->integer && GS_IsTeamDamage( &targ->s, &attacker->s ) )
			return;

		if( !g_damage_self->integer && targ == attacker )
			return;

		// count snap damage for effects
		targ->snap.damage_taken += damage;
		if( attacker != worldEntity ) 
		{
			if( GS_IsTeamDamage( &targ->s, &attacker->s ) )
				attacker->snap.damage_teamgiven += damage;
			else
				attacker->snap.damage_given += damage;
		}

		// apply the damage
		targ->health = targ->health - damage;

		if( G_IsDead( targ ) )
		{
			clamp( targ->health, -999, 0 );
			if( targ->die )
				targ->die( targ, inflictor, attacker, damage );
			else
				G_FreeEntity( targ );
		}
		else
		{
			if( targ->pain )
				targ->pain( targ, attacker, knockback, damage );
		}
	}
}

/*
* G_RadiusDamage
*/
void G_RadiusDamage( gentity_t *inflictor, gentity_t *attacker, vec3_t plane_normal,
					float radius, float maxdamage, float maxknockback, int ignore )
{
	static gs_traceentitieslist clipList;
	vec3_t mins, maxs, pushDir;
	entity_state_t *clipState;
	float mindamage, minknockback;
	float frac, damage, knockback;
	int i, entNum;

	if( radius < 1.0f )
		return;

	assert( inflictor );
	assert( plane_normal );

	VectorSet( mins, -radius, -radius, -radius );
	VectorSet( maxs, radius, radius, radius );

	GS_OctreeCreatePotentialCollideList( &clipList, inflictor->s.ms.origin, mins, maxs, inflictor->s.ms.origin );
	if( !clipList.numEntities )
		return;

	mindamage = 0.0f;
	minknockback = 0.0f;

	clamp_high( mindamage, maxdamage );
	clamp_high( minknockback, maxknockback );

	for( i = 0; i < clipList.numEntities; i++ )
	{
		entNum = clipList.entityNums[i];
		if( entNum == ignore || entNum == ENTITY_WORLD )
			continue;

		clipState = GClip_GetClipStateForDeltaTime( entNum, inflictor->timeDelta );
		if( !clipState )
			continue;

		if( !( clipState->flags & SFL_TAKEDAMAGE ) )
			continue;

		frac = G_KnockbackPushFrac( pushDir, inflictor->s.ms.origin, clipState->ms.origin, clipState->local.mins, clipState->local.maxs, radius );

		damage = ( maxdamage - mindamage ) * frac;
		if( damage < 1.0f )
			damage = 0.0f;
		knockback = ( maxknockback - minknockback ) * frac;
		if( knockback < 1.0f )
			knockback = 0.0f;

		if( damage <= 0.0f && knockback <= 0.0f )
			continue;

		if( G_CanSplashDamage( clipState, &inflictor->s, plane_normal, inflictor->timeDelta ) )
		{
			G_TakeDamage( &game.entities[entNum], inflictor, attacker,
				pushDir, damage, knockback, DAMAGE_RADIUS, MOD_UNKNOWN );
		}
	}
}

/*
* G_TakeFallDamage
*/
#define MIN_FAll_DAMAGE 5
#define MAX_FALL_DAMAGE	125
void G_TakeFallDamage( gentity_t *ent, int delta )
{
	float frac;
	float damage;

	frac = (float)delta / 255.0f;

	damage = frac * MAX_FALL_DAMAGE;
	if( damage < MIN_FAll_DAMAGE )
		return;

	if( g_damage_fall->integer )
		G_TakeDamage( ent, worldEntity, worldEntity, gs.environment.inverseGravityDir, damage, 0, 0, 0 );
}
#undef MIN_FAll_DAMAGE
#undef MAX_FALL_DAMAGE
