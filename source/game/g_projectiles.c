/*
   Copyright (C) 2007 German Garcia
 */

#include "g_local.h"

#define PROJECTIONSOURCE_PRESTEP_FIXED_DISTANCE	96

//==================================================
// LINEAR PROJECTILES
//==================================================

/*
* G_SpawnLinearProjectile
*/
static inline gentity_t *G_SpawnLinearProjectile( const vec3_t origin, const vec3_t angles, float speed, int gravityFactor, unsigned int timeout, gentity_t *owner )
{
	gentity_t *projectile;
	vec3_t dir;
	int i;

	projectile = G_Spawn();

	projectile->s.solid = SOLID_PROJECTILE;
	projectile->s.cmodeltype = CMODEL_BBOX;
	projectile->netflags &= ~SVF_NOCLIENT;

	AngleVectors( angles, dir, NULL, NULL );
	VectorNormalize( dir );
	VectorScale( dir, speed, projectile->s.ms.velocity );
	GS_SnapVector( projectile->s.ms.velocity );

	for( i = 0; i < 3; i++ )
		projectile->s.ms.origin[i] = projectile->s.origin2[i] = origin[i];

	projectile->s.ms.type = MOVE_TYPE_LINEAR_PROJECTILE;

	projectile->touch = NULL;
	projectile->think = G_FreeEntity;
	projectile->nextthink = level.time + timeout;

	if( owner )
	{
		projectile->owner = owner;
		projectile->s.team = owner->s.team;
		if( owner->client )
			projectile->timeDelta = owner->client->timeDelta;
	}

	projectile->s.ms.linearProjectileTimeStamp = game.serverTime;
	projectile->s.weapon = min( abs( projectile->timeDelta ), 255 );
	if( gravityFactor )
		projectile->s.skinindex = (int)( gravityFactor / 8 ); // parabolic trajectories

	projectile->env.groundentity = ENTITY_INVALID;
	projectile->env.waterlevel = ( GS_PointContents( projectile->s.ms.origin, projectile->timeDelta ) & MASK_WATER ) ? WATERLEVEL_FLOAT : qfalse;

	GClip_LinkEntity( projectile );

	return projectile;
}

/*
* G_BounceLinearProjectileVelocity
*/
static void G_BounceLinearProjectileVelocity( const vec3_t velocity, vec3_t planenormal, vec3_t newvelocity, float bounceFrac, float friction )
{
	float dot;

	dot = DotProduct( velocity, planenormal );
	if( friction && friction != 1.0f )
		VectorScale( velocity, friction, newvelocity );
	VectorMA( newvelocity, -bounceFrac*dot, planenormal, newvelocity );
	GS_SnapVector( newvelocity );
}

/*
* G_Bounce_LinearProjectile
*/
static void G_Bounce_LinearProjectile( gentity_t *projectile, gentity_t *other, cplane_t *plane, int surfFlags, float bounceFrac )
{
#define STOPSPEED 30
	if( projectile->s.ms.type == MOVE_TYPE_LINEAR_PROJECTILE )
	{
		vec3_t curVelocity;

		// parabolic projectile velocities don't match the current movement direction
		if( projectile->s.ms.linearProjectileTimeStamp >= game.serverTime - gs.snapFrameTime )
		{
			VectorCopy( projectile->s.ms.velocity, curVelocity );
		}
		else // we have to figure out the momentum's velocity
		{
			vec3_t dir, curorigin, oldorigin;
			float flyTime = 0.001f * ( ( game.serverTime - gs.snapFrameTime ) - projectile->s.ms.linearProjectileTimeStamp );

			VectorMA( projectile->s.origin2, flyTime, projectile->s.ms.velocity, oldorigin );
			if( projectile->s.skinindex )
			{
				VectorScale( gs.environment.gravityDir, projectile->s.skinindex * 8.0f, dir );
				VectorMA( oldorigin, ( flyTime*flyTime )*0.5, dir, oldorigin );
			}

			flyTime = 0.001f * ( game.serverTime - projectile->s.ms.linearProjectileTimeStamp );
			VectorMA( projectile->s.origin2, flyTime, projectile->s.ms.velocity, curorigin );
			if( projectile->s.skinindex )
			{
				VectorScale( gs.environment.gravityDir, projectile->s.skinindex * 8.0f, dir );
				VectorMA( curorigin, ( flyTime*flyTime )*0.5, dir, curorigin );
			}

			VectorSubtract( curorigin, oldorigin, dir );
			VectorScale( dir, 1000.0f/gs.snapFrameTime, curVelocity );
		}

		// restart the trajectory
		G_BounceLinearProjectileVelocity( curVelocity, plane->normal, projectile->s.ms.velocity, bounceFrac, 0.8f );
		projectile->s.ms.linearProjectileTimeStamp = game.serverTime;
		VectorCopy( projectile->s.ms.origin, projectile->s.origin2 );

		// see if it's time to stop

		// if it's not a *pure* wall, or ceiling, stop. No need of being player's walkable
		if( ( DotProduct( plane->normal, gs.environment.gravityDir ) < -0.1f ) &&
			( fabs( DotProduct( plane->normal, projectile->s.ms.velocity ) ) < STOPSPEED ) )
		{
			//GS_Printf( "planevelocity: %f\n", DotProduct( plane->normal, projectile->s.ms.velocity ) );
			VectorClear( projectile->s.ms.velocity );
			projectile->s.skinindex = 0;
			projectile->s.weapon = 0;

			// set the last plane it touched as the ground entity
			projectile->env.groundentity = ENTNUM( other );
			projectile->env.groundplane = *plane;
			projectile->env.groundsurfFlags = surfFlags;
		}
		//else
		//	GS_Printf( "restarted trajectory\n" );
	}
#undef STOPSPEED
}

/*
* G_Projectile_LocalSpread
*/
static void G_Projectile_LocalSpread( vec3_t angles, int spread, int seed )
{
	vec3_t inDir, outDir;

	AngleVectors( angles, inDir, NULL, NULL );

	GS_SpreadDir( inDir, outDir, spread, spread, &seed );

	VecToAngles( outDir, angles );
}

/*
* G_Projectile_WaterEvent
*/
void G_Projectile_WaterEvent( gentity_t *projectile, int oldwaterlevel )
{
	if( !oldwaterlevel && projectile->env.waterlevel )
	{
		G_AddEvent( projectile, EV_WATERENTER, 0, qfalse );
	}
	else if( oldwaterlevel && !projectile->env.waterlevel )
	{
		G_AddEvent( projectile, EV_WATEREXIT, 0, qfalse );
	}
}

/*
* G_Projectile_Prestep
*/
static void G_Projectile_Prestep( gentity_t *projectile, int distance )
{
	vec3_t dir, dest;
	trace_t	trace;
	int mask = MASK_SHOT;
	int i;

	VectorNormalize2( projectile->s.ms.velocity, dir );

	GClip_UnlinkEntity( projectile );

	VectorMA( projectile->s.ms.origin, distance, dir, dest );
	GS_Trace( &trace, projectile->s.ms.origin, projectile->s.local.boundmins, projectile->s.local.boundmaxs, dest, ENTNUM( projectile->owner ), mask, projectile->timeDelta );
	for( i = 0; i < 3; i++ )
		projectile->s.ms.origin[i] = projectile->s.origin2[i] = trace.endpos[i];

	GClip_LinkEntity( projectile );

	if( trace.ent != ENTITY_INVALID )
	{
		static touchlist_t touchList;
		touchList.numtouch = 0;
		GS_AddTouchEnt( &touchList, trace.ent, &trace.plane, trace.surfFlags );
		G_FireTouches( projectile, &touchList );
	}

	// initial water event
	if( projectile->s.local.inuse )
	{
		int oldwaterlevel = projectile->env.waterlevel;
		projectile->env.waterlevel = ( GS_PointContents( projectile->s.ms.origin, projectile->timeDelta ) & MASK_WATER ) ? WATERLEVEL_FLOAT : qfalse;
		G_Projectile_WaterEvent( projectile, oldwaterlevel );
	}
}

/*
* G_Rocket_Touch
*/
static void G_Rocket_Touch( gentity_t *projectile, gentity_t *other, cplane_t *plane, int surfFlags )
{
	int dflags = 0;
	int mod = 0;
	if( !( surfFlags & SURF_NOIMPACT ) )
	{
		float frac;
		vec3_t pushDir;
		vec_t *plane_normal;

		plane_normal = plane ? plane->normal : gs.environment.inverseGravityDir;

		G_SpawnEvent( EV_EXPLOSION_ONE, DirToByte( plane_normal ), projectile->s.ms.origin );

		frac = G_KnockbackPushFrac( pushDir, projectile->s.ms.origin, other->s.ms.origin, other->s.local.mins, other->s.local.maxs, projectile->splashRadius );
		G_TakeDamage( other, projectile, projectile->owner, pushDir, projectile->damage, projectile->kick * frac, dflags, mod );
		G_RadiusDamage( projectile, projectile->owner, plane_normal, projectile->splashRadius, projectile->damage, projectile->kick, ENTNUM( other ) );
	}

	G_FreeEntity( projectile );
}

/*
* G_Rocket_Fire
*/
static gentity_t *G_Rocket_Fire( const vec3_t origin, const vec3_t angles, int spread, float speed, unsigned int timeout, int seed, gentity_t *owner )
{
	gentity_t *projectile;
	vec3_t fireAngles;

	VectorCopy( angles, fireAngles );
	if( spread )
		G_Projectile_LocalSpread( fireAngles, spread, seed );

	projectile = G_SpawnLinearProjectile( origin, fireAngles, speed, 0, timeout, owner );
	projectile->touch = G_Rocket_Touch;
	projectile->s.type = ET_MODEL;
	projectile->s.modelindex1 = trap_ModelIndex( "models/items/ammo/pack/pack.md3" );
	projectile->damage = 25;
	projectile->kick = 100;
	projectile->splashRadius = 150;

	GClip_LinkEntity( projectile );

	return projectile;
}

/*
* G_Grenade_Touch
*/
static void G_Grenade_Touch( gentity_t *projectile, gentity_t *other, cplane_t *plane, int surfFlags )
{
	int dflags = 0;
	int mod = 0;

	if( surfFlags & SURF_NOIMPACT )
	{
		G_FreeEntity( projectile );
	}
	else
	{
		if( plane && ( !( other->s.flags & SFL_TAKEDAMAGE ) || GS_IsBrushModel( other->s.modelindex1 ) ) )
		{
			G_Bounce_LinearProjectile( projectile, other, plane, surfFlags, 1.3f );
		}
		else
		{
			float frac;
			vec3_t pushDir;
			vec_t *plane_normal;

			plane_normal = plane ? plane->normal : gs.environment.inverseGravityDir;

			G_SpawnEvent( EV_EXPLOSION_ONE, DirToByte( plane_normal ), projectile->s.ms.origin );

			// add damage
			frac = G_KnockbackPushFrac( pushDir, projectile->s.ms.origin, other->s.ms.origin, other->s.local.mins, other->s.local.maxs, projectile->splashRadius );
			G_TakeDamage( other, projectile, projectile->owner, pushDir, projectile->damage, projectile->kick * frac, dflags, mod );

			G_RadiusDamage( projectile, projectile->owner, plane_normal, projectile->splashRadius, projectile->damage, projectile->kick, ENTNUM( other ) );

			G_FreeEntity( projectile );
		}
	}
}

/*
* G_Grenade_TimedOut
*/
static void G_Grenade_TimedOut( gentity_t *projectile )
{
	cplane_t *plane = NULL;
	vec_t *plane_normal;

	if( projectile->env.groundentity != ENTITY_INVALID )
	{
		plane = &projectile->env.groundplane;
		if( projectile->env.groundsurfFlags & SURF_NOIMPACT )
		{
			G_FreeEntity( projectile );
			return;
		}
	}

	plane_normal = plane ? plane->normal : gs.environment.inverseGravityDir;

	G_SpawnEvent( EV_EXPLOSION_ONE, DirToByte( plane_normal ), projectile->s.ms.origin );

	G_RadiusDamage( projectile, projectile->owner, plane_normal, projectile->splashRadius, projectile->damage, projectile->kick, ENTITY_INVALID );

	G_FreeEntity( projectile );
}

/*
* G_Grenade_Fire
*/
static gentity_t *G_Grenade_Fire( const vec3_t origin, const vec3_t angles, int spread, float speed, unsigned int timeout, int seed, gentity_t *owner )
{
	gentity_t *projectile;
	vec3_t fireAngles;
	int gravityFactor = gs.environment.gravity;

	VectorCopy( angles, fireAngles );
	if( spread )
		G_Projectile_LocalSpread( fireAngles, spread, seed );

	projectile = G_SpawnLinearProjectile( origin, fireAngles, speed, gravityFactor, timeout, owner );
	projectile->think = G_Grenade_TimedOut;
	projectile->touch = G_Grenade_Touch;
	projectile->s.type = ET_MODEL;
	projectile->s.modelindex1 = trap_ModelIndex( "models/items/ammo/pack/pack.md3" );
	projectile->damage = 50;
	projectile->kick = 150;
	projectile->splashRadius = 200;

	GClip_LinkEntity( projectile );

	return projectile;
}

//==================================================
// INSTANT HIT
//==================================================

/*
* G_Bullet_FireTouch
*/
static void G_Bullet_FireTouch( trace_t *trace, gentity_t *owner )
{
	gentity_t bullet;
	gentity_t *touched = &game.entities[trace->ent];

	if( trace->ent != ENTITY_INVALID && trace->ent != ENTITY_WORLD )
	{
		// create a fake entity for the touch function self-identified as projectile
		G_InitEntity( &bullet );
		bullet.s.number = ENTITY_INVALID;
		bullet.owner = owner;
		bullet.s.solid = SOLID_PROJECTILE;

		if( touched->s.local.inuse && touched->touch )
			touched->touch( touched, &bullet, &trace->plane, trace->surfFlags );
	}
}

/*
* G_Bullet_Fire
*/
static void G_Bullet_Fire( vec3_t origin, vec3_t angles, int spread, int range, int seed, gentity_t *owner )
{
	gentity_t *ev;
	trace_t	trace;
	vec3_t dir;
	int timeDelta = 0;
	int damage = 3, kick = 3;
	int dflags = 0, mod = 0;

	if( owner && owner->client )
		timeDelta = owner->client->timeDelta;

	clamp( spread, 0, 255 );

	AngleVectors( angles, dir, NULL, NULL );

	// spawn the event for the client
	ev = G_SpawnEvent( EV_FIRE_BULLET, seed, origin );
	ev->s.modelindex1 = ENTNUM( owner );
	ev->s.weapon = spread;
	VectorScale( dir, 2048, ev->s.origin2 );
	GS_SnapVector( ev->s.origin2 );

	// do the actual shot tracing
	GS_Trace( &trace, owner->s.ms.origin, vec3_origin, vec3_origin, origin, owner->s.number, MASK_SHOT, timeDelta );
	if( trace.ent == ENTITY_INVALID )
		GS_SeedFireBullet( &trace, origin, dir, range, spread, spread, &seed, owner->s.number, timeDelta, NULL );

	// add the damage
	if( trace.ent != ENTITY_INVALID )
	{
		if( trace.ent != ENTITY_WORLD )
		{
			G_Bullet_FireTouch( &trace, owner );
			G_TakeDamage( &game.entities[trace.ent], owner, owner,
			              dir, damage, kick, dflags, mod );
		}
		else if( !( trace.surfFlags & SURF_NOIMPACT ) )
		{
		}
	}
}

//==================================================
// SPECIAL BUILD WEAPOn
//==================================================

/*
* G_NanoForge_Fire
*/
static gentity_t *G_NanoForge_Fire( const vec3_t origin, const vec3_t angles, int weaponMode, int seed, gentity_t *owner )
{
	gsbuildableobject_t *buildable;
	trace_t trace;
	vec3_t start, dir, end;
	int timeDelta = 0;

	if( owner && owner->client )
		timeDelta = owner->client->timeDelta;

	if( !weaponMode ) // repair mode
	{
		GS_Printf( "repair mode\n" );
		return NULL;
	}

	buildable = GS_BuildableByIndex( weaponMode );
	if( !buildable )
	{
		GS_Printf( "buildable not found\n" );
		return NULL;
	}

	VectorCopy( origin, start );
	AngleVectors( angles, dir, NULL, NULL );
	VectorMA( origin, 80, dir, end ); // fixme: adjust distance by buildable outer radius

	// find the candidate position for building
	GS_Trace( &trace, start, buildable->mins, buildable->maxs, end, ENTNUM( owner ), MASK_SOLID, timeDelta );
	if( trace.ent == ENTITY_INVALID )
	{
		// drop to floor
		VectorCopy( trace.endpos, start );
		end[2] -= 128;
		GS_Trace( &trace, start, buildable->mins, buildable->maxs, end, ENTITY_INVALID, MASK_SOLID, timeDelta );
		if( trace.ent != ENTITY_INVALID && !trace.startsolid )
		{
			// found floor
			gentity_t *ent;

			ent = G_Spawn();
			ent->netflags &= ~SVF_NOCLIENT;
			ent->s.type = ET_MODEL;
			ent->s.ms.type = MOVE_TYPE_NONE;
			ent->s.solid = SOLID_SOLID;
			ent->s.cmodeltype = CMODEL_BBOX;//buildable->cmodelType;
			ent->s.modelindex1 = buildable->modelIndex;
			VectorCopy( buildable->mins, ent->s.local.mins );
			VectorCopy( buildable->maxs, ent->s.local.maxs );
			VectorCopy( trace.endpos, ent->s.ms.origin );
			
			GClip_LinkEntity( ent );

			return ent;
		}
	}

	GS_Printf( "Couldn't position the buildable\n" );

	return NULL;
}

//==================================================
// FIRING EVENT
//==================================================

/*
* G_FireWeapon
*/
void G_FireWeapon( gentity_t *ent )
{
	vec3_t origin, angles;
	vec3_t viewoffset = { 0, 0, 0 };
	gentity_t *projectile;
	int weaponmode = 0;
	int ucmdSeed = rand() & 255;

	// tmp: we have no projectile definitions yet
	float speed = 900;
	unsigned int timeout = 6000;
	int range = 8192;

	// find this entity projection source
	if( ent->client )
	{
		viewoffset[2] += ent->client->ps.viewHeight;
		weaponmode = ent->client->ps.stats[STAT_WEAPON_MODE];
		ucmdSeed = ent->client->ucmdTime & 255;
	}

	VectorAdd( ent->s.ms.origin, viewoffset, origin );
	VectorCopy( ent->s.ms.angles, angles );

	projectile = NULL;

	if( level.farplanedist )
	{
		if( level.farplanedist > timeout * speed * 0.001f )
			timeout = level.farplanedist / ( speed * 0.001f );
	}

	switch( ent->s.weapon )
	{
	default:
	case WEAP_NONE:
		break;

	case WEAP_MACHINEGUN:
		if( weaponmode )
			G_Bullet_Fire( origin, angles, 8, range, ucmdSeed, ent );
		else
			G_Bullet_Fire( origin, angles, 32, range, ucmdSeed, ent );
		break;

	case WEAP_GRENADELAUNCHER:
		projectile = G_Grenade_Fire( origin, angles, 400, 1000, timeout, ucmdSeed, ent );
		break;

	case WEAP_ROCKETLAUNCHER:
	case WEAP_LASERGUN:
		projectile = G_Rocket_Fire( origin, angles, 0, speed, timeout, ucmdSeed, ent );
		break;

	case WEAP_SNIPERRIFLE:
		if( weaponmode )
			G_Bullet_Fire( origin, angles, 0, range, ucmdSeed, ent );
		else
			G_Bullet_Fire( origin, angles, 32, range, ucmdSeed, ent );
		break;

	case WEAP_NANOFORGE:
			G_NanoForge_Fire( origin, angles, weaponmode, ucmdSeed, ent );
		break;
	}

	if( projectile )
		G_Projectile_Prestep( projectile, PROJECTIONSOURCE_PRESTEP_FIXED_DISTANCE );
}
