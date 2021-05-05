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

#include "cg_local.h"

// cg_view.c -- player rendering positioning

#define	WAVE_AMPLITUDE	0.015   // [0..1]
#define	WAVE_FREQUENCY	0.6     // [0..1]

static cvar_t *cg_view_x;
static cvar_t *cg_view_y;
static cvar_t *cg_view_width;
static cvar_t *cg_view_height;

static cvar_t *cg_thirdPerson;
static cvar_t *cg_thirdPersonAngle;
static cvar_t *cg_thirdPersonRange;

//===================================================================

void CG_CheckCrossHairActivation( void ); // removeme

/*
* CG_AddEntityToScene
*/
void CG_AddEntityToScene( entity_t *ent )
{
	if( ent->model && !ent->boneposes )
	{
		if( trap_R_SkeletalGetNumBones( ent->model, NULL ) )
			CG_SetBoneposesForTemporaryEntity( ent );
	}

	trap_R_AddEntityToScene( ent );
}

//=================
//CG_RenderFlags
//=================
static int CG_RenderFlags( void )
{
	int rdflags, contents;

	rdflags = 0;

	contents = GS_PointContents( cg.view.origin, 0 );
	if( contents & MASK_WATER )
	{
		rdflags |= RDF_UNDERWATER;

		// undewater, check above
		contents = GS_PointContents( tv( cg.view.origin[0], cg.view.origin[1], cg.view.origin[2] + 9 ), 0 );
		if( !(contents & MASK_WATER) )
			rdflags |= RDF_CROSSINGWATER;
	}
	else
	{
		// look down a bit
		contents = GS_PointContents( tv( cg.view.origin[0], cg.view.origin[1], cg.view.origin[2] - 9 ), 0 );
		if( contents & MASK_WATER )
			rdflags |= RDF_CROSSINGWATER;
	}

	if( cg.oldAreabits )
		rdflags |= RDF_OLDAREABITS;

	if( cg.portalInView )
		rdflags |= RDF_PORTALINVIEW;

	rdflags |= RDF_BLOOM;

	if( cg.skyportalInView )
	{
		rdflags |= RDF_SKYPORTALINVIEW;
		cg.view.refdef.skyportal = cgm.skyportal;
	}

	return rdflags;
}

//============================================================================

//=================
//CG_SetSensitivityScale
//Scale sensitivity for different view effects
//=================
float CG_SetSensitivityScale( const float sens )
{
	float sensScale = 1.0f;

	if( !cgs.demoPlaying && sens && ( cg.predictedPlayerState.controlTimers[USERINPUT_STAT_ZOOMTIME] > 0 ) )
	{
		if( cg_zoomSens->value )
			return cg_zoomSens->value/sens;
		
		return ( cg.predictedPlayerState.fov / cgs.clientInfo[ cgs.playerNum ].fov );
	}

	return sensScale;
}

//=================
//CG_CalcViewBob
//=================
static void CG_CalcViewBob( void )
{
	float bobMove, bobTime, bobScale;

	// calculate speed and cycle to be used for all cyclic walking effects
	cg.xyspeed = SQRTFAST( cg.predictedEntityState.ms.velocity[0]*cg.predictedEntityState.ms.velocity[0] + cg.predictedEntityState.ms.velocity[1]*cg.predictedEntityState.ms.velocity[1] );

	bobScale = 0;
	if( cg.xyspeed < 5 )
		cg.oldBobTime = 0;  // start at beginning of cycle again
	else
	{
		if( GS_PointContents( cg.predictedEntityState.ms.origin, 0 ) & MASK_WATER )
			bobScale =  0.75f;
		else
		{
			vec3_t mins, maxs;
			trace_t	trace;

			VectorCopy( cg.predictedEntityState.local.mins, mins );
			VectorCopy( cg.predictedEntityState.local.maxs, maxs );
			maxs[2] = mins[2];
			mins[2] -= ( 1.6f * STEPSIZE );
			GS_Trace( &trace, cg.predictedEntityState.ms.origin, mins, maxs, cg.predictedEntityState.ms.origin, cg.predictedEntityState.number, MASK_PLAYERSOLID, 0 );
			if( trace.startsolid || trace.allsolid )
			{
				if( cg.predictedEntityState.effects & EF_PLAYER_CROUCHED )
					bobScale = 1.5f;
				else
					bobScale = 2.5f;
			}
		}
	}

	bobMove = cg.frametime * bobScale;
	bobTime = ( cg.oldBobTime += bobMove );

	cg.bobCycle = (int)bobTime;
	cg.bobFracSin = fabs( sin( bobTime * M_PI ) );
}

//=================
//CG_ThirdPersonOffsetView
//=================
static void CG_ThirdPersonOffsetView( cg_viewdef_t *view )
{
	float dist, f, r;
	vec3_t dest, stop;
	vec3_t chase_dest;
	trace_t	trace;
	vec3_t mins = { -4, -4, -4 };
	vec3_t maxs = { 4, 4, 4 };
	vec3_t viewaxis[3];

	if( !view->thirdperson )
		return;

	AngleVectors( view->angles, viewaxis[FORWARD], viewaxis[RIGHT], viewaxis[UP] );

	// calc exact destination
	VectorCopy( view->origin, chase_dest );
	r = DEG2RAD( cg_thirdPersonAngle->value );
	f = -cos( r );
	r = -sin( r );
	VectorMA( chase_dest, cg_thirdPersonRange->value * f, viewaxis[FORWARD], chase_dest );
	VectorMA( chase_dest, cg_thirdPersonRange->value * r, viewaxis[RIGHT], chase_dest );
	chase_dest[2] += 8;

	// find the spot the player is looking at
	VectorMA( view->origin, 512, viewaxis[FORWARD], dest );
	GS_Trace( &trace, view->origin, mins, maxs, dest, cg.predictedPlayerState.POVnum, MASK_SOLID, 0 );

	// calculate pitch to look at the same spot from camera
	VectorSubtract( trace.endpos, view->origin, stop );
	dist = sqrt( stop[0] * stop[0] + stop[1] * stop[1] );
	if( dist < 1 )
		dist = 1;
	view->angles[PITCH] = RAD2DEG( -atan2( stop[2], dist ) );
	view->angles[YAW] -= cg_thirdPersonAngle->value;

	// move towards destination
	GS_Trace( &trace, view->origin, mins, maxs, chase_dest, cg.predictedPlayerState.POVnum, MASK_SOLID, 0 );

	if( trace.fraction != 1.0 )
	{
		VectorCopy( trace.endpos, stop );
		stop[2] += ( 1.0 - trace.fraction ) * 32;
		GS_Trace( &trace, view->origin, mins, maxs, stop, cg.predictedPlayerState.POVnum, MASK_SOLID, 0 );
		VectorCopy( trace.endpos, chase_dest );
	}

	VectorCopy( chase_dest, view->origin );
}

//=================
//CG_InterpolatePlayerPosition
//=================
static void CG_InterpolatePlayerPosition( int clientNum, vec3_t origin, vec3_t angles )
{
	const move_state_t *oms, *ms;
	int i;

	// player orientation from entity
	ms = &cg_entities[clientNum].current.ms;
	oms = &cg_entities[clientNum].prev.ms;

	// see if the player entity was teleported this frame
	if( abs( oms->origin[0] - ms->origin[0] ) > 256
	    || abs( oms->origin[1] - ms->origin[1] ) > 256
	    || abs( oms->origin[2] - ms->origin[2] ) > 256 )
		oms = ms; // don't interpolate

	// get the current mouse angles if not demoplaying nor chasecamming
	if( !cgs.demoPlaying && clientNum == cgs.playerNum  )
	{
		usercmd_t cmd;

		trap_NET_GetUserCmd( trap_NET_GetCurrentUserCmdNum(), &cmd );
		for( i = 0; i < 3; i++ )
		{
			origin[i] = oms->origin[i] + cg.lerpfrac * ( ms->origin[i] - oms->origin[i] );
			angles[i] = SHORT2ANGLE( cmd.angles[i] ) + SHORT2ANGLE( cg.predictedPlayerState.delta_angles[i] );
		}
	}
	else
	{
		for( i = 0; i < 3; i++ )
		{
			origin[i] = oms->origin[i] + cg.lerpfrac * ( ms->origin[i] - oms->origin[i] );
			angles[i] = LerpAngle( oms->angles[i], ms->angles[i], cg.lerpfrac );
		}
	}
}

//=================
//CG_SetupView - Find out the type of view we will be rendering and setup accordingly
//=================
void CG_SetupView( cg_viewdef_t *view, int x, int y, int width, int height )
{
	memset( view, 0, sizeof( cg_viewdef_t ) );

	cg.predictedPlayerState = *trap_GetPlayerStateFromSnapsBackup( cg.frame.snapNum );
	cg.predictedEntityState = cg_entities[cg.predictedPlayerState.POVnum].current;

	//
	// VIEW SETTINGS
	//

	// by now, it will always be a playerview
	view->type = cg.predictedPlayerState.viewType;

	if( view->type == VIEWDEF_PLAYERVIEW )
	{
		// check thirdperson
		view->thirdperson = ( qboolean )( cg_thirdPerson->integer != 0 );

		// check for drawing gun
		if( !view->thirdperson )
		{
			view->drawWeapon = qtrue;
		}

		// enable/disable prediction
		if( cg.predictedPlayerState.POVnum == cgs.playerNum )
		{
			if( cg_predict->integer && !cgs.demoPlaying )
			{
				// can be predicted
				if( !view->thirdperson || cg_predict_thirdperson->integer )
					view->playerPrediction = qtrue;
			}
		}
	}
	else
	{
		GS_Error( "CG_SetupView: Invalid view type %i\n", view->type );
	}

	//
	// SETUP REFDEF FOR THE VIEW SETTINGS
	//

	if( view->type == VIEWDEF_PLAYERVIEW )
	{
		int i;
		vec3_t viewoffset;

		if( view->playerPrediction )
		{
			CG_Predict();

			VectorSet( viewoffset, 0.0f, 0.0f, cg.predictedPlayerState.viewHeight );

			for( i = 0; i < 3; i++ )
			{
				view->origin[i] = cg.predictedEntityState.ms.origin[i] + viewoffset[i] - ( 1.0f - cg.lerpfrac ) * cg.predictionError[i];
				view->angles[i] = cg.predictedEntityState.ms.angles[i];
			}

			// smooth out stair climbing
			CG_PredictSmoothOriginForSteps( view->origin );
		}
		else
		{
			const player_state_t *ps, *ops;

			cg.predictFrom = 0;

			ps = trap_GetPlayerStateFromSnapsBackup( cg.frame.snapNum );
			ops = trap_GetPlayerStateFromSnapsBackup( cg.oldFrame.snapNum );

			cg.predictedPlayerState.fov = ops->fov + cg.lerpfrac * ( ps->fov - ops->fov );
			assert( cg.predictedPlayerState.fov > 0.0f );
			cg.predictedPlayerState.viewHeight = ops->viewHeight + cg.lerpfrac * ( ps->viewHeight - ops->viewHeight );

			CG_InterpolatePlayerPosition( cg.predictedPlayerState.POVnum, cg.predictedEntityState.ms.origin, cg.predictedEntityState.ms.angles );
			
			VectorSet( viewoffset, 0.0f, 0.0f, cg.predictedPlayerState.viewHeight );
			VectorAdd( cg.predictedEntityState.ms.origin, viewoffset, view->origin );
			VectorCopy( cg.predictedEntityState.ms.angles, view->angles );
		}

		view->refdef.fov_x = cg.predictedPlayerState.fov;

		CG_CalcViewBob();

		VectorCopy( cg.predictedEntityState.ms.velocity, view->velocity );

		if( view->thirdperson )
			CG_ThirdPersonOffsetView( view );
	}

	// calculate width and height of our view setup

	// view rectangle size
	view->refdef.x = x;
	view->refdef.y = y;
	view->refdef.width = width;
	view->refdef.height = height;
	view->refdef.time = cg.time;
	view->refdef.areabits = cg.frame.areabits;

	view->refdef.fov_y = CalcFov( view->refdef.fov_x, view->refdef.width, view->refdef.height );
	view->fracDistFOV = tan( view->refdef.fov_x * ( M_PI/180 ) * 0.5f );

	AngleVectors( view->angles, view->axis[FORWARD], view->axis[RIGHT], view->axis[UP] );

	VectorCopy( view->origin, view->refdef.vieworg );
	Matrix_Copy( view->axis, view->refdef.viewaxis );
	VectorInverse( view->refdef.viewaxis[1] );

	if( view->drawWeapon )
		CG_CalcViewWeapon( &cg.weapon );
}

//=================
//CG_CalcViewRectangle
//=================
void CG_CalcViewRectangle( int vpos[4] )
{
	int x, y, width, height;

	// calculate the view sizes
	width = cgs.vidWidth;
	height = cgs.vidHeight;
	if( cg_view_width->integer > 0 )
	{
		width = cg_view_width->integer;
		clamp( width, 32, cgs.vidWidth );
	}
	if( cg_view_height->integer > 0 )
	{
		height = cg_view_height->integer;
		clamp( height, 32, cgs.vidHeight );
	}

	x = ( cgs.vidWidth * 0.5 ) - ( width * 0.5 );
	y = ( cgs.vidHeight * 0.5 ) - ( height * 0.5 );

	x += cg_view_x->integer;
	clamp( x, 0, cgs.vidWidth - width );

	y += cg_view_y->integer;
	clamp( y, 0, cgs.vidHeight - height );

	vpos[0] = x;
	vpos[1] = y;
	vpos[2] = width;
	vpos[3] = height;

	// fixme : quick clear
	if( ( width < cgs.vidWidth ) || ( height < cgs.vidHeight ) )
		trap_R_DrawStretchPic( 0, 0, cgs.vidWidth, cgs.vidHeight, 0, 1, 0, 1, colorBlack, CG_LocalShader( SHADER_WHITE ) );
}

//=================
//CG_InitView
//=================
void CG_InitView( void )
{

	cg_view_x = trap_Cvar_Get( "cg_view_x", "0", CVAR_ARCHIVE );
	cg_view_y = trap_Cvar_Get( "cg_view_y", "0", CVAR_ARCHIVE );
	cg_view_width =	trap_Cvar_Get( "cg_view_width", "0", CVAR_ARCHIVE );
	cg_view_height =    trap_Cvar_Get( "cg_view_height", "0", CVAR_ARCHIVE );

	cg_thirdPersonAngle =	trap_Cvar_Get( "cg_thirdPersonAngle", "0", CVAR_ARCHIVE );
	cg_thirdPersonRange =	trap_Cvar_Get( "cg_thirdPersonRange", "70", CVAR_ARCHIVE );
	cg_thirdPerson =	trap_Cvar_Get( "cg_thirdPerson", "0", CVAR_ARCHIVE );
}

//=================
//CG_RenderView
//=================
void CG_RenderView( float frameTime, int realTime, unsigned int serverTime, unsigned int extrapolationTime, float stereo_separation )
{
	int vpos[4];
	refdef_t *rd = &cg.view.refdef;

	cg.realTime = realTime;
	cg.frametime = frameTime;
	cg.frameCount++;
	cg.time = serverTime;
	cg.serverTime = serverTime;
	cg.extrapolationTime = extrapolationTime;

	if( !cgm.precacheDone || !cg.frame.valid )
	{
		CG_DrawLoadingScreen();
		return;
	}

	if( cg.oldFrame.timeStamp == cg.frame.timeStamp )
		cg.lerpfrac = 1.0f;
	else
		cg.lerpfrac = ( (double)( cg.time - cg.extrapolationTime ) - (double)cg.oldFrame.timeStamp ) / (double)( cg.frame.timeStamp - cg.oldFrame.timeStamp );

	if( cg.extrapolationTime )
	{
		cg.xerpTime = 0.001f * ( (double)cg.time - (double)cg.frame.timeStamp );
		cg.oldXerpTime = 0.001f * ( (double)cg.time - (double)cg.oldFrame.timeStamp );
		cg.xerpSmoothFrac = ( (double)cg.time - (double)( cg.frame.timeStamp - cg.extrapolationTime ) ) / (double)( cg.extrapolationTime );

		clamp( cg.xerpSmoothFrac, 0.0f, 1.0f );
	}
	else
	{
		cg.xerpTime = 0.0f;
		cg.xerpSmoothFrac = 0.0f;
	}

	CG_FixVolumeCvars();

	CG_RunLightStyles();
	
	CG_ClearFragmentedDecals();
	trap_R_ClearScene();

	CG_CalcViewRectangle( vpos );

	CG_SetupView( &cg.view, vpos[0], vpos[1], vpos[2], vpos[3] );

	CG_InterpolateEntities();   // interpolate packet entities positions

	if( cg.snap.newEntityEvents )
		CG_FireEvents();

	// JALFIXME TEMP REMOVE ME
	if( trap_Cvar_Value( "cg_drawOcTree" ) )
		GS_DrawSpacePartition();

	// build a refresh entity list
	CG_AddEntitiesToScene();
	CG_AddLocalEntities();
	CG_AddDlights();
	CG_AddDecals();
	CG_AddLightStyles();
#ifdef CGAMEGETLIGHTORIGIN
	CG_AddShadeBoxes();
#endif
	CG_CheckCrossHairActivation(); // temporary location so it can draw polygons
	CG_AddPolys();
	if( cg.view.drawWeapon )
		CG_AddViewWeapon( &cg.weapon );

	// finish the refdef
	rd->rdflags = CG_RenderFlags();
	// warp if underwater
	if( rd->rdflags & RDF_UNDERWATER )
	{
		float phase = rd->time * 0.001 * WAVE_FREQUENCY * M_TWOPI;
		float v = WAVE_AMPLITUDE * ( sin( phase ) - 1.0 ) + 1;
		rd->fov_x *= v;
		rd->fov_y *= v;
	}

	// offset vieworg appropriately if we're doing stereo separation
	if( stereo_separation != 0 )
		VectorMA( cg.view.origin, stereo_separation, cg.view.axis[RIGHT], rd->vieworg );

	// never let it sit exactly on a node line, because a water plane can
	// disappear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/16 pixel, so add 1/16 in each axis
	rd->vieworg[0] += 1.0/PM_VECTOR_SNAP;
	rd->vieworg[1] += 1.0/PM_VECTOR_SNAP;
	rd->vieworg[2] += 1.0/PM_VECTOR_SNAP;

	if( cg_entities[cg.predictedPlayerState.POVnum].current.solid == SOLID_NOT )  // force clear if can reach out world
		trap_Cvar_ForceSet( "scr_forceclear", "1" );
	else
		trap_Cvar_ForceSet( "scr_forceclear", "0" );

	CG_AddLocalSceneSounds();
	CG_SetSceneTeamColors(); // update the team colors in the renderer

	trap_R_RenderScene( rd );

	cg.oldAreabits = qtrue;

	trap_S_Update( cg.view.origin, cg.view.velocity, cg.view.axis[FORWARD], cg.view.axis[RIGHT], cg.view.axis[UP] );

	CG_Draw2D();

	CG_ResetTemporaryBoneposesCache();

	return;
}
