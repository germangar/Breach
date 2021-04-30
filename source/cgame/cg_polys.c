/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

//==================================================
// POLYGONS
//==================================================

#define MAX_CGPOLYS		    800
#define MAX_CGPOLY_VERTS	    16

// poly effects flags
#define	CGPOLY_EF_NOORIENT		    0x00000001
#define	CGPOLY_EF_AUTOSPRITE_PITCH	    0x00000002
#define	CGPOLY_EF_AUTOSPRITE_YAW	    0x00000004
#define CGPOLY_EF_AUTOSPRITE_ROLL	    0x00000008
#define	CGPOLY_EF_DRAWONCE		    0x00000010


typedef struct cpoly_s
{
	struct cpoly_s *prev, *next;

	struct shader_s	*shader;
	unsigned int die;                   // remove after this time
	unsigned int fadetime;
	float fadefreq;
	float color[4];


	poly_t *poly;

	int effects;
	vec3_t verts[MAX_CGPOLY_VERTS];
	vec3_t origin;
	vec3_t angles;

} cpoly_t;

static cpoly_t cg_polys[MAX_CGPOLYS];
static cpoly_t cg_polys_headnode, *cg_free_polys;

static poly_t cg_poly_polys[MAX_CGPOLYS];
static vec3_t cg_poly_verts[MAX_CGPOLYS][MAX_CGPOLY_VERTS];
static vec2_t cg_poly_stcoords[MAX_CGPOLYS][MAX_CGPOLY_VERTS];
static byte_vec4_t cg_poly_colors[MAX_CGPOLYS][MAX_CGPOLY_VERTS];

//=================
//CG_Clearpolys
//=================
void CG_ClearPolys( void )
{
	int i;

	memset( cg_polys, 0, sizeof( cg_polys ) );

	// link polys
	cg_free_polys = cg_polys;
	cg_polys_headnode.prev = &cg_polys_headnode;
	cg_polys_headnode.next = &cg_polys_headnode;
	for( i = 0; i < MAX_CGPOLYS; i++ )
	{
		if( i < MAX_CGPOLYS - 1 )
		{
			cg_polys[i].next = &cg_polys[i+1];
		}
		cg_polys[i].poly = &cg_poly_polys[i];
		cg_polys[i].poly->verts = cg_poly_verts[i];
		cg_polys[i].poly->stcoords = cg_poly_stcoords[i];
		cg_polys[i].poly->colors = cg_poly_colors[i];
	}
}

//=================
//CG_Allocpoly
//
//Returns either a free poly or the oldest one
//=================
static cpoly_t *CG_AllocPoly( void )
{
	cpoly_t *pl;

	if( cg_free_polys )
	{                   // take a free poly if possible
		pl = cg_free_polys;
		cg_free_polys = pl->next;
	}
	else
	{                   // grab the oldest one otherwise
		pl = cg_polys_headnode.prev;
		pl->prev->next = pl->next;
		pl->next->prev = pl->prev;
	}

	// put the poly at the start of the list
	pl->prev = &cg_polys_headnode;
	pl->next = cg_polys_headnode.next;
	pl->next->prev = pl;
	pl->prev->next = pl;

	return pl;
}

//=================
//CG_FreePoly
//=================
static void CG_FreePoly( cpoly_t *dl )
{
	// remove from linked active list
	dl->prev->next = dl->next;
	dl->next->prev = dl->prev;

	// insert into linked free list
	dl->next = cg_free_polys;
	cg_free_polys = dl;
}

//=================
//CG_SpawnPolygon
//=================
static cpoly_t *CG_SpawnPolygon( float r, float g, float b, float a,
                                 float die, float fadetime, struct shader_s *shader )
{
	cpoly_t *pl;
	float dietime, fadefreq;

	dietime = cg.time + die * 1000;
	fadefreq = 0.001f / min( fadetime, die );
	fadetime = cg.time + ( die - min( fadetime, die ) ) * 1000;

	// allocate poly
	pl = CG_AllocPoly();
	pl->die = dietime;
	pl->fadetime = fadetime;
	pl->fadefreq = fadefreq;
	pl->shader = shader;
	pl->color[0] = r;
	pl->color[1] = g;
	pl->color[2] = b;
	pl->color[3] = a;
	clamp( pl->color[0], 0.0f, 1.0f );
	clamp( pl->color[1], 0.0f, 1.0f );
	clamp( pl->color[2], 0.0f, 1.0f );
	clamp( pl->color[3], 0.0f, 1.0f );
	pl->effects = 0;

	if( !die )
	{
		pl->die = cg.time + 100;
		pl->effects |= CGPOLY_EF_DRAWONCE;
	}

	return pl;
}

//=================
//CG_OrientPolygon
//=================
static void CG_OrientPolygon( vec3_t origin, vec3_t angles, poly_t *poly )
{
	int i;
	vec3_t perp;
	vec3_t ax[3], localAxis[3];

	AnglesToAxis( angles, ax );
	Matrix_Transpose( ax, localAxis );
	for( i = 0; i < poly->numverts; i++ )
	{
		Matrix_TransformVector( localAxis, poly->verts[i], perp );
		VectorAdd( perp, origin, poly->verts[i] );
	}
}

//=================
//CG_SpawnPolyBeam
// spawns a polygon from start to end points length and given width.
// shaderlenght makes reference to size of the texture it will draw, so it can be repeated.
//=================
static cpoly_t *CG_SpawnPolyBeam( vec3_t start, vec3_t end, int width, float dietime, float fadetime, struct shader_s *shader, int shaderlength, vec4_t color )
{
	cpoly_t *cgpoly;
	poly_t *poly;
	vec3_t angles, dir;
	int i;
	float xmin, ymin, xmax, ymax;
	float stx = 1.0f, sty = 1.0f;

	//find out beam polygon sizes
	VectorSubtract( end, start, dir );
	VecToAngles( dir, angles );
	xmin = 0;
	xmax = VectorNormalize( dir );
	ymin = -( width*0.5 );
	ymax = width*0.5;

	if( xmax > shaderlength )
	{
		stx = xmax/shaderlength;
	}

	if( !color )
		color = colorWhite;

	cgpoly = CG_SpawnPolygon( color[0], color[1], color[2], color[3], dietime, fadetime, shader );

	VectorCopy( angles, cgpoly->angles );
	VectorCopy( start, cgpoly->origin );
	cgpoly->effects = CGPOLY_EF_AUTOSPRITE_ROLL;
	if( !dietime )
		cgpoly->effects |= CGPOLY_EF_DRAWONCE;

	// create the polygon inside the cgpolygon
	poly = cgpoly->poly;
	poly->shader = cgpoly->shader;
	poly->numverts = 0;
	poly->fognum = 0;

	//A
	VectorSet( poly->verts[poly->numverts], xmin, 0, ymin );
	poly->stcoords[poly->numverts][0] = 0;
	poly->stcoords[poly->numverts][1] = 0;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	//B
	VectorSet( poly->verts[poly->numverts], xmin, 0, ymax );
	poly->stcoords[poly->numverts][0] = 0;
	poly->stcoords[poly->numverts][1] = sty;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	//C
	VectorSet( poly->verts[poly->numverts], xmax, 0, ymax );
	poly->stcoords[poly->numverts][0] = stx;
	poly->stcoords[poly->numverts][1] = sty;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	//D
	VectorSet( poly->verts[poly->numverts], xmax, 0, ymin );
	poly->stcoords[poly->numverts][0] = stx;
	poly->stcoords[poly->numverts][1] = 0;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	//the verts data is stored inside cgpoly, cause it can be moved later
	for( i = 0; i < poly->numverts; i++ )
		VectorCopy( poly->verts[i], cgpoly->verts[i] );

	return cgpoly;
}

//=================
//CG_Addpolys
//=================
void CG_AddPolys( void )
{
	int i;
	float fade;
	cpoly_t	*cgpoly, *next, *hnode;
	poly_t *poly;
	static vec3_t angles;

	// add polys in first-spawned - first-drawn order
	hnode = &cg_polys_headnode;
	for( cgpoly = hnode->prev; cgpoly != hnode; cgpoly = next )
	{
		next = cgpoly->prev;

		// it's time to DIE
		if( cgpoly->die <= cg.time )
		{
			CG_FreePoly( cgpoly );
			continue;
		}
		poly = cgpoly->poly;

		// composite the poly with cgpoly's data
		if( !( cgpoly->effects & CGPOLY_EF_NOORIENT ) )
		{

			for( i = 0; i < poly->numverts; i++ )
				VectorCopy( cgpoly->verts[i], poly->verts[i] );
			for( i = 0; i < 3; i++ )
				angles[i] = anglemod( cgpoly->angles[i] );

			if( cgpoly->effects & CGPOLY_EF_AUTOSPRITE_PITCH )
			{
			}

			if( cgpoly->effects & CGPOLY_EF_AUTOSPRITE_YAW )
			{
			}

			if( cgpoly->effects & CGPOLY_EF_AUTOSPRITE_ROLL )
			{
			}

			CG_OrientPolygon( cgpoly->origin, angles, poly );
		}

		// fade out
		if( cgpoly->fadetime < cg.time )
		{
			fade = ( cgpoly->die - cg.time ) * cgpoly->fadefreq;
			for( i = 0; i < poly->numverts; i++ )
			{
				poly->colors[i][0] = ( qbyte )( cgpoly->color[0] * fade * 255 );
				poly->colors[i][1] = ( qbyte )( cgpoly->color[1] * fade * 255 );
				poly->colors[i][2] = ( qbyte )( cgpoly->color[2] * fade * 255 );
				poly->colors[i][3] = ( qbyte )( cgpoly->color[3] * fade * 255 );
			}
		}

		if( cgpoly->effects & CGPOLY_EF_DRAWONCE )
		{
			cgpoly->die = cg.time; // will be ignored and freed the next time
		}

		trap_R_AddPolyToScene( poly );
	}
}

//==================================================
// POLYGONAL EFFECTS
//==================================================

//=================
//CG_QuickPolyBeam
//=================
void CG_QuickPolyBeam( vec3_t start, vec3_t end, int width, struct shader_s *shader, vec4_t color )
{
	cpoly_t *cgpoly, *cgpoly2;

	if( !color )
		color = colorWhite;

	if( !shader )
		shader = CG_LocalShader( SHADER_LINEBEAM );

	cgpoly = CG_SpawnPolyBeam( start, end, width, 0, 0, shader, 128, color );
	cgpoly->effects |= CGPOLY_EF_DRAWONCE;

	// since autosprite doesn't work, spawn a second and rotate it 90º
	cgpoly2 = CG_SpawnPolyBeam( start, end, width, 0, 0, shader, 128, color );
	cgpoly2->angles[ROLL] += 90;
	cgpoly2->effects |= CGPOLY_EF_DRAWONCE;
}

//=================
//CG_DrawBox
//=================
void CG_DrawBox( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t angles, vec4_t color )
{
	vec3_t start, end, vec;
	float linewidth = 4;
	vec3_t localAxis[3];
#if 1
	vec3_t ax[3];
	AnglesToAxis( angles, ax );
	Matrix_Transpose( ax, localAxis );
#else
	Matrix_Copy( axis_identity, localAxis );
	if( angles[YAW] ) Matrix_Rotate( localAxis, -angles[YAW], 0, 0, 1 );
	if( angles[PITCH] ) Matrix_Rotate( localAxis, -angles[PITCH], 0, 1, 0 );
	if( angles[ROLL] ) Matrix_Rotate( localAxis, -angles[ROLL], 1, 0, 0 );
#endif

	//horizontal projection
	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = mins[0];
	end[1] = mins[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = mins[0];
	start[1] = maxs[1];
	start[2] = mins[2];

	end[0] = mins[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = maxs[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = mins[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = maxs[0];
	start[1] = maxs[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	//x projection
	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = mins[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = mins[0];
	start[1] = maxs[1];
	start[2] = maxs[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = mins[0];
	start[1] = maxs[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = maxs[2];

	end[0] = maxs[0];
	end[1] = mins[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	//z projection
	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = mins[0];
	end[1] = maxs[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = maxs[0];
	start[1] = mins[1];
	start[2] = maxs[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = maxs[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );

	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = maxs[2];

	end[0] = mins[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL, color );
}
