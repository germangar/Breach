
#include "g_local.h"

//==================================================
// ITEMS
//==================================================

#define MAX_ITEM_COUNT		256

#define ITEM_DROP_TOUCH_DELAY	1000

#define SPAWNFLAG_ITEM_FLOAT	1

/*
* G_Item_DropToFloor
* fixme?: would it be better to have a generic drop-to-floor function?
*/
static void G_Item_DropToFloor( gentity_t *ent )
{
	trace_t	trace;
	vec3_t dest;
	if( !( ent->spawnflags & SPAWNFLAG_ITEM_FLOAT ) )
	{
		// see if they start in a solid position
		GS_Trace( &trace, ent->s.ms.origin, ent->s.local.mins, ent->s.local.maxs, ent->s.ms.origin, ENTNUM( ent ), MASK_SOLID, 0 );
		if( trace.startsolid )
		{
			// move it 1 unit up, cause it's typical they share the leaf with the floor
			VectorSet( dest, ent->s.ms.origin[0], ent->s.ms.origin[1], ent->s.ms.origin[2] + 1 );
			GS_Trace( &trace, dest, ent->s.local.mins, ent->s.local.maxs, ent->s.ms.origin, ENTNUM( ent ), MASK_SOLID, 0 );
			if( trace.startsolid )
			{
				GS_Printf( "Warning: %s %s spawns inside solid. Inhibited\n", ent->classname, vtos( ent->s.ms.origin ) );
				G_FreeEntity( ent );
				return;
			}
			VectorCopy( dest, ent->s.ms.origin );
		}

		VectorSet( dest, ent->s.ms.origin[0], ent->s.ms.origin[1], ent->s.ms.origin[2] - 128 );
		GS_Trace( &trace, ent->s.ms.origin, ent->s.local.mins, ent->s.local.maxs, dest, ENTNUM( ent ), MASK_SOLID, 0 );
		if( trace.startsolid )
		{
			GS_Printf( "Warning: %s %s spawns inside solid. Inhibited\n", ent->classname, vtos( ent->s.ms.origin ) );
			G_FreeEntity( ent );
			return;
		}

		VectorCopy( trace.endpos, ent->s.ms.origin );
	}
}

/*
* G_Item_Use
*/
void G_Item_Use( gentity_t *ent, gsitem_t *item )
{
	int count;

	if( !item )
		return;

	if( item->type & IT_WEAPON )
	{
		ent->client->ps.stats[STAT_PENDING_WEAPON] = item->tag;
	}
	else if( item->type & IT_AMMO )
	{
		// if the current weapon is the selected weapon, reload, otherwise, can't use now
	}
	else if( item->type & IT_HEALTH )
	{
		// we have some health in the inventory and decided to apply it
		count = ent->client->ps.inventory[item->inventorySlot][ICOUNT];
		if( ent->client->ps.inventory[item->inventorySlot][ITAG] == item->tag )
		{
			if( ent->health + count > ent->maxHealth )
				count = ent->maxHealth - ent->health;
			ent->health += count;
			ent->client->ps.inventory[item->inventorySlot][ICOUNT] -= count;
			if( ent->client->ps.inventory[item->inventorySlot][ICOUNT] == 0 )
				ent->client->ps.inventory[item->inventorySlot][ITAG] = 0; // completely wasted
		}
	}
}

/*
* G_Item_AddWeaponToInventory
*/
qboolean G_Item_AddWeaponToInventory( gentity_t *ent, gsitem_t *item, int ammocount )
{
	int newcount;
	player_state_t *playerState;

	if( !item || !ent->client )
		return qfalse;

	if( !( item->flags & ITFLAG_PICKABLE ) )
		return qfalse;

	if( !G_Gametype_CanPickUpItem( item ) )
		return qfalse;

	if( item->inventorySlot == INV_INSTANT_USE )
		return qfalse;

	playerState = &ent->client->ps;

	// drop current one if any
	if( playerState->inventory[item->inventorySlot][ITAG]
	    && playerState->inventory[item->inventorySlot][ITAG] != item->tag )
	{
		// if it's the weapon being held, replace it
		if( playerState->stats[STAT_PENDING_WEAPON] == playerState->inventory[item->inventorySlot][ITAG] )
		{
			playerState->stats[STAT_PENDING_WEAPON] = item->tag;
		}

		G_Item_Drop( ent, GS_FindItemByIndex( playerState->inventory[item->inventorySlot][ITAG] ), playerState->inventory[item->inventorySlot][ICOUNT] );
		ent->client->ps.inventory[item->inventorySlot][ITAG] = 0;
		ent->client->ps.inventory[item->inventorySlot][ICOUNT] = 0;
	}

	// pick up
	playerState->inventory[item->inventorySlot][ITAG] = item->tag;

	newcount = playerState->inventory[item->inventorySlot][ICOUNT] + ammocount;
	clamp( newcount, 0, MAX_ITEM_COUNT );
	playerState->inventory[item->inventorySlot][ICOUNT] = newcount;

	return qtrue;
}

/*
* G_Item_AddAmmoToInventory
*/
qboolean G_Item_AddAmmoToInventory( gentity_t *ent, gsitem_t *item, int count )
{
	int newcount;

	if( !item || !ent->client )
		return qfalse;

	if( !( item->flags & ITFLAG_PICKABLE ) )
		return qfalse;

	if( !G_Gametype_CanPickUpItem( item ) )
		return qfalse;

	if( item->inventorySlot == INV_INSTANT_USE )
		return qfalse;

	if( ent->client->ps.inventory[item->inventorySlot][ITAG] != item->tag )
		ent->client->ps.inventory[item->inventorySlot][ICOUNT] = 0;

	ent->client->ps.inventory[item->inventorySlot][ITAG] = item->tag;

	newcount = ent->client->ps.inventory[item->inventorySlot][ICOUNT] + count;
	clamp( newcount, 0, MAX_ITEM_COUNT );
	ent->client->ps.inventory[item->inventorySlot][ICOUNT] = newcount;

	return qtrue;
}

/*
* G_Item_AddHealthToInventory
*/
qboolean G_Item_AddHealthToInventory( gentity_t *ent, gsitem_t *item, int count )
{
	int newcount;

	if( !item || !ent->client )
		return qfalse;

	if( !( item->flags & ITFLAG_PICKABLE ) )
		return qfalse;

	if( !G_Gametype_CanPickUpItem( item ) )
		return qfalse;

	if( item->inventorySlot == INV_INSTANT_USE )
		return qfalse;

	if( ent->client->ps.inventory[item->inventorySlot][ITAG] != item->tag )
		ent->client->ps.inventory[item->inventorySlot][ICOUNT] = 0;

	ent->client->ps.inventory[item->inventorySlot][ITAG] = item->tag;

	newcount = ent->client->ps.inventory[item->inventorySlot][ICOUNT] + count;
	clamp( newcount, 0, MAX_ITEM_COUNT );
	ent->client->ps.inventory[item->inventorySlot][ICOUNT] = newcount;

	return qtrue;
}

/*
* G_Item_AddToInventory
*/
qboolean G_Item_AddToInventory( gentity_t *ent, gsitem_t *item, int count )
{
	if( item->type & IT_WEAPON )
	{
		return G_Item_AddWeaponToInventory( ent, item, count );
	}
	else if( item->type & IT_AMMO )
	{
		return G_Item_AddAmmoToInventory( ent, item, count );
	}
	else if( item->type & IT_HEALTH )
	{
		return G_Item_AddHealthToInventory( ent, item, count );
	}

	return qfalse;
}

/*
* G_Item_PickInstantItem
*/
qboolean G_Item_PickInstantItem( gentity_t *ent, gsitem_t *item, int count )
{
	if( !item || !ent->client )
		return qfalse;

	if( !( item->flags & ITFLAG_PICKABLE ) )
		return qfalse;

	if( !G_Gametype_CanPickUpItem( item ) )
		return qfalse;

	if( item->type & IT_HEALTH )
	{
		ent->health += count;
		if( ent->maxHealth && ( ent->health > ent->maxHealth ) )
			ent->health = ent->maxHealth;
		return qtrue;
	}

	return qfalse;
}

/*
* G_Item_EntityPickUp
*/
void G_Item_EntityPickUp( gentity_t *ent, gentity_t *other )
{
	qboolean taken;
	gsitem_t *item;

	if( ent->s.type != ET_ITEM )
		return;

	if( GS_IsGhostState( &other->s ) )
		return;

	if( ent->owner && ( ent->owner == other ) )
	{                                       // item was dropped by someone
		if( level.time < ent->timestamp + ITEM_DROP_TOUCH_DELAY )
		{
			return;
		}
	}

	item = GS_FindItemByIndex( ent->s.modelindex1 );
	if( !item )
		return;

	// proceed

	taken = qfalse;

	if( item->inventorySlot == INV_INSTANT_USE )
		taken = G_Item_PickInstantItem( other, item, ent->count );
	else
		taken = G_Item_AddToInventory( other, item, ent->count );

	if( taken )
	{
		if( item->type & IT_WEAPON )
			G_AddEvent( other, EV_PICKUP_WEAPON, 0, qtrue );
		else if( item->type & IT_AMMO )
			G_AddEvent( other, EV_PICKUP_AMMO, 0, qtrue );
		else if( item->type & IT_HEALTH )
			G_AddEvent( other, EV_PICKUP_HEALTH, 0, qtrue );

		G_ActivateTargets( ent, other );
		G_FreeEntity( ent ); // items do not respawn
	}
}

/*
* G_Item_Touch
*/
void G_Item_Touch( gentity_t *ent, gentity_t *other, cplane_t *plane, int surfFlags )
{
	gsitem_t *item;

	if( ent->s.type != ET_ITEM )
		return;

	if( ent->s.team && ( ent->s.team != other->s.team ) )
		return;

	item = GS_FindItemByIndex( ent->s.modelindex1 );
	if( !item )
		return;

	// can not touch items if we have a different item
	// in the same inventory spot (but could pick them in other ways)
	if( other->client && ( item->inventorySlot != INV_INSTANT_USE ) )
	{
		player_state_t *playerState = &other->client->ps;
		if( playerState->inventory[item->inventorySlot][ITAG]
		    && playerState->inventory[item->inventorySlot][ITAG] != item->tag )
		{
			return;
		}
	}

	G_Item_EntityPickUp( ent, other );
}

/*
* G_Item_Drop
*/
gentity_t *G_Item_Drop( gentity_t *ent, gsitem_t *item, int count )
{
	gentity_t *drop;
	trace_t	trace;

	drop = G_Spawn();
	drop->classname = item->classname;
	drop->s.team = ent->s.team;

	drop->s.type = ET_ITEM;
	drop->s.solid = SOLID_TRIGGER;
	drop->s.cmodeltype = CMODEL_BBOX;
	VectorCopy( item->mins, drop->s.local.mins );
	VectorCopy( item->maxs, drop->s.local.maxs );

	drop->s.ms.type = MOVE_TYPE_OBJECT;
	drop->netflags &= ~SVF_NOCLIENT;
	drop->s.modelindex1 = item->tag;
	drop->count = count;

	drop->touch = G_Item_Touch;
	drop->owner = ent;

	VectorCopy( ent->s.ms.origin, drop->s.ms.origin );

	// give it some velocity
	VectorCopy( ent->s.ms.velocity, drop->s.ms.velocity );
	{
		vec3_t vel, forward, up, point;
		AngleVectors( ent->s.ms.angles, forward, NULL, up );
		VectorMA( drop->s.ms.origin, 24, forward, point );

		// check it's not inside solid
		GS_Trace( &trace, drop->s.ms.origin, drop->s.local.mins, drop->s.local.maxs, point, ent->s.number, MASK_PLAYERSOLID, 0 );
		VectorCopy( trace.endpos, drop->s.ms.origin );

		VectorScale( forward, 400, vel );
		VectorMA( vel, 200, up, vel );
		VectorAdd( drop->s.ms.velocity, vel, drop->s.ms.velocity );
	}

	GClip_LinkEntity( drop );

	return drop;
}

/*
* G_Item_Spawn
*/
void G_Item_Spawn( gentity_t *ent, gsitem_t *item )
{
	ent->s.type = ET_ITEM;
	ent->s.solid = SOLID_TRIGGER;
	ent->s.cmodeltype = CMODEL_BBOX;
	VectorCopy( item->mins, ent->s.local.mins );
	VectorCopy( item->maxs, ent->s.local.maxs );

	ent->s.ms.type = MOVE_TYPE_OBJECT;
	ent->netflags &= ~SVF_NOCLIENT;
	ent->s.modelindex1 = item->tag;
	ent->count = item->count;

	ent->touch = G_Item_Touch;

	GClip_LinkEntity( ent );

	G_Item_DropToFloor( ent );
}

/*
* G_Item_Precache
*/
void G_Item_Precache( int index )
{
	char string[MAX_STRING_CHARS];
	gsitem_t *item;
	if( index < 0 || index >= MAX_ITEMS )
		return;

	item = GS_FindItemByIndex( index );
	if( item && item->name && item->name[0] && item->model && item->model[0] )
	{
		// index the gameObject
		if( item->type & IT_WEAPON )
		{
			item->objectIndex = G_WeaponObjectIndex( item->model );

			// index the game object
			string[0] = 0;
			Info_SetValueForKey( string, "n", item->name );
			Info_SetValueForKey( string, "t", va( "%i", item->type ) );
			Info_SetValueForKey( string, "o", item->model );
			if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
				GS_Error( "G_Item_Precache: item info is too large for a configstring: %s\n", string );

			trap_ConfigString( CS_ITEMS + index, string );
		}
		else
		{ 
			// does not contain an object, but a simple model
			Q_snprintfz( string, sizeof( string ), "%s%s%s", GS_ItemObjects_BasePath(), item->model + 1, GS_ItemObjects_Extension() );
			if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
				GS_Error( "G_Item_Precache: model path is too large for a configstring: %s\n", string );

			// index the actual model so it's added to the auto download process
			item->objectIndex = trap_ModelIndex( string );

			// index the game object
			string[0] = 0;
			Info_SetValueForKey( string, "n", item->name );
			Info_SetValueForKey( string, "t", va( "%i", item->type ) );
			Info_SetValueForKey( string, "o", item->model );
			if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
				GS_Error( "G_Item_Precache: item info is too large for a configstring: %s\n", string );

			trap_ConfigString( CS_ITEMS + index, string );
		}
	}
	else
	{
		trap_ConfigString( CS_ITEMS + index, "" );
	}
}

/*
* G_PrecacheItems
*/
void G_PrecacheItems( void )
{
	int i;

	for( i = 0; GS_FindItemByIndex( i ) != NULL; i++ )
	{
		G_Item_Precache( i );
	}
}

