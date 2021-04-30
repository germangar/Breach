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
// cg_effects.c -- entity effects parsing and management

#include "cg_local.h"

/*
   ==============================================================

   LIGHT STYLE MANAGEMENT

   ==============================================================
 */

typedef struct
{
	int length;
	float value[3];
	float map[MAX_QPATH];
} cg_lightStyle_t;

cg_lightStyle_t	cg_lightStyle[MAX_LIGHTSTYLES];

/*
   ================
   CG_ClearLightStyles
   ================
 */
void CG_ClearLightStyles( void )
{
	memset( cg_lightStyle, 0, sizeof( cg_lightStyle ) );
}

/*
   ================
   CG_RunLightStyles
   ================
 */
void CG_RunLightStyles( void )
{
	int i;
	float f;
	int ofs;
	cg_lightStyle_t	*ls;

	f = cg.time / 100.0f;
	ofs = (int)floor( f );
	f = f - ofs;

	for( i = 0, ls = cg_lightStyle; i < MAX_LIGHTSTYLES; i++, ls++ )
	{
		if( !ls->length )
		{
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}
		if( ls->length == 1 )
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		else
			ls->value[0] = ls->value[1] = ls->value[2] = ( ls->map[ofs % ls->length] * f + ( 1 - f ) * ls->map[( ofs-1 ) % ls->length] );
	}
}

/*
   ================
   CG_SetLightStyle
   ================
 */
void CG_SetLightStyle( int i, const char *cstring )
{
	int j, k;

	if( cstring )
	{
		j = strlen( cstring );
		if( j >= MAX_QPATH )
			GS_Error( "CL_SetLightstyle length = %i", j );
		cg_lightStyle[i].length = j;

		for( k = 0; k < j; k++ )
			cg_lightStyle[i].map[k] = (float)( cstring[k]-'a' ) / (float)( 'm'-'a' );
	}
}

/*
   ================
   CG_AddLightStyles
   ================
 */
void CG_AddLightStyles( void )
{
	int i;
	cg_lightStyle_t	*ls;

	for( i = 0, ls = cg_lightStyle; i < MAX_LIGHTSTYLES; i++, ls++ )
		trap_R_AddLightStyleToScene( i, ls->value[0], ls->value[1], ls->value[2] );
}

/*
   ==============================================================

   DLIGHT MANAGEMENT

   ==============================================================
 */

typedef struct cdlight_s
{
	struct cdlight_s *prev, *next;
	vec3_t color;
	vec3_t origin;
	float radius;
	struct shader_s *shader;
} cdlight_t;

cdlight_t cg_dlights[MAX_DLIGHTS];
cdlight_t cg_dlights_headnode, *cg_free_dlights;

/*
   ================
   CG_ClearDlights
   ================
 */
void CG_ClearDlights( void )
{
	int i;

	memset( cg_dlights, 0, sizeof( cg_dlights ) );

	// link dynamic lights
	cg_free_dlights = cg_dlights;
	cg_dlights_headnode.prev = &cg_dlights_headnode;
	cg_dlights_headnode.next = &cg_dlights_headnode;
	for( i = 0; i < MAX_DLIGHTS - 1; i++ )
		cg_dlights[i].next = &cg_dlights[i+1];
}

/*
   ===============
   CG_AllocDlight
   ===============
 */
static void CG_AllocDlight( vec3_t origin, float radius, float r, float g, float b, struct shader_s *shader )
{
	cdlight_t *dl;

	if( radius <= 0 )
		return;

	if( cg_free_dlights )
	{                   // take a free light if possible
		dl = cg_free_dlights;
		cg_free_dlights = dl->next;
	}
	else
	{                       // grab the oldest one otherwise
		dl = cg_dlights_headnode.prev;
		dl->prev->next = dl->next;
		dl->next->prev = dl->prev;
	}

	dl->radius = radius;
	VectorCopy( origin, dl->origin );
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->shader = shader;

	// put the light at the start of the list
	dl->prev = &cg_dlights_headnode;
	dl->next = cg_dlights_headnode.next;
	dl->next->prev = dl;
	dl->prev->next = dl;
}

void CG_AddLightToScene( vec3_t org, float radius, float r, float g, float b, struct shader_s *shader )
{
	CG_AllocDlight( org, radius, r, g, b, shader );
}

/*
   =================
   CG_FreeDlight
   =================
 */
static void CG_FreeDlight( cdlight_t *dl )
{
	// remove from linked active list
	dl->prev->next = dl->next;
	dl->next->prev = dl->prev;

	// insert into linked free list
	dl->next = cg_free_dlights;
	cg_free_dlights = dl;
}

/*
   ===============
   CG_AddDlights
   ===============
 */
void CG_AddDlights( void )
{
	cdlight_t *dl, *hnode, *next;

	hnode = &cg_dlights_headnode;
	for( dl = hnode->next; dl != hnode; dl = next )
	{
		next = dl->next;

		trap_R_AddLightToScene( dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2], dl->shader );
		CG_FreeDlight( dl );
	}
}

/*
   ==============================================================

   BLOB SHADOWS MANAGEMENT

   ==============================================================
 */
#ifdef CGAMEGETLIGHTORIGIN

#define MAX_CGSHADEBOXES 128

#define MAX_BLOBSHADOW_VERTS 128
#define MAX_BLOBSHADOW_FRAGMENTS 64

typedef struct
{
	vec3_t origin;
	vec3_t mins, maxs;
	int entNum;
	struct shader_s *shader;

	vec3_t verts[MAX_BLOBSHADOW_VERTS];
	vec2_t stcoords[MAX_BLOBSHADOW_VERTS];
	byte_vec4_t colors[MAX_BLOBSHADOW_VERTS];
} cgshadebox_t;

cgshadebox_t cg_shadeBoxes[MAX_CGSHADEBOXES];
static int cg_numShadeBoxes = 0;   // cleared each frame

/*
   ================
   CG_AddBlobShadow

   Ok, to not use decals space we need these arrays to store the
   polygons info. We do not need the linked list nor registration
   ================
 */
static void CG_AddBlobShadow( vec3_t origin, vec3_t dir, float orient, float radius,
                              float r, float g, float b, float a, cgshadebox_t *shadeBox )
{
	int i, j, c, nverts;
	vec3_t axis[3];
	byte_vec4_t color;
	fragment_t *fr, fragments[MAX_BLOBSHADOW_FRAGMENTS];
	int numfragments;
	poly_t poly;
	vec3_t verts[MAX_BLOBSHADOW_VERTS];

	if( radius <= 0 || VectorCompare( dir, vec3_origin ) )
		return; // invalid

	// calculate orientation matrix
	VectorNormalize2( dir, axis[0] );
	PerpendicularVector( axis[1], axis[0] );
	RotatePointAroundVector( axis[2], axis[0], axis[1], orient );
	CrossProduct( axis[0], axis[2], axis[1] );

	numfragments = trap_R_GetClippedFragments( origin, radius, axis, // clip it
	                                           MAX_BLOBSHADOW_VERTS, verts, MAX_BLOBSHADOW_FRAGMENTS, fragments );

	// no valid fragments
	if( !numfragments )
		return;

	// clamp and scale colors
	if( r < 0 ) r = 0;else if( r > 1 ) r = 255;else r *= 255;
	if( g < 0 ) g = 0;else if( g > 1 ) g = 255;else g *= 255;
	if( b < 0 ) b = 0;else if( b > 1 ) b = 255;else b *= 255;
	if( a < 0 ) a = 0;else if( a > 1 ) a = 255;else a *= 255;

	color[0] = ( qbyte )( r );
	color[1] = ( qbyte )( g );
	color[2] = ( qbyte )( b );
	color[3] = ( qbyte )( a );
	c = *( int * )color;

	radius = 0.5f / radius;
	VectorScale( axis[1], radius, axis[1] );
	VectorScale( axis[2], radius, axis[2] );

	for( i = 0, nverts = 0, fr = fragments; i < numfragments; i++, fr++ )
	{
		if( nverts+fr->numverts > MAX_BLOBSHADOW_VERTS )
			return;
		if( fr->numverts <= 0 )
			continue;

		poly.shader = shadeBox->shader;
		poly.verts = &shadeBox->verts[nverts];
		poly.stcoords = &shadeBox->stcoords[nverts];
		poly.colors = &shadeBox->colors[nverts];
		poly.numverts = fr->numverts;
		poly.fognum = fr->fognum;
		VectorCopy( axis[0], poly.normal );
		nverts += fr->numverts;

		for( j = 0; j < fr->numverts; j++ )
		{
			vec3_t v;

			VectorCopy( verts[fr->firstvert+j], poly.verts[j] );
			VectorSubtract( poly.verts[j], origin, v );
			poly.stcoords[j][0] = DotProduct( v, axis[1] ) + 0.5f;
			poly.stcoords[j][1] = DotProduct( v, axis[2] ) + 0.5f;
			*( int * )poly.colors[j] = c;
		}

		trap_R_AddPolyToScene( &poly );
	}
}

/*
   ================
   CG_ClearShadeBoxes
   ================
 */
void CG_ClearShadeBoxes( void )
{
	cg_numShadeBoxes = 0;
	memset( cg_shadeBoxes, 0, sizeof( cg_shadeBoxes ) );
}

/*
   ===============
   CG_AllocShadeBox
   ===============
 */
void CG_AllocShadeBox( int entNum, const vec3_t origin, const vec3_t mins, const vec3_t maxs, struct shader_s *shader )
{
	float dist;
	vec3_t dir;
	cgshadebox_t *sb;

	if( cg_shadows->integer != 1 )
		return;
	if( cg_numShadeBoxes == MAX_CGSHADEBOXES )
		return;

	// Kill if behind the view or if too far away
	VectorSubtract( origin, cg.view.origin, dir );
	dist = VectorNormalize2( dir, dir ) * cg.view.fracDistFOV;
	if( dist > 1024 )
		return;

	if( DotProduct( dir, cg.view.axis[FORWARD] ) < 0 )
		return;

	sb = &cg_shadeBoxes[cg_numShadeBoxes++];
	VectorCopy( origin, sb->origin );
	VectorCopy( mins, sb->mins );
	VectorCopy( maxs, sb->maxs );
	sb->entNum = entNum;
	sb->shader = shader;
	if( !sb->shader )
		sb->shader = CG_LocalShader( SHADER_PLAYERSHADOW );
}

/*
   ===============
   CG_AddShadeBoxes - Which in reality means CalcBlobShadows
   Note:	This function should be called after every dynamic light has been added to the rendering list.
   	ShadeBoxes exist for the solely reason of waiting until all dlights are sent before doing the shadows.
   ===============
 */
#define SHADOW_PROJECTION_DISTANCE 96
#define SHADOW_MAX_SIZE 100
#define SHADOW_MIN_SIZE 24

void CG_AddShadeBoxes( void )
{
	// ok, what we have to do here is finding the light direction of each of the shadeboxes origins
	int i;
	cgshadebox_t *sb;
	vec3_t lightdir, end, sborigin;
	trace_t	trace;

	if( cg_shadows->integer != 1 )
		return;

	for( i = 0, sb = cg_shadeBoxes; i < cg_numShadeBoxes; i++, sb++ )
	{
		VectorClear( lightdir );
		trap_R_LightForOrigin( sb->origin, lightdir, NULL, NULL, RadiusFromBounds( sb->mins, sb->maxs ) );

		// move the point we will project close to the bottom of the bbox (so shadow doesn't dance much to the sides)
		VectorSet( sborigin, sb->origin[0], sb->origin[1], sb->origin[2] + sb->mins[2] + 8 );
		VectorMA( sborigin, -SHADOW_PROJECTION_DISTANCE, lightdir, end );
		//CG_QuickPolyBeam( sborigin, end, 4, NULL, colorWhite ); // lightdir testline
		GS_Trace( &trace, sborigin, vec3_origin, vec3_origin, end, sb->entNum, MASK_OPAQUE, 0 );
		if( trace.fraction < 1.0f )
		{                     // we have a shadow
			float blobradius;
			float alpha, maxalpha = 0.95f;
			vec3_t shangles;

			VecToAngles( lightdir, shangles );
			blobradius = SHADOW_MIN_SIZE + trace.fraction * ( SHADOW_MAX_SIZE-SHADOW_MIN_SIZE );

			alpha = ( 1.0f - trace.fraction ) * maxalpha;

			CG_AddBlobShadow( trace.endpos, trace.plane.normal, shangles[YAW], blobradius,
			                  1, 1, 1, alpha, sb );
		}
	}

	// clean up the polygons list from old frames
	cg_numShadeBoxes = 0;
}
#endif

/*
==============================================================

TEMPORARY (ONE-FRAME) DECALS

==============================================================
*/

#define MAX_TEMPDECALS				32		// in fact, a semi-random multiplier
#define MAX_TEMPDECAL_VERTS			128
#define MAX_TEMPDECAL_FRAGMENTS		64

static int cg_numDecalVerts = 0;

/*
================
CG_ClearFragmentedDecals
================
*/
void CG_ClearFragmentedDecals( void )
{
	cg_numDecalVerts = 0;
}

/*
================
CG_AddFragmentedDecal
================
*/
void CG_AddFragmentedDecal( vec3_t origin, vec3_t dir, float orient, float radius,
							 float r, float g, float b, float a, struct shader_s *shader )
{
	int i, j, c;
	vec3_t axis[3];
	byte_vec4_t color;
	fragment_t *fr, fragments[MAX_TEMPDECAL_FRAGMENTS];
	int numfragments;
	poly_t poly;
	vec3_t verts[MAX_BLOBSHADOW_VERTS];
	static vec3_t t_verts[MAX_TEMPDECAL_VERTS*MAX_TEMPDECALS];
	static vec2_t t_stcoords[MAX_TEMPDECAL_VERTS*MAX_TEMPDECALS];
	static byte_vec4_t t_colors[MAX_TEMPDECAL_VERTS*MAX_TEMPDECALS];

	if( radius <= 0 || VectorCompare( dir, vec3_origin ) )
		return; // invalid

	// calculate orientation matrix
	VectorNormalize2( dir, axis[0] );
	PerpendicularVector( axis[1], axis[0] );
	RotatePointAroundVector( axis[2], axis[0], axis[1], orient );
	CrossProduct( axis[0], axis[2], axis[1] );

	numfragments = trap_R_GetClippedFragments( origin, radius, axis, // clip it
		MAX_BLOBSHADOW_VERTS, verts, MAX_TEMPDECAL_FRAGMENTS, fragments );

	// no valid fragments
	if( !numfragments )
		return;

	// clamp and scale colors
	if( r < 0 ) r = 0;else if( r > 1 ) r = 255;else r *= 255;
	if( g < 0 ) g = 0;else if( g > 1 ) g = 255;else g *= 255;
	if( b < 0 ) b = 0;else if( b > 1 ) b = 255;else b *= 255;
	if( a < 0 ) a = 0;else if( a > 1 ) a = 255;else a *= 255;

	color[0] = ( qbyte )( r );
	color[1] = ( qbyte )( g );
	color[2] = ( qbyte )( b );
	color[3] = ( qbyte )( a );
	c = *( int * )color;

	radius = 0.5f / radius;
	VectorScale( axis[1], radius, axis[1] );
	VectorScale( axis[2], radius, axis[2] );

	for( i = 0, fr = fragments; i < numfragments; i++, fr++ )
	{
		if( cg_numDecalVerts+fr->numverts > sizeof( t_verts ) / sizeof( t_verts[0] ) )
			return;
		if( fr->numverts <= 0 )
			continue;

		poly.shader = shader;
		poly.verts = &t_verts[cg_numDecalVerts];
		poly.stcoords = &t_stcoords[cg_numDecalVerts];
		poly.colors = &t_colors[cg_numDecalVerts];
		poly.numverts = fr->numverts;
		poly.fognum = fr->fognum;
		VectorCopy( axis[0], poly.normal );
		cg_numDecalVerts += fr->numverts;

		for( j = 0; j < fr->numverts; j++ )
		{
			vec3_t v;

			VectorCopy( verts[fr->firstvert+j], poly.verts[j] );
			VectorSubtract( poly.verts[j], origin, v );
			poly.stcoords[j][0] = DotProduct( v, axis[1] ) + 0.5f;
			poly.stcoords[j][1] = DotProduct( v, axis[2] ) + 0.5f;
			*( int * )poly.colors[j] = c;
		}

		trap_R_AddPolyToScene( &poly );
	}
}
