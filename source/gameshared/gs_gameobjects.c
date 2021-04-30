/*
   Copyright (C) 2007 German Garcia
 */

// GameObjects are objects that with a single path make reference
// to multiple models and sounds.

#include "gs_local.h"

//==================================================
// BUILDABLES
//==================================================

static gsbuildableobject_t *buildablesHeadNode = NULL;

const char *GS_BuildableObjects_BasePath( void )
{
	static const char gsGS_BuildableObjectsBasePath[] = "models/weapons/";
	return gsGS_BuildableObjectsBasePath;
}

const char *GS_GS_BuildableObjects_Extension( void )
{
	static const char gsGS_BuildableObjectsExtension[] = ".md3";
	return gsGS_BuildableObjectsExtension;
}

/*
* GS_BuildableByName
*/
gsbuildableobject_t *GS_BuildableByName( const char *name )
{
	gsbuildableobject_t *buildable;

	if( name && name[0] )
	{
		for( buildable = buildablesHeadNode; buildable != NULL; buildable = buildable->next )
		{
			if( !Q_stricmp( buildable->name, name ) )
				return buildable;
		}
	}

	return NULL;
}

/*
* GS_BuildableByIndex
*/
gsbuildableobject_t *GS_BuildableByIndex( int index )
{
	gsbuildableobject_t *buildable;

	if( !index )
		return NULL;

	if( index > 0 && index <= gs.numBuildables )
	{
		for( buildable = buildablesHeadNode; buildable != NULL; buildable = buildable->next )
		{
			if( buildable->index == index )
				return buildable;
		}
	}

	return NULL;
}

/*
* GS_Buildables_Register
*/
gsbuildableobject_t *GS_Buildables_Register( int index, const char *dataString )
{
	gsbuildableobject_t *buildable;
	char name[MAX_QPATH];

	int mins0, mins1, mins2, maxs0, maxs1, maxs2, modelIndex, cmodelType;
	int i;

	if( !index || !dataString || !dataString[0] )
		return NULL;

	// data string

	i = sscanf( dataString, "%s %i %i %i %i %i %i %i %i",
		name, &modelIndex, &cmodelType,
		&mins0, &mins1, &mins2,
		&maxs0, &maxs1, &maxs2 );

	// we only accept the data if the string is perfect
	if( i != 9 )
		GS_Error( "GS_Buildables_Register: Invalid dataString string: %s (%i tokens)\n", dataString, i );

	// see if it was already registered
	buildable = GS_BuildableByIndex( index );
	if( buildable != NULL )
	{
		if( buildable->name )
			module_Free( name );
	}
	else
	{
		// create the new buildable container
		buildable = ( gsbuildableobject_t * )module_Malloc( sizeof( gsbuildableobject_t ) );
		buildable->next = buildablesHeadNode;
		buildablesHeadNode = buildable;

		gs.numBuildables++;
		if( index > gs.numBuildables )
			GS_Error( "GS_Buildables_Register: index > gs.numBuildables\n" );
	}

	buildable->index = index;
	buildable->modelIndex = modelIndex;
	buildable->name = GS_CopyString( name );

	buildable->mins[0] = mins0;
	buildable->mins[1] = mins1;
	buildable->mins[2] = mins2;
	buildable->maxs[0] = maxs0;
	buildable->maxs[1] = maxs1;
	buildable->maxs[2] = maxs2;

	return buildable;
}

/*
* GS_Buildables_Free
*/
void GS_Buildables_Free( void )
{
	gsbuildableobject_t *buildable;

	buildable = buildablesHeadNode;

	while( buildable )
	{
		buildablesHeadNode = buildable->next;
		if( buildable->name )
			module_Free( buildable->name );

		module_Free( buildable );
		buildable = buildablesHeadNode;
	}

	gs.numBuildables = 0;
}

/*
entity_state_t *GS_Buildables_BuildPosition( gsbuildableobject_t *buildable )
{
	static entity_state_t state;

	if( !buildable )
		return NULL;

	memset( &state, 0, sizeof( state ) );
	state.number = -1;
	state.modelindex1 = buildable->modelIndex;
	state.cmodeltype = buildable->cmodelType;
	state.type = ET_MODEL;
	VectorCopy( buildable->mins, state.local.mins );
	VectorCopy( buildable->maxs, state.local.maxs );
	
}
*/


//==================================================
// WEAPON OBJECTS
//==================================================

const char *GS_WeaponObjects_BasePath( void )
{
	static const char gsWeaponObjectsBasePath[] = "models/weapons/";
	return gsWeaponObjectsBasePath;
}

const char *GS_WeaponObjects_Extension( void )
{
	static const char gsWeaponObjectsExtension[] = ".md3";
	return gsWeaponObjectsExtension;
}

//==================================================
// PLAYER OBJECTS
//==================================================

const char *GS_PlayerObjects_BasePath( void )
{
	static const char gsPlayerObjectsBasePath[] = "models/players/";
	return gsPlayerObjectsBasePath;
}

const char *GS_PlayerObjects_Extension( void )
{
	static const char gsPlayerObjectsExtension[] = ".skm";
	return gsPlayerObjectsExtension;
}

//==================================================
// ITEM OBJECTS
//==================================================

const char *GS_ItemObjects_BasePath( void )
{
	static const char gsItemObjectsBasePath[] = "models/items/";
	return gsItemObjectsBasePath;
}

const char *GS_ItemObjects_Extension( void )
{
	static const char gsItemObjectsExtension[] = ".md3";
	return gsItemObjectsExtension;
}
