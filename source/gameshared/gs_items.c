/*
   Copyright (C) 2007 German Garcia
 */

// gs_items.c	-	game shared items definitions

#include "gs_local.h"

//==================================================
// ITEMS
//==================================================

movespecificts_t defaultObjectMoveSpecs = {
	2000,   // frictionGround
	175,    // frictionAir
	600,    // frictionWater
	0,      // bounceFrac
};

gsitem_t itemdefs[] =
{
	{
		"item_nothing",
		"nothing", "no",
		IT_NONE,
		( itemtype_t )0,
		0,
		0,
		INV_INSTANT_USE,

		"",
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		// locally derived information
		0,
	},

	{
		"weapon_machinegun",
		"Machinegun", "MG",
		WEAP_MACHINEGUN,
		IT_WEAPON,
		ITFLAG_PICKABLE|ITFLAG_DROPABLE,
		0,
		INV_SLOT1,
		"#machinegun",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},

	{
		"weapon_grenadelauncher",
		"Grenade Launcher", "GL",
		WEAP_GRENADELAUNCHER,
		IT_WEAPON,
		ITFLAG_PICKABLE|ITFLAG_DROPABLE,
		0,
		INV_SLOT3,
		"#glauncher",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},

	{
		"weapon_rocketlauncher",
		"Rocket Launcher", "RL",
		WEAP_ROCKETLAUNCHER,
		IT_WEAPON,
		ITFLAG_PICKABLE|ITFLAG_DROPABLE,
		0,
		INV_SLOT2,
		"#rlauncher",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},

	{
		"weapon_lasergun",
		"Lasergun", "LG",
		WEAP_LASERGUN,
		IT_WEAPON,
		ITFLAG_PICKABLE|ITFLAG_DROPABLE,
		0,
		INV_SLOT2,
		"#lasergun",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},

	{
		"weapon_electrobolt",
		"Sniper Rifle", "SR",
		WEAP_SNIPERRIFLE,
		IT_WEAPON,
		ITFLAG_PICKABLE|ITFLAG_DROPABLE,
		0,
		INV_SLOT2,
		"#snipergun",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},

	{
		"weapon_nanoforge",
		"NanoForge", "NF",
		WEAP_NANOFORGE,
		IT_WEAPON,
		ITFLAG_PICKABLE|ITFLAG_DROPABLE,
		0,
		INV_SLOT3,
		"#gunblade",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},

	{
		"ammo_cells",
		"Energy Cells", "cells",
		AMMO_CELLS,
		IT_AMMO,
		ITFLAG_PICKABLE,
		10,
		INV_SLOT8,
		"#ammo/pack/pack",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},

	{
		"health_pack",
		"Health Pack", "health",
		HEALTH_PACK,
		IT_HEALTH,
		ITFLAG_PICKABLE,
		50,
		INV_INSTANT_USE,
		"#ammo/pack/pack",
		{ -16.0f, -16.0f, -16.0f },
		{ 16.0f, 16.0f, 48.0f },
		// locally derived information
		0,
	},
};

//====================================================================

#define GS_NUMITEMDEFS ( sizeof( itemdefs )/sizeof( gsitem_t ) )

/*
* 
*/
gsitem_t *GS_FindItemByIndex( int index )
{
	assert( GS_NUMITEMS == GS_NUMITEMDEFS );
	if( index >= 0 && index < GS_NUMITEMDEFS )
		return &itemdefs[index];

	return NULL;
}

/*
* 
*/
gsitem_t *GS_FindItemByClassname( const char *classname )
{
	gsitem_t *it;
	int i, numItems = GS_NUMITEMDEFS;

	if( !classname )
		return NULL;

	for( it = itemdefs, i = 0; i < numItems && it->classname; it++, i++ )
	{
		if( !Q_stricmp( classname, it->classname ) )
			return it;
	}

	return NULL;
}

/*
* 
*/
gsitem_t *GS_FindItemByName( const char *name )
{
	gsitem_t *it;
	int i, numItems = GS_NUMITEMDEFS;

	if( !name )
		return NULL;

	for( i = 0, it = itemdefs; i < numItems && it->name; it++, i++ )
	{
		if( !Q_stricmp( name, it->name ) || !Q_stricmp( name, it->shortname ) )
			return it;
	}

	return NULL;
}

/*
* 
*/
gsitem_t *GS_Cmd_UseItem( entity_state_t *state, player_state_t *playerState, char *string, int typeMask )
{
	gsitem_t *item, *ammoItem;

	if( GS_IsGhostState( state ) )
		return NULL;

	if( !string || !string[0] )
		return NULL;

	if( Q_isdigit( string ) )
	{
		int slot = atoi( string );
		if( playerState->inventory[slot][ITAG] )
			item = GS_FindItemByIndex( playerState->inventory[slot][ITAG] );
		else
			item = NULL;
	}
	else
		item = GS_FindItemByName( string );

	if( !item )
		return NULL;

	if( !( item->type & typeMask ) )
		return NULL;

	// this item is never available in the inventory
	if( item->inventorySlot == INV_INSTANT_USE )
		return NULL;

	// we don't have this item in the inventory
	if( playerState->inventory[item->inventorySlot][ITAG] != item->tag )
	{
		if( module.type == GS_MODULE_CGAME && !( item->type & IT_WEAPON ) ) 
			GS_Printf( "Item %s is not in inventory\n", item->name );

		return NULL;
	}

	// see if we can use it

	if( item->type & IT_WEAPON )
	{
		if( item->tag == playerState->stats[STAT_PENDING_WEAPON] )  // it's already being loaded
			return NULL;

		// check for need of any kind of ammo/fuel/whatever
		if( GS_GetWeaponDef( item->tag )->ammoItem != AMMO_NONE )
		{
			if( !playerState->inventory[item->inventorySlot][ICOUNT] )
			{                                                // doesn't have ammo in the weapon
				ammoItem = GS_FindItemByIndex( GS_GetWeaponDef( item->tag )->ammoItem );

				// see if we have the correct ammo clip type
				if( playerState->inventory[ammoItem->inventorySlot][ITAG] != ammoItem->tag )
					return NULL;
				// see if the ammo clip has anything inside
				if( !playerState->inventory[ammoItem->inventorySlot][ICOUNT] )
				{
					if( module.type == GS_MODULE_CGAME ) GS_Printf( "No ammo for %s\n", item->name );
					return NULL;
				}
			}
		}
	}
	else if( item->type & IT_AMMO )
	{
		gsitem_t *weaponItem;
		// ammo can be used as long as our current weapon has the same ammo type
		weaponItem = GS_FindItemByIndex( state->weapon );
		if( weaponItem && ( weaponItem->type & IT_WEAPON ) )
		{
			if( GS_GetWeaponDef( weaponItem->tag )->ammoItem != item->tag )  // different type
				return NULL;
		}
	}
	else if( item->type & IT_HEALTH )
	{
		return item;
	}
	else
		return NULL;

	return item;
}

/*
* 
*/
static gsitem_t *GS_Cmd_UseWeaponStep_f( entity_state_t *state, player_state_t *playerState, int step, int predictedWeaponSwitch )
{
	gsitem_t *item;
	int curSlot, newSlot;

	if( GS_IsGhostState( state ) )
		return NULL;

	if( step != -1 && step != 1 )
		step = 1;

	if( predictedWeaponSwitch && predictedWeaponSwitch != playerState->stats[STAT_PENDING_WEAPON] )
		curSlot = GS_FindItemByIndex( predictedWeaponSwitch )->inventorySlot;
	else
		curSlot = GS_FindItemByIndex( playerState->stats[STAT_PENDING_WEAPON] )->inventorySlot;
	clamp( curSlot, 0, INV_MAX_SLOTS - 1 );

	newSlot = curSlot;
	do
	{
		newSlot += step;
		if( newSlot >= INV_MAX_SLOTS )
			newSlot = 0;
		if( newSlot < 0 )
			newSlot = INV_MAX_SLOTS - 1;

		if( ( item = GS_Cmd_UseItem( state, playerState, va( "%i", newSlot ), IT_WEAPON ) ) != NULL )
			return item;
	}
	while( newSlot != curSlot );

	return NULL;
}

/*
* 
*/
gsitem_t *GS_Cmd_NextWeapon_f( entity_state_t *state, player_state_t *playerState, int predictedWeaponSwitch )
{
	return GS_Cmd_UseWeaponStep_f( state, playerState, 1, predictedWeaponSwitch );
}

/*
* 
*/
gsitem_t *GS_Cmd_PrevWeapon_f( entity_state_t *state, player_state_t *playerState, int predictedWeaponSwitch )
{
	return GS_Cmd_UseWeaponStep_f( state, playerState, -1, predictedWeaponSwitch );
}
