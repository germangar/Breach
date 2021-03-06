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

#ifdef CGAME_HARD_LINKED
#define CGAME_IMPORT cgi_imp_local
#endif

extern cgame_import_t CGAME_IMPORT;

static inline void trap_Print( const char *msg )
{
	CGAME_IMPORT.Print( msg );
}

static inline void trap_Error( const char *msg )
{
	CGAME_IMPORT.Error( msg );
}

static inline const snapshot_t *trap_GetSnapshot( int snapNum )
{
	return CGAME_IMPORT.GetSnapshot( snapNum );
}

static inline const entity_state_t *trap_GetEntityFromSnapsBackup( unsigned int snapNum, int entNum )
{
	return CGAME_IMPORT.GetEntityFromSnapsBackup( snapNum, entNum );
}

static inline const player_state_t *trap_GetPlayerStateFromSnapsBackup( unsigned int snapNum )
{
	return CGAME_IMPORT.GetPlayerStateFromSnapsBackup( snapNum );
}

static inline const game_state_t *trap_GetGameStateFromSnapsBackup( unsigned int snapNum )
{
	return CGAME_IMPORT.GetGameStateFromSnapsBackup( snapNum );
}

// cvars
static inline cvar_t *trap_Cvar_Get( const char *name, const char *value, int flags )
{
	return CGAME_IMPORT.Cvar_Get( name, value, flags );
}

static inline cvar_t *trap_Cvar_Set( const char *name, const char *value )
{
	return CGAME_IMPORT.Cvar_Set( name, value );
}

static inline void trap_Cvar_SetValue( const char *name, float value )
{
	CGAME_IMPORT.Cvar_SetValue( name, value );
}

static inline cvar_t *trap_Cvar_ForceSet( const char *name, char *value )
{
	return CGAME_IMPORT.Cvar_ForceSet( name, value );
}

static inline float trap_Cvar_Value( const char *name )
{
	return CGAME_IMPORT.Cvar_Value( name );
}

static inline const char *trap_Cvar_String( const char *name )
{
	return CGAME_IMPORT.Cvar_String( name );
}

static inline int trap_Cmd_Argc( void )
{
	return CGAME_IMPORT.Cmd_Argc();
}

static inline char *trap_Cmd_Argv( int arg )
{
	return CGAME_IMPORT.Cmd_Argv( arg );
}

static inline char *trap_Cmd_Args( void )
{
	return CGAME_IMPORT.Cmd_Args();
}

static inline void trap_Cmd_AddCommand( const char *name, void ( *cmd )(void) )
{
	CGAME_IMPORT.Cmd_AddCommand( name, cmd );
}

static inline void trap_Cmd_RemoveCommand( const char *cmd_name )
{
	CGAME_IMPORT.Cmd_RemoveCommand( cmd_name );
}

static inline void trap_Cmd_ExecuteText( int exec_when, const char *text )
{
	CGAME_IMPORT.Cmd_ExecuteText( exec_when, text );
}

static inline void trap_Cmd_Execute( void )
{
	CGAME_IMPORT.Cmd_Execute();
}

static inline int trap_FS_FOpenFile( const char *filename, int *filenum, int mode )
{
	return CGAME_IMPORT.FS_FOpenFile( filename, filenum, mode );
}

static inline int trap_FS_Read( void *buffer, size_t len, int file )
{
	return CGAME_IMPORT.FS_Read( buffer, len, file );
}

static inline int trap_FS_Write( const void *buffer, size_t len, int file )
{
	return CGAME_IMPORT.FS_Write( buffer, len, file );
}

static inline int trap_FS_Print( int file, const char *msg )
{
	return CGAME_IMPORT.FS_Print( file, msg );
}

static inline int trap_FS_Tell( int file )
{
	return CGAME_IMPORT.FS_Tell( file );
}

static inline int trap_FS_Seek( int file, int offset, int whence )
{
	return CGAME_IMPORT.FS_Seek( file, offset, whence );
}

static inline int trap_FS_Eof( int file )
{
	return CGAME_IMPORT.FS_Eof( file );
}

static inline int trap_FS_Flush( int file )
{
	return CGAME_IMPORT.FS_Flush( file );
}

static inline void trap_FS_FCloseFile( int file )
{
	CGAME_IMPORT.FS_FCloseFile( file );
}

static inline int trap_FS_GetFileList( const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end )
{
	return CGAME_IMPORT.FS_GetFileList( dir, extension, buf, bufsize, start, end );
}

static inline const char *trap_FS_FirstExtension( const char *filename, const char *extensions[], int num_extensions )
{
	return CGAME_IMPORT.FS_FirstExtension( filename, extensions, num_extensions );
}

static inline qboolean trap_FS_IsPureFile( const char *filename )
{
	return CGAME_IMPORT.FS_IsPureFile( filename );
}

static inline const char *trap_Key_GetBindingBuf( int binding )
{
	return CGAME_IMPORT.Key_GetBindingBuf( binding );
}

static inline const char *trap_Key_KeynumToString( int keynum )
{
	return CGAME_IMPORT.Key_KeynumToString( keynum );
}

static inline const char *trap_GetConfigString( int i )
{
	return CGAME_IMPORT.GetConfigString( i );
}

static inline unsigned int trap_Milliseconds( void )
{
	return CGAME_IMPORT.Milliseconds();
}

static inline qboolean trap_DownloadRequest( const char *filename, qboolean requestpak )
{
	return CGAME_IMPORT.DownloadRequest( filename, requestpak );
}

static inline void trap_RefreshMouseAngles( void )
{
	CGAME_IMPORT.RefreshMouseAngles();
}

static inline void trap_NET_GetUserCmd( int frame, usercmd_t *cmd )
{
	CGAME_IMPORT.NET_GetUserCmd( frame, cmd );
}

static inline int trap_NET_GetCurrentUserCmdNum( void )
{
	return CGAME_IMPORT.NET_GetCurrentUserCmdNum();
}

static inline void trap_NET_GetCurrentState( int *ucmdAcknowledged, int *ucmdHead, int *ucmdSent )
{
	CGAME_IMPORT.NET_GetCurrentState( ucmdAcknowledged, ucmdHead, ucmdSent );
}

static inline void trap_R_UpdateScreen( void )
{
	CGAME_IMPORT.R_UpdateScreen();
}

static inline int trap_R_GetClippedFragments( const vec3_t origin, float radius, vec3_t axis[3], int maxfverts, vec3_t *fverts, int maxfragments, fragment_t *fragments )
{
	return CGAME_IMPORT.R_GetClippedFragments( origin, radius, axis, maxfverts, fverts, maxfragments, fragments );
}

static inline void trap_R_ClearScene( void )
{
	CGAME_IMPORT.R_ClearScene();
}

static inline void trap_R_AddEntityToScene( const entity_t *ent )
{
	CGAME_IMPORT.R_AddEntityToScene( ent );
}

static inline void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, struct shader_s *shader )
{
	CGAME_IMPORT.R_AddLightToScene( org, intensity, r, g, b, shader );
}

static inline void trap_R_AddPolyToScene( const poly_t *poly )
{
	CGAME_IMPORT.R_AddPolyToScene( poly );
}

static inline void trap_R_AddLightStyleToScene( int style, float r, float g, float b )
{
	CGAME_IMPORT.R_AddLightStyleToScene( style, r, g, b );
}

static inline void trap_R_RenderScene( const refdef_t *fd )
{
	CGAME_IMPORT.R_RenderScene( fd );
}

static inline const char *trap_R_SpeedsMessage( char *out, size_t size )
{
	return CGAME_IMPORT.R_SpeedsMessage( out, size );
}

static inline void trap_R_RegisterWorldModel( const char *name )
{
	CGAME_IMPORT.R_RegisterWorldModel( name );
}

static inline struct model_s *trap_R_RegisterModel( const char *name )
{
	return CGAME_IMPORT.R_RegisterModel( name );
}

static inline void trap_R_ModelBounds( const struct model_s *mod, vec3_t mins, vec3_t maxs )
{
	CGAME_IMPORT.R_ModelBounds( mod, mins, maxs );
}

static inline struct shader_s *trap_R_RegisterPic( const char *name )
{
	return CGAME_IMPORT.R_RegisterPic( name );
}

static inline struct shader_s *trap_R_RegisterSkin( const char *name )
{
	return CGAME_IMPORT.R_RegisterSkin( name );
}

static inline struct skinfile_s *trap_R_RegisterSkinFile( const char *name )
{
	return CGAME_IMPORT.R_RegisterSkinFile( name );
}

static inline qboolean trap_R_LerpTag( orientation_t *orient, const struct model_s *mod, int oldframe, int frame, float lerpfrac, const char *name )
{
	return CGAME_IMPORT.R_LerpTag( orient, mod, oldframe, frame, lerpfrac, name );
}

static inline void trap_R_SetCustomColor( int num, int r, int g, int b )
{
	CGAME_IMPORT.R_SetCustomColor( num, r, g, b );
}

static inline void trap_R_LightForOrigin( const vec3_t origin, vec3_t dir, vec4_t ambient, vec4_t diffuse, float radius )
{
	CGAME_IMPORT.R_LightForOrigin( origin, dir, ambient, diffuse, radius );
}

static inline void trap_R_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, const vec4_t color, const struct shader_s *shader )
{
	CGAME_IMPORT.R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, color, shader );
}

static inline void trap_R_TransformVectorToScreen( const refdef_t *rd, const vec3_t in, vec2_t out )
{
	CGAME_IMPORT.R_TransformVectorToScreen( rd, in, out );
}

static inline int trap_R_SkeletalGetNumBones( const struct model_s *mod, int *numFrames )
{
	return CGAME_IMPORT.R_SkeletalGetNumBones( mod, numFrames );
}

static inline int trap_R_SkeletalGetBoneInfo( const struct model_s *mod, int bone, char *name, size_t name_size, int *flags )
{
	return CGAME_IMPORT.R_SkeletalGetBoneInfo( mod, bone, name, name_size, flags );
}

static inline void trap_R_SkeletalGetBonePose( const struct model_s *mod, int bone, int frame, bonepose_t *bonepose )
{
	CGAME_IMPORT.R_SkeletalGetBonePose( mod, bone, frame, bonepose );
}

static inline struct cmodel_s *trap_CM_InlineModel( int num )
{
	return CGAME_IMPORT.CM_InlineModel( num );
}

static inline struct cmodel_s *trap_CM_ModelForBBox( vec3_t mins, vec3_t maxs )
{
	return CGAME_IMPORT.CM_ModelForBBox( mins, maxs );
}

static inline int trap_CM_NumInlineModels( void )
{
	return CGAME_IMPORT.CM_NumInlineModels();
}

static inline void trap_CM_TransformedBoxTrace( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles )
{
	CGAME_IMPORT.CM_TransformedBoxTrace( tr, start, end, mins, maxs, cmodel, brushmask, origin, angles );
}

static inline int trap_CM_TransformedPointContents( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles )
{
	return CGAME_IMPORT.CM_TransformedPointContents( p, cmodel, origin, angles );
}

static inline void trap_CM_InlineModelBounds( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs )
{
	CGAME_IMPORT.CM_InlineModelBounds( cmodel, mins, maxs );
}

static inline int trap_CM_BoxLeafnums( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode )
{
	return CGAME_IMPORT.CM_BoxLeafnums( mins, maxs, list, listsize, topnode );
}
static inline int trap_CM_LeafCluster( int leafnum )
{
	return CGAME_IMPORT.CM_LeafCluster( leafnum );
}

static inline char *trap_CM_LoadMapMessage( char *name, char *message, int size )
{
	return CGAME_IMPORT.CM_LoadMapMessage( name, message, size );
}

static inline struct sfx_s *trap_S_RegisterSound( const char *name )
{
	return CGAME_IMPORT.S_RegisterSound( name );
}

static inline void trap_S_Update( const vec3_t origin, const vec3_t velocity, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up )
{
	CGAME_IMPORT.S_Update( origin, velocity, v_forward, v_right, v_up );
}

static inline void trap_S_StartFixedSound( struct sfx_s *sfx, const vec3_t origin, int channel, float fvol, float attenuation )
{
	CGAME_IMPORT.S_StartFixedSound( sfx, origin, channel, fvol, attenuation );
}

static inline void trap_S_StartRelativeSound( struct sfx_s *sfx, int entnum, int channel, float fvol, float attenuation )
{
	CGAME_IMPORT.S_StartRelativeSound( sfx, entnum, channel, fvol, attenuation );
}

static inline void trap_S_StartGlobalSound( struct sfx_s *sfx, int channel, float fvol )
{
	CGAME_IMPORT.S_StartGlobalSound( sfx, channel, fvol );
}

static inline void trap_S_AddLoopSound( struct sfx_s *sfx, int entnum, float fvol, float attenuation )
{
	CGAME_IMPORT.S_AddLoopSound( sfx, entnum, fvol, attenuation );
}

static inline void trap_S_StartBackgroundTrack( const char *intro, const char *loop )
{
	CGAME_IMPORT.S_StartBackgroundTrack( intro, loop );
}

static inline void trap_S_StopBackgroundTrack( void )
{
	CGAME_IMPORT.S_StopBackgroundTrack();
}

static inline struct mufont_s *trap_SCR_RegisterFont( const char *name )
{
	return CGAME_IMPORT.SCR_RegisterFont( name );
}

static inline void trap_SCR_DrawString( int x, int y, int align, const char *str, struct mufont_s *font, vec4_t color )
{
	CGAME_IMPORT.SCR_DrawString( x, y, align, str, font, color );
}

static inline int trap_SCR_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	return CGAME_IMPORT.SCR_DrawStringWidth( x, y, align, str, maxwidth, font, color );
}

static inline void trap_SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color )
{
	CGAME_IMPORT.SCR_DrawClampString( x, y, str, xmin, ymin, xmax, ymax, font, color );
}

static inline size_t trap_SCR_strHeight( struct mufont_s *font )
{
	return CGAME_IMPORT.SCR_strHeight( font );
}

static inline size_t trap_SCR_strWidth( const char *str, struct mufont_s *font, int maxlen )
{
	return CGAME_IMPORT.SCR_strWidth( str, font, maxlen );
}

static inline size_t trap_SCR_StrlenForWidth( const char *str, struct mufont_s *font, size_t maxwidth )
{
	return CGAME_IMPORT.SCR_StrlenForWidth( str, font, maxwidth );
}

static inline void trap_UI_InitWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic )
{
	CGAME_IMPORT.UI_InitWindow( name, x, y, w, h, blockdraw, blockfocus, custompic );
}

static inline void trap_UI_OpenWindow( const char *name )
{
	CGAME_IMPORT.UI_OpenWindow( name );
}

static inline void trap_UI_CloseWindow( const char *name )
{
	CGAME_IMPORT.UI_CloseWindow( name );
}

static inline void trap_UI_ApplyWindowChanges( const char *name )
{
	CGAME_IMPORT.UI_ApplyWindowChanges( name );
}

static inline struct menuitem_public_s *trap_UI_InitItemWindowDragger( const char *windowname, const char *itemname, int x, int y, int align )
{
	return CGAME_IMPORT.UI_InitItemWindowDragger( windowname, itemname, x, y, align );
}

static inline struct menuitem_public_s *trap_UI_InitItemStatic( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font )
{
	return CGAME_IMPORT.UI_InitItemStatic( windowname, itemname, x, y, align, tittle, font );
}

static inline struct menuitem_public_s *trap_UI_InitItemButton( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString )
{
	return CGAME_IMPORT.UI_InitItemButton( windowname, itemname, x, y, align, tittle, font, Action, targetString );
}

static inline struct menuitem_public_s *trap_UI_InitItemCheckBox( const char *windowname, const char *itemname, int x, int y, int align, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	return CGAME_IMPORT.UI_InitItemCheckBox( windowname, itemname, x, y, align, Update, Apply, alwaysApply, targetString );
}

static inline struct menuitem_public_s *trap_UI_InitItemSpinner( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *spinnerNames, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	return CGAME_IMPORT.UI_InitItemSpinner( windowname, itemname, x, y, align, font, spinnerNames, Update, Apply, alwaysApply, targetString );
}

static inline struct menuitem_public_s *trap_UI_InitItemSlider( const char *windowname, const char *itemname, int x, int y, int align, int min, int max, float stepsize, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	return CGAME_IMPORT.UI_InitItemSlider( windowname, itemname, x, y, align, min, max, stepsize, Update, Apply, alwaysApply, targetString );
}

static inline struct menuitem_public_s *trap_UI_InitItemListbox( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *namesList, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, void *Action, char *targetString )
{
	return CGAME_IMPORT.UI_InitItemListbox( windowname, itemname, x, y, align, font, namesList, Update, Apply, alwaysApply, Action, targetString );
}

static inline void *trap_MemAlloc( size_t size, const char *filename, int fileline )
{
	return CGAME_IMPORT.Mem_Alloc( size, filename, fileline );
}

static inline void trap_MemFree( void *data, const char *filename, int fileline )
{
	CGAME_IMPORT.Mem_Free( data, filename, fileline );
}
