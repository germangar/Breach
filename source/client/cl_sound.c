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

static sound_export_t *se;
static mempool_t *cl_soundmodulepool;

static void *sound_library = NULL;

static cvar_t *s_module = NULL;
static cvar_t *s_module_fallback = NULL;

/*
===============
CL_SetSoundExtension
===============
*/
static char *CL_SetSoundExtension( const char *name )
{
	unsigned int i;
	char *finalname;
	size_t finalname_size, maxlen;
	const char *extension;

	assert( name );

	if( COM_FileExtension( name ) )
		return TempCopyString( name );

	maxlen = 0;
	for( i = 0; i < NUM_SOUND_EXTENSIONS; i++ )
	{
		if( strlen( SOUND_EXTENSIONS[i] ) > maxlen )
			maxlen = strlen( SOUND_EXTENSIONS[i] );
	}

	finalname_size = strlen( name ) + maxlen + 1;
	finalname = Mem_TempMalloc( finalname_size );
	Q_strncpyz( finalname, name, finalname_size );

	extension = FS_FirstExtension( finalname, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS );
	if( extension )
		Q_strncatz( finalname, extension, finalname_size );
	// if not found, we just pass it without the extension

	return finalname;
}

/*
===============
CL_SoundModule_Error
===============
*/
static void CL_SoundModule_Error( const char *msg )
{
	Com_Error( ERR_FATAL, "%s", msg );
}

/*
===============
CL_SoundModule_Print
===============
*/
static void CL_SoundModule_Print( const char *msg )
{
	Com_Printf( "%s", msg );
}

/*
===============
CL_SoundModule_MemAlloc
===============
*/
static void *CL_SoundModule_MemAlloc( mempool_t *pool, int size, const char *filename, int fileline )
{
	return _Mem_Alloc( pool, size, MEMPOOL_SOUND, 0, filename, fileline );
}

/*
===============
CL_SoundModule_MemFree
===============
*/
static void CL_SoundModule_MemFree( void *data, const char *filename, int fileline )
{
	_Mem_Free( data, MEMPOOL_SOUND, 0, filename, fileline );
}

/*
===============
CL_SoundModule_MemAllocPool
===============
*/
static mempool_t *CL_SoundModule_MemAllocPool( const char *name, const char *filename, int fileline )
{
	return _Mem_AllocPool( cl_soundmodulepool, name, MEMPOOL_SOUND, filename, fileline );
}

/*
===============
CL_SoundModule_MemFreePool
===============
*/
static void CL_SoundModule_MemFreePool( mempool_t **pool, const char *filename, int fileline )
{
	_Mem_FreePool( pool, MEMPOOL_SOUND, 0, filename, fileline );
}

/*
===============
CL_SoundModule_MemEmptyPool
===============
*/
static void CL_SoundModule_MemEmptyPool( mempool_t *pool, const char *filename, int fileline )
{
	_Mem_EmptyPool( pool, MEMPOOL_SOUND, 0, filename, fileline );
}

/*
===============
CL_SoundModule_Load

Helper function to try loading sound module with certain name
===============
*/
static qboolean CL_SoundModule_Load( const char *name, sound_import_t *import, qboolean verbose )
{
	int apiversion;
	size_t file_size;
	char *file;
	void *( *GetSoundAPI )(void *);
	dllfunc_t funcs[2];

	if( verbose )
		Com_Printf( "Loading sound module: %s\n", name );

	file_size = strlen( LIB_DIRECTORY "/" ) + strlen( "snd_" ) + strlen( name ) + 1 + strlen( ARCH ) + strlen( LIB_SUFFIX ) + 1;
	file = Mem_TempMalloc( file_size );
	Q_snprintfz( file, file_size, LIB_DIRECTORY "/snd_%s_" ARCH LIB_SUFFIX, name );

	funcs[0].name = "GetSoundAPI";
	funcs[0].funcPointer = ( void ** )&GetSoundAPI;
	funcs[1].name = NULL;
	sound_library = Com_LoadLibrary( file, funcs );

	Mem_TempFree( file );

	se = ( sound_export_t * )GetSoundAPI( import );
	if( !se )
	{
		Com_Printf( "Loading %s failed\n", name );
		return qfalse;
	}

	apiversion = se->API();
	if( apiversion != SOUND_API_VERSION )
	{
		CL_SoundModule_Shutdown( verbose );
		Com_Printf( "Wrong module version for %s: %i, not %i", name, apiversion, SOUND_API_VERSION );
		return qfalse;
	}

	if( !se->Init( VID_GetWindowHandle(), MAX_EDICTS, verbose ) )
	{
		CL_SoundModule_Shutdown( verbose );
		Com_Printf( "Initialization of %s failed\n", name );
		return qfalse;
	}

	if( verbose )
		Com_Printf( "Initialization of %s successful\n", name );

	return qtrue;
}

/*
===============
CL_SoundModule_Init
===============
*/
void CL_SoundModule_Init( qboolean verbose )
{
	static const char *sound_modules[] = { "qf", "openal" };
	static const int num_sound_modules = sizeof( sound_modules )/sizeof( sound_modules[0] );
	sound_import_t import;

	if( !s_module )
		s_module = Cvar_Get( "s_module", "1", CVAR_ARCHIVE|CVAR_LATCH_SOUND );
	if( !s_module_fallback )
		s_module_fallback = Cvar_Get( "s_module_fallback", "1", CVAR_LATCH_SOUND );

	// unload anything we have now
	CL_SoundModule_Shutdown( verbose );

	if( verbose )
		Com_Printf( "------- sound initialization -------\n" );

	Cvar_GetLatchedVars( CVAR_LATCH_SOUND );

	if( s_module->integer < 0 || s_module->integer > num_sound_modules )
	{
		Com_Printf( "Invalid value for s_module (%i), reseting to default\n", s_module->integer );
		Cvar_ForceSet( "s_module", s_module->dvalue );
	}

	if( s_module_fallback->integer < 0 || s_module_fallback->integer > num_sound_modules )
	{
		Com_Printf( "Invalid value for s_module_fallback (%i), reseting to default\n", s_module_fallback->integer );
		Cvar_ForceSet( "s_module_fallback", s_module_fallback->dvalue );
	}

	if( !s_module->integer )
	{
		if( verbose )
		{
			Com_Printf( "Not loading a sound module\n" );
			Com_Printf( "------------------------------------\n" );
		}
		return;
	}

	cl_soundmodulepool = Mem_AllocPool( NULL, "Client Sound Module" );

	import.Error = CL_SoundModule_Error;
	import.Print = CL_SoundModule_Print;

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

	import.Milliseconds = Sys_Milliseconds;
	import.PageInMemory = Com_PageInMemory;

	import.Mem_Alloc = CL_SoundModule_MemAlloc;
	import.Mem_Free = CL_SoundModule_MemFree;
	import.Mem_AllocPool = CL_SoundModule_MemAllocPool;
	import.Mem_FreePool = CL_SoundModule_MemFreePool;
	import.Mem_EmptyPool = CL_SoundModule_MemEmptyPool;

	import.GetEntitySpatilization = CL_GameModule_GetEntitySpatilization;

	import.LoadLibrary = Com_LoadLibrary;
	import.UnloadLibrary = Com_UnloadLibrary;

	if( !CL_SoundModule_Load( sound_modules[s_module->integer-1], &import, verbose ) )
	{
		if( s_module->integer == s_module_fallback->integer ||
			!CL_SoundModule_Load( sound_modules[s_module_fallback->integer-1], &import, verbose ) )
		{
			if( verbose )
			{
				Com_Printf( "Couldn't load a sound module\n" );
				Com_Printf( "------------------------------------\n" );
			}
			Mem_FreePool( &cl_soundmodulepool );
			return;
		}
		Cvar_ForceSet( "s_module", va( "%i", s_module_fallback->integer ) );
	}

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	if( verbose )
		Com_Printf( "------------------------------------\n" );
}

/*
===============
CL_SoundModule_Shutdown
===============
*/
void CL_SoundModule_Shutdown( qboolean verbose )
{
	if( !se )
		return;

	se->Shutdown( verbose );

	Com_UnloadLibrary( &sound_library );
	Mem_FreePool( &cl_soundmodulepool );
	se = NULL;
}

/*
==============
CL_SoundModule_SoundsInMemory
==============
*/
void CL_SoundModule_SoundsInMemory( void )
{
	if( se )
		se->SoundsInMemory();
}

/*
==============
CL_SoundModule_FreeSounds
==============
*/
void CL_SoundModule_FreeSounds( void )
{
	if( se )
		se->FreeSounds();
}

/*
==============
CL_SoundModule_StopAllSounds
==============
*/
void CL_SoundModule_StopAllSounds( void )
{
	if( se )
		se->StopAllSounds();
}

/*
==============
CL_SoundModule_Clear
==============
*/
void CL_SoundModule_Clear( void )
{
	if( se )
		se->Clear();
}

/*
==============
CL_SoundModule_Update
==============
*/
void CL_SoundModule_Update( const vec3_t origin, const vec3_t velocity, const vec3_t v_forward, const vec3_t v_right,
						   const vec3_t v_up, qboolean avidump )
{
	if( se )
		se->Update( origin, velocity, v_forward, v_right, v_up, avidump );
}

/*
==============
CL_SoundModule_Activate
==============
*/
void CL_SoundModule_Activate( qboolean activate )
{
	if( se )
		se->Activate( activate );
}

/*
==============
CL_SoundModule_RegisterSound
==============
*/
struct sfx_s *CL_SoundModule_RegisterSound( const char *name )
{
	assert( name );

	if( se )
	{
		struct sfx_s *retval;
		char *finalname;

		finalname = CL_SetSoundExtension( name );
		retval = se->RegisterSound( finalname );
		Mem_TempFree( finalname );

		return retval;
	}
	else
	{
		return NULL;
	}
}

/*
==============
CL_SoundModule_StartFixedSound
==============
*/
void CL_SoundModule_StartFixedSound( struct sfx_s *sfx, const vec3_t origin, int channel, float fvol,
									float attenuation )
{
	if( se )
		se->StartFixedSound( sfx, origin, channel, fvol, attenuation );
}

/*
==============
CL_SoundModule_StartRelativeSound
==============
*/
void CL_SoundModule_StartRelativeSound( struct sfx_s *sfx, int entnum, int channel, float fvol, float attenuation )
{
	if( se )
		se->StartRelativeSound( sfx, entnum, channel, fvol, attenuation );
}

/*
==============
CL_SoundModule_StartGlobalSound
==============
*/
void CL_SoundModule_StartGlobalSound( struct sfx_s *sfx, int channel, float fvol )
{
	if( se )
		se->StartGlobalSound( sfx, channel, fvol );
}

/*
==============
CL_SoundModule_StartLocalSound
==============
*/
void CL_SoundModule_StartLocalSound( const char *name )
{
	assert( name );

	if( se )
	{
		char *finalname;

		finalname = CL_SetSoundExtension( name );
		se->StartLocalSound( finalname );
		Mem_TempFree( finalname );
	}
}

/*
==============
CL_SoundModule_AddLoopSound
==============
*/
void CL_SoundModule_AddLoopSound( struct sfx_s *sfx, int entnum, float fvol, float attenuation )
{
	if( se )
		se->AddLoopSound( sfx, entnum, fvol, attenuation );
}

/*
==============
CL_SoundModule_RawSamples
==============
*/
void CL_SoundModule_RawSamples( int samples, int rate, int width, int channels, const qbyte *data, qboolean music )
{
	if( se )
		se->RawSamples( samples, rate, width, channels, data, music );
}

/*
==============
CL_SoundModule_StartBackgroundTrack
==============
*/
void CL_SoundModule_StartBackgroundTrack( const char *intro, const char *loop )
{
	assert( intro );

	if( se )
	{
		char *finalintro, *finalloop;

		finalintro = CL_SetSoundExtension( intro );
		if( loop )
		{
			finalloop = CL_SetSoundExtension( loop );
		}
		else
		{
			finalloop = NULL;
		}
		se->StartBackgroundTrack( finalintro, finalloop );
		Mem_TempFree( finalintro );
		if( finalloop )
			Mem_TempFree( finalloop );
	}
}

/*
==============
CL_SoundModule_StopBackgroundTrack
==============
*/
void CL_SoundModule_StopBackgroundTrack( void )
{
	if( se )
		se->StopBackgroundTrack();
}

/*
==============
CL_SoundModule_BeginAviDemo
==============
*/
void CL_SoundModule_BeginAviDemo( void )
{
	if( se )
		se->BeginAviDemo();
}

/*
==============
CL_SoundModule_StopAviDemo
==============
*/
void CL_SoundModule_StopAviDemo( void )
{
	if( se )
		se->StopAviDemo();
}
