//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_pickupitem.cpp
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ammodef.h"
#include "ge_entitytracker.h"

#include "ge_shareddefs.h"
#include "ge_tokenmanager.h"
#include "gebot_player.h"
#include "gemp_gamerules.h"
#include "ge_loadoutmanager.h"

#include "ge_pickupitem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CGEPickupItem )
	DEFINE_ENTITYFUNC( ItemTouch ),
END_DATADESC();

CGEPickupItem::CGEPickupItem( void )
{
}

CGEPickupItem::~CGEPickupItem( void )
{
}

void CGEPickupItem::Spawn( void )
{
	BaseClass::Spawn();
	
	// Override base's ItemTouch for NPC's
	SetTouch( &CGEPickupItem::ItemTouch );

	// So NPC's can "see" us
	AddFlag( FL_OBJECT );
}

void CGEPickupItem::Materialize( void )
{
	BaseClass::Materialize();

	// Override base's ItemTouch for NPC's
	SetTouch( &CGEPickupItem::ItemTouch );
}

void CGEPickupItem::RespawnNow()
{
	Respawn();
	SetNextThink( gpGlobals->curtime );
}

void CGEPickupItem::ItemTouch( CBaseEntity *pOther )
{
	if ( pOther->IsNPC() )
	{
		// If the NPC is a Bot hand it off to it's proxy, normal NPC's don't use ammo crates
		CBaseCombatCharacter *pNPC = pOther->MyCombatCharacterPointer();
		CGEBotPlayer *pBot = ((CNPC_GEBase*)pOther)->GetBotPlayer();

		if ( pBot && pNPC )
		{
			// ok, a player is touching this item, but can he have it?
			if ( !g_pGameRules->CanHaveItem( pBot, this ) )
				return;

			if ( MyTouch( pBot ) )
			{
				SetTouch( NULL );
				SetThink( NULL );

				// player grabbed the item. 
				g_pGameRules->PlayerGotItem( pBot, this );
				Respawn();
			}
		}
	}
	else
	{
		BaseClass::ItemTouch( pOther );
	}
}


// More or less just an "is player allowed to have this according to item specific rules" function
// but it's deceptively named.  No item specific rules on the baseclass, so just return true.

bool CGEPickupItem::MyTouch( CBasePlayer *pPlayer )
{
	return true; // By default always allow pickup.
}