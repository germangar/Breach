/*
   Copyright (C) 2007 German Garcia
 */

#include "gs_local.h"

gameshared_locals_t gs;
/*
void *( *_module_Malloc )( size_t size, const char *filename, const int fileline );
void ( *_module_Free )( void *data, const char *filename, const int fileline );
void ( *module_trap_Print )( const char *msg );
void ( *module_trap_Error )( const char *msg );
#define module_Malloc(size) _module_Malloc(size,__FILE__,__LINE__)
#define module_Free(data) _module_Free(data,__FILE__,__LINE__)

// file system
int ( *module_trap_FS_FOpenFile )( const char *filename, int *filenum, int mode );
int ( *module_trap_FS_Read )( void *buffer, size_t len, int file );
void ( *module_trap_FS_FCloseFile )( int file );

// collision traps
int ( *module_trap_CM_NumInlineModels )( void );
int ( *module_trap_CM_TransformedPointContents )( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles );
struct cmodel_s *( *module_trap_CM_InlineModel )( int num );
struct cmodel_s *( *module_trap_CM_ModelForBBox )( vec3_t mins, vec3_t maxs );
void ( *module_trap_CM_InlineModelBounds )( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs );
void ( *module_trap_CM_TransformedBoxTrace )( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles );
int ( *module_trap_CM_BoxLeafnums )( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode );
int ( *module_trap_CM_LeafCluster )( int leafnum );

entity_state_t *( *module_GetClipStateForDeltaTime )( int entNum, int deltaTime );

void ( *module_predictedEvent )( int entNum, int ev, int parm );
void ( *module_DrawBox )( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t angles, vec4_t color );
*/

gs_moduleapi_t module;

//==================================================

/*
* GS_CopyString
*/
char *_GS_CopyString( const char *in, const char *filename, const int fileline )
{
	char *out;

	out = ( char * )module.Malloc( strlen( in ) + 1, filename, fileline );
	strcpy( out, in );

	return out;
}

/*
* GS_Error
*/
void GS_Error( const char *format, ... )
{
	char msg[1024];
	va_list	argptr;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	module.trap_Error( msg );
}

/*
* GS_Printf
*/
void GS_Printf( const char *format, ... )
{
	char msg[1024];
	va_list	argptr;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	module.trap_Print( msg );
}

/*
* GS_Init
*/
void GS_Init( gs_moduleapi_t *moduleapi, unsigned int maxclients, unsigned int snapFrameTime, int protocol )
{
	float env_defaultGravityValue = 980.0f;
	vec3_t env_defaultGravityDirection = { 0, 0, -1 };

	module = *moduleapi;
	if( module.type < GS_MODULE_GAME || module.type >= GS_MODULE_TOTAL )
		GS_Error( "GS_Init: bad module id\n" );

	memset( &gs, 0, sizeof( gameshared_locals_t ) );
	gs.maxclients = maxclients;
	gs.snapFrameTime = snapFrameTime;
	gs.protocol = protocol;

	gs.environment.gravity = env_defaultGravityValue;
	VectorCopy( env_defaultGravityDirection, gs.environment.gravityDir );
	VectorScale( env_defaultGravityDirection, -1, gs.environment.inverseGravityDir );

	GS_Teams_Init();
}
