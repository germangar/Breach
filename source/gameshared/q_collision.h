
#ifndef __Q_COLLISION_H__
#define __Q_COLLISION_H__

#include "q_arch.h"
#include "q_math.h"

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================
//
//COLLISION DETECTION
//
//==============================================================

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1			// an eye is never valid in a solid
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_FOG			64

#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

#define	CONTENTS_TELEPORTER		0x40000
#define	CONTENTS_JUMPPAD		0x80000
#define CONTENTS_CLUSTERPORTAL	0x100000
#define CONTENTS_DONOTENTER		0x200000
#define CONTENTS_PROJECTILE		0x400000    // ignore projectiles when clipping movement, but not by pushers

#define	CONTENTS_ORIGIN			0x1000000   // removed before bsping an entity

#define	CONTENTS_BODY			0x2000000   // should never be on a brush, only in game
#define	CONTENTS_CORPSE			0x4000000
#define	CONTENTS_DETAIL			0x8000000   // brushes not used for the bsp
#define	CONTENTS_STRUCTURAL		0x10000000  // brushes used for the bsp
#define	CONTENTS_TRANSLUCENT	0x20000000  // don't consume surface fragments inside
#define	CONTENTS_TRIGGER		0x40000000
#define	CONTENTS_NODROP			0x80000000  // don't leave bodies or items (death fog, lava)


#define	SURF_NODAMAGE			0x1			// never give falling damage
#define	SURF_SLICK				0x2			// effects game physics
#define	SURF_SKY				0x4			// lighting from environment map
#define	SURF_LADDER				0x8
#define	SURF_NOIMPACT			0x10		// don't make missile explosions
#define	SURF_NOMARKS			0x20		// don't leave missile marks
#define	SURF_FLESH				0x40		// make flesh sounds and effects
#define	SURF_NODRAW				0x80		// don't generate a drawsurface at all
#define	SURF_HINT				0x100		// make a primary bsp splitter
#define	SURF_SKIP				0x200		// completely ignore, allowing non-closed brushes
#define	SURF_NOLIGHTMAP			0x400		// surface doesn't need a lightmap
#define	SURF_POINTLIGHT			0x800		// generate lighting info at vertexes
#define	SURF_METALSTEPS			0x1000		// clanking footsteps
#define	SURF_NOSTEPS			0x2000		// no footstep sounds
#define	SURF_NONSOLID			0x4000		// don't collide against curves with this set
#define SURF_LIGHTFILTER		0x8000		// act as a light filter during q3map -light
#define	SURF_ALPHASHADOW		0x10000		// do per-pixel light shadow casting in q3map
#define	SURF_NODLIGHT			0x20000		// never add dynamic lights
#define SURF_DUST				0x40000		// leave a dust trail when walking on this surface

#define SURF_SNOW				0x80000		// snow steps when walking on this surface
#define SURF_GRASS				0x100000	// grass steps when walking on this surface


// content masks
#define	MASK_ALL			( -1 )
#define	MASK_SOLID			( CONTENTS_SOLID )
#define	MASK_PLAYERSOLID	( CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY )
#define	MASK_DEADSOLID		( CONTENTS_SOLID|CONTENTS_PLAYERCLIP )
#define	MASK_MONSTERSOLID	( CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY )
#define	MASK_WATER			( CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME )
#define	MASK_OPAQUE			( CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA )
#define	MASK_SHOT			( CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE )


// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID	1
#define	AREA_TRIGGERS	2

// a trace is returned when a box is swept through the world
typedef struct
{
	qboolean allsolid;			// if true, plane is not valid
	qboolean startsolid;		// if true, the initial point was in a solid area
	float fraction;				// time completed, 1.0 = didn't hit anything
	vec3_t endpos;				// final position
	cplane_t plane;				// surface normal at impact
	int surfFlags;				// surface hit
	int contents;				// contents on other side of surface hit
	int ent;					// not set by CM_*() functions
} trace_t;


#ifdef __cplusplus
};
#endif

#endif // __Q_COLLISION_H__
