/*
Copyright (C) 2007 German Garcia
*/

#include "cg_local.h"

static cvar_t *cg_show2D;
static cvar_t *cg_showFPS;
static cvar_t *cg_showSpeed;
static cvar_t *cg_showHUD;

static cvar_t *cg_clientHUD;

/*
* CG_EscapeKey
*/
void CG_EscapeKey( void )
{
	/*if( cgs.demoPlaying )
	{
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
	}*/

	CG_IngameMenu();
}

//==================================================
// LOADING SCREEN
//==================================================

/*
* CG_LoadingScreen_Topicname
*/
void CG_LoadingScreen_Topicname( char *string )
{
	if( Q_stricmp( cg.loadingScreen.topic, string ) )
	{
		Q_strncpyz( cg.loadingScreen.topic, string, sizeof( cg.loadingScreen.topic ) );
		trap_R_UpdateScreen();
	}
}

/*
* CG_LoadingScreen_Filename
*/
void CG_LoadingScreen_Filename( const char *string )
{
	if( Q_stricmp( cg.loadingScreen.file, string ) )
	{
		Q_strncpyz( cg.loadingScreen.file, string, sizeof( cg.loadingScreen.file ) );
		trap_R_UpdateScreen();
	}
}

/*
* CG_DrawLoadingScreen
*/
void CG_DrawLoadingScreen( void )
{
	// topic: what we're loading at the moment
	//if( cg.loadingScreen.topic[0] ) {
	//	trap_SCR_DrawStringWidth( 16, cgs.vidHeight - 8, ALIGN_LEFT_BOTTOM, cg.loadingScreen.topic, 64, cgm.fontSystemMedium, colorWhite );
	//}

	// file: current file being loaded
	if( cg.loadingScreen.file[0] )
	{
		trap_SCR_DrawString( 16, cgs.vidHeight - 8, ALIGN_LEFT_BOTTOM, cg.loadingScreen.file, cgm.fontSystemSmall, colorWhite );
	}
}

//==================================================
// 2D DRAWING
//==================================================

int CG_HorizontalAlignForWidth( const int x, int align, int width )
{
	int nx = x;

	if( align % 3 == 0 )  // left
		nx = x;
	if( align % 3 == 1 )  // center
		nx = x - width / 2;
	if( align % 3 == 2 )  // right
		nx = x - width;

	return nx;
}

int CG_VerticalAlignForHeight( const int y, int align, int height )
{
	int ny = y;

	if( align / 3 == 0 )  // top
		ny = y;
	else if( align / 3 == 1 )  // middle
		ny = y - height / 2;
	else if( align / 3 == 2 )  // bottom
		ny = y - height;

	return ny;
}

/*
* CG_DrawFPS
*/
void CG_DrawFPS( int x, int y, int align, struct mufont_s *font, vec4_t color )
{
#define FPSSAMPLESCOUNT 16
#define FPSSAMPLESMASK ( FPSSAMPLESCOUNT-1 )
	static float frameTimes[FPSSAMPLESCOUNT];
	int fps;
	float avFrameTime;
	int i;

	if( !cg_showFPS->integer )
		return;

	frameTimes[cg.frameCount & FPSSAMPLESMASK] = cg.frametime;

	for( avFrameTime = 0.0f, i = 0; i < FPSSAMPLESCOUNT; i++ )
		avFrameTime += frameTimes[( cg.frameCount - i ) & FPSSAMPLESMASK];

	avFrameTime /= FPSSAMPLESCOUNT;
	fps = (int)( 1.0f/avFrameTime );

	trap_SCR_DrawString( x, y, align, va( "%i", fps ), font, color );
}

/*
* CG_DrawClock
*/
void CG_DrawClock( int x, int y, int align, struct mufont_s *font, vec4_t color )
{
	unsigned int clocktime, startTime, duration, curtime;
	double seconds;
	int minutes;

	if( GS_Paused() )
		curtime = cg.frame.timeStamp;
	else
		curtime = cg.time;

	duration = gs.gameState.longstats[GAMELONG_MATCHDURATION];
	startTime = gs.gameState.longstats[GAMELONG_MATCHSTART];

	// count downwards when having a duration
	if( duration )
	{
		if( duration + startTime < curtime ) 
			duration = curtime - startTime; // avoid negative results

		clocktime = startTime + duration - curtime;
	}
	else
	{
		clocktime = curtime - startTime;
	}

	seconds = (double)clocktime * 0.001;
	minutes = (int)( seconds / 60 );
	seconds -= minutes * 60;

	trap_SCR_DrawString( x, y, align, va( "%i:%i", minutes, (int)seconds ), font, color );
}

/*
* CG_DrawCrosshair
*/
void CG_DrawCrosshair( int x, int y, int w, int h, int align, vec4_t color )
{
	x = CG_HorizontalAlignForWidth( x, align, w );
	y = CG_VerticalAlignForHeight( y, align, h );

	trap_R_DrawStretchPic( x, y, w, h,
		0, 0, 1, 1, color, CG_LocalShader( SHADER_CROSSHAIR ) );
}

/*
* CG_DrawPlayerSpeed - not to be included in huds. Just for development
*/
static void CG_DrawPlayerSpeed( int x, int y, int align, struct mufont_s *font, vec4_t color )
{
	vec3_t vel;

	if( !cg_showSpeed->integer )
		return;

	VectorSet( vel, cg.predictedEntityState.ms.velocity[0], cg.predictedEntityState.ms.velocity[1], 0 );
	trap_SCR_DrawString( x, y, align, va( "%i", (int)VectorLengthFast( vel ) ), font, color );
}

/*
* CG_DrawItem
*/
void CG_DrawItem( int x, int y, int align, int w, int h, int itemID, vec3_t angles )
{
	refdef_t refdef;
	gsitem_t *item;
	int renderfx, i;
	float radius = 0.0f;
	vec3_t origin, offset, axis[3];

	item = GS_FindItemByIndex( itemID );
	if( !item || !item->objectIndex )
		return;

	x = CG_HorizontalAlignForWidth( x, align, w );
	y = CG_VerticalAlignForHeight( y, align, h );

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;
	refdef.fov_x = 30;
	refdef.fov_y = CalcFov( refdef.fov_x, w, h );
	refdef.time = cg.time;
	refdef.rdflags = RDF_NOWORLDMODEL;
	Matrix_Copy( axis_identity, refdef.viewaxis );

	trap_R_ClearScene();

	renderfx = RF_FULLBRIGHT | RF_NOSHADOW | RF_FORCENOLOD;

	if( item->type & IT_WEAPON )
	{
		weaponobject_t *weaponObject;

		//trap_R_DrawStretchPic( refdef.x, refdef.y, refdef.width, refdef.height, 0, 0, 1, 1, colorLtGrey, CG_LocalShader( SHADER_WHITE ) );

		weaponObject = CG_WeaponObjectFromIndex( item->objectIndex );

		// the weapon model might not be centered in it's origin, so find its bounds radius
		radius = RadiusFromBounds( weaponObject->mins, weaponObject->maxs );

		VectorClear( origin );
		AnglesToAxis( angles, axis );
		for( i = 0; i < 3; i++ )
		{
			offset[i] = 0.5 * ( weaponObject->maxs[i] + weaponObject->mins[i] );
			VectorMA( origin, -offset[i], axis[i], origin );
		}

		CG_AddWeaponObject( weaponObject, origin, axis, origin, renderfx, 0 );
	}
	else
	{
		entity_t ent;
		vec3_t mins, maxs;
		struct model_s *model;

		//trap_R_DrawStretchPic( refdef.x, refdef.y, refdef.width, refdef.height, 0, 0, 1, 1, colorLtGrey, CG_LocalShader( SHADER_WHITE ) );

		model = cgm.indexedModels[item->objectIndex];
		if( !model )
			return;

		trap_R_ModelBounds( model, mins, maxs );
		radius = RadiusFromBounds( mins, maxs );

		VectorClear( origin );
		AnglesToAxis( angles, axis );
		for( i = 0; i < 3; i++ )
		{
			offset[i] = 0.5 * ( maxs[i] + mins[i] );
			VectorMA( origin, -offset[i], axis[i], origin );
		}

		memset( &ent, 0, sizeof( ent ) );
		ent.rtype = RT_MODEL;
		Vector4Set( ent.shaderRGBA, 255, 255, 255, 255 );
		ent.scale = 1.0f;
		ent.renderfx = renderfx;
		ent.frame = 0;
		ent.oldframe = 0;
		ent.model = model;
		VectorCopy( origin, ent.origin );
		VectorCopy( origin, ent.origin2 );
		VectorCopy( origin, ent.lightingOrigin );
		Matrix_Copy( axis, ent.axis );

		CG_AddEntityToScene( &ent );
	}

	// fixme : the scaling. I don't know what I'm doing
	VectorMA( refdef.vieworg, (-radius * 0.45) * ( 1.0 / 0.179 ), axis_identity[FORWARD], refdef.vieworg );
	trap_R_RenderScene( &refdef );
}

/*
* CG_DrawWaterViewBlend
*/
static void CG_DrawWaterViewBlend( void )
{
	int index;
	float u;

	index = cg.predictedPlayerState.stats[STAT_WATER_BLEND];
	if( index <= 0 || index >= MAX_IMAGES )
		return;

	// the texture is assumed to be square
	u = (float)cg.view.refdef.width / (float)cg.view.refdef.height;

	if( cgm.indexedShaders[index] )
		trap_R_DrawStretchPic( cg.view.refdef.x, cg.view.refdef.y, cg.view.refdef.width, cg.view.refdef.height, 0, 0, 1, u, colorWhite, cgm.indexedShaders[index] );
}

/*
* CG_DrawRSpeeds
*/
void CG_DrawRSpeeds( int x, int y, int align, struct mufont_s *font, vec4_t color )
{
	char msg[1024];

	trap_R_SpeedsMessage( msg, sizeof( msg ) );

	if( msg[0] )
	{
		int height;
		const char *p, *start, *end;

		height = trap_SCR_strHeight( font );

		p = start = msg;
		do
		{
			end = strchr( p, '\n' );
			if( end )
				msg[end-start] = '\0';

			trap_SCR_DrawString( x, y, align, p, font, color );
			y += height;

			if( end )
				p = end + 1;
			else
				break;
		} while( 1 );
	}
}

/*
* CG_Init2D
*/
void CG_Init2D( void )
{
	cg_show2D = trap_Cvar_Get( "cg_show2D", "1", CVAR_ARCHIVE );
	cg_showFPS = trap_Cvar_Get( "cg_showFPS", "1", CVAR_ARCHIVE );
	cg_showSpeed = trap_Cvar_Get( "cg_showSpeed", "0", CVAR_ARCHIVE );
	cg_showHUD = trap_Cvar_Get( "cg_showHUD", "1", CVAR_ARCHIVE );

	cg_clientHUD = trap_Cvar_Get( "cg_clientHUD", "default", CVAR_ARCHIVE );
	cg_clientHUD->modified = qtrue;
}

/*
* CG_Draw2D
*/
void CG_Draw2D( void )
{
	if( cg.view.refdef.rdflags & RDF_UNDERWATER )
		CG_DrawWaterViewBlend();

	if( !cg_show2D->integer )
		return;

	CG_DrawPlayerSpeed( 0, cgs.vidHeight - 32, ALIGN_LEFT_BOTTOM, cgm.fontSystemSmall, colorWhite );
	CG_DrawRSpeeds( cgs.vidWidth, cgs.vidHeight*2/3, ALIGN_RIGHT_TOP, cgm.fontSystemSmall, colorWhite );

	if( cg_showHUD->integer )
	{
		CG_DrawCrosshair( cgs.vidWidth/2, cgs.vidHeight/2, 32, 32, ALIGN_CENTER_MIDDLE, colorWhite );
		if( cg_clientHUD->modified )
		{
			cg_clientHUD->modified = qfalse;
			if( !CG_LoadHUDScript( cg_clientHUD->string ) && Q_stricmp( cg_clientHUD->string, cg_clientHUD->dvalue ) )
				CG_LoadHUDScript( cg_clientHUD->dvalue );
		}

		CG_ExecuteHUDScript();
	}
}
