/*
   Copyright (C) 2007 German Garcia
 */

#include "gs_local.h"

//==================================================
// WEAPONS
//==================================================

//int numModes;
//int smoothRefire;
//int dropTime;
//int activationTime;
//int refireTime;
//int clipTime;
//int noAmmoClickTime;

gs_weapon_definition_t gs_weaponDefs[] =
{
	{
		"no weapon",
		AMMO_NONE,
		1,
		0,
		0,
		0,
		0,
		0,
		0
	},

	{
		"Gunblade",
		AMMO_NONE,
		2,
		0,
		300,
		300,
		200,
		0,
		500
	},

	{
		"Grenade Launcher",
		AMMO_NONE,
		1,
		0,
		300,
		300,
		600,
		0,
		500
	},

	{
		"Rocket Launcher",
		AMMO_CELLS,
		1,
		0,
		300,
		300,
		800,
		0,
		500
	},

	{
		"Lasergun",
		AMMO_CELLS,
		1,
		0,
		300,
		300,
		300,
		600,
		500
	},
	
	{
		"Sniper Rifle",
		AMMO_CELLS,
		2,
		0,
		300,
		300,
		800,
		800,
		500
	},

	{
		"NanoForge",
		AMMO_NONE,
		2,
		0,
		300,
		300,
		1000,
		0,
		0
	},
};

#define GS_NUMWEAPONDEFS ( sizeof( gs_weaponDefs )/sizeof( gs_weapon_definition_t ) )

//=================
//GS_GetWeaponDef
//=================
gs_weapon_definition_t *GS_GetWeaponDef( int weapon )
{
	assert( GS_NUMWEAPONDEFS == WEAP_TOTAL );
	if( weapon >= 0 && weapon < GS_NUMWEAPONDEFS )
		return &gs_weaponDefs[weapon];

	return NULL;
}

//=================
//GS_SelectBestWeapon
//=================
int GS_SelectBestWeapon( player_state_t *playerState )
{
	int i;
	gsitem_t *item, *ammoItem;
	for( i = WEAP_TOTAL - 1; i > WEAP_NONE; i-- )
	{
		item = GS_FindItemByIndex( i );
		if( item && ( playerState->inventory[item->inventorySlot][ITAG] == item->tag ) )
		{
			if( gs_weaponDefs[item->tag].ammoItem == AMMO_NONE )  // check for ammo
				return i;

			ammoItem = GS_FindItemByIndex( gs_weaponDefs[item->tag].ammoItem );
			if( ammoItem && ( playerState->inventory[ammoItem->inventorySlot][ITAG] == ammoItem->tag ) )
				return i;
		}
	}
	return WEAP_NONE;
}

inline qboolean GS_CanChangeWeaponMode( player_state_t *playerState, gsitem_t *weaponItem )
{
	if( gs_weaponDefs[weaponItem->tag].numModes <= 1 )
		return qfalse;

	return qtrue;
}

// check if we have ammo on the weapon
inline int GS_CheckAmmoInWeapon( player_state_t *playerState, gsitem_t *weaponItem )
{
	int ammoCount = 0;

	if( weaponItem )
	{
		if( weaponItem->tag == playerState->inventory[weaponItem->inventorySlot][ITAG] )
			ammoCount = playerState->inventory[weaponItem->inventorySlot][ICOUNT];
	}

	return ammoCount;
}

qboolean GS_Weapons_LoadAmmoClip( player_state_t *playerState, gsitem_t *weaponItem )
{
	gsitem_t *ammoItem;
	int clipCount;

	if( !weaponItem )
		return qfalse;

	ammoItem = GS_FindItemByIndex( gs_weaponDefs[weaponItem->tag].ammoItem );
	if( !ammoItem )
		return qfalse;

	// check if the player carries ammo for this weapon
	if( playerState->inventory[ammoItem->inventorySlot][ITAG] != ammoItem->tag )
		return qfalse;

	if( !playerState->inventory[ammoItem->inventorySlot][ICOUNT] )
		return qfalse;

	// the player may call to reload when there is no need to
	if( playerState->inventory[weaponItem->inventorySlot][ICOUNT] >= ammoItem->count )
		return qfalse;

	// find the amount : the player may call it with some ammo in his weapon
	clipCount = ammoItem->count - playerState->inventory[weaponItem->inventorySlot][ICOUNT];
	if( clipCount > playerState->inventory[ammoItem->inventorySlot][ICOUNT] )
		clipCount = playerState->inventory[ammoItem->inventorySlot][ICOUNT];

	// move the ammo
	playerState->inventory[weaponItem->inventorySlot][ICOUNT] += clipCount;
	playerState->inventory[ammoItem->inventorySlot][ICOUNT] -= clipCount;

	return qtrue;
}

//=================
//GS_ThinkPlayerWeapon
//=================
void GS_ThinkPlayerWeapon( entity_state_t *state, player_state_t *playerState, usercmd_t *ucmd )
{
	gsitem_t *weaponItem;
	qboolean refire = qfalse;

	assert( state->weapon >= 0 && state->weapon < WEAP_TOTAL );

	if( GS_IsGhostState( state ) )
		return;

	if( !ucmd->msec )
		return;

	weaponItem = GS_FindItemByIndex( state->weapon );

	// during cool-down time it can shoot again or go into reload time
	if( ( playerState->weaponState == WEAPON_STATE_REFIRE ) && ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		playerState->weaponState = WEAPON_STATE_READY;
		refire = qtrue;
	}

	// nothing can be done during reload time
	if( ( playerState->weaponState == WEAPON_STATE_RELOADING ) && ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		playerState->weaponState = WEAPON_STATE_READY;
	}

	if( ( playerState->weaponState == WEAPON_STATE_NOAMMOCLICK ) && ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		playerState->weaponState = WEAPON_STATE_READY;
	}

	// there is a weapon to be changed
	if( state->weapon != playerState->stats[STAT_PENDING_WEAPON] )
	{
		if( ( playerState->weaponState == WEAPON_STATE_READY ) ||
			( playerState->weaponState == WEAPON_STATE_DROPPING ) ||
			( playerState->weaponState == WEAPON_STATE_ACTIVATING ) )
		{
			if( playerState->weaponState != WEAPON_STATE_DROPPING )
			{
				playerState->weaponState = WEAPON_STATE_DROPPING;
				playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] += gs_weaponDefs[state->weapon].dropTime;

				if( gs_weaponDefs[state->weapon].dropTime )
					module.predictedEvent( state->number, EV_WEAPONDROP, 0 );
			}
		}
	}

	// do the change
	if( ( playerState->weaponState == WEAPON_STATE_DROPPING ) && ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		playerState->stats[STAT_WEAPON_MODE] = 0;
		state->weapon = playerState->stats[STAT_PENDING_WEAPON];
		playerState->weaponState = WEAPON_STATE_ACTIVATING;
		playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] += gs_weaponDefs[state->weapon].activationTime;
		weaponItem = GS_FindItemByIndex( state->weapon );

		module.predictedEvent( state->number, EV_WEAPONACTIVATE, playerState->stats[STAT_PENDING_WEAPON] );
	}

	if( ( playerState->weaponState == WEAPON_STATE_ACTIVATING ) && ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		playerState->weaponState = WEAPON_STATE_READY;
	}

	// latch weapon mode switch presses
	if( playerState->weaponState == WEAPON_STATE_READY ||
		playerState->weaponState == WEAPON_STATE_REFIRE || 
		playerState->weaponState == WEAPON_STATE_ACTIVATING ||
		playerState->weaponState == WEAPON_STATE_RELOADING )
	{
		if( ( ucmd->buttons & BUTTON_MODE ) && GS_CanChangeWeaponMode( playerState, weaponItem ) )
			playerState->stats[STAT_WEAPON_MODE] |= EV_INVERSE;
	}

	// can change weapon mode now
	if( ( playerState->stats[STAT_WEAPON_MODE] & EV_INVERSE )
		&& ( playerState->weaponState == WEAPON_STATE_READY ) 
		&& ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		playerState->stats[STAT_WEAPON_MODE] &= ~EV_INVERSE;

		playerState->weaponState = WEAPON_STATE_CHANGING_MODE;
		playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] += gs_weaponDefs[state->weapon].activationTime;

		// add the mode change event
		if( playerState->stats[STAT_WEAPON_MODE] == 1 && gs_weaponDefs[state->weapon].numModes < 3 )
			// if it's a toggle, and toggles from 1 to 0, use the "down" switch
			module.predictedEvent( state->number, EV_WEAPON_MODEDOWN, state->weapon );
		else
			module.predictedEvent( state->number, EV_WEAPON_MODEUP, state->weapon );
	}

	if( ( playerState->weaponState == WEAPON_STATE_CHANGING_MODE ) 
		&& ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		playerState->stats[STAT_WEAPON_MODE]++;
		if( playerState->stats[STAT_WEAPON_MODE] >= gs_weaponDefs[state->weapon].numModes )
			playerState->stats[STAT_WEAPON_MODE] = 0;
#ifdef _DEBUG
		if( module.type == GS_MODULE_GAME )
			GS_Printf( "weapon mode: %i\n", playerState->stats[STAT_WEAPON_MODE] );
#endif
		playerState->weaponState = WEAPON_STATE_READY;
	}

	if( ( playerState->weaponState == WEAPON_STATE_READY ) && ( playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] <= 0 ) )
	{
		if( ucmd->buttons & BUTTON_ATTACK )
		{
			if( !gs_weaponDefs[weaponItem->tag].ammoItem || GS_CheckAmmoInWeapon( playerState, weaponItem ) )
			{
				playerState->weaponState = WEAPON_STATE_FIRING;
			}
			// see if we can fix it by reloading
			else if( GS_Weapons_LoadAmmoClip( playerState, weaponItem ) )
			{
				if( !gs_weaponDefs[state->weapon].clipTime )
				{
					// instant reload is like no using clips system
					playerState->weaponState = WEAPON_STATE_FIRING;
				}
				else
				{
					module.predictedEvent( state->number, EV_RELOADING, 0 );
					playerState->weaponState = WEAPON_STATE_RELOADING;
					playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] += gs_weaponDefs[state->weapon].clipTime;
				}
			}
			else // player has no ammo nor clips
			{
				module.predictedEvent( state->number, EV_NOAMMOCLICK, 0 );
				playerState->weaponState = WEAPON_STATE_NOAMMOCLICK;
				playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] += gs_weaponDefs[state->weapon].noAmmoClickTime;
				playerState->stats[STAT_PENDING_WEAPON] = GS_SelectBestWeapon( playerState );
			}
		}
	}

	if( playerState->weaponState == WEAPON_STATE_FIRING )
	{
		if( refire && gs_weaponDefs[state->weapon].smoothRefire )
			module.predictedEvent( state->number, EV_SMOOTHREFIREWEAPON, 0 );
		else
			module.predictedEvent( state->number, EV_FIREWEAPON, 0 );

		playerState->weaponState = WEAPON_STATE_REFIRE;
		playerState->controlTimers[USERINPUT_STAT_WEAPONTIME] += gs_weaponDefs[state->weapon].refireTime;

		// waste ammo
		if( playerState->inventory[weaponItem->inventorySlot][ICOUNT] > 0 )
			playerState->inventory[weaponItem->inventorySlot][ICOUNT]--;
	}
}
