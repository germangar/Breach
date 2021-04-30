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

// cg_lents.c -- client side temporary entities

#include "cg_local.h"

#define	MAX_LOCAL_ENTITIES  512

typedef enum
{
	LE_FREE,
	LE_NO_FADE,
	LE_RGB_FADE,
	LE_SCALE_RGB_FADE,
	LE_INVERSESCALE_RGB_FADE,
	LE_ALPHA_FADE,
	LE_SCALE_ALPHA_FADE,
	LE_INVERSESCALE_ALPHA_FADE
} letype_t;

typedef struct lentity_s
{
	struct lentity_s *prev, *next;

	letype_t type;

	entity_t ent;
	vec4_t color;

	unsigned int start;

	float light;
	vec3_t lightcolor;

	vec3_t velocity;
	vec3_t accel;

	float scale;
	int bounce;         //is activator and bounceability value at once

	unsigned int time;
} lentity_t;

static lentity_t cg_localents[MAX_LOCAL_ENTITIES];
static lentity_t cg_localents_headnode, *cg_free_lents;

//=================
//CG_ClearLocalEntities
//=================
void CG_ClearLocalEntities( void )
{
	int i;

	memset( cg_localents, 0, sizeof( cg_localents ) );

	// link local entities
	cg_free_lents = cg_localents;
	cg_localents_headnode.prev = &cg_localents_headnode;
	cg_localents_headnode.next = &cg_localents_headnode;
	for( i = 0; i < MAX_LOCAL_ENTITIES - 1; i++ )
		cg_localents[i].next = &cg_localents[i+1];
}

//=================
//CG_AllocLocalEntity
//=================
static lentity_t *CG_AllocLocalEntity( letype_t type, float r, float g, float b, float a )
{
	lentity_t *le;

	if( cg_free_lents )
	{                   // take a free decal if possible
		le = cg_free_lents;
		cg_free_lents = le->next;
	}
	else
	{                   // grab the oldest one otherwise
		le = cg_localents_headnode.prev;
		le->prev->next = le->next;
		le->next->prev = le->prev;
	}

	memset( le, 0, sizeof( *le ) );
	le->type = type;
	le->start = cg.time;
	le->scale = 1.0f;
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;

	switch( le->type )
	{
	case LE_NO_FADE:
		le->ent.scale = 1.0f;
		break;
	case LE_RGB_FADE:
	case LE_SCALE_RGB_FADE:
	case LE_INVERSESCALE_RGB_FADE:
		le->ent.scale = 1.0f;
		le->ent.shaderRGBA[3] = ( qbyte )( 255 * a );
		break;
	case LE_SCALE_ALPHA_FADE:
	case LE_INVERSESCALE_ALPHA_FADE:
	case LE_ALPHA_FADE:
		le->ent.scale = 1.0f;
		le->ent.shaderRGBA[0] = ( qbyte )( 255 * r );
		le->ent.shaderRGBA[1] = ( qbyte )( 255 * g );
		le->ent.shaderRGBA[2] = ( qbyte )( 255 * b );
		break;
	default:
		break;
	}

	// put the decal at the start of the list
	le->prev = &cg_localents_headnode;
	le->next = cg_localents_headnode.next;
	le->next->prev = le;
	le->prev->next = le;

	return le;
}

//=================
//CG_FreeLocalEntity
//=================
static void CG_FreeLocalEntity( lentity_t *le )
{
	// remove from linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// insert into linked free list
	le->next = cg_free_lents;
	cg_free_lents = le;
}

//=================
//CG_AllocModel
//=================
static lentity_t *CG_AllocModel( letype_t type, const vec3_t origin, const vec3_t angles, unsigned int time,
                                 float r, float g, float b, float a, float light, float lr, float lg, float lb, struct model_s *model, struct shader_s *shader )
{
	lentity_t *le;

	le = CG_AllocLocalEntity( type, r, g, b, a );
	le->time = time;
	le->light = light;
	le->lightcolor[0] = lr;
	le->lightcolor[1] = lg;
	le->lightcolor[2] = lb;

	le->ent.rtype = RT_MODEL;
	le->ent.renderfx = RF_NOSHADOW;
	le->ent.model = model;
	le->ent.customShader = shader;
	le->ent.shaderTime = cg.time;
	le->ent.scale = 1.0f;

	AnglesToAxis( angles, le->ent.axis );
	VectorCopy( origin, le->ent.origin );

	return le;
}

//=================
//CG_AllocSprite
//=================
static lentity_t *CG_AllocSprite( letype_t type, vec3_t origin, float radius, unsigned int time,
                                  float r, float g, float b, float a, float light, float lr, float lg, float lb, struct shader_s *shader )
{
	lentity_t *le;

	le = CG_AllocLocalEntity( type, r, g, b, a );
	le->time = time;
	le->light = light;
	le->lightcolor[0] = lr;
	le->lightcolor[1] = lg;
	le->lightcolor[2] = lb;

	le->ent.rtype = RT_SPRITE;
	le->ent.renderfx = RF_NOSHADOW;
	le->ent.radius = radius;
	le->ent.customShader = shader;
	le->ent.shaderTime = cg.time;
	le->ent.scale = 1.0f;

	Matrix_Identity( le->ent.axis );
	VectorCopy( origin, le->ent.origin );

	return le;
}

//=================
//CG_AddLocalEntities
//=================
void CG_AddLocalEntities( void )
{
	lentity_t *le, *next, *hnode;
	entity_t *ent;
	float backfrac, frac, fade;

	hnode = &cg_localents_headnode;
	for( le = hnode->next; le != hnode; le = next )
	{
		next = le->next;

		// it's time to go away
		if( cg.time > le->start + le->time )
		{
			le->type = LE_FREE;
			CG_FreeLocalEntity( le );
			continue;
		}

		frac =  (float)( cg.time - le->start ) / le->time;
		clamp( frac, 0.0f, 1.0f );
		backfrac = 1.0f - frac;
		fade = backfrac * 255.0f;

		ent = &le->ent;

		if( le->light && backfrac )
			CG_AddLightToScene( ent->origin, le->light * backfrac, le->lightcolor[0], le->lightcolor[1], le->lightcolor[2], NULL );

		switch( le->type )
		{
		case LE_NO_FADE:
			ent->scale = le->scale;
			break;
		case LE_RGB_FADE:
			ent->scale = le->scale;
			ent->shaderRGBA[0] = ( qbyte )( fade * le->color[0] );
			ent->shaderRGBA[1] = ( qbyte )( fade * le->color[1] );
			ent->shaderRGBA[2] = ( qbyte )( fade * le->color[2] );
			break;
		case LE_SCALE_RGB_FADE:
			ent->scale = 1.0f + ( frac * le->scale );
			ent->shaderRGBA[0] = ( qbyte )( fade * le->color[0] );
			ent->shaderRGBA[1] = ( qbyte )( fade * le->color[1] );
			ent->shaderRGBA[2] = ( qbyte )( fade * le->color[2] );
			break;
		case LE_INVERSESCALE_RGB_FADE:
			ent->scale = 0.1f + ( backfrac * le->scale );
			ent->shaderRGBA[0] = ( qbyte )( fade * le->color[0] );
			ent->shaderRGBA[1] = ( qbyte )( fade * le->color[1] );
			ent->shaderRGBA[2] = ( qbyte )( fade * le->color[2] );
			break;
		case LE_ALPHA_FADE:
			ent->scale = le->scale;
			ent->shaderRGBA[3] = ( qbyte )( fade * le->color[3] );
			break;
		case LE_SCALE_ALPHA_FADE:
			ent->scale = 1.0f + ( frac * le->scale );
			ent->shaderRGBA[3] = ( qbyte )( fade * le->color[3] );
			break;
		case LE_INVERSESCALE_ALPHA_FADE:
			ent->scale = 0.1f + ( backfrac * le->scale );
			ent->shaderRGBA[3] = ( qbyte )( fade * le->color[3] );
			break;
		default:
			break;
		}

		ent->backlerp = backfrac;

		//if( !le->bounce ) {
		VectorMA( ent->origin, cg.frametime, le->velocity, ent->origin );
		VectorCopy( ent->origin, ent->origin2 );
		/*
		   }
		   else {
		    trace_t	trace;
		    vec3_t	next_origin;

		    VectorMA( ent->origin, time, le->velocity, next_origin );

		    CG_Trace ( &trace, ent->origin, debris_mins, debris_maxs, next_origin, 0, MASK_SOLID );
		    if ( trace.fraction != 1.0 ) //found solid
		    {
		   	float	dot;
		   	vec3_t	vel;
		   	float	xzyspeed;

		   	// Reflect velocity
		   	VectorSubtract( next_origin, ent->origin, vel );
		   	dot = -2 * DotProduct( vel, trace.plane.normal );
		   	VectorMA( vel, dot, trace.plane.normal, le->velocity );
		   	//put new origin in the impact point, but move it out a bit along the normal
		   	VectorMA( trace.endpos, 1, trace.plane.normal, ent->origin );

		   	//the entity has not speed enough. Stop checks
		   	xzyspeed = sqrt(le->velocity[0]*le->velocity[0] + le->velocity[1]*le->velocity[1] + le->velocity[2]*le->velocity[2]);
		   	if( xzyspeed * time < 1.0f) {
		   	    trace_t traceground;
		   	    vec3_t	ground_origin;
		   	    //see if we have ground
		   	    VectorCopy(ent->origin, ground_origin);
		   	    ground_origin[2] += (debris_mins[2] - 4);
		   	    CG_Trace ( &traceground, ent->origin, debris_mins, debris_maxs, ground_origin, 0, MASK_SOLID );
		   	    if( traceground.fraction != 1.0) {
		   		le->bounce = qfalse;
		   		VectorClear(le->velocity);
		   		VectorClear(le->accel);
		   	    }
		   	} else
		   	    VectorScale( le->velocity, le->bounce * time, le->velocity );
		    } else {
		   	VectorCopy( ent->origin, ent->origin2 );
		   	VectorCopy( next_origin, ent->origin );
		    }
		   }
		 */

		VectorCopy( ent->origin, ent->lightingOrigin );
		VectorMA( le->velocity, cg.frametime, le->accel, le->velocity );

		CG_AddEntityToScene( ent );
	}
}

//==================================================
// EFFECTS
//==================================================

//====================
//CG_TestImpact
//====================
void CG_TestImpact( vec3_t origin, vec3_t dir )
{
	lentity_t *le;
	vec3_t angles;

	VecToAngles( dir, angles );

	le = CG_AllocModel( LE_INVERSESCALE_RGB_FADE, origin, angles, 1000,
	                    1, 1, 1, 1,
	                    150, 1, 0.8f, 0,
	                    CG_LocalModel( MODEL_TESTIMPACT ),
	                    NULL );
	le->scale = 2.0f;
	le->ent.rotation = rand() % 360;

	//CG_SpawnDecal( pos, dir, 90, 16, 1, 1, 1, 1, 4, 1, qfalse, CG_MediaShader(cgs.media.shaderPlasmaMark) );
}

void CG_Event_ExplosionOne( vec3_t origin, vec3_t dir )
{
#define EXPLOSIONRADIUS 100
#define EXPLOSIONSPEED 80
	lentity_t *le;

	// ToDo: UnderWater and decals

	VectorMA( origin, EXPLOSIONRADIUS * 0.35f, dir, origin ); // offset it

	le = CG_AllocSprite( LE_SCALE_RGB_FADE, origin, EXPLOSIONRADIUS, 800, 1, 1, 1, 1,
		EXPLOSIONRADIUS * 8,  1, 0.8f, 0, CG_LocalShader( SHADER_EXPLOSION_ONE ) );

	le->ent.rotation = brandom( 0, 360 );
	VectorMA( le->velocity, EXPLOSIONSPEED, dir, le->velocity );

#undef EXPLOSIONRADIUS
#undef EXPLOSIONSPEED
}
