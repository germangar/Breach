/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

//==================================================
// WEAPON MODELS MANAGEMENT
//==================================================

static weaponobject_t *cg_weaponObjects = NULL;

static const char *woPartNames[] = { "weapon", "barrel", "flash", "hand", NULL };

static char *weaponObjectDefaultAnimation =
        "// SYNTHAX : [firstframe] [lastframe] [frameback] [fps]\n"
        "rotationscale 1\n"
        "0  29  0  15 // HOLD\n"
		"0  29  0  15 // HOLD_ALTFIRE\n"
        "31 40  0  15 // ATTACK WEAK\n"
        "41 50  0  15 // ATTACK ALTFIRE\n"
		"55 59  0  15 // WEAPONUP\n"
        "51 55  1  15 // WEAPONDOWN\n"
        "55 59  0  15 // ALTFIRE_UP\n"
		"51 55  1  15 // ALTFIRE_DOWN\n";

cvar_t *cg_gun;
cvar_t *cg_gun_forward;
cvar_t *cg_gun_up;
cvar_t *cg_gun_right;
cvar_t *cg_gun_handoffset;
cvar_t *cg_gun_fov;


/*
* CG_vWeap_ParseAnimationScript
* 
* script:
* 0 = first frame
* 1 = lastframe
* 2 = looping frames
* 3 = frame time
* 
* keywords:
*   "rotationscale": value witch will scale the barrel rotation speed
*/
static qboolean CG_WeaponObject_ParseAnimationScript( char *buf, weaponobject_t *weaponObject, char *filename )
{
	char *ptr, *token;
	int rounder, counter, i;
	qboolean debug = qtrue;
	int anim_data[4][WEAPMODEL_MAXANIMS];

	if( !buf )
	{
		if( debug )
			GS_Printf( "%sCouldn't load weapon object script:%s%s\n", S_COLOR_BLUE, filename, S_COLOR_WHITE );
		return qfalse;
	}

	rounder = 0;
	counter = 1; // reserve 0 for 'no animation'

	memset( anim_data, 0, sizeof( anim_data ) );

	weaponObject->rotationscale = 1; // default
	VectorClear( weaponObject->handpositionOrigin );
	VectorClear( weaponObject->handpositionAngles );
	weaponObject->flashTime = 150;
	weaponObject->flashRadius = 300;
	weaponObject->flashFade = qtrue;
	VectorSet( weaponObject->flashColor, 1, 1, 1 );
	weaponObject->num_fire_sounds = 0;

	if( debug )
		GS_Printf( "%sLoading weapon object script:%s%s\n", S_COLOR_BLUE, filename, S_COLOR_WHITE );

	//proceed
	ptr = buf;
	while( ptr )
	{
		token = COM_ParseExt( &ptr, qtrue );
		if( !token[0] )
			break;

		//see if it is keyword or number
		if( *token < '0' || *token > '9' )
		{

			if( !Q_stricmp( token, "rotationscale" ) )
			{
				if( debug )
					GS_Printf( "%sScript: rotation scale:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				token = COM_ParseExt( &ptr, qfalse );
				if( !token[0] ) break;
				weaponObject->rotationscale = atoi( token );

				if( debug )
					GS_Printf( "%s%f%s\n", S_COLOR_BLUE, weaponObject->rotationscale, S_COLOR_WHITE );

			}
			else if( !Q_stricmp( token, "handOffset" ) )
			{
				if( debug )
					GS_Printf( "%sScript: handPosition:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				weaponObject->handpositionOrigin[FORWARD] = atof( COM_ParseExt( &ptr, qfalse ) );
				weaponObject->handpositionOrigin[RIGHT] = atof( COM_ParseExt( &ptr, qfalse ) );
				weaponObject->handpositionOrigin[UP] = atof( COM_ParseExt( &ptr, qfalse ) );
				weaponObject->handpositionAngles[PITCH] = atof( COM_ParseExt( &ptr, qfalse ) );
				weaponObject->handpositionAngles[YAW] = atof( COM_ParseExt( &ptr, qfalse ) );
				weaponObject->handpositionAngles[ROLL] = atof( COM_ParseExt( &ptr, qfalse ) );

				if( debug )
					GS_Printf( "%s%f %f %f %f %f %f%s\n", S_COLOR_BLUE,
					           weaponObject->handpositionOrigin[0], weaponObject->handpositionOrigin[1], weaponObject->handpositionOrigin[2],
					           weaponObject->handpositionAngles[0], weaponObject->handpositionAngles[1], weaponObject->handpositionAngles[2],
					           S_COLOR_WHITE );

			}
			else if( !Q_stricmp( token, "flash" ) )
			{
				if( debug )
					GS_Printf( "%sScript: flash:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				// time
				i = atoi( COM_ParseExt( &ptr, qfalse ) );
				weaponObject->flashTime = (float)( i > 0 ? i : 0 );

				// radius
				i = atoi( COM_ParseExt( &ptr, qfalse ) );
				weaponObject->flashRadius = (float)( i > 0 ? i : 0 );

				// fade
				token = COM_ParseExt( &ptr, qfalse );
				if( !Q_stricmp( token, "no" ) )
					weaponObject->flashFade = qfalse;

				if( debug )
					GS_Printf( "%s time:%i, radius:%i, fade:%s%s\n", S_COLOR_BLUE, (int)weaponObject->flashTime, (int)weaponObject->flashRadius, weaponObject->flashFade ? "YES" : "NO", S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "flashColor" ) )
			{
				if( debug )
					GS_Printf( "%sScript: flashColor:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				weaponObject->flashColor[0] = atoi( token = COM_ParseExt( &ptr, qfalse ) );
				weaponObject->flashColor[1] = atof( token = COM_ParseExt( &ptr, qfalse ) );
				weaponObject->flashColor[2] = atof( token = COM_ParseExt( &ptr, qfalse ) );

				if( debug )
					GS_Printf( "%s%f %f %f%s\n", S_COLOR_BLUE,
					           weaponObject->flashColor[0], weaponObject->flashColor[1], weaponObject->flashColor[2],
					           S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "firesound" ) )
			{
				if( debug )
					GS_Printf( "%sScript: firesound:%s", S_COLOR_BLUE, S_COLOR_WHITE );
				if( weaponObject->num_fire_sounds >= WEAPONOBJECT_MAX_FIRE_SOUNDS )
				{
					if( debug )
						GS_Printf( S_COLOR_BLUE "too many firesounds defined. Max is %i" S_COLOR_WHITE "\n", WEAPONOBJECT_MAX_FIRE_SOUNDS );
					break;
				}

				token = COM_ParseExt( &ptr, qfalse );
				if( Q_stricmp( token, "NULL" ) )
				{
					weaponObject->sound_fire[weaponObject->num_fire_sounds] = trap_S_RegisterSound( token );
					if( weaponObject->sound_fire[weaponObject->num_fire_sounds] != NULL )
						weaponObject->num_fire_sounds++;
				}
				if( debug )
					GS_Printf( "%s%s%s\n", S_COLOR_BLUE, token, S_COLOR_WHITE );
			}
			else if( !Q_stricmp( token, "reloadsound" ) )
			{
				if( debug )
					GS_Printf( "%sScript: reloadsound:%s", S_COLOR_BLUE, S_COLOR_WHITE );

				token = COM_ParseExt( &ptr, qfalse );
				if( Q_stricmp( token, "NULL" ) )
					weaponObject->sound_reload = trap_S_RegisterSound( token );
				if( debug )
					GS_Printf( "%s%s%s\n", S_COLOR_BLUE, token, S_COLOR_WHITE );
			}
			else if( token[0] && debug )
				GS_Printf( "%signored: %s%s\n", S_COLOR_YELLOW, token, S_COLOR_WHITE );

		}
		else
		{

			// frame & animation values
			i = (int)atoi( token );
			if( debug )
			{
				if( rounder == 0 )
					GS_Printf( "%sScript: %s", S_COLOR_BLUE, S_COLOR_WHITE );
				GS_Printf( "%s%i - %s", S_COLOR_BLUE, i, S_COLOR_WHITE );
			}
			anim_data[rounder][counter] = i;
			rounder++;
			if( rounder > 3 )
			{
				rounder = 0;
				if( debug )
					GS_Printf( "%s anim: %i%s\n", S_COLOR_BLUE, counter, S_COLOR_WHITE );
				counter++;
				if( counter == WEAPMODEL_MAXANIMS )
					break;
			}
		}
	}

	if( counter < WEAPMODEL_MAXANIMS )
	{
		GS_Printf( "%sERROR: incomplete WEAPON script: %s - Using default%s\n", S_COLOR_YELLOW, filename, S_COLOR_WHITE );
		return qfalse;
	}

	//reorganize to make my life easier
	for( i = 0; i < WEAPMODEL_MAXANIMS; i++ )
	{
		weaponObject->firstframe[i] = anim_data[0][i];
		weaponObject->lastframe[i] = anim_data[1][i];

		if( weaponObject->lastframe[i] < weaponObject->firstframe[i] )
			weaponObject->lastframe[i] = weaponObject->firstframe[i];

		weaponObject->loopingframes[i] = anim_data[2][i];
		if( weaponObject->loopingframes[i] > weaponObject->lastframe[i] - weaponObject->firstframe[i] + 1 )
			weaponObject->loopingframes[i] = weaponObject->lastframe[i] - weaponObject->firstframe[i] + 1;

		weaponObject->frametime[i] = 1000.0f/(float)( ( anim_data[3][i] < 10 ) ? 10 : anim_data[3][i] );
	}

	if( weaponObject->loopingframes[WEAPMODEL_WEAPDOWN] <= 0 )
		weaponObject->loopingframes[WEAPMODEL_WEAPDOWN] = 1;

	return qtrue;
}

/* 
* CG_WeaponObject_BuildProjectionTag
* store the orientation_t closer to the tag_flash we can create,
* or create one using an offset we consider acceptable.
* NOTE: This tag will ignore weapon models animations. You'd have to
* do it in realtime to use it with animations. Or be careful on not
* moving the weapon too much
*/
static void CG_WeaponObject_BuildProjectionTag( weaponobject_t *weaponObject )
{
	orientation_t tag, tag_barrel;
	static entity_t	ent;

	if( !weaponObject )
		return;

	if( weaponObject->model[WEAPON] )
	{
		// assign the model to an entity_t, so we can build boneposes
		memset( &ent, 0, sizeof( ent ) );
		ent.rtype = RT_MODEL;
		ent.scale = 1.0f;
		ent.model = weaponObject->model[WEAPON];
		CG_SetBoneposesForTemporaryEntity( &ent ); // assigns and builds the skeleton so we can use grabtag

		// try getting the tag_flash from the weapon model
		if( CG_GrabTag( &weaponObject->tag_projectionsource, &ent, "tag_flash" ) )
			return; // successfully

		// if it didn't work, try getting it from the barrel model
		if( CG_GrabTag( &tag_barrel, &ent, "tag_barrel" ) && weaponObject->model[BARREL] )
		{
			// assign the model to an entity_t, so we can build boneposes
			memset( &ent, 0, sizeof( ent ) );
			ent.rtype = RT_MODEL;
			ent.scale = 1.0f;
			ent.model = weaponObject->model[BARREL];
			CG_SetBoneposesForTemporaryEntity( &ent );
			if( CG_GrabTag( &tag, &ent, "tag_flash" ) && weaponObject->model[BARREL] )
			{
				VectorCopy( vec3_origin, weaponObject->tag_projectionsource.origin );
				Matrix_Identity( weaponObject->tag_projectionsource.axis );
				CG_MoveToTag( weaponObject->tag_projectionsource.origin,
				              weaponObject->tag_projectionsource.axis,
				              tag_barrel.origin,
				              tag_barrel.axis,
				              tag.origin,
				              tag.axis );
				return; // successfully
			}
		}
	}

	// doesn't have a weapon model, or the weapon model doesn't have a tag
	VectorSet( weaponObject->tag_projectionsource.origin, 16, 0, 8 );
	Matrix_Identity( weaponObject->tag_projectionsource.axis );
}

// compute full object bounds
void CG_WeaponObject_CalcBounds( weaponobject_t *weaponObject )
{
	orientation_t tag;
	vec3_t points[8], point;
	vec3_t axis[3];
	entity_t ent;
	int i;
	vec3_t mins[WEAPMODEL_PARTS], maxs[WEAPMODEL_PARTS];

	VectorClear( weaponObject->mins );
	VectorClear( weaponObject->maxs );

	for( i = 0; i < WEAPMODEL_PARTS; i++ )
	{
		if( weaponObject->model[i] && ( i < FLASH ) )
		{
			// fixme: this will NOT work with skm models, and we want to use them
			trap_R_ModelBounds( weaponObject->model[i], mins[i], maxs[i] );

		}
	}

	// convert the relative bounds to weaponObject space (which is origin zero, so absolute space too)

	// WEAPON
	BuildBoxPoints( points, vec3_origin, mins[WEAPON], maxs[WEAPON] );
	for( i = 0; i < 8; i++ )
		AddPointToBounds( points[i], weaponObject->mins, weaponObject->maxs );

	memset( &ent, 0, sizeof( ent ) );

	ent.model = weaponObject->model[WEAPON];
	ent.backlerp = 1.0f;

	// BARREL
	if( !CG_GrabTag( &tag, &ent, "tag_barrel" ) )
		return;

	// rotate the barrel AAbox to the tag orientation
	Matrix_Transpose( tag.axis, axis );

	// add rotated points to bounds
	BuildBoxPoints( points, vec3_origin, mins[BARREL], maxs[BARREL] );
	for( i = 0; i < 8; i++ )
	{
		Matrix_TransformVector( axis, points[i], point );
		VectorAdd( point, tag.origin, point );
		AddPointToBounds( point, weaponObject->mins, weaponObject->maxs );
	}
}

/*
* CG_LoadWeaponObject
*/
static weaponobject_t *CG_LoadWeaponObject( const char *name )
{
	int i;
	weaponobject_t *weaponObject;
	char scratch[MAX_QPATH];
	qbyte *buf;
	int length, filenum;

	weaponObject = ( weaponobject_t * )CG_Malloc( sizeof( weaponobject_t ) );

	for( i = 0; i < WEAPMODEL_PARTS; i++ )
	{
		if( !weaponObject->model[i] ) // skm
		{
			Q_snprintfz( scratch, sizeof( scratch ), "%s%s/%s.skm", GS_WeaponObjects_BasePath(), name, woPartNames[i] );
			weaponObject->model[i] = CG_RegisterModel( scratch );
		}
		if( !weaponObject->model[i] ) // md3
		{
			Q_snprintfz( scratch, sizeof( scratch ), "%s%s/%s.md3", GS_WeaponObjects_BasePath(), name, woPartNames[i] );
			weaponObject->model[i] = CG_RegisterModel( scratch );
		}

		weaponObject->skel[i] = CG_SkeletonForModel( weaponObject->model[i] );
	}

	//load failed
	if( !weaponObject->model[HAND] )
	{
		CG_Free( weaponObject );
		return NULL;
	}

	Q_strncpyz( weaponObject->name, name, sizeof( weaponObject->name ) );

	//Load animation script for the hand model
	Q_snprintfz( scratch, sizeof( scratch ), "%s%s/weaponobject.cfg", GS_WeaponObjects_BasePath(), name );

	// load the file
	buf = NULL;
	length = trap_FS_FOpenFile( scratch, &filenum, FS_READ );
	if( !length )
	{
		trap_FS_FCloseFile( filenum );
	}
	else if( length != -1 )
	{
		buf = ( qbyte * )CG_Malloc( length + 1 );
		trap_FS_Read( buf, length, filenum );
		trap_FS_FCloseFile( filenum );
	}

	if( !CG_WeaponObject_ParseAnimationScript( (char *)buf, weaponObject, scratch ) )
		if( !CG_WeaponObject_ParseAnimationScript( weaponObjectDefaultAnimation, weaponObject, "default" ) )
			GS_Error( "CG_LoadWeaponObject: failed to parse default weapon animation script" );

	if( buf )
		CG_Free( buf );

	CG_WeaponObject_CalcBounds( weaponObject );
	CG_WeaponObject_BuildProjectionTag( weaponObject );

	return weaponObject;
}

/*
* CG_RegisterWeaponObject
*/
weaponobject_t *CG_RegisterWeaponObject( const char *cstring )
{
	weaponobject_t *weaponObject;
	char name[MAX_QPATH];

	if( !cstring || !cstring[0] )
		return NULL;

	if( cstring[0] != '#' )
	{
		GS_Printf( "WARNING: CG_RegisterWeaponObject: Bad weapon object name %s\n", cstring );
		return NULL;
	}

	Q_strncpyz( name, cstring + 1, sizeof( name ) );
	if( !name[0] )
		return NULL;

	COM_StripExtension( name );

	// see if it's already registered
	for( weaponObject = cg_weaponObjects; weaponObject; weaponObject = weaponObject->next )
	{
		if( !Q_stricmp( weaponObject->name, name ) )
			return weaponObject;
	}

	// register it
	weaponObject = CG_LoadWeaponObject( name );
	if( !weaponObject )
		return NULL;

	weaponObject->next = cg_weaponObjects;
	cg_weaponObjects = weaponObject;

	return weaponObject;
}

/*
* CG_WeaponObjectFromIndex
*/
struct weaponobject_s *CG_WeaponObjectFromIndex( int index )
{
	assert( index >= 0 && ( index < WEAP_TOTAL ) );
	return cgm.indexedWeaponObjects[index] ? cgm.indexedWeaponObjects[index] : cgm.indexedWeaponObjects[WEAP_NONE];
}

/*
* CG_InitWeapons
*/
void CG_InitWeapons( void )
{
	cg_gun = trap_Cvar_Get( "cg_gun", "1", CVAR_ARCHIVE );
	cg_gun_forward = trap_Cvar_Get( "cg_gun_forward", "0", CVAR_DEVELOPER );
	cg_gun_up = trap_Cvar_Get( "cg_gun_up", "0", CVAR_DEVELOPER );
	cg_gun_right = trap_Cvar_Get( "cg_gun_right", "0", CVAR_DEVELOPER );
	cg_gun_handoffset = trap_Cvar_Get( "cg_gun_handoffset", "7", CVAR_DEVELOPER );
	cg_gun_fov = trap_Cvar_Get( "cg_gun_fov", "90", CVAR_ARCHIVE );
}

/*
* CG_AddWeaponOnTag
*/
void CG_AddWeaponObject( weaponobject_t *weaponObject, vec3_t ref_origin, vec3_t ref_axis[3], vec3_t ref_lightorigin, int renderfx, unsigned int flash_time )
{
	entity_t weapon;
	orientation_t tag;
	cgs_skeleton_t *skel;

	if( !weaponObject )
		return;

	//weapon
	memset( &weapon, 0, sizeof( weapon ) );
	Vector4Set( weapon.shaderRGBA, 255, 255, 255, 255 );
	weapon.rtype = RT_MODEL;
	weapon.scale = 1.0f;
	weapon.renderfx = renderfx;
	weapon.frame = 0;
	weapon.oldframe = 0;
	weapon.model = weaponObject->model[WEAPON];
	VectorCopy( ref_origin, weapon.origin );
	VectorCopy( ref_origin, weapon.origin2 );
	VectorCopy( ref_lightorigin, weapon.lightingOrigin );
	Matrix_Copy( ref_axis, weapon.axis );

	if( weaponObject->skel[WEAPON] )
	{
		skel = weaponObject->skel[WEAPON];
		weapon.boneposes = weapon.oldboneposes = CG_RegisterTemporaryExternalBoneposes( skel );
		CG_LerpSkeletonPoses( skel, weapon.frame, weapon.oldframe, weapon.boneposes, 1.0 - weapon.backlerp );
		CG_TransformBoneposes( skel, weapon.boneposes, weapon.boneposes );
	}

	CG_AddEntityToScene( &weapon );

	if( !weapon.model )
		return;

	// barrel
	if( weaponObject->model[BARREL] )
	{
		if( CG_GrabTag( &tag, &weapon, "tag_barrel" ) )
		{
			//float scaledTime;

			entity_t barrel;
			memset( &barrel, 0, sizeof( barrel ) );
			barrel = weapon;
			Vector4Set( barrel.shaderRGBA, 255, 255, 255, 255 );
			barrel.model = weaponObject->model[BARREL];
			barrel.scale = weapon.scale;
			barrel.renderfx = weapon.renderfx;
			barrel.frame = 0;
			barrel.oldframe = 0;

			/*
			   // rotation
			   scaledTime = cg.frameTime*100; // not precise, but enough

			   pweapon->rotationSpeed += scaledTime*((pweapon->barreltime > cg.time) * (pweapon->rotationSpeed < 8));
			   pweapon->rotationSpeed -= scaledTime/15;
			   if( pweapon->rotationSpeed < 0 )
			    pweapon->rotationSpeed = 0.0f;

			   pweapon->angles[2] += scaledTime * pweapon->rotationSpeed * weaponObject->rotationscale;
			   if( pweapon->angles[2] > 360 )
			    pweapon->angles[2] -= 360;

			   AnglesToAxis( pweapon->angles, barrel.axis );
			 */
			Matrix_Identity( barrel.axis );

			CG_PutRotatedEntityAtTag( &barrel, &tag, weapon.origin, weapon.axis, weapon.lightingOrigin );

			if( weaponObject->skel[BARREL] )
			{
				skel = weaponObject->skel[BARREL];
				barrel.boneposes = barrel.oldboneposes = CG_RegisterTemporaryExternalBoneposes( skel );
				CG_LerpSkeletonPoses( skel, barrel.frame, barrel.oldframe, barrel.boneposes, 1.0 - barrel.backlerp );
				CG_TransformBoneposes( skel, barrel.boneposes, barrel.boneposes );
			}

			CG_AddEntityToScene( &barrel );
		}
	}

	if( flash_time <= cg.time )
		return;

	// flash
	if( !CG_GrabTag( &tag, &weapon, "tag_flash" ) )
		return;

	if( weaponObject->model[FLASH] )
	{
		entity_t flash;
		float intensity;
		qbyte c;

		if( weaponObject->flashFade )
		{
			intensity =  (float)( flash_time - cg.time )/weaponObject->flashTime;
			c = ( qbyte )( 255 * intensity );
		}
		else
		{
			intensity = 1.0f;
			c = 255;
		}

		memset( &flash, 0, sizeof( flash ) );
		flash = weapon;
		Vector4Set( flash.shaderRGBA, c, c, c, c );
		flash.model = weaponObject->model[FLASH];
		flash.scale = weapon.scale;
		flash.renderfx = weapon.renderfx | RF_NOSHADOW;
		flash.frame = 0;
		flash.oldframe = 0;

		CG_PutEntityAtTag( &flash, &tag, weapon.origin, weapon.axis, weapon.lightingOrigin );

		if( weaponObject->skel[FLASH] )
		{
			skel = weaponObject->skel[FLASH];
			flash.boneposes = flash.oldboneposes = CG_RegisterTemporaryExternalBoneposes( skel );
			CG_LerpSkeletonPoses( skel, flash.frame, flash.oldframe, flash.boneposes, 1.0 - flash.backlerp );
			CG_TransformBoneposes( skel, flash.boneposes, flash.boneposes );
		}

		CG_AddEntityToScene( &flash );

		CG_AddLightToScene( flash.origin, weaponObject->flashRadius * intensity,
		                    weaponObject->flashColor[0], weaponObject->flashColor[1], weaponObject->flashColor[2], NULL );
	}
}

//==================================================
// VIEW WEAPON
//==================================================

static void CG_AddBobbingAngles( vec3_t angles )
{
	int i;
	float delta;

	// gun angles from bobbing
	if( cg.bobCycle & 1 )
	{
		angles[ROLL] -= cg.xyspeed * cg.bobFracSin * 0.012;
		angles[YAW] -= cg.xyspeed * cg.bobFracSin * 0.006;
	}
	else
	{
		angles[ROLL] += cg.xyspeed * cg.bobFracSin * 0.012;
		angles[YAW] += cg.xyspeed * cg.bobFracSin * 0.006;
	}
	angles[PITCH] += cg.xyspeed * cg.bobFracSin * 0.012;

	// gun angles from delta movement
	for( i = 0; i < 3; i++ )
	{
		if( ISVIEWERENTITY( cg.predictedPlayerState.POVnum ) )
			delta = ( cg_entities[cg.predictedPlayerState.POVnum].current.ms.angles[i] - cg_entities[cg.predictedPlayerState.POVnum].prev.ms.angles[i] ) * cg.lerpfrac;
		else
			delta = 0;

		if( delta > 180 )
			delta -= 360;
		if( delta < -180 )
			delta += 360;
		clamp( delta, -45, 45 );


		if( i == YAW )
			angles[ROLL] += 0.001 * delta;
		angles[i] += 0.002 * delta;
	}
}

void CG_AddKickAngles( vec3_t angles )
{
}

static int CG_ViewWeapon_baseAnimFromWeaponState( int weaponState, int weapon, int mode )
{
	int anim;

	switch( weaponState )
	{
	case WEAPON_STATE_ACTIVATING:
		anim = WEAPMODEL_WEAPONUP;
		break;

	case WEAPON_STATE_DROPPING:
		anim = WEAPMODEL_WEAPDOWN;
		break;

		/* fall through. Activated by event */
	case WEAPON_STATE_RELOADING:
	case WEAPON_STATE_NOAMMOCLICK:
		/* fall through. Not used */
	default:
		anim = WEAPMODEL_STANDBY;
		break;

	case WEAPON_STATE_POWERING:
	case WEAPON_STATE_FIRING:
	case WEAPON_STATE_REFIRE:
	case WEAPON_STATE_READY:
	case WEAPON_STATE_CHANGING_MODE:
		anim = ( mode & ~EV_INVERSE ) ? WEAPMODEL_HOLD_ALTFIRE : WEAPMODEL_STANDBY;
		break;
	}

	return anim;
}

static void CG_ViewWeapon_UpdateAnimation( cg_viewweapon_t *viewweapon )
{
	int baseAnim;
	weaponobject_t *weaponObject;
	int curframe = 0;
	float framefrac;
	qboolean nolerp = qfalse;

	// if the pov changed, force restart
	if( viewweapon->POVnum != cg.predictedPlayerState.POVnum
		|| viewweapon->lastWeapon != cg.predictedEntityState.weapon )
	{
		nolerp = qtrue;
		viewweapon->eventAnim = 0;
		viewweapon->eventAnimStartTime = 0;
		viewweapon->baseAnim = 0;
		viewweapon->baseAnimStartTime = 0;
	}

	viewweapon->POVnum = cg.predictedPlayerState.POVnum;
	viewweapon->lastWeapon = cg.predictedEntityState.weapon;

	// hack cause of missing animation config
	if( viewweapon->lastWeapon == WEAP_NONE )
	{
		viewweapon->ent.frame = viewweapon->ent.oldframe = 0;
		viewweapon->ent.backlerp = 0.0f;
		viewweapon->eventAnim = 0;
		viewweapon->eventAnimStartTime = 0;
		return;
	}

	baseAnim = CG_ViewWeapon_baseAnimFromWeaponState( cg.predictedPlayerState.weaponState, cg.predictedEntityState.weapon, cg.predictedPlayerState.stats[STAT_WEAPON_MODE] );
	weaponObject = CG_WeaponObjectFromIndex( cg.predictedEntityState.weapon );

	// full restart
	if( !viewweapon->baseAnim || !viewweapon->baseAnimStartTime ) 
	{
		viewweapon->baseAnim = baseAnim;
		viewweapon->baseAnimStartTime = cg.time;
		nolerp = qtrue;
	}

	// base animation changed?
	if( baseAnim != viewweapon->baseAnim )
	{
		viewweapon->baseAnim = baseAnim;
		viewweapon->baseAnimStartTime = cg.time;
	}

	// if a eventual animation is running override the baseAnim
	if( viewweapon->eventAnim )
	{
		if( !viewweapon->eventAnimStartTime )
			viewweapon->eventAnimStartTime = cg.time;

		framefrac = GS_FrameForTime( &curframe, cg.time, viewweapon->eventAnimStartTime, weaponObject->frametime[viewweapon->eventAnim],
			weaponObject->firstframe[viewweapon->eventAnim], weaponObject->lastframe[viewweapon->eventAnim],
			weaponObject->loopingframes[viewweapon->eventAnim], qfalse );

		if( curframe >= 0 )
			goto setupframe;

		// disable event anim and fall through
		viewweapon->eventAnim = 0;
		viewweapon->eventAnimStartTime = 0;
	}

	// find new frame for the current animation
	framefrac = GS_FrameForTime( &curframe, cg.time, viewweapon->baseAnimStartTime, weaponObject->frametime[viewweapon->baseAnim],
		weaponObject->firstframe[viewweapon->baseAnim], weaponObject->lastframe[viewweapon->baseAnim],
		weaponObject->loopingframes[viewweapon->baseAnim], qtrue );

	if( curframe < 0 )
		GS_Error( "CG_ViewWeapon_UpdateAnimation(2): Base Animation without a defined loop.\n" );

setupframe:
	if( nolerp )
	{
		framefrac = 0;
		viewweapon->ent.oldframe = curframe;
	}
	else
	{
		clamp( framefrac, 0, 1 );
		if( curframe != viewweapon->ent.frame )
			viewweapon->ent.oldframe = viewweapon->ent.frame;
	}

	viewweapon->ent.frame = curframe;
	viewweapon->ent.backlerp = 1.0f - framefrac;
}

/*
* CG_ViewWeapon_AddAnimation
*/
void CG_ViewWeapon_StartAnimationEvent( int newAnim )
{
	assert( newAnim > 0 && newAnim < WEAPMODEL_MAXANIMS );

	if( cg.view.drawWeapon )
	{
		cg.weapon.eventAnim = newAnim;
		cg.weapon.eventAnimStartTime = cg.time;
	}
}

/*
* CG_CalcViewWeapon
*/
void CG_CalcViewWeapon( cg_viewweapon_t *viewweapon )
{
	weaponobject_t *weaponObject;

	CG_ViewWeapon_UpdateAnimation( viewweapon ); // weapon index may change here

	weaponObject = CG_WeaponObjectFromIndex( cg.predictedEntityState.weapon );

	viewweapon->ent.model = weaponObject->model[HAND];
	viewweapon->ent.renderfx = ( RF_MINLIGHT|RF_WEAPONMODEL|RF_NOSHADOW|RF_FORCENOLOD );
	viewweapon->ent.scale = 1.0f;
	viewweapon->ent.customShader = NULL;
	viewweapon->ent.customSkin = NULL;
	viewweapon->ent.rtype = RT_MODEL;
	Vector4Set( viewweapon->ent.shaderRGBA, 255, 255, 255, 255 );

	// set up hand position
	{
		vec3_t angles;
		vec3_t offset;

		// find angles
		VectorAdd( cg.view.angles, weaponObject->handpositionAngles, angles );

		CG_AddBobbingAngles( angles ); // add angles from bobbing
		CG_AddKickAngles( angles ); // add angles from kicks

		AnglesToAxis( angles, viewweapon->ent.axis );

		// find origin
		VectorCopy( cg.view.origin, viewweapon->ent.origin );

		offset[FORWARD] = cg_gun_forward->value + weaponObject->handpositionOrigin[FORWARD];
		offset[RIGHT] = cg_gun_right->value + weaponObject->handpositionOrigin[RIGHT];
		offset[UP] = cg_gun_up->value + weaponObject->handpositionOrigin[UP];

		// add the left/right hand offset
		offset[RIGHT] += cg_gun_handoffset->value;

		// mirror if left handed
		if( cgs.clientInfo[cg.predictedPlayerState.POVnum].hand )
		{
			VectorInverse( viewweapon->ent.axis[RIGHT] );
			viewweapon->ent.flags |= RF_CULLHACK;
			offset[RIGHT] = -offset[RIGHT];
		}

		// offset the origin
		VectorMA( viewweapon->ent.origin, offset[FORWARD], cg.view.axis[FORWARD], viewweapon->ent.origin );
		VectorMA( viewweapon->ent.origin, offset[RIGHT], cg.view.axis[RIGHT], viewweapon->ent.origin );
		VectorMA( viewweapon->ent.origin, offset[UP], cg.view.axis[UP], viewweapon->ent.origin );
	}

	if( weaponObject->skel[HAND] )
	{
		cgs_skeleton_t *skel;
		skel = weaponObject->skel[HAND];
		viewweapon->ent.boneposes = viewweapon->ent.oldboneposes = CG_RegisterTemporaryExternalBoneposes( skel );
		CG_LerpSkeletonPoses( skel, viewweapon->ent.frame, viewweapon->ent.oldframe, viewweapon->ent.boneposes, 1.0 - viewweapon->ent.backlerp );
		CG_TransformBoneposes( skel, viewweapon->ent.boneposes, viewweapon->ent.boneposes );
	}

	if( cg_gun_fov->integer && ( cg.predictedPlayerState.controlTimers[USERINPUT_STAT_ZOOMTIME] <= 0 ) )
	{
		float fracWeapFOV = ( 1.0f / cg.view.fracDistFOV ) * tan( cg_gun_fov->integer * ( M_PI/180 ) * 0.5f );
		VectorScale( viewweapon->ent.axis[FORWARD], fracWeapFOV, viewweapon->ent.axis[FORWARD] );
	}

	

	/*
	   //hand entity

	   //if the player doesn't want to view the weapon we still have to build the projection source
	   if( CG_GrabTag( &tag, &viewweapon->ent, "tag_weapon" ) )
	    CG_ViewWeapon_UpdateProjectionSource( viewweapon->origin, viewweapon->axis, tag.origin, tag.axis );
	   else
	    CG_ViewWeapon_UpdateProjectionSource( viewweapon->origin, viewweapon->axis, vec3_origin, axis_identity );
	 */
}

/*
* CG_AddViewWeapon
*/
void CG_AddViewWeapon( cg_viewweapon_t *viewweapon )
{
	orientation_t tag;

	if( !cg_gun->integer || !cg.view.drawWeapon || cg.predictedEntityState.weapon == WEAP_NONE )
		return;

	// update lightingOrigin with the player entity one
	VectorCopy( cg_entities[cg.predictedPlayerState.POVnum].ent.lightingOrigin, viewweapon->ent.lightingOrigin );
	VectorCopy( viewweapon->ent.origin, viewweapon->ent.origin2 );

	CG_AddEntityToScene( &viewweapon->ent );

	// add attached weapon
	if( CG_GrabTag( &tag, &viewweapon->ent, "tag_weapon" ) )
	{
		vec3_t origin, axis[3];
		VectorClear( origin );
		Matrix_Identity( axis );
		CG_MoveToTag( origin, axis, tag.origin, tag.axis, viewweapon->ent.origin, viewweapon->ent.axis );
		CG_AddWeaponObject( CG_WeaponObjectFromIndex( cg.predictedEntityState.weapon ), origin, axis,
			viewweapon->ent.lightingOrigin, viewweapon->ent.renderfx,
			cg_entities[cg.predictedPlayerState.POVnum].flash_time );
	}
}
