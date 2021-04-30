/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

static gsplayerclass_t g_playerClasses[] =
{
	{
		"soldier",
		0,
		150, // initial health
		200, // max health

		"#soldier",
		0, // playerObject index

		&defaultPlayerMoveSpec,
		{ -16, -16, -24 }, // box mins
		{ 16, 16, 40 }, // box maxs
		PMOVEFEAT_CROUCH|PMOVEFEAT_JUMP|PMOVEFEAT_SPRINT,
		24,		// crouch height
		0,		// prone height
		300,	// run-speed
		450,    // sprint-speed
		175,    // crouch-speed
		75,    // prone-speed
		350,    // jumpspeed
		3000,   // accelspeed
		1,      // controlFracGround
		0.2f,  // controlFracAir
		0.6f,   // controlFracWater

		// locally derived information
		NULL,
	},

	{
		"sniper",
		0,
		100, // initial health
		100, // max health

		"#sniper",
		0, // playerObject index

		&defaultPlayerMoveSpec,
		{ -16, -16, -24 }, // box mins
		{ 16, 16, 40 }, // box maxs
		PMOVEFEAT_CROUCH|PMOVEFEAT_JUMP|PMOVEFEAT_PRONE,
		24,		// crouch height
		0,		// prone height
		300,	// run-speed
		450,    // sprint-speed
		175,    // crouch-speed
		75,    // prone-speed
		350,    // jumpspeed
		3000,   // accelspeed
		1,      // controlFracGround
		0.2f,  // controlFracAir
		0.6f,   // controlFracWater

		// locally derived information
		NULL,
	},

	{
		"engineer",
		0,
		75, // initial health
		75, // max health

		"#engineer",
		0,  // playerObject index

		&defaultPlayerMoveSpec,
		{ -16, -16, -24 }, // box mins
		{ 16, 16, 40 }, // box maxs
		PMOVEFEAT_CROUCH|PMOVEFEAT_JUMP|PMOVEFEAT_PRONE|PMOVEFEAT_SPRINT,
		24,		// crouch height
		0,		// prone height
		300,	// run-speed
		450,    // sprint-speed
		175,    // crouch-speed
		75,    // prone-speed
		350,    // jumpspeed
		3000,   // accelspeed
		1,      // controlFracGround
		0.2f,  // controlFracAir
		0.6f,   // controlFracWater

		// locally derived information
		NULL,
	}
};

/*
* G_PlayerClass_Register
* Registers a new player class into the class configstrings, and
* also registers a new player class struct into the classes array.
* The class struct used for input is temporary and useless after 
* registration. So class can also be registered from scripts or files.
*/
gsplayerclass_t *G_PlayerClass_Register( const gsplayerclass_t *inputClass )
{
	int index;
	char classString[MAX_STRING_CHARS], dataString[MAX_STRING_CHARS];
	gsplayerclass_t *playerClass;

	if( !inputClass || !inputClass->classname[0] )
		return NULL;

	// create the class string
	classString[0] = 0;
	Info_SetValueForKey( classString, "n", inputClass->classname );
	Info_SetValueForKey( classString, "o", inputClass->playerObject );
	Info_SetValueForKey( classString, "p", va( "%i", G_PlayerObjectIndex( inputClass->playerObject ) ) );

	if( strlen( classString ) >= MAX_CONFIGSTRING_CHARS - 1 )
		GS_Error( "G_PlayerClass_Register: playerclass info is too large for a configstring: %s\n", classString );

	// create the data block string
	dataString[0] = 0;
	Q_snprintfz( dataString, sizeof( dataString ), 
		"%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %.2f %.2f %.2f",
		(int)inputClass->mins[0], (int)inputClass->mins[1], (int)inputClass->mins[2],
		(int)inputClass->maxs[0], (int)inputClass->maxs[1], (int)inputClass->maxs[2],
		(int)inputClass->movement_features,
		(int)inputClass->crouch_height,
		(int)inputClass->prone_height,
		(int)inputClass->runspeed, (int)inputClass->sprintspeed, (int)inputClass->crouchspeed,
		(int)inputClass->pronespeed, (int)inputClass->jumpspeed, (int)inputClass->accelspeed,
		inputClass->controlFracGround,
		inputClass->controlFracAir,
		inputClass->controlFracWater );

	if( strlen( dataString ) >= MAX_CONFIGSTRING_CHARS - 1 )
		GS_Error( "G_PlayerClass_Register: playerclass info is too large for a configstring: %s\n", dataString );

	// register it
	index = G_ConfigstringIndex( classString, CS_PLAYERCLASSES, MAX_PLAYERCLASSES );
	if( index < 0 )
		GS_Error( "G_PlayerClass_Register: MAX_PLAYERCLASSES reached\n" );

	trap_ConfigString( CS_PLAYERCLASSESDATA + index, dataString );

	playerClass = GS_PlayerClass_Register( index, classString, dataString );
	if( playerClass == NULL ) // something was incorrect
	{
		trap_ConfigString( CS_PLAYERCLASSES + index, "" );
		trap_ConfigString( CS_PLAYERCLASSESDATA + index, "" );
	}

	// fixme: these are server side only fields
	playerClass->health = inputClass->health;
	playerClass->maxHealth = inputClass->maxHealth;

	return playerClass;
}

/*
* G_PlayerClass_RegisterClasses
*/
void G_PlayerClass_RegisterClasses( void )
{
	int numClasses = sizeof( g_playerClasses ) / sizeof( gsplayerclass_t );
	int i;
	const char *cstring;

	// remove all old information
	GS_PlayerClass_FreeAll();

	// clear old configstrings
	for( i = 1; i < MAX_PLAYERCLASSES; i++ )
	{
		cstring = trap_GetConfigString( CS_PLAYERCLASSES + i );
		if( cstring && cstring[0] )
		{
			trap_ConfigString( CS_PLAYERCLASSES + i, "" );
			trap_ConfigString( CS_PLAYERCLASSESDATA + i, "" );
		}
	}

	// register all classes
	for( i = 0; i < numClasses; i++ )
		G_PlayerClass_Register( &g_playerClasses[i] );
}
