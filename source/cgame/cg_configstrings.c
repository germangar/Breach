/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"


/*
* CG_RegisterLevelScreenShot
*/
static void CG_RegisterLevelScreenShot( const char *cstring )
{
	char filename[MAX_QPATH], shotname[MAX_QPATH];

	if( cstring[0] )
		Q_strncpyz( shotname, cstring, sizeof( shotname ) );
	else
		Q_strncpyz( shotname, "unknown", sizeof( shotname ) );

	Q_snprintfz( filename, sizeof( filename ), "levelshots/%s.jpg", shotname );
	if( trap_FS_FOpenFile( filename, NULL, FS_READ ) == -1 )
		Q_snprintfz( filename, sizeof( filename ), "levelshots/%s.tga", shotname );
	if( trap_FS_FOpenFile( filename, NULL, FS_READ ) == -1 )
		Q_snprintfz( filename, sizeof( filename ), "levelshots/unknown" );

	cgm.shaderLevelshot = trap_R_RegisterPic( filename );
}

/*
* CG_UpdateEnvironmentInfo
*/
static void CG_UpdateEnvironmentInfo( void )
{
	const char *cstring;
	int gravity, i;
	vec3_t dir;

	cstring = trap_GetConfigString( CS_GRAVITY );
	i = 0;
	gravity = -1;
	if( cstring && cstring[0] != '\0' )
	{
		if( sscanf( cstring, "%i %f %f %f", &gravity, &dir[0], &dir[1], &dir[2] ) == 4 )
		{
			gs.environment.gravity = gravity;
			VectorCopy( dir, gs.environment.gravityDir );
			VectorNormalizeFast( gs.environment.gravityDir );
		}
	}

	if( gravity == -1 )
	{
		GS_Printf( "WARNING: CG_UpdateEnvironmentInfo: Invalid CS_GRAVITY string\n" );
	}
}

/*
* CG_RegisterSkyPortalFromConfigstrings
*/
static void CG_RegisterSkyPortalFromConfigstrings( const char *cstring )
{
	float fov = 0;
	float scale = 0;
	int noents = 0;
	float pitchspeed = 0, yawspeed = 0, rollspeed = 0;
	skyportal_t *skyportal = &cgm.skyportal;

	memset( skyportal, 0, sizeof( skyportal_t ) );
	cg.skyportalInView = qfalse;

	if( cstring[0] != '\0' )
	{
		if( sscanf( cstring, "%f %f %f %f %f %i %f %f %f", &skyportal->vieworg[0], &skyportal->vieworg[1], &skyportal->vieworg[2],
			&fov, &scale, &noents,
			&pitchspeed, &yawspeed, &rollspeed ) >= 3 )
		{
			float off = cg.time * 0.001f;

			skyportal->fov = fov;
			skyportal->scale = scale ? 1.0f / scale : 0;
			skyportal->noEnts = ( noents ? qtrue : qfalse );
			VectorSet( skyportal->viewanglesOffset, anglemod( off * pitchspeed ), anglemod( off * yawspeed ), anglemod( off * rollspeed ) );
			cg.skyportalInView = qtrue;
		}
	}
}

/*
* CG_RegisterModelFromConfigstring
*/
static void CG_RegisterModelFromConfigstring( int index, const char *cstring )
{
	char token;

	if( !index )  // zero index means no model
		return;

	// world model can not be registered here
	token = cstring[0];
	switch( token )
	{
	case '#':
		GS_Printf( "WARNING: object %s indexed as model\n", cstring );
		break; // objects are registered from their respective strings
	case '$':
		GS_Printf( "WARNING: object %s indexed as model\n", cstring );
		break;
	default:
		cgm.indexedModels[index] = CG_RegisterModel( (char *)cstring );
		break;
	}
}

/*
* CG_RegisterSoundFromConfigstring
*/
static void CG_RegisterSoundFromConfigstring( int index, const char *cstring )
{
	if( index && cstring[0] != '*' )
		cgm.indexedSounds[index] = trap_S_RegisterSound( cstring );
}

/*
* CG_RegisterImageFromConfigstring
*/
static void CG_RegisterImageFromConfigstring( int index, const char *cstring )
{
	if( index )  // zero index means no image
		cgm.indexedShaders[index] = trap_R_RegisterPic( (char *)cstring );
}

/*
* CG_RegisterSkinfileFromConfigstring
*/
static void CG_RegisterSkinfileFromConfigstring( int index, const char *cstring )
{
	if( index )  // zero index means no skin
		cgm.indexedSkins[index] = trap_R_RegisterSkinFile( (char *)cstring );
}

/*
* CG_RegisterLightstyleFromConfigstring
*/
static void CG_RegisterLightstyleFromConfigstring( int index, const char *cstring )
{
	CG_SetLightStyle( index, cstring );
}

/*
* CG_RegisterPlayerObjectFromConfigstring
*/
static void CG_RegisterPlayerObjectFromConfigstring( int index, const char *cstring )
{
	if( index )
		cgm.indexedPlayerObjects[index] = CG_RegisterPlayerObject( cstring );
}

/*
* CG_RegisterWeaponObjectFromConfigstring
*/
static void CG_RegisterWeaponObjectFromConfigstring( int index, const char *cstring )
{
	if( index )
		cgm.indexedWeaponObjects[index] = CG_RegisterWeaponObject( cstring );
}

/*
* CG_RegisterItemFromConfigstring
*/
static void CG_RegisterItemFromConfigstring( int index, const char *cstring )
{
	const char *s;
	char string[MAX_STRING_CHARS];
	gsitem_t *item;
	int type, i;
	qboolean found = qfalse;
	char name[MAX_QPATH], object[MAX_QPATH];

	s = Info_ValueForKey( cstring, "n" );
	if( !s || !s[0] )
		GS_Error( "CG_RegisterItemFromConfigstring: Tried to precache item %i without a name\n", index );
	Q_strncpyz( name, s, sizeof( name ) );

	s = Info_ValueForKey( cstring, "t" );
	if( !s || !s[0] )
		GS_Error( "CG_RegisterItemFromConfigstring: Tried to precache item %i without a type\n", index );
	type = atoi( s );

	s = Info_ValueForKey( cstring, "o" );
	if( !s || !s[0] )
		GS_Error( "CG_RegisterItemFromConfigstring: Tried to precache item %i without an object\n", index );
	Q_strncpyz( object, s, sizeof( object ) );


	item = GS_FindItemByName( name );
	if( !item )
		GS_Error( "Client/Server items list mismatch (Game/CGame data differs). Item '%s' not found\n", name );
	if( item->tag != index )
		GS_Error( "Client/Server item list mismatch (Game/CGame data differs). Different index for item %s\n", name );
	if( type != item->type )
		GS_Error( "Client/Server item list mismatch (Game/CGame data differs). Types differ for item %s\n", name );

	// validate the gameObject
	if( Q_stricmp( item->model, object ) != 0 )
		GS_Error( "Client/Server item list mismatch (Game/CGame data differs). Object differs for item %s\n", name );

	// validate the model referenced from the object, and find its index in its respective array
	if( item->type & IT_WEAPON )
	{
		for( i = 1; i < MAX_WEAPONOBJECTS; i++ )
		{
			s = trap_GetConfigString( CS_WEAPONOBJECTS+i );
			if( !s || !s[0] )
				continue;

			if( !Q_stricmp( item->model, s ) )
			{
				found = qtrue;
				item->objectIndex = i;
				if( !GS_GetWeaponDef( item->tag ) )
					GS_Error( "CG_RegisterItemFromConfigstring: Weapon %s doesn't have a weapon definition\n", item->name );
				break;
			}
		}
	}
	else
	{
		Q_snprintfz( string, sizeof( string ), "%s%s%s", GS_ItemObjects_BasePath(), item->model + 1, GS_ItemObjects_Extension() );
		for( i = 1; i < MAX_MODELS; i++ )
		{
			s = trap_GetConfigString( CS_MODELS + i );
			if( !s || !s[0] )
				continue;

			if( !Q_stricmp( string, s ) )
			{
				found = qtrue;
				item->objectIndex = i;
				break;
			}
		}
	}

	if( !found )
		GS_Error( "Client/Server item list mismatch (Game/CGame data differs). Couldn't find the model for item %s in the server precache\n", name );
}

/*
* CG_RegisterPlayerClassFromConfigstring
*/
static void CG_RegisterPlayerClassFromConfigstring( int index, const char *cstring )
{
	const char *dataString;
	gsplayerclass_t *playerClass;

	if( !cstring || !cstring[0] )
		return;

	dataString = trap_GetConfigString( CS_PLAYERCLASSESDATA + index );

	playerClass = GS_PlayerClass_Register( index, cstring, dataString );

	if( !playerClass )
		GS_Error( "CG_RegisterPlayerClassFromConfigstring: Invalid configstrings\n" );

	if( cgm.indexedPlayerObjects[playerClass->objectIndex] != CG_RegisterPlayerObject( playerClass->playerObject ) )
		GS_Error( "CG_RegisterPlayerClassFromConfigstring: player object index mismatch\n" );
}

/*
* CG_RegisterBuildableFromConfigstring
*/
static void CG_RegisterBuildableFromConfigstring( int index, const char *cstring )
{
	gsbuildableobject_t *buildable;

	if( !cstring || !cstring[0] )
		return;

	buildable = GS_Buildables_Register( index, cstring );
	if( !buildable )
		GS_Error( "CG_RegisterBuildableFromConfigstring: Invalid configstring\n" );
}

/*
* CG_RegisterClientInfoFromConfigstring
*/
static void CG_RegisterClientInfoFromConfigstring( int index, const char *cstring )
{
	CG_LoadClientInfo( &cgs.clientInfo[index], cstring, index );
}

/*
* CG_RegisterGamecommandFromConfigstring
*/
static void CG_RegisterGamecommandFromConfigstring( int index, const char *cstring )
{
	CG_Cmd_RegisterCommand( cstring, NULL );
}

/*
* CG_UpdateConfigstringRegistration
*/
static void CG_UpdateConfigstringRegistration( int id, const char *cstring )
{
	// see what type of configstring it is, and act
	if( id < 0 || id >= MAX_CONFIGSTRINGS )
	{
		GS_Error( "configstring > MAX_CONFIGSTRINGS" );
	}
	else if( id < CS_MODELS )
	{
		switch( id )
		{
		case CS_MAPNAME:
			CG_RegisterLevelScreenShot( cstring );
			break;
		case CS_GRAVITY:
			CG_UpdateEnvironmentInfo();
			break;
		case CS_SKYBOX:
			CG_RegisterSkyPortalFromConfigstrings( cstring );
			break;
		case CS_MESSAGE:
		case CS_AUDIOTRACK:
		case CS_HOSTNAME:
		case CS_MAPCHECKSUM:
		default:
			break;
		}
	}
	else if( id >= CS_MODELS && id < ( CS_MODELS+MAX_MODELS ) )
	{
		CG_LoadingScreen_Topicname( "models" );
		CG_RegisterModelFromConfigstring( id-CS_MODELS, cstring );
	}
	else if( id >= CS_SOUNDS && id < ( CS_SOUNDS+MAX_SOUNDS ) )
	{
		CG_LoadingScreen_Topicname( "sounds" );
		CG_RegisterSoundFromConfigstring( id-CS_SOUNDS, cstring );
	}
	else if( id >= CS_IMAGES && id < ( CS_IMAGES+MAX_IMAGES ) )
	{
		CG_LoadingScreen_Topicname( "shaders" );
		CG_RegisterImageFromConfigstring( id-CS_IMAGES, cstring );
	}
	else if( id >= CS_SKINFILES && id < ( CS_SKINFILES+MAX_SKINFILES ) )
	{
		CG_LoadingScreen_Topicname( "skins" );
		CG_RegisterSkinfileFromConfigstring( id-CS_SKINFILES, cstring );
	}
	else if( id >= CS_LIGHTS && id < ( CS_LIGHTS+MAX_LIGHTSTYLES ) )
	{
		CG_LoadingScreen_Topicname( "ligthstyles" );
		CG_RegisterLightstyleFromConfigstring( id-CS_LIGHTS, cstring );
	}
	else if( id >= CS_PLAYEROBJECTS && id < ( CS_PLAYEROBJECTS+MAX_PLAYEROBJECTS ) )
	{
		CG_LoadingScreen_Topicname( "player models" );
		CG_RegisterPlayerObjectFromConfigstring( id-CS_PLAYEROBJECTS, cstring );
	}
	else if( id >= CS_WEAPONOBJECTS && id < ( CS_WEAPONOBJECTS+MAX_WEAPONOBJECTS ) )
	{
		CG_LoadingScreen_Topicname( "weapon models" );
		CG_RegisterWeaponObjectFromConfigstring( id-CS_WEAPONOBJECTS, cstring );
	}
	else if( id >= CS_ITEMS && id < ( CS_ITEMS+MAX_ITEMS ) )
	{
		CG_LoadingScreen_Topicname( "items" );
		CG_RegisterItemFromConfigstring( id-CS_ITEMS, cstring );
	}
	else if( id >= CS_PLAYERINFOS && id < ( CS_PLAYERINFOS+MAX_CLIENTS ) )
	{
		CG_LoadingScreen_Topicname( va( "%s %i", "client", id-CS_PLAYERINFOS ) );
		CG_RegisterClientInfoFromConfigstring( id-CS_PLAYERINFOS, cstring );
	}
	else if( id >= CS_GAMECOMMANDS && id < ( CS_GAMECOMMANDS+MAX_GAMECOMMANDS ) )
	{
		CG_LoadingScreen_Topicname( "commands" );
		CG_RegisterGamecommandFromConfigstring( id-CS_GAMECOMMANDS, cstring );
	}
	else if( id >= CS_PLAYERCLASSES && id < ( CS_PLAYERCLASSES+MAX_PLAYERCLASSES ) )
	{
		CG_LoadingScreen_Topicname( "player classes" );
		CG_RegisterPlayerClassFromConfigstring( id-CS_PLAYERCLASSES, cstring );
	}
	else if( id >= CS_BUILDABLES && id < ( CS_BUILDABLES+MAX_BUILDABLES ) )
	{
		CG_LoadingScreen_Topicname( "buildables" );
		CG_RegisterBuildableFromConfigstring( id-CS_BUILDABLES, cstring );
	}
}

/*
* CG_ConfigStringUpdate
*/
void CG_ConfigStringUpdate( int id )
{
	const char *cstring;

	cstring = trap_GetConfigString( id );
	if( !cstring )
		GS_Error( "CG_ConfigString: NULL configstring\n" );
	if( !cstring[0] )
		return;

	CG_UpdateConfigstringRegistration( id, cstring );
}

/*
* CG_RegisterConfigStrings
*/
void CG_RegisterConfigStrings( void )
{
	int i;
	const char *cstring;

	// free old data
	GS_PlayerClass_FreeAll();
	GS_Buildables_Free();

	// the world model is special and must be loaded the first
	cstring = trap_GetConfigString( CS_WORLDMODEL );
	CG_LoadingScreen_Filename( cstring );
	trap_R_RegisterWorldModel( (char *)cstring );

	GS_Printf( "trap_CM_NumInlineModels %i\n", trap_CM_NumInlineModels() );

	// create the local space partition
	GS_CreateSpacePartition();

	cgm.indexedWeaponObjects[WEAP_NONE] = CG_RegisterWeaponObject( "#generic" ); // base weapon model

	// parse the configstrings array
	for( i = 0; i < MAX_CONFIGSTRINGS; i++ )
	{
		cstring = trap_GetConfigString( i );
		if( !cstring )
			GS_Error( "CG_RegisterConfigStrings: NULL configstring\n" );
		if( !cstring[0] )
			continue;

		CG_LoadingScreen_Filename( cstring );
		CG_UpdateConfigstringRegistration( i, cstring );
	}

	// clear all loading screen names
	CG_LoadingScreen_Topicname( "" );
	CG_LoadingScreen_Filename( "" );
}
