/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

void G_Buildables_Register( const char *name, const char *model, vec3_t mins, vec3_t maxs )
{
	char string[MAX_STRING_CHARS];
	gsbuildableobject_t *buildable;
	int cmodelType;
	int index;

	if( !model || !name || !model[0] || !name[0] )
		return;

	if( ( !mins || !maxs ) && GS_IsBrushModel( trap_ModelIndex( model ) ) )
		cmodelType = CMODEL_BRUSH;
	else
		cmodelType = CMODEL_BBOX;

	if( cmodelType == CMODEL_BRUSH )
	{
		mins = vec3_origin;
		maxs = vec3_origin;
	}

	Q_snprintfz( string, sizeof( string ), "%s %i %i %i %i %i %i %i %i", 
		name, trap_ModelIndex( model ), cmodelType,
		(int)mins[0], (int)mins[1], (int)mins[2],
		(int)maxs[0], (int)maxs[1], (int)maxs[2] );

	index = G_ConfigstringIndex( string, CS_BUILDABLES, MAX_BUILDABLES );
	if( index < 0 )
		GS_Error( "G_Buildables_Register: MAX_BUILDABLES reached\n" );

	buildable = GS_Buildables_Register( index, string );
	if( buildable == NULL ) // something was incorrect
	{
		GS_Printf( "WARNING: Failed to register buildable '%s' with string '%s'\n", name, string );
		trap_ConfigString( CS_BUILDABLES + index, "" );
		return;
	}

	trap_PureModel( model );
}

/*
* G_Buildables_Init
*/
void G_Buildables_Init( void )
{
	int i;
	const char *cstring;
	vec3_t mins, maxs;

	GS_Buildables_Free();

	// clear old configstrings
	for( i = 1; i < MAX_BUILDABLES; i++ )
	{
		cstring = trap_GetConfigString( CS_BUILDABLES + i );
		if( cstring && cstring[0] )
			trap_ConfigString( CS_BUILDABLES + i, "" );
	}

	VectorSet( mins, -16, -16, -16 );
	VectorSet( maxs, 16, 16, 16 );

	G_Buildables_Register( "tesla", "models/buildables/tesla/tesla.md3", mins, maxs );
}
