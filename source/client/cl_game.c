/*
Copyright (C) 2002-2003 Victor Luchits

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
#include "client.h"

static cgame_export_t *cge;

EXTERN_API_FUNC void *GetCGameAPI( void * );

static mempool_t *cl_gamemodulepool;
static void *module_handle;

/*
* CL_GameModule_Error
*/
static void CL_GameModule_Error( const char *msg )
{
	Com_Error( ERR_DROP, "%s", msg );
}

/*
* CL_GameModule_Print
*/
static void CL_GameModule_Print( const char *msg )
{
	Com_Printf( "%s", msg );
}

/*
* CL_GameModule_GetSnapshot
*/
static const snapshot_t *CL_GameModule_GetSnapshot( int snapNum )
{
	return &cl.snapShots[snapNum & SNAPS_BACKUP_MASK];
}

/*
* CL_GameModule_GetEntityFromSnapsBackup
*/
static const entity_state_t *CL_GameModule_GetEntityFromSnapsBackup( unsigned int snapNum, int entNum )
{
	assert( entNum >= 0 && entNum < MAX_EDICTS );
	return &cl.snapsData.entityStateBackups[( snapNum & SNAPS_BACKUP_MASK )][entNum];
}

/*
* CL_GameModule_GetPlayerStateFromSnapsBackup
*/
static const player_state_t *CL_GameModule_GetPlayerStateFromSnapsBackup( unsigned int snapNum )
{
	assert( snapNum > 0 );
	return &cl.snapsData.playerStateBackups[( snapNum & SNAPS_BACKUP_MASK )];
}

/*
* CL_GameModule_GetGameStateFromSnapsBackup
*/
static const game_state_t *CL_GameModule_GetGameStateFromSnapsBackup( unsigned int snapNum )
{
	assert( snapNum > 0 );
	return &cl.snapsData.gameStateBackups[( snapNum & SNAPS_BACKUP_MASK )];
}

/*
* CL_GameModule_GetConfigString
*/
static const char *CL_GameModule_GetConfigString( int i )
{
	if( i < 0 || i >= MAX_CONFIGSTRINGS )
	{
		Com_DPrintf( S_COLOR_RED "CL_GameModule_GetConfigString: i > MAX_CONFIGSTRINGS" );
		return NULL;
	}
	return cl.configstrings[i];
}

/*
* CL_GameModule_RefreshMouseAngles
*/
static void CL_GameModule_RefreshMouseAngles( void )
{
	CL_UpdateCommandInput(); // force a mouse refresh when requesting the in-work user command
}

/*
* CL_GameModule_NET_GetUserCmd
*/
static void CL_GameModule_NET_GetUserCmd( int frame, usercmd_t *cmd )
{
	if( cmd )
	{
		if( frame < 0 )
			frame = 0;

		*cmd = cl.cmds[frame & CMD_MASK];
	}
}

/*
* CL_GameModule_NET_GetCurrentUserCmdNum
*/
static int CL_GameModule_NET_GetCurrentUserCmdNum( void )
{
	return cls.ucmdHead;
}

/*
* CL_GameModule_NET_GetCurrentState
*/
static void CL_GameModule_NET_GetCurrentState( int *ucmdAcknowledged, int *ucmdHead, int *ucmdSent )
{
	if( ucmdAcknowledged )
		*ucmdAcknowledged = cls.ucmdAcknowledged;
	if( ucmdHead )
		*ucmdHead = cls.ucmdHead;
	if( ucmdSent )
		*ucmdSent = cls.ucmdSent;
}

/*
* CL_GameModule_R_RegisterWorldModel
*/
static void CL_GameModule_R_RegisterWorldModel( const char *model )
{
	R_RegisterWorldModel( model, cl.cms ? CM_PVSData( cl.cms ) : NULL, cl.cms ? CM_PHSData( cl.cms ) : NULL );
}

/*
* CL_GameModule_MemAlloc
*/
static void *CL_GameModule_MemAlloc( size_t size, const char *filename, int fileline )
{
	return _Mem_Alloc( cl_gamemodulepool, size, MEMPOOL_CLIENTGAME, 0, filename, fileline );
}

/*
* CL_GameModule_MemFree
*/
static void CL_GameModule_MemFree( void *data, const char *filename, int fileline )
{
	_Mem_Free( data, MEMPOOL_CLIENTGAME, 0, filename, fileline );
}

/*
* CL_GameModule_SoundUpdate
*/
static void CL_GameModule_SoundUpdate( const vec3_t origin, const vec3_t velocity, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up )
{
	CL_SoundModule_Update( origin, velocity, v_forward, v_right, v_up, CL_WriteAvi() && cls.demo.avi_audio );
}

static int CL_GameModule_CM_NumInlineModels( void )
{
	return CM_NumInlineModels( cl.cms );
}

static struct cmodel_s *CL_GameModule_CM_InlineModel( int num )
{
	return CM_InlineModel( cl.cms, num );
}

static struct cmodel_s	*CL_GameModule_CM_ModelForBBox( vec3_t mins, vec3_t maxs )
{
	return CM_ModelForBBox( cl.cms, mins, maxs );
}

static void CL_GameModule_CM_TransformedBoxTrace( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles )
{
	CM_TransformedBoxTrace( cl.cms, tr, start, end, mins, maxs, cmodel, brushmask, origin, angles );
}

static int CL_GameModule_CM_TransformedPointContents( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles )
{
	return CM_TransformedPointContents( cl.cms, p, cmodel, origin, angles );
}

static void CL_GameModule_CM_InlineModelBounds( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs )
{
	CM_InlineModelBounds( cl.cms, cmodel, mins, maxs );
}

static int CL_GameModule_CM_BoxLeafnums( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode )
{
	return CM_BoxLeafnums( cl.cms, mins, maxs, list, listsize, topnode );
}

static int CL_GameModule_CM_LeafCluster( int leafnum )
{
	return CM_LeafCluster( cl.cms, leafnum );
}

/*
* CL_GameModule_Init
*/
void CL_GameModule_Init( void )
{
	int apiversion, oldState;
	unsigned int start;
	cgame_import_t import;
	void *( *builtinAPIfunc )(void *) = NULL;
#ifdef CGAME_HARD_LINKED
	builtinAPIfunc = GetCGameAPI;
#endif

	// unload anything we have now
	CL_SoundModule_StopAllSounds();
	CL_GameModule_Shutdown();

	cl_gamemodulepool = _Mem_AllocPool( NULL, "Client Game Progs", MEMPOOL_CLIENTGAME, __FILE__, __LINE__ );

	import.Error = CL_GameModule_Error;
	import.Print = CL_GameModule_Print;

	import.GetSnapshot = CL_GameModule_GetSnapshot;
	import.GetEntityFromSnapsBackup = CL_GameModule_GetEntityFromSnapsBackup;
	import.GetPlayerStateFromSnapsBackup = CL_GameModule_GetPlayerStateFromSnapsBackup;
	import.GetGameStateFromSnapsBackup = CL_GameModule_GetGameStateFromSnapsBackup;

	import.Cvar_Get = Cvar_Get;
	import.Cvar_Set = Cvar_Set;
	import.Cvar_SetValue = Cvar_SetValue;
	import.Cvar_ForceSet = Cvar_ForceSet;
	import.Cvar_String = Cvar_String;
	import.Cvar_Value = Cvar_Value;

	import.Cmd_Argc = Cmd_Argc;
	import.Cmd_Argv = Cmd_Argv;
	import.Cmd_Args = Cmd_Args;

	import.Cmd_AddCommand = Cmd_AddCommand;
	import.Cmd_RemoveCommand = Cmd_RemoveCommand;
	import.Cmd_ExecuteText = Cbuf_ExecuteText;
	import.Cmd_Execute = Cbuf_Execute;

	import.FS_FOpenFile = FS_FOpenFile;
	import.FS_Read = FS_Read;
	import.FS_Write = FS_Write;
	import.FS_Print = FS_Print;
	import.FS_Tell = FS_Tell;
	import.FS_Seek = FS_Seek;
	import.FS_Eof = FS_Eof;
	import.FS_Flush = FS_Flush;
	import.FS_FCloseFile = FS_FCloseFile;
	import.FS_GetFileList = FS_GetFileList;
	import.FS_FirstExtension = FS_FirstExtension;
	import.FS_IsPureFile = FS_IsPureFile;

	import.Key_GetBindingBuf = Key_GetBindingBuf;
	import.Key_KeynumToString = Key_KeynumToString;

	import.GetConfigString = CL_GameModule_GetConfigString;
	import.Milliseconds = Sys_Milliseconds;
	import.DownloadRequest = CL_DownloadRequest;

	import.RefreshMouseAngles = CL_GameModule_RefreshMouseAngles;
	import.NET_GetUserCmd = CL_GameModule_NET_GetUserCmd;
	import.NET_GetCurrentUserCmdNum = CL_GameModule_NET_GetCurrentUserCmdNum;
	import.NET_GetCurrentState = CL_GameModule_NET_GetCurrentState;

	import.R_UpdateScreen = SCR_UpdateScreen;
	import.R_GetClippedFragments = R_GetClippedFragments;
	import.R_ClearScene = R_ClearScene;
	import.R_AddEntityToScene = R_AddEntityToScene;
	import.R_AddLightToScene = R_AddLightToScene;
	import.R_AddPolyToScene = R_AddPolyToScene;
	import.R_AddLightStyleToScene = R_AddLightStyleToScene;
	import.R_RenderScene = R_RenderScene;
	import.R_SpeedsMessage = R_SpeedsMessage;
	import.R_RegisterWorldModel = CL_GameModule_R_RegisterWorldModel;
	import.R_ModelBounds = R_ModelBounds;
	import.R_RegisterModel = R_RegisterModel;
	import.R_RegisterPic = R_RegisterPic;
	import.R_RegisterSkin = R_RegisterSkin;
	import.R_RegisterSkinFile = R_RegisterSkinFile;
	import.R_LerpTag = R_LerpTag;
	import.R_LightForOrigin = R_LightForOrigin;
	import.R_SetCustomColor = R_SetCustomColor;
	import.R_DrawStretchPic = R_DrawStretchPic;
	import.R_TransformVectorToScreen = R_TransformVectorToScreen;
	import.R_SkeletalGetNumBones = R_SkeletalGetNumBones;
	import.R_SkeletalGetBoneInfo = R_SkeletalGetBoneInfo;
	import.R_SkeletalGetBonePose = R_SkeletalGetBonePose;

	import.CM_NumInlineModels = CL_GameModule_CM_NumInlineModels;
	import.CM_InlineModel = CL_GameModule_CM_InlineModel;
	import.CM_TransformedBoxTrace = CL_GameModule_CM_TransformedBoxTrace;
	import.CM_TransformedPointContents = CL_GameModule_CM_TransformedPointContents;
	import.CM_ModelForBBox = CL_GameModule_CM_ModelForBBox;
	import.CM_InlineModelBounds = CL_GameModule_CM_InlineModelBounds;
	import.CM_BoxLeafnums = CL_GameModule_CM_BoxLeafnums;
	import.CM_LeafCluster = CL_GameModule_CM_LeafCluster;
	import.CM_LoadMapMessage = CM_LoadMapMessage;

	import.S_RegisterSound = CL_SoundModule_RegisterSound;
	import.S_StartFixedSound = CL_SoundModule_StartFixedSound;
	import.S_StartRelativeSound = CL_SoundModule_StartRelativeSound;
	import.S_StartGlobalSound = CL_SoundModule_StartGlobalSound;
	import.S_Update = CL_GameModule_SoundUpdate;
	import.S_AddLoopSound = CL_SoundModule_AddLoopSound;
	import.S_StartBackgroundTrack = CL_SoundModule_StartBackgroundTrack;
	import.S_StopBackgroundTrack = CL_SoundModule_StopBackgroundTrack;

	import.SCR_RegisterFont = SCR_RegisterFont;
	import.SCR_DrawString = SCR_DrawString;
	import.SCR_DrawStringWidth = SCR_DrawStringWidth;
	import.SCR_DrawClampString = SCR_DrawClampString;
	import.SCR_strHeight = SCR_strHeight;
	import.SCR_strWidth = SCR_strWidth;
	import.SCR_StrlenForWidth = SCR_StrlenForWidth;

	import.UI_InitWindow = CL_UI_InitWindow;
	import.UI_OpenWindow = CL_UI_OpenWindow;
	import.UI_CloseWindow = CL_UI_CloseWindow;
	import.UI_ApplyWindowChanges = CL_UI_ApplyWindowChanges;
	import.UI_InitItemStatic = CL_UI_InitItemStatic;
	import.UI_InitItemButton = CL_UI_InitItemButton;
	import.UI_InitItemCheckBox = CL_UI_InitItemCheckBox;
	import.UI_InitItemSpinner = CL_UI_InitItemSpinner;
	import.UI_InitItemSlider = CL_UI_InitItemSlider;
	import.UI_InitItemWindowDragger = CL_UI_InitItemWindowDragger;
	import.UI_InitItemListbox = CL_UI_InitItemListbox;

	import.Mem_Alloc = CL_GameModule_MemAlloc;
	import.Mem_Free = CL_GameModule_MemFree;

	cge = (cgame_export_t *)Com_LoadGameLibrary( "cgame", "GetCGameAPI", &module_handle, &import, builtinAPIfunc, cls.sv_pure, NULL );
	if( !cge )
		Com_Error( ERR_DROP, "Failed to load client game DLL" );

	apiversion = cge->API();
	if( apiversion != CGAME_API_VERSION )
	{
		Com_UnloadGameLibrary( &module_handle );
		Mem_FreePool( &cl_gamemodulepool );
		cge = NULL;
		Com_Error( ERR_DROP, "Client game is version %i, not %i", apiversion, CGAME_API_VERSION );
	}

	oldState = cls.state;
	CL_SetClientState( CA_LOADING );

	start = Sys_Milliseconds();

	cge->Init( cl.playernum, viddef.width, viddef.height, cls.demo.playing, cls.sv_pure, cls.sv_maxclients, cl.snapFrameTime, APP_PROTOCOL_VERSION );

	Com_DPrintf( "CL_GameModule_Init: %.2f seconds\n", (float)( Sys_Milliseconds() - start ) * 0.001f );

	CL_SetClientState( oldState );
	cls.cgameActive = qtrue;

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	CL_SoundModule_SoundsInMemory();

	Sys_SendKeyEvents(); // pump message loop
}

/*
* CL_GameModule_Initialized
*/
qboolean CL_GameModule_Initialized( void )
{
	return ( cge ? qtrue : qfalse );
}

/*
* CL_GameModule_Shutdown
*/
void CL_GameModule_Shutdown( void )
{
	if( !cge )
		return;

	cls.cgameActive = qfalse;

	cge->Shutdown();
	Com_UnloadGameLibrary( &module_handle );
	Mem_FreePool( &cl_gamemodulepool );
	cge = NULL;
}

/*
* CL_GameModule_EscapeKey
*/
void CL_GameModule_EscapeKey( void )
{
	if( cge )
		cge->EscapeKey();
	else if( SCR_GetCinematicTime() )
		SCR_FinishCinematic();
}

/*
* CL_GameModule_GetEntitySpatilization
*/
void CL_GameModule_GetEntitySpatilization( int entNum, vec3_t origin, vec3_t velocity )
{
	if( cge )
		cge->GetEntitySpatilization( entNum, origin, velocity );
}

/*
* CL_GameModule_NewSnapshot
*/
qboolean CL_GameModule_NewSnapshot( unsigned int snapNum, unsigned int serverTime )
{
	if( cge )
	{
		cge->NewSnapshot( snapNum, serverTime );
		return qtrue;
	}
	return qfalse;
}

/*
* CL_GameModule_ServerCommand
*/
void CL_GameModule_ServerCommand( void )
{
	if( cge )
		cge->ServerCommand();
}

/*
* CL_GameModule_ConfigStringUpdate
*/
void CL_GameModule_ConfigStringUpdate( int number )
{
	if( cge )
		cge->ConfigStringUpdate( number );
}

/*
* CL_GameModule_SetSensitivityScale
*/
float CL_GameModule_SetSensitivityScale( const float sens )
{
	if( cge )
		return cge->SetSensitivityScale( sens );
	else
		return 1.0f;
}

/*
* CL_GameModule_RenderView
*/
void CL_GameModule_RenderView( float stereo_separation )
{
	if( cge )
		cge->RenderView( cls.trueframetime, cls.realtime, cl.serverTime, cl.extrapolationTime, stereo_separation );
}
