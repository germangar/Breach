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
// g_misc.c

#include "g_local.h"


//========================================================
//
//	MISC_*
//
//========================================================


//===========================================================

//QUAKED misc_portal_surface (1 .5 .25) (-8 -8 -8) (8 8 8)
//The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted. This must be within 64 world units of the surface!
//-------- KEYS --------
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- NOTES --------
//The entity must be no farther than 64 units away from the portal surface to lock onto it. To make a mirror, apply the common/mirror shader to the surface, place this entity near it but don't target a misc_portal_camera.

static void misc_portal_surface_think( gentity_t *ent )
{
	if( !ent->owner || !ent->owner->s.local.inuse )
		VectorCopy( ent->s.ms.origin, ent->s.origin2 );
	else
		VectorCopy( ent->owner->s.ms.origin, ent->s.origin2 );

	ent->nextthink = level.time + 1;
}

static void locateCamera( gentity_t *ent )
{
	vec3_t dir;
	gentity_t *target;
	gentity_t *owner;

	owner = G_PickTarget( ent->target );
	if( !owner )
	{
		GS_Printf( "Couldn't find target for %s\n", ent->classname );
		G_FreeEntity( ent );
		return;
	}

	// swing camera ?
	// modelindex2 holds the rotate speed
	// set to 0 for no rotation at all
	ent->s.modelindex2 = 0;
	if( owner->spawnflags & 4 )
	{
		ent->s.modelindex2 = 50;
		if( owner->spawnflags & 1 )
			ent->s.modelindex2 = 25;
		else if( owner->spawnflags & 2 )
			ent->s.modelindex2 = 75;
	}

	// ignore entities ?
	if( owner->mass )
		ent->s.effects |= EF_NOPORTALENTS;

	ent->owner = owner;
	ent->think = misc_portal_surface_think;
	ent->nextthink = level.time + 1;

	// see if the portal_camera has a target
	if( owner->target )
		target = G_PickTarget( owner->target );
	else
		target = NULL;

	if( target )
	{
		VectorSubtract( target->s.ms.origin, owner->s.ms.origin, dir );
		VectorNormalize( dir );
	}
	else
	{
		G_SetMovedir( owner->s.ms.angles, dir );
	}

	ent->s.skinindex = DirToByte( dir );
	ent->s.effects = owner->count;
}

void G_misc_portal_surface( gentity_t *ent )
{
	VectorClear( ent->s.local.mins );
	VectorClear( ent->s.local.maxs );

	ent->netflags = SVF_PORTAL;
	ent->s.type = ET_PORTALSURFACE;
	ent->s.modelindex1 = 1;

	GClip_LinkEntity( ent );

	// mirror
	if( !ent->target )
	{
		ent->think = misc_portal_surface_think;
		ent->nextthink = level.time + 1;
	}
	else
	{
		ent->think = locateCamera;
		ent->nextthink = level.time + 1000;
	}
}

//===========================================================

//QUAKED misc_portal_camera (1 .5 .25) (-8 -8 -8) (8 8 8) SLOWROTATE FASTROTATE NOROTATE
//Portal camera. This camera is used to project its view onto a portal surface in the level through the intermediary of a misc_portal_surface entity. Use the "angles" key or target a position entity to set the camera's pointing direction.
//-------- KEYS --------
//angles: this sets the pitch and yaw aiming angles of the portal camera (default 0 0). Use "roll" key to set roll angle.
//target : point this to a position entity to set the camera's pointing direction.
//targetname : a misc_portal_surface portal surface indicator must point to this.
//roll: roll angle of camera. A value of 0 is upside down and 180 is the same as the player's view.
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//noents : ignore entities, only render world surfaces
//-------- SPAWNFLAGS --------
//SLOWROTATE : makes the portal camera rotate slowly along the roll axis.
//FASTROTATE : makes the portal camera rotate faster along the roll axis.
//NOROTATE : disables rotation
//-------- NOTES --------
//Both the setting "angles" key or "targeting a position" methods can be used to aim the camera. However, the position method is simpler. In both cases, the "roll" key must be used to set the roll angle. If either the SLOWROTATE or FASTROTATE spawnflag is set, then the "roll" value is irrelevant.

void G_misc_portal_camera( gentity_t *ent )
{
	float roll;
	int noents;

	VectorClear( ent->s.local.mins );
	VectorClear( ent->s.local.maxs );
	GClip_LinkEntity( ent );

	roll = atof( G_GetEntitySpawnKey( "roll", ent ) );
	noents = ( atoi( G_GetEntitySpawnKey( "noents", ent ) ) != 0 ) ? 1 : 0;

	ent->netflags = SVF_NOCLIENT;
	ent->count = (int)( roll / 360.0f * 256.0f );
	ent->mass = noents;
}

/*
QUAKED props_skyportal (.6 .7 .7) (-8 -8 0) (8 8 16)
"fov" for the skybox default is whatever client's fov is set to
"scale" is world/skyarea ratio if you want to keep a certain perspective when player moves around the world
"noents" makes the skyportal ignore entities within the sky area, making them invisible for the player
*/
void G_skyportal( gentity_t *ent )
{
	float fov, scale;
	int noents;

	fov = atof( G_GetEntitySpawnKey( "fov", ent ) );
	if( fov <= 0 )
		fov = 90;
	if( fov > 180 )
		fov = 180;

	scale = atof( G_GetEntitySpawnKey( "scale", ent ) );
	if( scale < 0 )
		scale = 1.0f;

	noents = ( atoi( G_GetEntitySpawnKey( "noents", ent ) ) != 0 ) ? 1 : 0;

	ent->netflags = SVF_NOCLIENT;
	ent->s.type = ET_SKYPORTAL;
	trap_ConfigString( CS_SKYBOX, va( "%.3f %.3f %.3f %.1f %.1f %i %.1f %.1f %.1f", 
		ent->s.ms.origin[0], ent->s.ms.origin[1], ent->s.ms.origin[2],
		fov, scale, noents, ent->s.ms.angles[0], ent->s.ms.angles[1], ent->s.ms.angles[2] ) );
	
	ent->s.effects |= noents;
}
