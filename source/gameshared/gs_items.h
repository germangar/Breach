/*
   Copyright (C) 2007 German Garcia
 */

//==================================================
//	ITEM OBJECTS - gs_items - shared items definitions
//==================================================

#define ITAG 0
#define ICOUNT 1

// inventory slot types
typedef enum
{
	INV_INSTANT_USE = -1, // these items can not be put in the inventory
	INV_SLOT1 = 0,
	INV_SLOT2,          // weapon mid
	INV_SLOT3,          // weapon high
	INV_SLOT4,
	INV_SLOT5,
	INV_SLOT6,
	INV_SLOT7,
	INV_SLOT8,          // ammo cells
	INV_MAX = INV_MAX_SLOTS
} inv_slot_t;

// max weapons allowed by the net protocol are 128
typedef enum
{
	ITEM_NONE = 0,

	// weapon items
	WEAP_MACHINEGUN,
	WEAP_GRENADELAUNCHER,
	WEAP_ROCKETLAUNCHER,
	WEAP_LASERGUN,
	WEAP_SNIPERRIFLE,
	WEAP_NANOFORGE,

	// ammo items
	AMMO_CELLS, 

	// health items
	HEALTH_PACK

} item_tag_t;

#define WEAP_NONE ITEM_NONE
#define AMMO_NONE ITEM_NONE
#define HEALTH_NONE ITEM_NONE

#define WEAP_TOTAL  ( WEAP_NANOFORGE + 1 )
#define AMMO_TOTAL	( AMMO_CELLS + 1 )
#define HEALTH_TOTAL	( HEALTH_PACK + 1 )

#define GS_NUMITEMS HEALTH_TOTAL

// actions flags
#define	ITFLAG_PICKABLE		1
#define ITFLAG_DROPABLE		2

// types: defined as bit flags so they can be masked
typedef enum
{
	IT_NONE = 0
	, IT_HEALTH = 1
	, IT_AMMO = 2
	, IT_WEAPON = 4
} itemtype_t;

typedef struct gsitem_s
{
	const char *classname;
	const char *name;
	const char *shortname;
	int tag;
	itemtype_t type;
	int flags;              // actions the item does in the game
	int count;
	inv_slot_t inventorySlot;

	char *model;
	vec3_t mins;
	vec3_t maxs;

	// locally derived information
	int objectIndex;
} gsitem_t;

extern movespecificts_t defaultObjectMoveSpecs;

extern gsitem_t *GS_FindItemByIndex( int index );
extern gsitem_t *GS_FindItemByClassname( const char *classname );
extern gsitem_t *GS_FindItemByName( const char *name );
extern gsitem_t *GS_Cmd_UseItem( entity_state_t *state, player_state_t *playerState, char *string, int typeMask );
extern gsitem_t *GS_Cmd_NextWeapon_f( entity_state_t *state, player_state_t *playerState, int predictedWeaponSwitch );
extern gsitem_t *GS_Cmd_PrevWeapon_f( entity_state_t *state, player_state_t *playerState, int predictedWeaponSwitch );
