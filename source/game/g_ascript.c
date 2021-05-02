#include "g_local.h"
#include "../gameshared/angelref.h"


// fixme: this should probably go into q_shared.h
#if defined ( _WIN64 ) || ( __x86_64__ )
#define FOFFSET(s,m)   (size_t)( (qintptr)&(((s *)0)->m) )
#else
#define FOFFSET(s,m)   (size_t)&(((s *)0)->m)
#endif

#define G_AsMalloc								G_Malloc
#define G_AsFree								G_Free

static angelwrap_api_t *angelExport = NULL;

#define SCRIPT_MODULE_NAME						"gametypes"

#define SCRIPT_ENUM_VAL(name)					{ #name,name }
#define SCRIPT_ENUM_VAL_NULL					{ NULL, 0 }

#define SCRIPT_ENUM_NULL						{ NULL, NULL }

#define SCRIPT_FUNCTION_DECL(type,name,params)	(#type " " #name #params)

#define SCRIPT_PROPERTY_DECL(type,name)			#type " " #name

#define SCRIPT_FUNCTION_NULL					NULL
#define SCRIPT_BEHAVIOR_NULL					{ 0, SCRIPT_FUNCTION_NULL, NULL, 0 }
#define SCRIPT_METHOD_NULL						{ SCRIPT_FUNCTION_NULL, NULL, 0 }
#define SCRIPT_PROPERTY_NULL					{ NULL, 0 }

typedef struct asEnumVal_s
{
	const char * const name;
	const int value;
} asEnumVal_t;

typedef struct asEnum_s
{
	const char * const name;
	const asEnumVal_t * const values;
} asEnum_t;

typedef struct asBehavior_s
{
	const unsigned int behavior;
	const char * const declaration;
	const void *funcPointer;
	const void *funcPointer_asGeneric;
	const int callConv;
} asBehavior_t;

typedef struct asMethod_s
{
	const char * const declaration;
	const void *funcPointer;
	const void *funcPointer_asGeneric;
	const int callConv;
} asMethod_t;

typedef struct asProperty_s
{
	const char * const declaration;
	const unsigned int offset;
} asProperty_t;

typedef struct asClassDescriptor_s
{
	const char * const name;
	const asEObjTypeFlags typeFlags; 
	const size_t size;
	const asBehavior_t * const objBehaviors;
	const asBehavior_t * const globalBehaviors;
	const asMethod_t * const objMethods;
	const asProperty_t * const objProperties;
	const void * const stringFactory;
	const void * const stringFactory_asGeneric;
} asClassDescriptor_t;

//=======================================================================

/*
* ASCRIPT_GENERIC
*/

#define G_asGetReturnBool( x ) (qboolean)angelExport->asGetReturnByte( x )
#define G_asGeneric_GetObject( x ) ( angelExport->asIScriptGeneric_GetObject(x) )

static qbyte G_asGeneric_GetArgByte( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT8 );
	assert( argument && argument->type >= 0 );

	return (qbyte)argument->integer;
}

static qboolean G_asGeneric_GetArgBool( void *gen, unsigned int arg )
{
	return (qboolean)G_asGeneric_GetArgByte( gen, arg );
}

static short G_asGeneric_GetArgShort( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT16 );
	assert( argument && argument->type >= 0 );

	return ( (unsigned short)argument->integer );
}

static int G_asGeneric_GetArgInt( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT );
	assert( argument && argument->type >= 0 );

	return ( (unsigned int)argument->integer );
}

static quint64 G_asGeneric_GetArgInt64( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT64 );
	assert( argument && argument->type >= 0 );

	return (quint64)argument->integer64;
}

static float G_asGeneric_GetArgFloat( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_FLOAT );
	assert( argument && argument->type >= 0 );

	return argument->value;
}

static double G_asGeneric_GetArgDouble( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_DOUBLE );
	assert( argument && argument->type >= 0 );

	return argument->dvalue;
}

static void *G_asGeneric_GetArgObject( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_OBJECT );
	assert( argument && argument->type >= 0 );

	return argument->ptr;
}

static void *G_asGeneric_GetArgAddress( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_ADDRESS );
	assert( argument && argument->type >= 0 );

	return argument->ptr;
}

static void *G_asGeneric_GetAddressOfArg( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_POINTER );
	assert( argument && argument->type >= 0 );

	return argument->ptr;
}

static void G_asGeneric_SetReturnByte( void *gen, qbyte value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT8;
	argument.integer = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnBool( void *gen, qboolean value )
{
	G_asGeneric_SetReturnByte( gen, value );
}

static void G_asGeneric_SetReturnShort( void *gen, unsigned short value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT16;
	argument.integer = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnInt( void *gen, unsigned int value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT;
	argument.integer = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnInt64( void *gen, quint64 value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT64;
	argument.integer64 = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnFloat( void *gen, float value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_FLOAT;
	argument.value = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnDouble( void *gen, double value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_DOUBLE;
	argument.dvalue = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnObject( void *gen, void *ptr )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_OBJECT;
	argument.ptr = ptr;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnAddress( void *gen, void *ptr )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_ADDRESS;
	argument.ptr = ptr;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnPointer( void *gen, void *ptr )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_POINTER;
	argument.ptr = ptr;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

//=======================================================================

static const asEnumVal_t asConfigstringEnumVals[] =
{
	SCRIPT_ENUM_VAL( CS_MESSAGE ),
	SCRIPT_ENUM_VAL( CS_MAPNAME ),
	SCRIPT_ENUM_VAL( CS_AUDIOTRACK ),
	SCRIPT_ENUM_VAL( CS_HOSTNAME ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asSpawnSystemEnumVals[] =
{
	SCRIPT_ENUM_VAL( SPAWNSYSTEM_INSTANT ),
	SCRIPT_ENUM_VAL( SPAWNSYSTEM_WAVES ),
	SCRIPT_ENUM_VAL( SPAWNSYSTEM_HOLD ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asEntityTypeEnumVals[] =
{
/*
	SCRIPT_ENUM_VAL( ET_GENERIC ),
	SCRIPT_ENUM_VAL( ET_PLAYER ),
	SCRIPT_ENUM_VAL( ET_CORPSE ),
	SCRIPT_ENUM_VAL( ET_BEAM ),
	SCRIPT_ENUM_VAL( ET_PORTALSURFACE ),
	SCRIPT_ENUM_VAL( ET_PUSH_TRIGGER ),
	SCRIPT_ENUM_VAL( ET_GIB ),
	SCRIPT_ENUM_VAL( ET_BLASTER ),
	SCRIPT_ENUM_VAL( ET_ELECTRO_WEAK ),
	SCRIPT_ENUM_VAL( ET_ROCKET ),
	SCRIPT_ENUM_VAL( ET_GRENADE ),
	SCRIPT_ENUM_VAL( ET_PLASMA ),
	SCRIPT_ENUM_VAL( ET_SPRITE ),
	SCRIPT_ENUM_VAL( ET_ITEM ),
	SCRIPT_ENUM_VAL( ET_LASERBEAM ),
	SCRIPT_ENUM_VAL( ET_CURVELASERBEAM ),
	SCRIPT_ENUM_VAL( ET_FLAG_BASE ),

	SCRIPT_ENUM_VAL( ET_EVENT ),
	SCRIPT_ENUM_VAL( ET_SOUNDEVENT ),
*/
	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asWeaponTagEnumVals[] =
{
/*
	SCRIPT_ENUM_VAL( WEAP_NONE ),
	SCRIPT_ENUM_VAL( WEAP_GUNBLADE ),
	SCRIPT_ENUM_VAL( WEAP_MACHINEGUN ),
	SCRIPT_ENUM_VAL( WEAP_RIOTGUN ),
	SCRIPT_ENUM_VAL( WEAP_GRENADELAUNCHER ),
	SCRIPT_ENUM_VAL( WEAP_ROCKETLAUNCHER ),
	SCRIPT_ENUM_VAL( WEAP_PLASMAGUN ),
	SCRIPT_ENUM_VAL( WEAP_LASERGUN ),
	SCRIPT_ENUM_VAL( WEAP_ELECTROBOLT ),
	SCRIPT_ENUM_VAL( WEAP_INSTAGUN ),
	SCRIPT_ENUM_VAL( WEAP_TOTAL ),
*/
	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asClientStateEnumVals[] =
{
	SCRIPT_ENUM_VAL( CS_FREE ),
	SCRIPT_ENUM_VAL( CS_ZOMBIE ),
	SCRIPT_ENUM_VAL( CS_CONNECTING ),
	SCRIPT_ENUM_VAL( CS_CONNECTED ),
	SCRIPT_ENUM_VAL( CS_SPAWNED ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asContentsEnumVals[] =
{
	SCRIPT_ENUM_VAL( CONTENTS_SOLID ),
	SCRIPT_ENUM_VAL( CONTENTS_LAVA ),
	SCRIPT_ENUM_VAL( CONTENTS_SLIME ),
	SCRIPT_ENUM_VAL( CONTENTS_WATER ),
	SCRIPT_ENUM_VAL( CONTENTS_FOG ),
	SCRIPT_ENUM_VAL( CONTENTS_AREAPORTAL ),
	SCRIPT_ENUM_VAL( CONTENTS_PLAYERCLIP ),
	SCRIPT_ENUM_VAL( CONTENTS_MONSTERCLIP ),
	SCRIPT_ENUM_VAL( CONTENTS_TELEPORTER ),
	SCRIPT_ENUM_VAL( CONTENTS_JUMPPAD ),
	SCRIPT_ENUM_VAL( CONTENTS_CLUSTERPORTAL ),
	SCRIPT_ENUM_VAL( CONTENTS_DONOTENTER ),
	SCRIPT_ENUM_VAL( CONTENTS_ORIGIN ),
	SCRIPT_ENUM_VAL( CONTENTS_BODY ),
	SCRIPT_ENUM_VAL( CONTENTS_CORPSE ),
	SCRIPT_ENUM_VAL( CONTENTS_DETAIL ),
	SCRIPT_ENUM_VAL( CONTENTS_STRUCTURAL ),
	SCRIPT_ENUM_VAL( CONTENTS_TRANSLUCENT ),
	SCRIPT_ENUM_VAL( CONTENTS_TRIGGER ),
	SCRIPT_ENUM_VAL( CONTENTS_NODROP ),
	SCRIPT_ENUM_VAL( MASK_ALL ),
	SCRIPT_ENUM_VAL( MASK_SOLID ),
	SCRIPT_ENUM_VAL( MASK_PLAYERSOLID ),
	SCRIPT_ENUM_VAL( MASK_DEADSOLID ),
	SCRIPT_ENUM_VAL( MASK_MONSTERSOLID ),
	SCRIPT_ENUM_VAL( MASK_WATER ),
	SCRIPT_ENUM_VAL( MASK_OPAQUE ),
	SCRIPT_ENUM_VAL( MASK_SHOT ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asSurfFlagEnumVals[] =
{
	SCRIPT_ENUM_VAL( SURF_NODAMAGE ),
	SCRIPT_ENUM_VAL( SURF_SLICK ),
	SCRIPT_ENUM_VAL( SURF_SKY ),
	SCRIPT_ENUM_VAL( SURF_LADDER ),
	SCRIPT_ENUM_VAL( SURF_NOIMPACT ),
	SCRIPT_ENUM_VAL( SURF_NOMARKS ),
	SCRIPT_ENUM_VAL( SURF_FLESH ),
	SCRIPT_ENUM_VAL( SURF_NODRAW ),
	SCRIPT_ENUM_VAL( SURF_HINT ),
	SCRIPT_ENUM_VAL( SURF_SKIP ),
	SCRIPT_ENUM_VAL( SURF_NOLIGHTMAP ),
	SCRIPT_ENUM_VAL( SURF_POINTLIGHT ),
	SCRIPT_ENUM_VAL( SURF_METALSTEPS ),
	SCRIPT_ENUM_VAL( SURF_NOSTEPS ),
	SCRIPT_ENUM_VAL( SURF_NONSOLID ),
	SCRIPT_ENUM_VAL( SURF_LIGHTFILTER ),
	SCRIPT_ENUM_VAL( SURF_ALPHASHADOW ),
	SCRIPT_ENUM_VAL( SURF_NODLIGHT ),
	SCRIPT_ENUM_VAL( SURF_DUST ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asCvarFlagEnumVals[] =
{
	SCRIPT_ENUM_VAL( CVAR_ARCHIVE ),
	SCRIPT_ENUM_VAL( CVAR_USERINFO ),
	SCRIPT_ENUM_VAL( CVAR_SERVERINFO ),
	SCRIPT_ENUM_VAL( CVAR_NOSET ),
	SCRIPT_ENUM_VAL( CVAR_LATCH ),
	SCRIPT_ENUM_VAL( CVAR_LATCH_VIDEO ),
	SCRIPT_ENUM_VAL( CVAR_LATCH_SOUND ),
	SCRIPT_ENUM_VAL( CVAR_CHEAT ),
	SCRIPT_ENUM_VAL( CVAR_READONLY ),

	SCRIPT_ENUM_VAL_NULL
};

//=======================================================================

static const asEnum_t asEnums[] =
{
	{ "configstrings_e", asConfigstringEnumVals },
	{ "spawnsystem_e", asSpawnSystemEnumVals },
	{ "entitytype_e", asEntityTypeEnumVals },
	{ "weapon_tag_e", asWeaponTagEnumVals },
	{ "client_statest_e", asClientStateEnumVals },
	{ "contents_e", asContentsEnumVals },
	{ "surfaceflags_e", asSurfFlagEnumVals },
	{ "cvarflags_e", asCvarFlagEnumVals },

	SCRIPT_ENUM_NULL
};

/*
* G_asRegisterEnums
*/
static void G_asRegisterEnums( int asEngineHandle )
{
	int i, j;
	const asEnum_t *asEnum;
	const asEnumVal_t *asEnumVal;

	for( i = 0, asEnum = asEnums; asEnum->name != NULL; i++, asEnum++ )
	{
		angelExport->asRegisterEnum( asEngineHandle, asEnum->name );

		for( j = 0, asEnumVal = asEnum->values; asEnumVal->name != NULL; j++, asEnumVal++ )
			angelExport->asRegisterEnumValue( asEngineHandle, asEnum->name, asEnumVal->name, asEnumVal->value );
	}
}

//=======================================================================

// CLASS: cString
static int asstring_factored_count = 0;
static int asstring_released_count = 0;

typedef struct
{
	char *buffer;
	size_t len, size;
	int asRefCount, asFactored;
} asstring_t;

static inline asstring_t *objectString_Alloc( void )
{
	static asstring_t *object;

	object = G_AsMalloc( sizeof( asstring_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;

	asstring_factored_count++;
	return object;
}

static asstring_t *objectString_FactoryBuffer( const char *buffer, unsigned int length )
{
	asstring_t *object;

	object = objectString_Alloc();
	object->buffer = G_AsMalloc( sizeof( char ) * ( length + 1 ) );
	object->len = length;
	object->buffer[length] = 0;
	object->size = length + 1;
	if( buffer )
		Q_strncpyz( object->buffer, buffer, object->size );

	return object;
}

static asstring_t *objectString_AssignString( asstring_t *self, const char *string, size_t strlen )
{
	if( strlen >= self->size )
	{
		G_AsFree( self->buffer );

		self->size = strlen + 1;
		self->buffer = G_AsMalloc( self->size );
	}

	self->len = strlen;
	Q_strncpyz( self->buffer, string, self->size );

	return self;
}

static asstring_t *objectString_AssignPattern( asstring_t *self, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AssignString( self, buf, strlen( buf ) );
}

static asstring_t *objectString_AddAssignString( asstring_t *self, const char *string, size_t strlen )
{
	if( strlen )
	{
		char *tem = self->buffer;

		self->len = strlen + self->len;
		self->size = self->len + 1;
		self->buffer = G_AsMalloc( self->size );

		Q_snprintfz( self->buffer, self->size, "%s%s", tem, string );
		G_AsFree( tem );
	}

	return self;
}

static asstring_t *objectString_AddAssignPattern( asstring_t *self, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AddAssignString( self, buf, strlen( buf ) );
}

static asstring_t *objectString_AddString( asstring_t *first, const char *second, size_t seclen )
{
	asstring_t *self = objectString_FactoryBuffer( NULL, first->len + seclen );

	Q_snprintfz( self->buffer, self->size, "%s%s", first->buffer, second );

	return self;
}

static asstring_t *objectString_AddPattern( asstring_t *first, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AddString( first, buf, strlen( buf ) );
}

static asstring_t *objectString_Factory( void )
{
	return objectString_FactoryBuffer( NULL, 0 );
}

static void objectString_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_Factory() );
}

static asstring_t *objectString_FactoryCopy( const asstring_t *other )
{
	return objectString_FactoryBuffer( other->buffer, other->len );
}

static void objectString_asGeneric_FactoryCopy( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_FactoryCopy( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static void objectString_Addref( asstring_t *obj ) { obj->asRefCount++; }

static void objectString_asGeneric_Addref( void *gen )
{
	objectString_Addref( (asstring_t *)G_asGeneric_GetObject( gen ) );
}

static void objectString_Release( asstring_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );

	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj->buffer );
		G_AsFree( obj );
		asstring_released_count++;
	}
}

static void objectString_asGeneric_Release( void *gen )
{
	objectString_Release( (asstring_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *StringFactory( unsigned int length, const char *s )
{
	return objectString_FactoryBuffer( s, length );
}

static void StringFactory_asGeneric( void *gen )
{
	unsigned int length = (unsigned int)G_asGeneric_GetArgInt( gen, 0 );
	char *s = G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, StringFactory( length, s ) );
}

static char *objectString_Index( unsigned int i, asstring_t *self )
{
	if( i > self->len )
	{
		GS_Printf( "* WARNING: objectString_Index: Out of range\n" );
		return NULL;
	}

	return &self->buffer[i];
}

static void objectString_asGeneric_Index( void *gen )
{
	unsigned int i = (unsigned int)G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );
	
	G_asGeneric_SetReturnAddress( gen, objectString_Index( i, self ) );
}

static asstring_t *objectString_AssignBehaviour( asstring_t *other, asstring_t *self )
{
	return objectString_AssignString( self, other->buffer, other->len );
}

static void objectString_asGeneric_AssignBehaviour( void *gen )
{
	asstring_t *other = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviour( other, self ) );
}

static asstring_t *objectString_AssignBehaviourI( int other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%i", other );
}

static void objectString_asGeneric_AssignBehaviourI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviourI( other, self ) );
}

static asstring_t *objectString_AssignBehaviourD( double other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%g", other );
}

static void objectString_asGeneric_AssignBehaviourD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviourD( other, self ) );
}

static asstring_t *objectString_AssignBehaviourF( float other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%f", other );
}

static void objectString_asGeneric_AssignBehaviourF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviourF( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSS( asstring_t *other, asstring_t *self )
{
	return objectString_AddAssignString( self, other->buffer, other->len );
}

static void objectString_asGeneric_AddAssignBehaviourSS( void *gen )
{
	asstring_t *other = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSS( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSI( int other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%i", other );
}

static void objectString_asGeneric_AddAssignBehaviourSI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSI( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSD( double other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%g", other );
}

static void objectString_asGeneric_AddAssignBehaviourSD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSD( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSF( float other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%f", other );
}

static void objectString_asGeneric_AddAssignBehaviourSF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSF( other, self ) );
}

static asstring_t *objectString_AddBehaviourSS( asstring_t *first, asstring_t *second )
{
	return objectString_AddString( first, second->buffer, second->len );
}

static void objectString_asGeneric_AddBehaviourSS( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSS( first, second ) );
}

static asstring_t *objectString_AddBehaviourSI( asstring_t *first, int second )
{
	return objectString_AddPattern( first, "%i", second );
}

static void objectString_asGeneric_AddBehaviourSI( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int second = G_asGeneric_GetArgInt( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSI( first, second ) );
}

static asstring_t *objectString_AddBehaviourIS( int first, asstring_t *second )
{
	asstring_t *res = objectString_Factory();
	return objectString_AssignPattern( res, "%i%s", first, second->buffer );
}

static void objectString_asGeneric_AddBehaviourIS( void *gen )
{
	int first = G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourIS( first, second ) );
}

static asstring_t *objectString_AddBehaviourSD( asstring_t *first, double second )
{
	return objectString_AddPattern( first, "%g", second );
}

static void objectString_asGeneric_AddBehaviourSD( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	double second = G_asGeneric_GetArgDouble( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSD( first, second ) );
}

static asstring_t *objectString_AddBehaviourDS( double first, asstring_t *second )
{
	asstring_t *res = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( res, "%g%s", first, second->buffer );
}

static void objectString_asGeneric_AddBehaviourDS( void *gen )
{
	double first = G_asGeneric_GetArgDouble( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourDS( first, second ) );
}

static asstring_t *objectString_AddBehaviourSF( asstring_t *first, float second )
{
	return objectString_AddPattern( first, "%f", second );
}

static void objectString_asGeneric_AddBehaviourSF( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	float second = G_asGeneric_GetArgFloat( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSF( first, second ) );
}

static asstring_t *objectString_AddBehaviourFS( float first, asstring_t *second )
{
	asstring_t *res = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( res, "%f%s", first, second->buffer );
}

static void objectString_asGeneric_AddBehaviourFS( void *gen )
{
	float first = G_asGeneric_GetArgFloat( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourFS( first, second ) );
}

static qboolean objectString_EqualBehaviour( asstring_t *first, asstring_t *second )
{
	if( !first->len && !second->len )
		return qtrue;

	return ( Q_stricmp( first->buffer, second->buffer ) == 0 );
}

static void objectString_asGeneric_EqualBehaviour( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectString_EqualBehaviour( first, second ) );
}

static qboolean objectString_NotEqualBehaviour( asstring_t *first, asstring_t *second )
{
	return !objectString_EqualBehaviour( first, second );
}

static void objectString_asGeneric_NotEqualBehaviour( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectString_NotEqualBehaviour( first, second ) );
}

static int objectString_Len( asstring_t *self )
{
	return self->len;
}

static void objectString_asGeneric_Len( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectString_Len( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_ToLower( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_strlwr( string->buffer );
	return string;
}

static void objectString_asGeneric_ToLower( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_ToLower( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_ToUpper( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_strupr( string->buffer );
	return string;
}

static void objectString_asGeneric_ToUpper( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_ToUpper( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_RemoveColorTokens( asstring_t *self )
{
	const char *s;

	if( !self->len )
		return objectString_FactoryBuffer( NULL, 0 );

	s = COM_RemoveColorTokens( self->buffer );
	return objectString_FactoryBuffer( s, strlen(s) );
}

static void objectString_asGeneric_RemoveColorTokens( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_RemoveColorTokens( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectString_toInt( asstring_t *self )
{
	return atoi( self->buffer );
}

static void objectString_asGeneric_toInt( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectString_toInt( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectString_toFloat( asstring_t *self )
{
	return atof( self->buffer );
}

static void objectString_asGeneric_toFloat( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectString_toFloat( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_getToken( int index, asstring_t *self )
{
	int i;
	char *s, *token = "";

	s = self->buffer;

	for( i = 0; i <= index; i++ )
	{
		token = COM_Parse( &s );
		if( !token[0] ) // string finished before finding the token
			break;
	}

	return objectString_FactoryBuffer( token, strlen( token ) );
}

static void objectString_asGeneric_getToken( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_getToken( G_asGeneric_GetArgInt( gen, 0 ), (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static struct asvec3_s *objectString_toVec3( asstring_t *self );
static void objectString_asGeneric_toVec3( void *gen );

static const asBehavior_t asstring_ObjectBehaviors[] =
{
	/* factory */
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cString @, f, ()), objectString_Factory, objectString_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in)), objectString_FactoryCopy, objectString_asGeneric_FactoryCopy, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectString_Addref, objectString_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectString_Release, objectString_asGeneric_Release, asCALL_CDECL_OBJLAST },

	/* assignments */
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (const cString &in)), objectString_AssignBehaviour, objectString_asGeneric_AssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (int)), objectString_AssignBehaviourI, objectString_asGeneric_AssignBehaviourI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (double)), objectString_AssignBehaviourD, objectString_asGeneric_AssignBehaviourD, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (float)), objectString_AssignBehaviourF, objectString_asGeneric_AssignBehaviourF, asCALL_CDECL_OBJLAST },

	/* register the index operator, both as a mutator and as an inspector */
	{ asBEHAVE_INDEX, SCRIPT_FUNCTION_DECL(uint8 &, f, (uint)), objectString_Index, objectString_asGeneric_Index, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_INDEX, SCRIPT_FUNCTION_DECL(const uint8 &, f, (uint)), objectString_Index, objectString_asGeneric_Index, asCALL_CDECL_OBJLAST },

	/* += */
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (cString &in)), objectString_AddAssignBehaviourSS, objectString_asGeneric_AddAssignBehaviourSS, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (int)), objectString_AddAssignBehaviourSI, objectString_asGeneric_AddAssignBehaviourSI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (double)), objectString_AddAssignBehaviourSD, objectString_asGeneric_AddAssignBehaviourSD, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (float)), objectString_AddAssignBehaviourSF, objectString_asGeneric_AddAssignBehaviourSF, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t asstring_GlobalBehaviors[] =
{
	/* + */
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, const cString &in)), objectString_AddBehaviourSS, objectString_asGeneric_AddBehaviourSS, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, int)), objectString_AddBehaviourSI, objectString_asGeneric_AddBehaviourSI, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (int, const cString &in)), objectString_AddBehaviourIS, objectString_asGeneric_AddBehaviourIS, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, double)), objectString_AddBehaviourSD, objectString_asGeneric_AddBehaviourSD, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (double, const cString &in)), objectString_AddBehaviourDS, objectString_asGeneric_AddBehaviourDS, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, float)), objectString_AddBehaviourSF, objectString_asGeneric_AddBehaviourSF, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (float, const cString &in)), objectString_AddBehaviourFS, objectString_asGeneric_AddBehaviourFS, asCALL_CDECL },

	/* == != */
	{ asBEHAVE_EQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cString &in, const cString &in)), objectString_EqualBehaviour, objectString_asGeneric_EqualBehaviour, asCALL_CDECL },
	{ asBEHAVE_NOTEQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cString &in, const cString &in)), objectString_NotEqualBehaviour, objectString_asGeneric_NotEqualBehaviour, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t asstring_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(int, len, ()), objectString_Len, objectString_asGeneric_Len, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, length, ()), objectString_Len, objectString_asGeneric_Len, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, tolower, ()), objectString_ToLower, objectString_asGeneric_ToLower, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, toupper, ()), objectString_ToUpper, objectString_asGeneric_ToUpper, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, removeColorTokens, ()), objectString_RemoveColorTokens, objectString_asGeneric_RemoveColorTokens, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getToken, (const int)), objectString_getToken, objectString_asGeneric_getToken, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(int, toInt, ()), objectString_toInt, objectString_asGeneric_toInt, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, toFloat, ()), objectString_toFloat, objectString_asGeneric_toFloat, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 &, toVec3, ()), objectString_toVec3, objectString_asGeneric_toVec3, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asClassDescriptor_t asStringClassDescriptor =
{
	"cString",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( asstring_t ),		/* size */
	asstring_ObjectBehaviors,	/* object behaviors */
	asstring_GlobalBehaviors,	/* global behaviors */
	asstring_Methods,			/* methods */
	NULL,						/* properties */

	StringFactory, StringFactory_asGeneric	/* string factory hack */
};

//=======================================================================

// CLASS: cVec3
static int asvector_factored_count = 0;
static int asvector_released_count = 0;

typedef struct asvec3_s
{
	vec3_t v;
	int asRefCount, asFactored;
} asvec3_t;

static asvec3_t *objectVector_Factory( void )
{
	asvec3_t *object;

	object = G_AsMalloc( sizeof( asvec3_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	VectorClear( object->v );
	asvector_factored_count++;
	return object;
}

static void objectVector_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectVector_Factory() );
}

static asvec3_t *objectVector_FactorySet( float x, float y, float z )
{
	asvec3_t *object = objectVector_Factory();
	object->v[0] = x;
	object->v[1] = y;
	object->v[2] = z;
	return object;
}

static void objectVector_asGeneric_FactorySet( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectVector_FactorySet( G_asGeneric_GetArgFloat( gen, 0 ), G_asGeneric_GetArgFloat( gen, 1 ),G_asGeneric_GetArgFloat( gen, 2 ) ) );
}

static void objectVector_Addref( asvec3_t *obj ) { obj->asRefCount++; }

static void objectVector_asGeneric_Addref( void *gen ) 
{
	objectVector_Addref( (asvec3_t *)G_asGeneric_GetObject( gen ) );
}

static void objectVector_Release( asvec3_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		asvector_released_count++;
	}
}

static void objectVector_asGeneric_Release( void *gen ) 
{
	objectVector_Release( (asvec3_t *)G_asGeneric_GetObject( gen ) );
}


static asvec3_t *objectString_toVec3( asstring_t *self )
{
	float x = 0, y = 0, z = 0;
	asvec3_t *vec = objectVector_Factory();

	sscanf( self->buffer, "%f %f %f", &x, &y, &z );

	VectorSet( vec->v, x, y, z );
	return vec;
}

static void objectString_asGeneric_toVec3( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_toVec3( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asvec3_t *objectVector_AssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorCopy( other->v, self->v );
	return self;
}

static void objectVector_asGeneric_AssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_AssignBehaviourD( double other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static void objectVector_asGeneric_AssignBehaviourD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviourD( other, self ) );
}

static asvec3_t *objectVector_AssignBehaviourF( float other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static void objectVector_asGeneric_AssignBehaviourF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviourF( other, self ) );
}

static asvec3_t *objectVector_AssignBehaviourI( int other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static void objectVector_asGeneric_AssignBehaviourI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviourI( other, self ) );
}

static asvec3_t *objectVector_AddAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorAdd( self->v, other->v, self->v );
	return self;
}

static void objectVector_asGeneric_AddAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AddAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_SubAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorSubtract( self->v, other->v, self->v );
	return self;
}

static void objectVector_asGeneric_SubAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_SubAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	vec_t product = DotProduct( self->v, other->v );

	VectorScale( self->v, product, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_XORAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	vec3_t product;

	CrossProduct( self->v, other->v, product );
	VectorCopy( product, self->v );
	return self;
}

static void objectVector_asGeneric_XORAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_XORAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviourI( int other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviourI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviourI( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviourD( double other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviourD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviourD( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviourF( float other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviourF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviourF( other, self ) );
}

static asvec3_t *objectVector_AddBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorAdd( first->v, second->v, vec->v );
	return vec;
}

static void objectVector_asGeneric_AddBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_AddBehaviour( first, second ) );
}

static asvec3_t *objectVector_SubtractBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorSubtract( first->v, second->v, vec->v );
	return vec;
}

static void objectVector_asGeneric_SubtractBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_SubtractBehaviour( first, second ) );
}

static float objectVector_MultiplyBehaviour( asvec3_t *first, asvec3_t *second )
{
	return DotProduct( first->v, second->v );
}

static void objectVector_asGeneric_MultiplyBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnFloat( gen, objectVector_MultiplyBehaviour( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourVD( asvec3_t *first, double second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorScale( first->v, second, vec->v );
	return vec;
}

static void objectVector_asGeneric_MultiplyBehaviourVD( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	double second = G_asGeneric_GetArgDouble( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourVD( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourDV( double first, asvec3_t *second )
{
	return objectVector_MultiplyBehaviourVD( second, first );
}

static void objectVector_asGeneric_MultiplyBehaviourDV( void *gen )
{
	double first = G_asGeneric_GetArgDouble( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourDV( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourVF( asvec3_t *first, float second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorScale( first->v, second, vec->v );
	return vec;
}

static void objectVector_asGeneric_MultiplyBehaviourVF( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	float second = G_asGeneric_GetArgFloat( gen, 1 );
	
	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourVF( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourFV( float first, asvec3_t *second )
{
	return objectVector_MultiplyBehaviourVF( second, first );
}

static void objectVector_asGeneric_MultiplyBehaviourFV( void *gen )
{
	float first = G_asGeneric_GetArgFloat( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourFV( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourVI( asvec3_t *first, int second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorScale( first->v, second, vec->v );
	return vec;
}

static void objectVector_asGeneric_MultiplyBehaviourVI( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int second = G_asGeneric_GetArgInt( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourVI( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourIV( int first, asvec3_t *second )
{
	return objectVector_MultiplyBehaviourVI( second, first );
}

static void objectVector_asGeneric_MultiplyBehaviourIV( void *gen )
{
	int first = G_asGeneric_GetArgInt( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourIV( first, second ) );
}

static asvec3_t *objectVector_XORBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t *vec = objectVector_Factory();

	CrossProduct( first->v, second->v, vec->v );
	return vec;
}

static void objectVector_asGeneric_XORBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_XORBehaviour( first, second ) );
}

static qboolean objectVector_EqualBehaviour( asvec3_t *first, asvec3_t *second )
{
	return VectorCompare( first->v, second->v );
}

static void objectVector_asGeneric_EqualBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectVector_EqualBehaviour( first, second ) );
}

static qboolean objectVector_NotEqualBehaviour( asvec3_t *first, asvec3_t *second )
{
	return !VectorCompare( first->v, second->v );
}

static void objectVector_asGeneric_NotEqualBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectVector_NotEqualBehaviour( first, second ) );
}

static void objectVector_Set( float x, float y, float z, asvec3_t *vec )
{
	VectorSet( vec->v, x, y, z );
}

static void objectVector_asGeneric_Set( void *gen )
{
	float x = G_asGeneric_GetArgFloat( gen, 0 );
	float y = G_asGeneric_GetArgFloat( gen, 1 );
	float z = G_asGeneric_GetArgFloat( gen, 2 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_Set( x, y, z, self );
}

static float objectVector_Length( const asvec3_t *vec )
{
	return VectorLength( vec->v );
}

static void objectVector_asGeneric_Length( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectVector_Length( (asvec3_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectVector_Normalize( asvec3_t *vec )
{
	return VectorNormalize( vec->v );
}

static void objectVector_asGeneric_Normalize( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectVector_Normalize( (asvec3_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectVector_toString( asvec3_t *vec )
{
	char *s = vtos( vec->v );
	return objectString_FactoryBuffer( s, strlen( s ) );
}

static void objectVector_asGeneric_toString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectVector_toString( (asvec3_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectVector_Distance( asvec3_t *other, asvec3_t *self )
{
	return Distance( self->v, other->v );
}

static void objectVector_asGeneric_Distance( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnFloat( gen, objectVector_Distance( other, self ) );
}

static void objectVector_AngleVectors( asvec3_t *f, asvec3_t *r, asvec3_t *u, asvec3_t *self )
{
	AngleVectors( self->v, f ? f->v : NULL, r ? r->v : NULL, u ? u->v : NULL );
}

static void objectVector_asGeneric_AngleVectors( void *gen )
{
	asvec3_t *f = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *r = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );
	asvec3_t *u = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 2 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_AngleVectors( f, r, u, self );
}

static void objectVector_VecToAngles( asvec3_t *angles, asvec3_t *self )
{
	VecToAngles( self->v, angles->v );
}

static void objectVector_asGeneric_VecToAngles( void *gen )
{
	asvec3_t *angles = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_VecToAngles( angles, self );
}

static const asBehavior_t asvector_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVec3 @, f, ()), objectVector_Factory, objectVector_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectVector_Addref, objectVector_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectVector_Release, objectVector_asGeneric_Release, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVec3 @, f, ( float x, float y, float z)), objectVector_FactorySet, objectVector_asGeneric_FactorySet, asCALL_CDECL },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_AssignBehaviour, objectVector_asGeneric_AssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (int)), objectVector_AssignBehaviourI, objectVector_asGeneric_AssignBehaviourI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (float)), objectVector_AssignBehaviourF, objectVector_asGeneric_AssignBehaviourF, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (double)), objectVector_AssignBehaviourD, objectVector_asGeneric_AssignBehaviourD, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_AddAssignBehaviour, objectVector_asGeneric_AddAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_SUB_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_SubAssignBehaviour, objectVector_asGeneric_SubAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_MulAssignBehaviour, objectVector_asGeneric_MulAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_XOR_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_XORAssignBehaviour, objectVector_asGeneric_XORAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (int)), objectVector_MulAssignBehaviourI, objectVector_asGeneric_MulAssignBehaviourI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (float)), objectVector_MulAssignBehaviourF, objectVector_asGeneric_MulAssignBehaviourF, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (double)), objectVector_MulAssignBehaviourD, objectVector_asGeneric_MulAssignBehaviourD, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t asvector_GlobalBehaviors[] =
{
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, const cVec3 &in)), objectVector_AddBehaviour, objectVector_asGeneric_AddBehaviour, asCALL_CDECL },
	{ asBEHAVE_SUBTRACT, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, const cVec3 &in)), objectVector_SubtractBehaviour, objectVector_asGeneric_SubtractBehaviour, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(float, f, (const cVec3 &in, const cVec3 &in)), objectVector_MultiplyBehaviour, objectVector_asGeneric_MultiplyBehaviour, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, double)), objectVector_MultiplyBehaviourVD, objectVector_asGeneric_MultiplyBehaviourVD, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (double, const cVec3 &in)), objectVector_MultiplyBehaviourDV, objectVector_asGeneric_MultiplyBehaviourDV, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, float)), objectVector_MultiplyBehaviourVF, objectVector_asGeneric_MultiplyBehaviourVF, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (float, const cVec3 &in)), objectVector_MultiplyBehaviourFV, objectVector_asGeneric_MultiplyBehaviourFV, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, int)), objectVector_MultiplyBehaviourVI, objectVector_asGeneric_MultiplyBehaviourVI, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (int, const cVec3 &in)), objectVector_MultiplyBehaviourIV, objectVector_asGeneric_MultiplyBehaviourIV, asCALL_CDECL },
	{ asBEHAVE_BIT_XOR, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, const cVec3 &in)), objectVector_XORBehaviour, objectVector_asGeneric_XORBehaviour, asCALL_CDECL },
	{ asBEHAVE_EQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cVec3 &in, const cVec3 &in)), objectVector_EqualBehaviour, objectVector_asGeneric_EqualBehaviour, asCALL_CDECL },
	{ asBEHAVE_NOTEQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cVec3 &in, const cVec3 &in)), objectVector_NotEqualBehaviour, objectVector_asGeneric_NotEqualBehaviour, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t asvector_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(void, set, ( float x, float y, float z )), objectVector_Set, objectVector_asGeneric_Set, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, length, ()), objectVector_Length, objectVector_asGeneric_Length, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, normalize, ()), objectVector_Normalize, objectVector_asGeneric_Normalize, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, toString, ()), objectVector_toString, objectVector_asGeneric_toString, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, distance, (const cVec3 &in)), objectVector_Distance, objectVector_asGeneric_Distance, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, angleVectors, (cVec3 @+, cVec3 @+, cVec3 @+)), objectVector_AngleVectors, objectVector_asGeneric_AngleVectors, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, toAngles, (cVec3 &)), objectVector_VecToAngles, objectVector_asGeneric_VecToAngles, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t asvector_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(float, x), FOFFSET(asvec3_t, v[0]) },
	{ SCRIPT_PROPERTY_DECL(float, y), FOFFSET(asvec3_t, v[1]) },
	{ SCRIPT_PROPERTY_DECL(float, z), FOFFSET(asvec3_t, v[2]) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asVectorClassDescriptor =
{
	"cVec3",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( asvec3_t ),			/* size */
	asvector_ObjectBehaviors,	/* object behaviors */
	asvector_GlobalBehaviors,	/* global behaviors */
	asvector_Methods,			/* methods */
	asvector_Properties,		/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cVar 
static int cvar_factored_count = 0;
static int cvar_released_count = 0;

typedef struct  
{
	cvar_t *cvar;
	int asFactored, asRefCount;
}ascvar_t;

static ascvar_t *objectCVar_Factory()
{
	static ascvar_t *object;

	object = G_AsMalloc( sizeof( ascvar_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	cvar_factored_count++;
	return object;
}

static void objectCVar_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_Factory() );
}

static ascvar_t *objectCVar_FactoryRegister( asstring_t *name, asstring_t *def, unsigned int flags )
{
	ascvar_t *self = objectCVar_Factory();

	self->cvar = trap_Cvar_Get( name->buffer, def->buffer, flags );

	return self;
}

static void objectCVar_asGeneric_FactoryRegister( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, 
		objectCVar_FactoryRegister( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
									(asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ),
									(unsigned int)G_asGeneric_GetArgInt( gen, 2 ) )
		);
}

static void objectCVar_Addref( ascvar_t *obj ) { obj->asRefCount++; }

static void objectCVar_asGeneric_Addref( void *gen )
{
	objectCVar_Addref( (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_Release( ascvar_t *obj ) 
{
	obj->asRefCount--; 
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		cvar_released_count++;
	}
}

static void objectCVar_asGeneric_Release( void *gen )
{
	objectCVar_Release( (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_Register( asstring_t *name, asstring_t *def, unsigned int flags, ascvar_t *self )
{
	if( !name || !def )
		return;

	self->cvar = trap_Cvar_Get( name->buffer, def->buffer, flags );
}

static void objectCVar_asGeneric_Register( void *gen )
{
	objectCVar_Register( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		(unsigned int)G_asGeneric_GetArgInt( gen, 2 ),
		(ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_Reset( ascvar_t *self )
{
	if( !self->cvar )
		return;

	trap_Cvar_Set( self->cvar->name, self->cvar->dvalue );
}

static void objectCVar_asGeneric_Reset( void *gen )
{
	objectCVar_Reset( (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setS( asstring_t *str, ascvar_t *self )
{
	if( !str || !self->cvar )
		return;

	trap_Cvar_Set( self->cvar->name, str->buffer );
}

static void objectCVar_asGeneric_setS( void *gen )
{
	objectCVar_setS( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setF( float value, ascvar_t *self )
{
	if( !self->cvar )
		return;

	trap_Cvar_SetValue( self->cvar->name, value );
}

static void objectCVar_asGeneric_setF( void *gen )
{
	objectCVar_setF( G_asGeneric_GetArgFloat( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setI( int value, ascvar_t *self )
{
	objectCVar_setF( (float)value, self );
}

static void objectCVar_asGeneric_setI( void *gen )
{
	objectCVar_setI( G_asGeneric_GetArgInt( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setD( double value, ascvar_t *self )
{
	objectCVar_setF( (float)value, self );
}

static void objectCVar_asGeneric_setD( void *gen )
{
	objectCVar_setD( G_asGeneric_GetArgDouble( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectCVar_getBool( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return ( self->cvar->integer != 0 );
}

static void objectCVar_asGeneric_getBool( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectCVar_getBool( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectCVar_getModified( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return self->cvar->modified;
}

static void objectCVar_asGeneric_getModified( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectCVar_getModified( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectCVar_getInteger( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return self->cvar->integer;
}

static void objectCVar_asGeneric_getInteger( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectCVar_getInteger( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectCVar_getValue( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return self->cvar->value;
}

static void objectCVar_asGeneric_getValue( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectCVar_getValue( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getName( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->name )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->name, strlen( self->cvar->name ) );
}

static void objectCVar_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getName( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getString( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->string )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->string, strlen( self->cvar->string ) );
}

static void objectCVar_asGeneric_getString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getString( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getDefaultString( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->dvalue )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->dvalue, strlen( self->cvar->dvalue ) );
}

static void objectCVar_asGeneric_getDefaultString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getDefaultString( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getLatchedString( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->latched_string )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->latched_string, strlen( self->cvar->latched_string ) );
}

static void objectCVar_asGeneric_getLatchedString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getLatchedString( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static const asBehavior_t ascvar_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVar@, f, ()), objectCVar_Factory, objectCVar_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectCVar_Addref, objectCVar_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectCVar_Release, objectCVar_asGeneric_Release, asCALL_CDECL_OBJLAST },

	// alternative factory
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVar@, f, ( cString &in, cString &in, uint flags )), objectCVar_FactoryRegister, objectCVar_asGeneric_FactoryRegister, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t ascvar_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(void , get, ( cString &in, cString &in, uint flags )), objectCVar_Register, objectCVar_asGeneric_Register, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , reset, ()), objectCVar_Reset, objectCVar_asGeneric_Reset, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( cString &in )), objectCVar_setS, objectCVar_asGeneric_setS, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( float value )), objectCVar_setF, objectCVar_asGeneric_setF, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( int value )), objectCVar_setI, objectCVar_asGeneric_setI, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( double value )), objectCVar_setD, objectCVar_asGeneric_setD, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool , modified, ()), objectCVar_getModified, objectCVar_asGeneric_getModified, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool , getBool, ()), objectCVar_getBool, objectCVar_asGeneric_getBool, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int , getInteger, ()), objectCVar_getInteger, objectCVar_asGeneric_getInteger, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float , getValue, ()), objectCVar_getValue, objectCVar_asGeneric_getValue, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getName, ()), objectCVar_getName, objectCVar_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getString, ()), objectCVar_getString, objectCVar_asGeneric_getString, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getDefaultString, ()), objectCVar_getDefaultString, objectCVar_asGeneric_getDefaultString, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getLatchedString, ()), objectCVar_getLatchedString, objectCVar_asGeneric_getLatchedString, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t ascvar_Properties[] =
{
	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asCVarClassDescriptor =
{
	"cVar",						/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( ascvar_t ),			/* size */
	ascvar_ObjectBehaviors,		/* object behaviors */
	NULL,						/* global behaviors */
	ascvar_Methods,				/* methods */
	ascvar_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cTime
static int astime_factored_count = 0;
static int astime_released_count = 0;

typedef struct  
{
	time_t time;
	struct tm localtime;
	int asFactored, asRefCount;
} astime_t;

static astime_t *objectTime_Factory( time_t time )
{
	static astime_t *object;

	object = G_AsMalloc( sizeof( *object ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	object->time = time;

	if( time )
	{
		struct tm *tm;
		tm = localtime( &time );
		object->localtime = *tm;
	}

	astime_factored_count++;
	return object;
}

static void objectTime_asGeneric_Factory( void *gen )
{
	time_t time = ( time_t )G_asGeneric_GetArgInt64( gen, 0 );
	G_asGeneric_SetReturnAddress( gen, objectTime_Factory( time ) );
}

static astime_t *objectTime_FactoryEmpty( void )
{
	return objectTime_Factory( 0 );
}

static void objectTime_asGeneric_FactoryEmpty( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTime_FactoryEmpty() );
}

static void objectTime_Addref( astime_t *obj ) { obj->asRefCount++; }

static void objectTime_asGeneric_Addref( void *gen )
{
	objectTime_Addref( (astime_t *)G_asGeneric_GetObject( gen ) );
}

static void objectTime_Release( astime_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		astime_factored_count++;
	}
}

static void objectTime_asGeneric_Release( void *gen )
{
	objectTime_Release( (astime_t *)G_asGeneric_GetObject( gen ) );
}

static astime_t *objectTime_AssignBehaviour( astime_t *other, astime_t *self )
{
	self->time = other->time;
	memcpy( &(self->localtime), &(other->localtime), sizeof( struct tm ) );
	return self;
}

static void objectTime_asGeneric_AssignBehaviour( void *gen )
{
	astime_t *other = (astime_t *)G_asGeneric_GetArgAddress( gen, 0 );
	astime_t *self = (astime_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectTime_AssignBehaviour( other, self ) );
}

static qboolean objectTime_EqualBehaviour( astime_t *first, astime_t *second )
{
	return (first->time == second->time);
}

static void objectTime_asGeneric_EqualBehaviour( void *gen )
{
	astime_t *first = (astime_t *)G_asGeneric_GetArgAddress( gen, 0 );
	astime_t *second = (astime_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectTime_EqualBehaviour( first, second ) );
}

static qboolean objectTime_NotEqualBehaviour( astime_t *first, astime_t *second )
{
	return !objectTime_EqualBehaviour( first, second );
}

static void objectTime_asGeneric_NotEqualBehaviour( void *gen )
{
	astime_t *first = (astime_t *)G_asGeneric_GetArgAddress( gen, 0 );
	astime_t *second = (astime_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectTime_NotEqualBehaviour( first, second ) );
}

static const asBehavior_t astime_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cTime @, f, ()), objectTime_FactoryEmpty, objectTime_asGeneric_FactoryEmpty, asCALL_CDECL },
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cTime @, f, (uint64 t)), objectTime_Factory, objectTime_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectTime_Addref, objectTime_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectTime_Release, objectTime_asGeneric_Release, asCALL_CDECL_OBJLAST },

	/* assignments */
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cTime &, f, (const cTime &in)), objectTime_AssignBehaviour, objectTime_asGeneric_AssignBehaviour, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t astime_GlobalBehaviors[] =
{
	/* == != */
	{ asBEHAVE_EQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cTime &in, const cTime &in)), objectTime_EqualBehaviour, objectTime_asGeneric_EqualBehaviour, asCALL_CDECL },
	{ asBEHAVE_NOTEQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cTime &in, const cTime &in)), objectTime_NotEqualBehaviour, objectTime_asGeneric_NotEqualBehaviour, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asProperty_t astime_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const uint64, time), FOFFSET(astime_t, time) },
	{ SCRIPT_PROPERTY_DECL(const int, sec), FOFFSET(astime_t, localtime.tm_sec) },
	{ SCRIPT_PROPERTY_DECL(const int, min), FOFFSET(astime_t, localtime.tm_min) },
	{ SCRIPT_PROPERTY_DECL(const int, hour), FOFFSET(astime_t, localtime.tm_hour) },
	{ SCRIPT_PROPERTY_DECL(const int, mday), FOFFSET(astime_t, localtime.tm_mday) },
	{ SCRIPT_PROPERTY_DECL(const int, mon), FOFFSET(astime_t, localtime.tm_mon) },
	{ SCRIPT_PROPERTY_DECL(const int, year), FOFFSET(astime_t, localtime.tm_year) },
	{ SCRIPT_PROPERTY_DECL(const int, wday), FOFFSET(astime_t, localtime.tm_wday) },
	{ SCRIPT_PROPERTY_DECL(const int, yday), FOFFSET(astime_t, localtime.tm_yday) },
	{ SCRIPT_PROPERTY_DECL(const int, isdst), FOFFSET(astime_t, localtime.tm_isdst) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asTimeClassDescriptor =
{
	"cTime",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( astime_t ),			/* size */
	astime_ObjectBehaviors,		/* object behaviors */
	astime_GlobalBehaviors,		/* global behaviors */
	NULL,						/* methods */
	astime_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cTrace 
static int trace_factored_count = 0;
static int trace_released_count = 0;

typedef struct  
{
	trace_t trace;
	int asFactored, asRefCount;
}astrace_t;

static astrace_t *objectTrace_Factory()
{
	static astrace_t *object;

	object = G_AsMalloc( sizeof( astrace_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	trace_factored_count++;
	return object;
}

static void objectTrace_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTrace_Factory() );
}

static void objectTrace_Addref( astrace_t *obj ) { obj->asRefCount++; }

static void objectTrace_asGeneric_Addref( void *gen )
{
	objectTrace_Addref( (astrace_t *)G_asGeneric_GetObject( gen ) );
}

static void objectTrace_Release( astrace_t *obj ) 
{
	obj->asRefCount--; 
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		trace_released_count++;
	}
}

static void objectTrace_asGeneric_Release( void *gen )
{
	objectTrace_Release( (astrace_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectTrace_doTrace( asvec3_t *start, asvec3_t *mins, asvec3_t *maxs, asvec3_t *end, int ignore, int contentMask, astrace_t *self )
{
	if( !start || !end ) // should never happen unless the coder explicitly feeds null
	{
		GS_Printf( "* WARNING: gametype plugin script attempted to call method 'trace.doTrace' with a null vector pointer\n* Tracing skept" );
		return qfalse;
	}

	GS_Trace( &self->trace, start->v, mins ? mins->v : vec3_origin, maxs ? maxs->v : vec3_origin, end->v, ignore, contentMask, 0 );
	
	return ( self->trace.ent != -1 );
}

static void objectTrace_asGeneric_doTrace( void *gen )
{
	asvec3_t *start = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *mins = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );
	asvec3_t *maxs = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 2 );
	asvec3_t *end = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 3 );
	int ignore = G_asGeneric_GetArgInt( gen, 4 );
	int contentMask = G_asGeneric_GetArgInt( gen, 5 );
	astrace_t *self = (astrace_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnBool( gen, objectTrace_doTrace( start, mins, maxs, end, ignore, contentMask, self ) );
}

static asvec3_t *objectTrace_getEndPos( astrace_t *self )
{
	asvec3_t *asvec = objectVector_Factory();

	VectorCopy( self->trace.endpos, asvec->v );
	return asvec;
}

static void objectTrace_asGeneric_getEndPos( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTrace_getEndPos( (astrace_t *)G_asGeneric_GetObject( gen ) ) );
}

static asvec3_t *objectTrace_getPlaneNormal( astrace_t *self )
{
	asvec3_t *asvec = objectVector_Factory();

	VectorCopy( self->trace.plane.normal, asvec->v );
	return asvec;
}

static void objectTrace_asGeneric_getPlaneNormal( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTrace_getPlaneNormal( (astrace_t *)G_asGeneric_GetObject( gen ) ) );
}

static const asBehavior_t astrace_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cTrace@, f, ()), objectTrace_Factory, objectTrace_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectTrace_Addref, objectTrace_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectTrace_Release, objectTrace_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t astrace_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(bool, doTrace, ( cVec3 &in, cVec3 &, cVec3 &, cVec3 &in, int ignore, int contentMask )), objectTrace_doTrace, objectTrace_asGeneric_doTrace, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getEndPos, ()), objectTrace_getEndPos, objectTrace_asGeneric_getEndPos, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getPlaneNormal, ()), objectTrace_getPlaneNormal, objectTrace_asGeneric_getPlaneNormal, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t astrace_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const bool, allSolid), FOFFSET(astrace_t, trace.allsolid) },
	{ SCRIPT_PROPERTY_DECL(const bool, startSolid), FOFFSET(astrace_t, trace.startsolid) },
	{ SCRIPT_PROPERTY_DECL(const float, fraction), FOFFSET(astrace_t, trace.fraction) },
	{ SCRIPT_PROPERTY_DECL(const int, surfFlags), FOFFSET(astrace_t, trace.surfFlags) },
	{ SCRIPT_PROPERTY_DECL(const int, contents), FOFFSET(astrace_t, trace.contents) },
	{ SCRIPT_PROPERTY_DECL(const int, entNum), FOFFSET(astrace_t, trace.ent) },
	{ SCRIPT_PROPERTY_DECL(const float, planeDist), FOFFSET(astrace_t, trace.plane.dist) },
	{ SCRIPT_PROPERTY_DECL(const int16, planeType), FOFFSET(astrace_t, trace.plane.type) },
	{ SCRIPT_PROPERTY_DECL(const int16, planeSignBits), FOFFSET(astrace_t, trace.plane.signbits) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asTraceClassDescriptor =
{
	"cTrace",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( astrace_t ),		/* size */
	astrace_ObjectBehaviors,	/* object behaviors */
	NULL,						/* global behaviors */
	astrace_Methods,			/* methods */
	astrace_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cEntity
static int edict_factored_count = 0;
static int edict_released_count = 0;

static gentity_t *objectGameEntity_Factory()
{
	static gentity_t *object;

	object = G_AsMalloc( sizeof( gentity_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	object->s.number = -1;
	edict_factored_count++;
	return object;
}

static void objectGameEntity_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_Factory() );
}

static void objectGameEntity_Addref( gentity_t *obj ) { obj->asRefCount++; }

static void objectGameEntity_asGeneric_Addref( void *gen )
{
	objectGameEntity_Addref( (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_Release( gentity_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		edict_released_count++;
	}
}

static void objectGameEntity_asGeneric_Release( void *gen )
{
	objectGameEntity_Release( (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGameEntity_getModelName( gentity_t *self )
{
	return objectString_FactoryBuffer( self->model, self->model ? strlen(self->model) : 0 );
}

static void objectGameEntity_asGeneric_getModelName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getModelName( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGameEntity_getModel2Name( gentity_t *self )
{
	return objectString_FactoryBuffer( self->model2, self->model2 ? strlen(self->model2) : 0 );
}

static void objectGameEntity_asGeneric_getModel2Name( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getModel2Name( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGameEntity_getClassname( gentity_t *self )
{
	return objectString_FactoryBuffer( self->classname, self->classname ? strlen(self->classname) : 0 );
}

static void objectGameEntity_asGeneric_getClassname( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getClassname( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGameEntity_isBrushModel( gentity_t *self )
{
	return GS_IsBrushModel( self->s.modelindex1 );
}

static void objectGameEntity_asGeneric_isBrushModel( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameEntity_isBrushModel( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGameEntity_IsGhosting( gentity_t *self )
{
	if( self->client && trap_GetClientState( ENTNUM( self ) ) < CS_SPAWNED )
		return qtrue;

	return GS_IsGhostState( &self->s );
}

static void objectGameEntity_asGeneric_IsGhosting( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameEntity_IsGhosting( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}


static void objectGameEntity_asGeneric_G_FreeEntity( void *gen )
{
	G_FreeEntity( (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_asGeneric_GClip_LinkEntity( void *gen )
{
	GClip_LinkEntity( (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_asGeneric_GClip_UnlinkEntity( void *gen )
{
	GClip_UnlinkEntity( (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static asvec3_t *objectGameEntity_GetVelocity( gentity_t *obj )
{
	asvec3_t *velocity = objectVector_Factory();

	VectorCopy( obj->s.ms.velocity, velocity->v );

	return velocity;
}

static void objectGameEntity_asGeneric_GetVelocity( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetVelocity( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_SetVelocity( asvec3_t *vel, gentity_t *self )
{
	VectorCopy( vel->v, self->s.ms.velocity );
}

static void objectGameEntity_asGeneric_SetVelocity( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetVelocity( vel, (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static asvec3_t *objectGameEntity_GetOrigin( gentity_t *obj )
{
	asvec3_t *origin = objectVector_Factory();

	VectorCopy( obj->s.ms.origin, origin->v );

	return origin;
}

static void objectGameEntity_asGeneric_GetOrigin( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetOrigin( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_SetOrigin( asvec3_t *vec, gentity_t *self )
{
	VectorCopy( vec->v, self->s.ms.origin );
}

static void objectGameEntity_asGeneric_SetOrigin( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetOrigin( vel, (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static asvec3_t *objectGameEntity_GetAngles( gentity_t *obj )
{
	asvec3_t *angles = objectVector_Factory();

	VectorCopy( obj->s.ms.angles, angles->v );

	return angles;
}

static void objectGameEntity_asGeneric_GetAngles( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetAngles( (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_SetAngles( asvec3_t *vec, gentity_t *self )
{
	VectorCopy( vec->v, self->s.ms.angles );
}

static void objectGameEntity_asGeneric_SetAngles( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetAngles( vel, (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_GetSize( asvec3_t *mins, asvec3_t *maxs, gentity_t *self )
{
	if( maxs )
		VectorCopy( self->s.local.maxs , maxs->v );
	if( mins )
		VectorCopy( self->s.local.mins, mins->v );
}

static void objectGameEntity_asGeneric_GetSize( void *gen )
{
	asvec3_t *mins = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *maxs = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	objectGameEntity_GetSize( mins, maxs, (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_SetSize( asvec3_t *mins, asvec3_t *maxs, gentity_t *self )
{
	if( mins )
		VectorCopy( mins->v, self->s.local.mins );
	if( maxs )
		VectorCopy( maxs->v, self->s.local.maxs );
}

static void objectGameEntity_asGeneric_SetSize( void *gen )
{
	asvec3_t *mins = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *maxs = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	objectGameEntity_SetSize( mins, maxs, (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static gentity_t *objectGameEntity_findTarget( gentity_t *from, gentity_t *self )
{
	if( !self->target )
		return NULL;

	return G_Find( from, FOFS( targetname ), self->target );
}

static void objectGameEntity_asGeneric_findTarget( void *gen )
{
	gentity_t *from = (gentity_t *)G_asGeneric_GetArgAddress( gen, 0 );

	G_asGeneric_SetReturnAddress( gen, objectGameEntity_findTarget( from, (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static gentity_t *objectGameEntity_findTargeting( gentity_t *from, gentity_t *self )
{
	if( !self->targetname )
		return NULL;

	return G_Find( from, FOFS( target ), self->targetname );
}

static void objectGameEntity_asGeneric_findTargeting( void *gen )
{
	gentity_t *from = (gentity_t *)G_asGeneric_GetArgAddress( gen, 0 );

	G_asGeneric_SetReturnAddress( gen, objectGameEntity_findTargeting( from, (gentity_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_Activate( gentity_t *targeter, gentity_t *activator, gentity_t *self )
{
	if( self->activate )
		self->activate( self, targeter, activator );
}

static void objectGameEntity_asGeneric_Activate( void *gen )
{
	gentity_t *targeter = (gentity_t *)G_asGeneric_GetArgAddress( gen, 0 );
	gentity_t *activator = (gentity_t *)G_asGeneric_GetArgAddress( gen, 1 );

	objectGameEntity_Activate( targeter, activator, (gentity_t *)G_asGeneric_GetObject( gen ) );
}


static void objectGameEntity_ActivateTargets( gentity_t *activator, gentity_t *self )
{
	G_ActivateTargets( self, activator );
}

static void objectGameEntity_asGeneric_ActivateTargets( void *gen )
{
	gentity_t *activator = (gentity_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectGameEntity_ActivateTargets( activator, (gentity_t *)G_asGeneric_GetObject( gen ) );
}

static const asBehavior_t gedict_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cEntity @, f, ()), objectGameEntity_Factory, objectGameEntity_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectGameEntity_Addref, objectGameEntity_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectGameEntity_Release, objectGameEntity_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t gedict_GlobalBehaviors[] =
{
	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t gedict_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(cString @, getClassName, ()), objectGameEntity_getClassname, objectGameEntity_asGeneric_getClassname, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(cString @, getModelString, ()), objectGameEntity_getModelName, objectGameEntity_asGeneric_getModelName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getModel2String, ()), objectGameEntity_getModel2Name, objectGameEntity_asGeneric_getModel2Name, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(bool, isBrushModel, ()), objectGameEntity_isBrushModel, objectGameEntity_asGeneric_isBrushModel, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isGhosting, ()), objectGameEntity_IsGhosting, objectGameEntity_asGeneric_IsGhosting, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(void, freeEntity, ()), G_FreeEntity, objectGameEntity_asGeneric_G_FreeEntity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, linkEntity, ()), GClip_LinkEntity, objectGameEntity_asGeneric_GClip_LinkEntity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, unlinkEntity, ()), GClip_UnlinkEntity, objectGameEntity_asGeneric_GClip_UnlinkEntity, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(cVec3 @, getVelocity, ()), objectGameEntity_GetVelocity, objectGameEntity_asGeneric_GetVelocity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setVelocity, (cVec3 &in)), objectGameEntity_SetVelocity, objectGameEntity_asGeneric_SetVelocity, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(cVec3 @, getOrigin, ()), objectGameEntity_GetOrigin, objectGameEntity_asGeneric_GetOrigin, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setOrigin, (cVec3 &in)), objectGameEntity_SetOrigin, objectGameEntity_asGeneric_SetOrigin, asCALL_CDECL_OBJLAST },
	
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getAngles, ()), objectGameEntity_GetAngles, objectGameEntity_asGeneric_GetAngles, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setAngles, (cVec3 &in)), objectGameEntity_SetAngles, objectGameEntity_asGeneric_SetAngles, asCALL_CDECL_OBJLAST },
	
	{ SCRIPT_FUNCTION_DECL(void, getSize, (cVec3 @+, cVec3 @+)), objectGameEntity_GetSize, objectGameEntity_asGeneric_GetSize, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setSize, (cVec3 @+, cVec3 @+)), objectGameEntity_SetSize, objectGameEntity_asGeneric_SetSize, asCALL_CDECL_OBJLAST },
	
	// utilities
	{ SCRIPT_FUNCTION_DECL(cEntity @, getTargetEntity, ( cEntity @from )), objectGameEntity_findTarget, objectGameEntity_asGeneric_findTarget, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cEntity @, getTargetingEntity, ( cEntity @from )), objectGameEntity_findTargeting, objectGameEntity_asGeneric_findTargeting, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(void, activate, ( cEntity @targeter, cEntity @activator )), objectGameEntity_Activate, objectGameEntity_asGeneric_Activate, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, activateTargets, ( cEntity @activator )), objectGameEntity_ActivateTargets, objectGameEntity_asGeneric_ActivateTargets, asCALL_CDECL_OBJLAST },

/*	To do:
	{ SCRIPT_FUNCTION_DECL(void, ghost, ()), objectGameEntity_GhostClient, objectGameEntity_asGeneric_GhostClient, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, spawnqueueAdd, ()), G_SpawnQueue_AddClient, objectGameEntity_asGeneric_G_SpawnQueue_AddClient, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setupModel, ( cString &in )), objectGameEntity_SetupModel, objectGameEntity_asGeneric_SetupModel, asCALL_CDECL_OBJLAST },
*/
	SCRIPT_METHOD_NULL
};

static const asProperty_t gedict_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(int, spawnFlags), FOFFSET(gentity_t, spawnflags) },
	{ SCRIPT_PROPERTY_DECL(int, netFlags), FOFFSET(gentity_t, netflags) },
	{ SCRIPT_PROPERTY_DECL(uint, delay), FOFFSET(gentity_t, delay) },
	{ SCRIPT_PROPERTY_DECL(float, wait), FOFFSET(gentity_t, wait) },
	{ SCRIPT_PROPERTY_DECL(int, count), FOFFSET(gentity_t, count) },
	{ SCRIPT_PROPERTY_DECL(float, health), FOFFSET(gentity_t, health) },
	{ SCRIPT_PROPERTY_DECL(int, maxHealth), FOFFSET(gentity_t, maxHealth) },
	{ SCRIPT_PROPERTY_DECL(int, damage), FOFFSET(gentity_t, damage) },
	{ SCRIPT_PROPERTY_DECL(int, kick), FOFFSET(gentity_t, kick) },
	{ SCRIPT_PROPERTY_DECL(int, splashRadius), FOFFSET(gentity_t, splashRadius) },
	{ SCRIPT_PROPERTY_DECL(int, mass), FOFFSET(gentity_t, mass) },
	{ SCRIPT_PROPERTY_DECL(uint, nextThink), FOFFSET(gentity_t, nextthink) },
	{ SCRIPT_PROPERTY_DECL(uint, painTimeStamp), FOFFSET(gentity_t, paintimestamp) },

	// fixme: entity_state_t
	{ SCRIPT_PROPERTY_DECL(const int, number), FOFFSET(gentity_t, s.number) },
	{ SCRIPT_PROPERTY_DECL(int, type), FOFFSET(gentity_t, s.type) },
	{ SCRIPT_PROPERTY_DECL(int, team), FOFFSET(gentity_t, s.team) },
	{ SCRIPT_PROPERTY_DECL(int, playerClass), FOFFSET(gentity_t, s.playerclass) },
	{ SCRIPT_PROPERTY_DECL(int, flags), FOFFSET(gentity_t, s.flags) },
	{ SCRIPT_PROPERTY_DECL(int, solid), FOFFSET(gentity_t, s.solid) },
	{ SCRIPT_PROPERTY_DECL(int, cmodeltype), FOFFSET(gentity_t, s.cmodeltype) },
	{ SCRIPT_PROPERTY_DECL(int, modelindex1), FOFFSET(gentity_t, s.modelindex1) },
	{ SCRIPT_PROPERTY_DECL(int, modelindex2), FOFFSET(gentity_t, s.modelindex2) },
	{ SCRIPT_PROPERTY_DECL(int, skinindex), FOFFSET(gentity_t, s.skinindex) },
	{ SCRIPT_PROPERTY_DECL(int, weapon), FOFFSET(gentity_t, s.weapon) },
	{ SCRIPT_PROPERTY_DECL(uint, effects), FOFFSET(gentity_t, s.effects) },
	{ SCRIPT_PROPERTY_DECL(uint, sound), FOFFSET(gentity_t, s.sound) },

	// fixme: moveenvironment_t
	{ SCRIPT_PROPERTY_DECL(int, env_groundEntity), FOFFSET(gentity_t, env.groundentity) },
	{ SCRIPT_PROPERTY_DECL(int, env_groundSurfFlags), FOFFSET(gentity_t, env.groundsurfFlags) },
	{ SCRIPT_PROPERTY_DECL(int, env_groundContents), FOFFSET(gentity_t, env.groundcontents) },
	{ SCRIPT_PROPERTY_DECL(int, env_waterLevel), FOFFSET(gentity_t, env.waterlevel) },
	{ SCRIPT_PROPERTY_DECL(int, env_waterType), FOFFSET(gentity_t, env.watertype) },
	{ SCRIPT_PROPERTY_DECL(float, env_frictionFrac), FOFFSET(gentity_t, env.frictionFrac) },
	// fixme: { SCRIPT_PROPERTY_DECL(cplane_t, env_groundPlane), FOFFSET(gentity_t, env.groundplane) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asGameEntityClassDescriptor =
{
	"cEntity",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( gentity_t ),		/* size */
	gedict_ObjectBehaviors,		/* object behaviors */
	gedict_GlobalBehaviors,		/* global behaviors */
	gedict_Methods,				/* methods */
	gedict_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cGametypeDesc
/*
static asstring_t *objectGametypeDescriptor_getTitle( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPETITLE );

	return StringFactory( strlen( s ), s );
}

static void objectGametypeDescriptor_asGeneric_getTitle( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getTitle( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGametypeDescriptor_setTitle( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPETITLE, other->buffer );
}

static void objectGametypeDescriptor_asGeneric_setTitle( void *gen )
{
	objectGametypeDescriptor_setTitle( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGametypeDescriptor_getName( gametype_descriptor_t *self )
{
	return StringFactory( strlen(gs.gametypeName), gs.gametypeName );
}

static void objectGametypeDescriptor_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getName( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGametypeDescriptor_getVersion( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPEVERSION );

	return StringFactory( strlen( s ), s );
}

static void objectGametypeDescriptor_asGeneric_getVersion( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getVersion( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGametypeDescriptor_setVersion( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPEVERSION, other->buffer );
}

static void objectGametypeDescriptor_asGeneric_setVersion( void *gen )
{
	objectGametypeDescriptor_setVersion( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGametypeDescriptor_getAuthor( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPEAUTHOR );

	return StringFactory( strlen( s ), s );
}

static void objectGametypeDescriptor_asGeneric_getAuthor( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getAuthor( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGametypeDescriptor_setAuthor( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPEAUTHOR, other->buffer );
}

static void objectGametypeDescriptor_asGeneric_setAuthor( void *gen )
{
	objectGametypeDescriptor_setAuthor( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGametypeDescriptor_SetTeamSpawnsystem( int team, int spawnsystem, int wave_time, int wave_maxcount, qboolean spectate_team, gametype_descriptor_t *self )
{
	G_SpawnQueue_SetTeamSpawnsystem( team, spawnsystem, wave_time, wave_maxcount, spectate_team );
}

static void objectGametypeDescriptor_asGeneric_SetTeamSpawnsystem( void *gen )
{
	objectGametypeDescriptor_SetTeamSpawnsystem( 
		G_asGeneric_GetArgInt( gen, 0 ),
		G_asGeneric_GetArgInt( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgBool( gen, 4 ), 
		(gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectGametypeDescriptor_isInstagib( gametype_descriptor_t *self )
{
	return GS_Instagib();
}

static void objectGametypeDescriptor_asGeneric_isInstagib( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_isInstagib( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGametypeDescriptor_hasFallDamage( gametype_descriptor_t *self )
{
	return GS_FallDamage();
}

static void objectGametypeDescriptor_asGeneric_hasFallDamage( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_hasFallDamage( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGametypeDescriptor_hasSelfDamage( gametype_descriptor_t *self )
{
	return GS_SelfDamage();
}

static void objectGametypeDescriptor_asGeneric_hasSelfDamage( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_hasSelfDamage( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGametypeDescriptor_isInvidualGameType( gametype_descriptor_t *self )
{
	return GS_InvidualGameType();
}

static void objectGametypeDescriptor_asGeneric_isInvidualGameType( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_isInvidualGameType( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}
*/
static const asBehavior_t gametypedescr_ObjectBehaviors[] =
{
	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t gametypedescr_Methods[] =
{
/*
	{ SCRIPT_FUNCTION_DECL(cString @, getName, ()), objectGametypeDescriptor_getName, objectGametypeDescriptor_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getTitle, ()), objectGametypeDescriptor_getTitle, objectGametypeDescriptor_asGeneric_getTitle, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setTitle, ( cString & )), objectGametypeDescriptor_setTitle, objectGametypeDescriptor_asGeneric_setTitle, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getVersion, ()), objectGametypeDescriptor_getVersion, objectGametypeDescriptor_asGeneric_getVersion, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setVersion, ( cString & )), objectGametypeDescriptor_setVersion, objectGametypeDescriptor_asGeneric_setVersion, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getAuthor, ()), objectGametypeDescriptor_getAuthor, objectGametypeDescriptor_asGeneric_getAuthor, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setAuthor, ( cString & )), objectGametypeDescriptor_setAuthor, objectGametypeDescriptor_asGeneric_setAuthor, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setTeamSpawnsystem, ( int team, int spawnsystem, int wave_time, int wave_maxcount, bool deadcam )), objectGametypeDescriptor_SetTeamSpawnsystem, objectGametypeDescriptor_asGeneric_SetTeamSpawnsystem, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isInstagib, ()), objectGametypeDescriptor_isInstagib, objectGametypeDescriptor_asGeneric_isInstagib, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, hasFallDamage, ()), objectGametypeDescriptor_hasFallDamage, objectGametypeDescriptor_asGeneric_hasFallDamage, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, hasSelfDamage, ()), objectGametypeDescriptor_hasSelfDamage, objectGametypeDescriptor_asGeneric_hasSelfDamage, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isInvidualGameType, ()), objectGametypeDescriptor_isInvidualGameType, objectGametypeDescriptor_asGeneric_isInvidualGameType, asCALL_CDECL_OBJLAST },
*/
	SCRIPT_METHOD_NULL
};

static const asProperty_t gametypedescr_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(uint, spawnableItemsMask), FOFFSET(gametype_descriptor_t, spawnableItemsMask) },
/*	{ SCRIPT_PROPERTY_DECL(uint, respawnableItemsMask), FOFFSET(gametype_descriptor_t, respawnableItemsMask) },
	{ SCRIPT_PROPERTY_DECL(uint, dropableItemsMask), FOFFSET(gametype_descriptor_t, dropableItemsMask) },
	{ SCRIPT_PROPERTY_DECL(uint, pickableItemsMask), FOFFSET(gametype_descriptor_t, pickableItemsMask) },
	{ SCRIPT_PROPERTY_DECL(bool, isTeamBased), FOFFSET(gametype_descriptor_t, isTeamBased) },
	{ SCRIPT_PROPERTY_DECL(bool, isRace), FOFFSET(gametype_descriptor_t, isRace) },
	{ SCRIPT_PROPERTY_DECL(bool, hasChallengersQueue), FOFFSET(gametype_descriptor_t, hasChallengersQueue) },
	{ SCRIPT_PROPERTY_DECL(int, maxPlayersPerTeam), FOFFSET(gametype_descriptor_t, maxPlayersPerTeam) },
	{ SCRIPT_PROPERTY_DECL(int, ammoRespawn), FOFFSET(gametype_descriptor_t, ammo_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, armorRespawn), FOFFSET(gametype_descriptor_t, armor_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, weaponRespawn), FOFFSET(gametype_descriptor_t, weapon_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, healthRespawn), FOFFSET(gametype_descriptor_t, health_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, powerupRespawn), FOFFSET(gametype_descriptor_t, powerup_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, megahealthRespawn), FOFFSET(gametype_descriptor_t, megahealth_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, ultrahealthRespawn), FOFFSET(gametype_descriptor_t, ultrahealth_respawn) },
	{ SCRIPT_PROPERTY_DECL(bool, readyAnnouncementEnabled), FOFFSET(gametype_descriptor_t, readyAnnouncementEnabled) },
	{ SCRIPT_PROPERTY_DECL(bool, scoreAnnouncementEnabled), FOFFSET(gametype_descriptor_t, scoreAnnouncementEnabled) },
	{ SCRIPT_PROPERTY_DECL(bool, countdownEnabled), FOFFSET(gametype_descriptor_t, countdownEnabled) },
	{ SCRIPT_PROPERTY_DECL(bool, mathAbortDisabled), FOFFSET(gametype_descriptor_t, mathAbortDisabled) },
	{ SCRIPT_PROPERTY_DECL(bool, shootingDisabled), FOFFSET(gametype_descriptor_t, shootingDisabled) },
	{ SCRIPT_PROPERTY_DECL(bool, infiniteAmmo), FOFFSET(gametype_descriptor_t, infiniteAmmo) },
	{ SCRIPT_PROPERTY_DECL(bool, canForceModels), FOFFSET(gametype_descriptor_t, canForceModels) },
	{ SCRIPT_PROPERTY_DECL(int, spawnpointRadius), FOFFSET(gametype_descriptor_t, spawnpoint_radius) },
*/
	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asGametypeClassDescriptor =
{
	"cGametypeDesc",				/* name */
	(asEObjTypeFlags) (asOBJ_REF|asOBJ_NOHANDLE),		/* object type flags */
	sizeof( gametype_descriptor_t ), /* size */
	gametypedescr_ObjectBehaviors, /* object behaviors */
	NULL,						/* global behaviors */
	gametypedescr_Methods,		/* methods */
	gametypedescr_Properties,	/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cGame

static const asBehavior_t game_ObjectBehaviors[] =
{
	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t game_Methods[] =
{
	SCRIPT_METHOD_NULL
};
/*
{ "const uint levelTime", &level.time },
	{ "const uint frameTime", &game.frametime }, done
	{ "const uint realTime", &game.realtime },
	{ "const uint64 localTime", &game.localTime },
	{ "const int maxEntities", &game.maxentities },
	{ "const int numEntities", &game.numentities },
	{ "const int maxClients", &gs.maxclients },
	{ "cGametypeDesc gametype", &level.gametype },
	{ "cMatch match", &level.gametype.match },
*/
static const asProperty_t game_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const uint, frameMsecs), FOFFSET(game_locals_t, framemsecs) },
	{ SCRIPT_PROPERTY_DECL(const uint, numEntities), FOFFSET(game_locals_t, numentities) },
	{ SCRIPT_PROPERTY_DECL(const uint, snapNum), FOFFSET(game_locals_t, snapNum) },
	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asGameClassDescriptor =
{
	"cGame",						/* name */
	(asEObjTypeFlags)(asOBJ_REF|asOBJ_NOHANDLE),		/* object type flags */
	sizeof( game_locals_t ),	/* size */
	game_ObjectBehaviors,		/* object behaviors */
	NULL,						/* global behaviors */
	game_Methods,				/* methods */
	game_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

static const asClassDescriptor_t * const asClassesDescriptors[] = 
{
	&asStringClassDescriptor,
	&asVectorClassDescriptor,
	&asTimeClassDescriptor,
	&asTraceClassDescriptor,
	&asCVarClassDescriptor,
	&asGametypeClassDescriptor,
	&asGameClassDescriptor,
	&asGameEntityClassDescriptor,

	NULL
};

/*
* G_RegisterObjectClasses
*/
static void G_RegisterObjectClasses( int asEngineHandle )
{
	int i, j;
	int error;
	const asClassDescriptor_t *cDescr;

	// first register all class names so methods using custom classes work
	for( i = 0; ; i++ )
	{
		if( !(cDescr = asClassesDescriptors[i]) )
			break;

		error = angelExport->asRegisterObjectType( asEngineHandle, cDescr->name, cDescr->size, cDescr->typeFlags );

		if( cDescr->stringFactory )
		{
			char decl[64];

			Q_snprintfz( decl, sizeof( decl ), "%s @", cDescr->name );

			if( level.gametype.asEngineIsGeneric )
				error = angelExport->asRegisterStringFactory( asEngineHandle, decl, cDescr->stringFactory_asGeneric, asCALL_GENERIC );
			else
				error = angelExport->asRegisterStringFactory( asEngineHandle, decl, cDescr->stringFactory, asCALL_CDECL );
			
			if( error < 0 )
				GS_Error( "angelExport->asRegisterStringFactory for object %s returned error %i\n", cDescr->name, error );
		}
	}

	// now register object and global behaviors, then methods and properties
	for( i = 0; ; i++ )
	{
		if( !(cDescr = asClassesDescriptors[i]) )
			break;

		// object behaviors
		if( cDescr->objBehaviors )
		{
			for( j = 0; ; j++ )
			{
				const asBehavior_t *objBehavior = &cDescr->objBehaviors[j];
				if( !objBehavior->declaration )
					break;

				if( level.gametype.asEngineIsGeneric )
					error = angelExport->asRegisterObjectBehaviour( asEngineHandle, cDescr->name, objBehavior->behavior, objBehavior->declaration, objBehavior->funcPointer_asGeneric, asCALL_GENERIC );
				else
					error = angelExport->asRegisterObjectBehaviour( asEngineHandle, cDescr->name, objBehavior->behavior, objBehavior->declaration, objBehavior->funcPointer, objBehavior->callConv );
			}
		}

		// global behaviors
		if( cDescr->globalBehaviors )
		{
			for( j = 0; ; j++ )
			{
				const asBehavior_t *globalBehavior = &cDescr->globalBehaviors[j];
				if( !globalBehavior->declaration )
					break;

				if( level.gametype.asEngineIsGeneric )
					error = angelExport->asRegisterGlobalBehaviour( asEngineHandle, globalBehavior->behavior, globalBehavior->declaration, globalBehavior->funcPointer_asGeneric, asCALL_GENERIC );
				else
					error = angelExport->asRegisterGlobalBehaviour( asEngineHandle, globalBehavior->behavior, globalBehavior->declaration, globalBehavior->funcPointer, globalBehavior->callConv );
			}
		}

		// object methods
		if( cDescr->objMethods )
		{
			for( j = 0; ; j++ )
			{
				const asMethod_t *objMethod = &cDescr->objMethods[j];
				if( !objMethod->declaration )
					break;

				if( level.gametype.asEngineIsGeneric )
					error = angelExport->asRegisterObjectMethod( asEngineHandle, cDescr->name, objMethod->declaration, objMethod->funcPointer_asGeneric, asCALL_GENERIC );
				else
					error = angelExport->asRegisterObjectMethod( asEngineHandle, cDescr->name, objMethod->declaration, objMethod->funcPointer, objMethod->callConv );
			}
		}

		// object properties
		if( cDescr->objProperties )
		{
			for( j = 0; ; j++ )
			{
				const asProperty_t *objProperty = &cDescr->objProperties[j];
				if( !objProperty->declaration )
					break;

				error = angelExport->asRegisterObjectProperty( asEngineHandle, cDescr->name, objProperty->declaration, objProperty->offset );
			}
		}
	}
}

//=======================================================================

static void asFunc_Print( const asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	GS_Printf( "%s", str->buffer );
}

static void asFunc_asGeneric_Print( void *gen )
{
	asFunc_Print( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) );
}

typedef struct
{
	char *declaration;
	void *pointer;
	void *pointer_asGeneric;
}asglobfuncs_t;

static asglobfuncs_t asGlobFuncs[] =
{
	{ "void G_Print( cString &in )", asFunc_Print, asFunc_asGeneric_Print },

	{ NULL, NULL }
};

typedef struct
{
	char *declaration;
	void *pointer;
}asglobproperties_t;

static asglobproperties_t asGlobProps[] =
{
	{ NULL, NULL }
};

static void G_asRegisterGlobalFunctions( int asEngineHandle )
{
	int error;
	int count = 0, failedcount = 0;
	asglobfuncs_t *func;

	for( func = asGlobFuncs; func->declaration; func++ )
	{
		if( level.gametype.asEngineIsGeneric )
			error = angelExport->asRegisterGlobalFunction( asEngineHandle, func->declaration, func->pointer_asGeneric, asCALL_GENERIC );
		else
			error = angelExport->asRegisterGlobalFunction( asEngineHandle, func->declaration, func->pointer, asCALL_CDECL );

		if( error < 0 )
		{
			failedcount++;
			continue;
		}

		count++;
	}
}

static void G_asRegisterGlobalProperties( int asEngineHandle )
{
	int error;
	int count = 0, failedcount = 0;
	asglobproperties_t *prop;

	for( prop = asGlobProps; prop->declaration; prop++ )
	{
		error = angelExport->asRegisterGlobalProperty( asEngineHandle, prop->declaration, prop->pointer );
		if( error < 0 )
		{
			failedcount++;
			continue;
		}

		count++;
	}
}

void G_InitializeGameModuleSyntax( int asEngineHandle )
{
	GS_Printf( "* Initializing Game module syntax\n" );

	// register global variables
	G_asRegisterEnums( asEngineHandle );

	// register classes
	G_RegisterObjectClasses( asEngineHandle );

	// register global functions
	G_asRegisterGlobalFunctions( asEngineHandle );

	// register global properties
	G_asRegisterGlobalProperties( asEngineHandle );
}


qboolean G_asExecutionErrorReport( int asEngineHandle, int asContextHandle, int error )
{
	int funcID;

	if( error == asEXECUTION_FINISHED )
		return qfalse;

	// The execution didn't finish as we had planned. Determine why.
	if( error == asEXECUTION_ABORTED )
		GS_Printf( "* The script was aborted before it could finish. Probably it timed out.\n" );

	else if( error == asEXECUTION_EXCEPTION )
	{
		GS_Printf( "* The script ended with an exception.\n" );

		// Write some information about the script exception
		funcID = angelExport->asGetExceptionFunction( asContextHandle );
		GS_Printf( "* func: %s\n", angelExport->asGetFunctionDeclaration( asEngineHandle, SCRIPT_MODULE_NAME, funcID ) );
		GS_Printf( "* modl: %s\n", SCRIPT_MODULE_NAME );
		GS_Printf( "* sect: %s\n", angelExport->asGetFunctionSection( asEngineHandle, SCRIPT_MODULE_NAME, funcID ) );
		GS_Printf( "* line: %i\n", angelExport->asGetExceptionLineNumber( asContextHandle ) );
		GS_Printf( "* desc: %s\n", angelExport->asGetExceptionString( asContextHandle ) );
	}
	else
		GS_Printf( "* The script ended for some unforeseen reason ( %i ).\n", error );

	return qtrue;
}

static void G_ResetGametypeScriptData( void )
{
	level.gametype.asEngineHandle = -1;
	level.gametype.asEngineIsGeneric = qfalse;
	level.gametype.initFuncID = -1;
	level.gametype.spawnFuncID = -1;
	level.gametype.matchStateStartedFuncID = -1;
	level.gametype.matchStateFinishedFuncID = -1;
	level.gametype.thinkRulesFuncID = -1;
	level.gametype.playerRespawnFuncID = -1;
	level.gametype.scoreEventFuncID = -1;
	level.gametype.scoreboardMessageFuncID = -1;
	level.gametype.selectSpawnPointFuncID = -1;
	level.gametype.clientCommandFuncID = -1;
	level.gametype.botStatusFuncID = -1;
}

void G_asShutdownGametypeScript( void )
{
	if( level.gametype.asEngineHandle > -1 )
	{
		if( angelExport )
			angelExport->asReleaseScriptEngine( level.gametype.asEngineHandle );
	}

	G_ResetGametypeScriptData();
}
/*
//"void GT_SpawnGametype()"
void G_asCallLevelSpawnScript( void )
{
	int error, asContextHandle;

	if( level.gametype.spawnFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.spawnFuncID );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"void GT_MatchStateStarted()"
void G_asCallMatchStateStartedScript( void )
{
	int error, asContextHandle;

	if( level.gametype.matchStateStartedFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateStartedFuncID );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"bool GT_MatchStateFinished( int incomingMatchState )"
qboolean G_asCallMatchStateFinishedScript( int incomingMatchState )
{
	int error, asContextHandle;
	qboolean result;

	if( level.gametype.matchStateFinishedFuncID < 0 )
		return qtrue;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateFinishedFuncID );
	if( error < 0 ) 
		return qtrue;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgDWord( asContextHandle, 0, incomingMatchState );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	// Retrieve the return from the context
	result = G_asGetReturnBool( asContextHandle );

	return result;
}
*/
//"void GT_Frame( void )"
void G_asCallRunFrameScript( void )
{
	int error, asContextHandle;

	if( level.gametype.thinkRulesFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.thinkRulesFuncID );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}
/*
//"void GT_playerRespawn( cEntity @ent, int old_team, int new_team )"
void G_asCallPlayerRespawnScript( edict_t *ent, int old_team, int new_team )
{
	int error, asContextHandle;

	if( level.gametype.playerRespawnFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.playerRespawnFuncID );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgDWord( asContextHandle, 1, old_team );
	angelExport->asSetArgDWord( asContextHandle, 2, new_team );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"void GT_scoreEvent( cClient @client, cString &score_event, cString &args )"
void G_asCallScoreEventScript( gclient_t *client, const char *score_event, const char *args )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( level.gametype.scoreEventFuncID < 0 )
		return;

	if( !score_event || !score_event[0] )
		return;

	if( !args )
		args = "";

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreEventFuncID );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	s1 = objectString_FactoryBuffer( score_event, strlen( score_event ) );
	s2 = objectString_FactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	objectString_Release( s1 );
	objectString_Release( s2 );
}

//"cString @GT_ScoreboardMessage( int maxlen )"
char *G_asCallScoreboardMessage( int maxlen )
{
	asstring_t *string;
	int error, asContextHandle;

	scoreboardString[0] = 0;

	if( level.gametype.scoreboardMessageFuncID < 0 )
		return NULL;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreboardMessageFuncID );
	if( error < 0 ) 
		return NULL;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgDWord( asContextHandle, 0, maxlen );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	string = (asstring_t *)angelExport->asGetReturnObject( asContextHandle );
	if( !string || !string->len || !string->buffer )
		return NULL;

	Q_strncpyz( scoreboardString, string->buffer, sizeof( scoreboardString ) );

	return scoreboardString;
}

//"cEntity @GT_SelectSpawnPoint( cEntity @ent )"
edict_t *G_asCallSelectSpawnPointScript( edict_t *ent )
{
	int error, asContextHandle;
	edict_t *spot;

	if( level.gametype.selectSpawnPointFuncID < 0 )
		return SelectDeathmatchSpawnPoint( ent ); // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.selectSpawnPointFuncID );
	if( error < 0 ) 
		return SelectDeathmatchSpawnPoint( ent );

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	spot = (edict_t *)angelExport->asGetReturnObject( asContextHandle );
	if( !spot )
		spot = SelectDeathmatchSpawnPoint( ent );

	return spot;
}

//"bool GT_Command( cClient @client, cString &cmdString, cString &argsString, int argc )"
qboolean G_asCallGameCommandScript( gclient_t *client, char *cmd, char *args, int argc )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( level.gametype.clientCommandFuncID < 0 )
		return qfalse; // should have a hardcoded backup

	// check for having any command to parse
	if( !cmd || !cmd[0] )
		return qfalse;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.clientCommandFuncID );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	s1 = objectString_FactoryBuffer( cmd, strlen( cmd ) );
	s2 = objectString_FactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );
	angelExport->asSetArgDWord( asContextHandle, 3, argc );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	objectString_Release( s1 );
	objectString_Release( s2 );

	// Retrieve the return from the context
	return G_asGetReturnBool( asContextHandle );
}

//"bool GT_UpdateBotStatus( cEntity @ent )"
qboolean G_asCallBotStatusScript( edict_t *ent )
{
	int error, asContextHandle;

	if( level.gametype.botStatusFuncID < 0 )
		return qfalse; // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.botStatusFuncID );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	// Retrieve the return from the context
	return G_asGetReturnBool( asContextHandle );
}

void G_asGetEntityEventScriptFunctions( const char *classname, gentity_t *ent )
{
	char fdeclstr[MAX_STRING_CHARS];

	if( !classname )
		return;

	ent->think = NULL;
	ent->touch = NULL;
	ent->use = NULL;
	ent->pain = NULL;
	ent->die = NULL;
	ent->stop = NULL;

	// _think
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_think( cEntity @ent )", classname );
	ent->asThinkFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _touch
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )", classname );
	ent->asTouchFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _use
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_use( cEntity @ent, cEntity @other, cEntity @activator )", classname );
	ent->asUseFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _pain
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_pain( cEntity @ent, cEntity @other, float kick, float damage )", classname );
	ent->asPainFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _die
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_die( cEntity @ent, cEntity @inflicter, cEntity @attacker )", classname );
	ent->asDieFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _stop
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_stop( cEntity @ent )", classname );
	ent->asStopFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
}

// map entity spawning
qboolean G_asCallMapEntitySpawnScript( const char *classname, edict_t *ent )
{
	char fdeclstr[MAX_STRING_CHARS];
	int error, asContextHandle;

	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s( cEntity @ent )", classname );

	ent->asSpawnFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( ent->asSpawnFuncID < 0 )
		return qfalse;

	// call the spawn function
	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );
	error = angelExport->asPrepare( asContextHandle, ent->asSpawnFuncID );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
	{
		G_asShutdownGametypeScript();
		ent->asSpawnFuncID = -1;
		return qfalse;
	}

	// the spawn function may remove the entity
	if( ent->r.inuse )
	{
		ent->scriptSpawned = qtrue;

		// if we found a spawn function, try also to find a _think, _use and _touch functions
		// and keep their ids so they don't have to be re-found each execution
		G_asGetEntityEventScriptFunctions( classname, ent );
	}

	return qtrue;
}

//"void %s_think( cEntity @ent )"
void G_asCallMapEntityThink( edict_t *ent )
{
	int error, asContextHandle;

	if( ent->asThinkFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asThinkFuncID );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )"
void G_asCallMapEntityTouch( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	int error, asContextHandle;
	asvec3_t asv, *normal = NULL;
	
	if( ent->asTouchFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asTouchFuncID );
	if( error < 0 ) 
		return;

	if( plane )
	{
		normal = &asv;
		VectorCopy( plane->normal, normal->v );
		normal->asFactored = qfalse;
	}

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, other );
	angelExport->asSetArgObject( asContextHandle, 2, normal );
	angelExport->asSetArgDWord( asContextHandle, 3, surfFlags );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_use( cEntity @ent, cEntity @other, cEntity @activator )"
void G_asCallMapEntityUse( edict_t *ent, edict_t *other, edict_t *activator )
{
	int error, asContextHandle;

	if( ent->asUseFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asUseFuncID );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, other );
	angelExport->asSetArgObject( asContextHandle, 2, activator );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_pain( cEntity @ent, cEntity @other, float kick, float damage )"
void G_asCallMapEntityPain( edict_t *ent, edict_t *other, float kick, float damage )
{
	int error, asContextHandle;

	if( ent->asPainFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asPainFuncID );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, other );
	angelExport->asSetArgFloat( asContextHandle, 2, kick );
	angelExport->asSetArgFloat( asContextHandle, 3, damage );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_die( cEntity @ent, cEntity @inflicter, cEntity @attacker )"
void G_asCallMapEntityDie( edict_t *ent, edict_t *inflicter, edict_t *attacker )
{
	int error, asContextHandle;

	if( ent->asDieFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asDieFuncID );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, inflicter );
	angelExport->asSetArgObject( asContextHandle, 2, attacker );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"void %s_stop( cEntity @ent )"
void G_asCallMapEntityStop( edict_t *ent )
{
	int error, asContextHandle;

	if( ent->asStopFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asStopFuncID );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}
*/
char *G_LoadScriptSection( const char *script, int sectionNum )
{
	char filename[MAX_QPATH];
	qbyte *data;
	int length, filenum;
	char *sectionName;

	sectionName = G_ListNameForPosition( script, sectionNum, GAMETYPE_CHAR_SEPARATOR );
	if( !sectionName )
		return NULL;

	COM_StripExtension( sectionName );

	while( *sectionName == '\n' || *sectionName == ' ' || *sectionName == '\r' )
		sectionName++;
	
	Q_snprintfz( filename, sizeof( filename ), "progs/gametypes/%s%s", sectionName, GAMETYPE_SCRIPT_EXTENSION );
	Q_strlwr( filename );

	length = trap_FS_FOpenFile( filename, &filenum, FS_READ );

	if( length == -1 )
	{
		GS_Printf( "Couldn't find script section: '%s'\n", filename );
		return NULL;
	}

	//load the script data into memory
	data = G_Malloc( length + 1 );
	trap_FS_Read( data, length, filenum );
	trap_FS_FCloseFile( filenum );

	GS_Printf( "* Loaded script section '%s'\n", filename );
	return (char *)data;
}

qboolean G_asInitializeGametypeScript( const char *script, const char *gametypeName )
{
	int asEngineHandle, asContextHandle, error;
	int numSections, sectionNum;
	int funcCount;
	char *section;
	const char *fdeclstr;

	angelExport = trap_asGetAngelExport();
	if( !angelExport )
	{
		GS_Printf( "G_asInitializeGametypeScript: Angelscript API unavailable\n" );
		return qfalse;
	}

	GS_Printf( "* Initializing gametype scripts\n" );

	// count referenced script sections
	for( numSections = 0; ( section = G_ListNameForPosition( script, numSections, GAMETYPE_CHAR_SEPARATOR ) ) != NULL; numSections++ );

	if( !numSections )
	{
		GS_Printf( "* Invalid gametype script: The gametype has no valid script sections included.\n" );
		goto releaseAll;
	}

	// initialize the engine
	asEngineHandle = angelExport->asCreateScriptEngine( &level.gametype.asEngineIsGeneric );
	if( asEngineHandle < 0 )
	{
		GS_Printf( "* Couldn't initialize angelscript.\n" );
		goto releaseAll;
	}

	G_InitializeGameModuleSyntax( asEngineHandle );

	// load up the script sections
	
	for( sectionNum = 0; ( section = G_LoadScriptSection( script, sectionNum ) ) != NULL; sectionNum++ )
	{
		char *sectionName = G_ListNameForPosition( script, sectionNum, GAMETYPE_CHAR_SEPARATOR );
		error = angelExport->asAddScriptSection( asEngineHandle, SCRIPT_MODULE_NAME, sectionName, section, strlen( section ) );
		
		G_Free( section );
		
		if( error )
		{
			GS_Printf( "* Failed to add the script section %s with error %i\n", gametypeName, error );
			goto releaseAll;
		}
	}

	if( sectionNum != numSections )
	{
		GS_Printf( "* Couldn't load all script sections. Can't continue.\n" );
		goto releaseAll;
	}

	error = angelExport->asBuildModule( asEngineHandle, SCRIPT_MODULE_NAME );
	if( error )
	{
		GS_Printf( "* Failed to build the script %s\n", gametypeName );
		goto releaseAll;
	}

	// grab script function calls
	funcCount = 0;
	fdeclstr = "void GT_InitGametype()";
	level.gametype.initFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.initFuncID < 0 )
	{
		GS_Printf( "* The function '%s' was not found. Can not continue.\n", fdeclstr );
		goto releaseAll;
	}
	else
		funcCount++;
/*
	fdeclstr = "void GT_SpawnGametype()";
	level.gametype.spawnFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.spawnFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_MatchStateStarted()";
	level.gametype.matchStateStartedFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.matchStateStartedFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_MatchStateFinished( int incomingMatchState )";
	level.gametype.matchStateFinishedFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.matchStateFinishedFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;
*/
	fdeclstr = "void GT_Frame()";
	level.gametype.thinkRulesFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.thinkRulesFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;
/*
	fdeclstr = "void GT_playerRespawn( cEntity @ent, int old_team, int new_team )";
	level.gametype.playerRespawnFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.playerRespawnFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_scoreEvent( cClient @client, cString &score_event, cString &args )";
	level.gametype.scoreEventFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.scoreEventFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "cString @GT_ScoreboardMessage( int maxlen )";
	level.gametype.scoreboardMessageFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.scoreboardMessageFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "cEntity @GT_SelectSpawnPoint( cEntity @ent )";
	level.gametype.selectSpawnPointFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.selectSpawnPointFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_Command( cClient @client, cString &cmdString, cString &argsString, int argc )";
	level.gametype.clientCommandFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.clientCommandFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_UpdateBotStatus( cEntity @ent )";
	level.gametype.botStatusFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.botStatusFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			GS_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;
*/
	//
	// execute the GT_InitGametype function
	//

	level.gametype.asEngineHandle = asEngineHandle;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.initFuncID );
	if( error < 0 ) 
		goto releaseAll;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		goto releaseAll;

	return qtrue;

releaseAll:
	G_asShutdownGametypeScript();
	return qfalse;
}

qboolean G_asLoadGametypeScript( const char *gametypeName )
{
	int length, filenum;
	qbyte *data;
	char filename[MAX_QPATH];

	G_ResetGametypeScriptData();

	Q_snprintfz( filename, sizeof( filename ), "progs/gametypes/%s%s", gametypeName, GAMETYPE_PROJECT_EXTENSION );
	Q_strlwr( filename );

	length = trap_FS_FOpenFile( filename, &filenum, FS_READ );

	if( length == -1 )
	{
		GS_Printf( "Failed to initialize: Couldn't find '%s'.\n", filename );
		return qfalse;
	}

	if( !length )
	{
		GS_Printf( "Failed to initialize: Gametype '%s' is empty.\n", filename );
		trap_FS_FCloseFile( filenum );
		return qfalse;
	}

	//load the script data into memory
	data = G_Malloc( length + 1 );
	trap_FS_Read( data, length, filenum );
	trap_FS_FCloseFile( filenum );

	// Initialize the script
	if( !G_asInitializeGametypeScript( (const char *)data, gametypeName ) )
	{
		GS_Printf( "Failed to initialize gametype: '%s'.\n", filename );
		G_Free( data );
		return qfalse;
	}

	G_Free( data );
	return qtrue;
}

/*
* G_asGarbageCollect
*
* Perform garbage collection procedure
*/
void G_asGarbageCollect( void )
{
	unsigned int currentSize, totalDestroyed, totalDetected;

	angelExport->asGetGCStatistics( level.gametype.asEngineHandle, &currentSize, &totalDestroyed, &totalDetected );

	angelExport->asGarbageCollect( level.gametype.asEngineHandle );
}
