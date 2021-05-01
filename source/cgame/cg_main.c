/*
 */

#include "cg_local.h"

cg_static_t cgs;
cg_state_t cg;

centity_t cg_entities[MAX_EDICTS];

cvar_t *cg_predict;
cvar_t *cg_predict_optimize;
cvar_t *cg_predict_gun;
cvar_t *cg_predict_thirdperson;
cvar_t *cg_predict_debug;

cvar_t *model;
cvar_t *skin;
cvar_t *hand;

cvar_t *cg_addDecals;
#ifdef CGAMEGETLIGHTORIGIN
cvar_t *cg_shadows;
#endif

cvar_t *developer;

cvar_t *cg_fov;
cvar_t *cg_zoomSens;

cvar_t *cg_drawEntityBoxes;


//============
//CG_API
//============
int CG_API( void )
{
	return CGAME_API_VERSION;
}

//============
//CG_GS_Malloc - Used only for gameshared linking
//============
static void *CG_module_Malloc( size_t size, const char *filename, const int fileline )
{
	return trap_MemAlloc( size, filename, fileline );
}

//============
//CG_GS_Free - Used only for gameshared linking
//============
static void CG_module_Free( void *data, const char *filename, const int fileline )
{
	trap_MemFree( data, filename, fileline );
}

//============
//CG_InitGameShared
//give gameshared access to some utilities
//============
static void CG_InitGameShared( unsigned int maxclients, unsigned int snapFrameTime, int protocol )
{
	gs_moduleapi_t module;

	module.type = GS_MODULE_CGAME;

	module.Malloc = CG_module_Malloc;
	module.Free = CG_module_Free;
	module.trap_Print = trap_Print;
	module.trap_Error = trap_Error;

	// file system
	module.trap_FS_FOpenFile = trap_FS_FOpenFile;
	module.trap_FS_Read = trap_FS_Read;
	module.trap_FS_FCloseFile = trap_FS_FCloseFile;

	// collision traps
	module.trap_CM_NumInlineModels = trap_CM_NumInlineModels;
	module.trap_CM_TransformedPointContents = trap_CM_TransformedPointContents;
	module.trap_CM_InlineModel = trap_CM_InlineModel;
	module.trap_CM_ModelForBBox = trap_CM_ModelForBBox;
	module.trap_CM_InlineModelBounds = trap_CM_InlineModelBounds;
	module.trap_CM_TransformedBoxTrace = trap_CM_TransformedBoxTrace;
	module.trap_CM_BoxLeafnums = trap_CM_BoxLeafnums;
	module.trap_CM_LeafCluster = trap_CM_LeafCluster;

	// the plan is to make this one inside gameshared later on
	module.GetClipStateForDeltaTime = CG_GetClipStateForDeltaTime;

	module.predictedEvent = CG_PredictedEvent;
	module.DrawBox = CG_DrawBox;

	GS_Init( &module, maxclients, snapFrameTime, protocol );
}

//=================
//CG_RegisterVariables
//=================
static void CG_RegisterVariables( void )
{
	cg_predict =	    trap_Cvar_Get( "cg_predict", "1", 0 );
	cg_predict_optimize = trap_Cvar_Get( "cg_predict_optimize", "1", 0 );
	cg_predict_gun = trap_Cvar_Get( "cg_predict_gun", "1", 0 );
	cg_predict_thirdperson = trap_Cvar_Get( "cg_predict_thirdperson", "1", 0 );
	cg_predict_debug =	    trap_Cvar_Get( "cg_predict_debug", "0", 0 );

	model =		    trap_Cvar_Get( "model", "", CVAR_USERINFO | CVAR_ARCHIVE );
	skin =		    trap_Cvar_Get( "skin", "", CVAR_USERINFO | CVAR_ARCHIVE );
	hand =		    trap_Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	cg_fov =	    trap_Cvar_Get( "fov", "90", CVAR_USERINFO | CVAR_ARCHIVE );
	cg_zoomSens =	    trap_Cvar_Get( "zoomsens", "0", CVAR_ARCHIVE );

	cg_drawEntityBoxes = trap_Cvar_Get( "cg_drawEntityBoxes", "0", CVAR_DEVELOPER );

	cg_addDecals =	    trap_Cvar_Get( "cg_decals", "1", CVAR_ARCHIVE );
#ifdef CGAMEGETLIGHTORIGIN
	cg_shadows =		trap_Cvar_Get( "cg_shadows", "1", CVAR_ARCHIVE );
#endif

	developer = trap_Cvar_Get( "developer", "0", CVAR_CHEAT );
}

//============
//CG_Init
//============
void CG_Init( int playerNum, int vidWidth, int vidHeight, qboolean demoplaying, qboolean pure,
			 int maxclients, unsigned int snapFrameTime, int protocol )
{
	CG_InitGameShared( maxclients, snapFrameTime, protocol );

	memset( &cg, 0, sizeof( cg_state_t ) );
	memset( &cgs, 0, sizeof( cg_static_t ) );
	memset( &cgm, 0, sizeof( cgame_media_t ) );

	memset( cg_entities, 0, sizeof( cg_entities ) );

	// save local player number
	cgs.playerNum = playerNum;

	// save current width and height
	cgs.vidWidth = vidWidth;
	cgs.vidHeight = vidHeight;

	// demo
	cgs.demoPlaying = demoplaying;

	// whether to only allow pure files
	cgs.pure = pure;
	cgs.gameProtocol = protocol;
	cgs.snapFrameTime = snapFrameTime;

	CG_RegisterVariables();
	CG_InitView();
	CG_Init2D();

	CG_InitTemporaryBoneposesCache();
	CG_InitSound();
	CG_InitWeapons();

	// effects
	CG_ClearLocalEntities();
	CG_ClearDecals();
	CG_ClearDlights();
	CG_ClearLightStyles();
#ifdef CGAMEGETLIGHTORIGIN
	CG_ClearShadeBoxes();
#endif
	CG_ClearPolys();

	// register fonts here so loading screen works
	CG_RegisterFonts();

	CG_InitCommands(); // do before configstrings so local commands have preference over server ones

	// get configstrings : note: models can't be registered before this
	CG_RegisterConfigStrings();

	CG_PrecacheLocalModels();
	CG_PrecacheLocalSounds();
	CG_PrecacheLocalShaders();

	CG_LoadingScreen_Topicname( "awaiting snapshot..." );

	// start background track
	CG_StartBackgroundTrack();

	cgm.precacheDone = qtrue;
}

//============
//CG_Shutdown
//============
void CG_Shutdown( void )
{
	GS_FreeSpacePartition();
	GS_PlayerClass_FreeAll();
	GS_Buildables_Free();
	CG_ShutdownCommands();
}

//======================================================================

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Error( msg );
}

void Com_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Print( msg );
}
#endif
