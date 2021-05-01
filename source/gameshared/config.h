/************************************************************************/
/* WARNING                                                              */
/* define this when we compile for a release                            */
/* this will protect dangerous and untested pieces of code              */
/************************************************************************/
//#define PUBLIC_BUILD

//==============================================
//	these defines affect every project file. They are
//	work-in-progress stuff which is, sooner or later,
//	going to be removed by keeping or discarding it.
//==============================================

// pretty solid
#define CGAMEGETLIGHTORIGIN

//#define CM_LEGACYBSP

// collision options
#define CM_ALLOW_ROTATED_BBOXES

// renderer options
#define HALFLAMBERTLIGHTING
//#define CELLSHADEDMATERIAL

//==============================================

// always disabled
//#define SERVER_DOWNLOAD_COMMAND // needs curl on server side
//==============================================
// undecided status

#define PUTCPU2SLEEP

//#define UCMDTIMENUDGE

//#define SV_NO_SNAP_CULL

//#define TRACEVICFIX
//#define TRACE_NOAXIAL

#define MOVE_SNAP_ORIGIN
#ifndef MOVE_SNAP_ORIGIN
    #define FLOATPMOVE  // origin snapping is a must cause of the collision code so there's no point in using float until the collision is fixed
#endif

//#define QUAKE2_JUNK

// collaborations
//==============================================

// symbol address retrieval
//==============================================
// #define SYS_SYMBOL		// adds "sys_symbol" command and symbol exports to binary release
#if defined ( SYS_SYMBOL ) && defined ( _WIN32 )
#define SYMBOL_EXPORT __declspec( dllexport )
#else
#define SYMBOL_EXPORT
#endif
