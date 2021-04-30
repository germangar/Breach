/*
   Copyright (C) 1997-2001 Id Software, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 */
// g_phys.c

#include "g_local.h"

//===============================================================================
//
//PUSHMOVE
//
//===============================================================================

//============
//SV_TestEntityPosition
//============
static gentity_t *SV_TestEntityPosition( gentity_t *ent )
{
	trace_t	trace;
	int mask;

	if( ent->s.type == ET_PLAYER )
		mask = MASK_PLAYERSOLID;
	else
		mask = MASK_SOLID;

	GS_Trace( &trace, ent->s.ms.origin, ent->s.local.mins, ent->s.local.maxs, ent->s.ms.origin, ENTNUM( ent ), mask, ent->timeDelta );
	if( trace.startsolid )
		return worldEntity;

	return NULL;
}

typedef struct
{
	gentity_t *ent;
	vec3_t origin;
	vec3_t angles;
	float deltayaw;
} pushed_t;
pushed_t pushed[MAX_EDICTS], *pushed_p;

gentity_t *obstacle;

//============
//SV_Push
//
//Objects need to be moved back on a failed push,
//otherwise riders would continue to slide.
//============
static qboolean SV_Push( gentity_t *pusher, vec3_t move, vec3_t amove )
{
	int i;
	gentity_t *check, *block;
	vec3_t pusher_absmins, pusher_absmaxs;
	pushed_t *p;
	vec3_t axis[3];
	vec3_t org, org2, move2;

	// clamp the move to 1/16 units, so the position will
	// be accurate for client side prediction
	GS_SnapVector( move );

	// find the bounding box after the move
	for( i = 0; i < 3; i++ )
	{
		pusher_absmins[i] = pusher->s.local.absmins[i] + move[i];
		pusher_absmaxs[i] = pusher->s.local.absmaxs[i] + move[i];
	}

	// we need this for pushing things later
	VectorNegate( amove, org );
	AnglesToAxis( org, axis );

	// save the pusher's original position
	pushed_p->ent = pusher;
	VectorCopy( pusher->s.ms.origin, pushed_p->origin );
	VectorCopy( pusher->s.ms.angles, pushed_p->angles );

	if( pusher->client )
		pushed_p->deltayaw = pusher->client->ps.delta_angles[YAW];

	pushed_p++;

	// move the pusher to its final position
	VectorAdd( pusher->s.ms.origin, move, pusher->s.ms.origin );
	VectorAdd( pusher->s.ms.angles, amove, pusher->s.ms.angles );
	GClip_LinkEntity( pusher );

	// see if any solid entities are inside the final position
	for( check = game.entities; ENTNUM( check ) < game.numentities; check++ )
	{
		if( !check->s.local.inuse )
			continue;
		if( check->s.ms.type == MOVE_TYPE_PUSHER
		    || check->s.ms.type == MOVE_TYPE_NONE )
			continue;

		if( !check->s.solid )  // not linked in anywhere
			continue;

		// if the entity is standing on the pusher, it will definitely be moved
		if( check->env.groundentity != pusher->s.number )
		{
			// see if the ent needs to be tested
			if( check->s.local.absmins[0] >= pusher_absmaxs[0]
			    || check->s.local.absmins[1] >= pusher_absmaxs[1]
			    || check->s.local.absmins[2] >= pusher_absmaxs[2]
			    || check->s.local.absmaxs[0] <= pusher_absmins[0]
			    || check->s.local.absmaxs[1] <= pusher_absmins[1]
			    || check->s.local.absmaxs[2] <= pusher_absmins[2] )
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if( !SV_TestEntityPosition( check ) )
				continue;
		}

		if( ( pusher->s.ms.type == MOVE_TYPE_PUSHER ) || ( check->env.groundentity == pusher->s.number ) )
		{
			// move this entity
			pushed_p->ent = check;
			VectorCopy( check->s.ms.origin, pushed_p->origin );
			VectorCopy( check->s.ms.angles, pushed_p->angles );
			pushed_p++;

			// try moving the contacted entity
			VectorAdd( check->s.ms.origin, move, check->s.ms.origin );
			if( check->client )
			{ // FIXME: doesn't rotate monsters?
				check->client->ps.delta_angles[YAW] += amove[YAW];
			}

			// figure movement due to the pusher's amove
			VectorSubtract( check->s.ms.origin, pusher->s.ms.origin, org );
			Matrix_TransformVector( axis, org, org2 );
			VectorSubtract( org2, org, move2 );
			VectorAdd( check->s.ms.origin, move2, check->s.ms.origin );

			//if( check->movetype != MOVETYPE_BOUNCEGRENADE ) {
			// may have pushed them off an edge
			//	if( check->env.groundentity != pusher->s.number ) {
			//		check->env.groundentity = ENTITY_INVALID;
			//	}
			//}

			block = SV_TestEntityPosition( check );
			if( !block )
			{ // pushed ok
				GClip_LinkEntity( check );
				// impact?
				continue;
			}
			else
			{ // try to fix block
				// if it is ok to leave in the old position, do it
				// this is only relevant for riding entities, not pushed
				VectorSubtract( check->s.ms.origin, move, check->s.ms.origin );
				VectorSubtract( check->s.ms.origin, move2, check->s.ms.origin );
				block = SV_TestEntityPosition( check );
				if( !block )
				{
					pushed_p--;
					continue;
				}
			}
		}

		// save off the obstacle so we can call the block function
		obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for( p = pushed_p-1; p >= pushed; p-- )
		{
			VectorCopy( p->origin, p->ent->s.ms.origin );
			VectorCopy( p->angles, p->ent->s.ms.angles );
			if( p->ent->client )
			{
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
			}
			GClip_LinkEntity( p->ent );
		}
		return qfalse;
	}

	//FIXME: is there a better way to handle this?
	// see if anything we moved has touched a trigger
	for( p = pushed_p-1; p >= pushed; p-- )
		GClip_TouchTriggers( p->ent );

	return qtrue;
}

//================
//SV_Physics_Pusher
//
//Bmodel objects don't interact with each other, but
//push all box objects
//================
static void G_Physics_Pusher( gentity_t *ent, unsigned int msecs )
{
	vec3_t move, amove;

	// make sure all team slaves can move before committing
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	//retry:
	pushed_p = pushed;
	if( ent->s.ms.velocity[0] || ent->s.ms.velocity[1] || ent->s.ms.velocity[2] ||
	    ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2] )
	{ // object is moving

		VectorScale( ent->s.ms.velocity, msecs * 0.001f, move );
		VectorScale( ent->avelocity, msecs * 0.001f, amove );

		if( !SV_Push( ent, move, amove ) )
		{
			// move was blocked

			if( pushed_p > &pushed[MAX_EDICTS] )
				GS_Error( "pushed_p > &pushed[MAX_EDICTS], memory corrupted" );

			if( ent )
			{
				// the move failed, bump all nextthink times and back out moves
				if( ent->nextthink > 0 )
					ent->nextthink += msecs;

				// if the pusher has a "blocked" function, call it
				// otherwise, just stay in place until the obstacle is gone
				if( ent->blocked )
					ent->blocked( ent, obstacle );
			}
		}
	}

	if( pushed_p > &pushed[MAX_EDICTS] )
		GS_Error( "pushed_p > &pushed[MAX_EDICTS], memory corrupted" );
}

//==============================================================================
//
//
//
//==============================================================================

/*
* G_Physics_LinearProjectile
*/
static void G_Physics_LinearProjectile( gentity_t *ent, unsigned int msecs )
{
	touchlist_t *touchList;
	int oldwaterlevel;

	assert( ent->s.ms.linearProjectileTimeStamp );

	GClip_UnlinkEntity( ent );

	touchList = GS_Move_LinearProjectile( &ent->s, game.serverTime, ent->s.ms.origin, ENTNUM( ent->owner ), MASK_SHOT, ent->timeDelta );

	GClip_LinkEntity( ent );

	if( VectorLengthFast( ent->s.ms.velocity ) )
		ent->env.groundentity = ENTITY_INVALID;

	G_FireTouches( ent, touchList );

	if( ent->s.local.inuse )
	{
		oldwaterlevel = ent->env.waterlevel;
		ent->env.waterlevel = ( GS_PointContents( ent->s.ms.origin, ent->timeDelta ) & MASK_WATER ) ? WATERLEVEL_FLOAT : qfalse;
		G_Projectile_WaterEvent( ent, oldwaterlevel );
	}
}

/*
* G_RunPhysicsTick
*/
void G_RunPhysicsTick( gentity_t *ent, vec3_t accel, unsigned int msecs )
{
	move_t move;

	if( ISEVENTENTITY( &ent->s ) )  // events do not move
		return;

	if( accel == NULL )
		accel = vec3_origin;

	switch( ent->s.ms.type )
	{
	case MOVE_TYPE_NONE:
		return;
	default:
		GS_Error( "G_RunEntity: bad movetype %i", ent->s.ms.type );

	// fix me: linear projectiles and pushers are exceptional cases which don't use the generic physics code
	case MOVE_TYPE_LINEAR_PROJECTILE:
		G_Physics_LinearProjectile( ent, msecs );
		return;

	case MOVE_TYPE_PUSHER:
		G_Physics_Pusher( ent, msecs );
		return;

	/* fall through to generic physics */
	case MOVE_TYPE_STEP:
	case MOVE_TYPE_FREEFLY:
	case MOVE_TYPE_OBJECT:
	case MOVE_TYPE_LINEAR:
		break;
	}

	memset( &move, 0, sizeof( move_t ) );

	move.ms = &ent->s.ms;
	move.entNum = ENTNUM( ent );
	move.env = ent->env;
	move.contentmask = GS_ContentMaskForState( &ent->s );
	VectorCopy( accel, move.accel );

	if( ent->s.type == ET_PLAYER || ent->client )
	{
		gsplayerclass_t	*playerClass = GS_PlayerClassByIndex( ent->s.playerclass );

		if( playerClass )
			move.specifics = playerClass->movespec;

		if( !move.specifics )
			move.specifics = &defaultPlayerMoveSpec;
	}
	else
	{
		move.specifics = &defaultObjectMoveSpecs;
	}

	GClip_UnlinkEntity( ent );

	GS_Move( &move, msecs, ent->s.local.boundmins, ent->s.local.boundmaxs );

	ent->env = move.env;

	GClip_LinkEntity( ent );
	G_FireTouches( ent, &move.output.touchList );
}
