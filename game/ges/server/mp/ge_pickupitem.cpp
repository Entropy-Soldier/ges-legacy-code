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

extern int gEvilImpulse101;

void CGEPickupItem::ItemTouch( CBaseEntity *pOther )
{
	if ( pOther->IsNPC() )
	{
		// If the NPC is a Bot hand it off to it's proxy, normal NPC's don't use ammo crates
		CBaseCombatCharacter *pNPC = pOther->MyCombatCharacterPointer();
		CGEBotPlayer *pBot = ((CNPC_GEBase*)pOther)->GetBotPlayer();

		if ( pBot && pNPC )
		{
            int pickupCode = GEGameplay()->GetScenario()->HandleItemPickup(ToGEPlayer(pBot), this);

			// ok, a player is touching this item, but can he have it?
			if ( pickupCode == PICKUP_DENY )
				return;

			if ( (pickupCode == PICKUP_DELETE) || MyTouch( pBot ) || (pickupCode == PICKUP_FORCE) )
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
	    // if it's not a player, ignore
	    if ( !pOther->IsPlayer() )
		    return;

	    CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	    // Must be a valid pickup scenario (no blocking). Though this is a more expensive
	    // check than some that follow, this has to be first Obecause it's the only one
	    // that inhibits firing the output OnCacheInteraction.
	    if ( ItemCanBeTouchedByPlayer( pPlayer ) == false )
		    return;

	    m_OnCacheInteraction.FireOutput(pOther, this);

	    // Can I even pick stuff up?
	    if ( !pPlayer->IsAllowedToPickupWeapons() )
		    return;

        int pickupCode = GEGameplay()->GetScenario()->HandleItemPickup(ToGEPlayer(pPlayer), this);

	    // ok, a player is touching this item, but can he have it?
	    if ( pickupCode == PICKUP_DENY )
	    {
		    // no? Ignore the touch.
		    return;
	    }

        // Order is important here due to short circuiting.  DELETE will prevent MyTouch from being called,
        // thus stopping anything from being given by the item.  FORCE will only be relevant if MyTouch is called and fails.
	    if ( (pickupCode == PICKUP_DELETE) || MyTouch( pPlayer ) || (pickupCode == PICKUP_FORCE) )
	    {
		    m_OnPlayerTouch.FireOutput(pOther, this);

		    SetTouch( NULL );
		    SetThink( NULL );

		    // player grabbed the item. 
		    g_pGameRules->PlayerGotItem( pPlayer, this );
		    if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		    {
			    Respawn(); 
		    }
		    else
		    {
			    UTIL_Remove( this );
		    }
	    }
	    else if (gEvilImpulse101)
	    {
		    UTIL_Remove( this );
	    }
	}
}


// More or less just an "is player allowed to have this according to item specific rules" function
// but it's deceptively named.  No item specific rules on the baseclass, so just return true.

bool CGEPickupItem::MyTouch( CBasePlayer *pPlayer )
{
	return true; // By default always allow pickup.
}