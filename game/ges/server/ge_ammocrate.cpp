//////////  Copyright � 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.cpp
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 6/20/2011
// Created By: Killer Monkey
////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ammodef.h"
#include "ge_entitytracker.h"

#include "ge_shareddefs.h"
#include "ge_tokenmanager.h"
#include "gebot_player.h"
#include "gemp_gamerules.h"

#include "ge_ammocrate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( ge_ammocrate, CGEAmmoCrate );
PRECACHE_REGISTER( ge_ammocrate );

BEGIN_DATADESC( CGEAmmoCrate )
	// Function Pointers
	DEFINE_ENTITYFUNC( ItemTouch ),
	DEFINE_THINKFUNC( RefillThink ),
END_DATADESC();

ConVar ge_partialammopickups("ge_partialammopickups", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Allow players to pick up ammo from a crate without picking up all of it.");

CGEAmmoCrate::CGEAmmoCrate( void )
{
}

CGEAmmoCrate::~CGEAmmoCrate( void )
{
	m_pContents.PurgeAndDeleteElements();
}

void CGEAmmoCrate::Spawn( void )
{
	Precache();

	SetModel( AMMOCRATE_MODEL );
	BaseClass::Spawn();
	
	// Override base's ItemTouch for NPC's
	SetTouch( &CGEAmmoCrate::ItemTouch );

	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

	GEEntityTracker()->AddItemToTracker( this, ET_LIST_AMMO );

	Respawn();
}

void CGEAmmoCrate::Precache( void )
{
	PrecacheModel( AMMOCRATE_MODEL );
	PrecacheModel( BABYCRATE_MODEL );
	BaseClass::Precache();
}

void CGEAmmoCrate::Materialize( void )
{
	BaseClass::Materialize();

	// Notify Python about the ammobox
	if ( GetScenario() )
		GetScenario()->OnAmmoSpawned( this );

	// Override base's ItemTouch for NPC's
	SetTouch( &CGEAmmoCrate::ItemTouch );
}

CBaseEntity *CGEAmmoCrate::Respawn( void )
{
	if ( !m_pContents.Count() ) // There's nothing for this crate to give so we shouldn't actually spawn it.
	{
		BaseClass::Respawn();
		SetThink( NULL );
		return this;
	}

	// Notify Python about the ammobox
	if ( GetScenario() )
		GetScenario()->OnAmmoRemoved( this );

	// Refill all the ammo slots.
	RefillThink();

	return BaseClass::Respawn();
}

void CGEAmmoCrate::RespawnNow()
{
	Respawn();
	SetNextThink( gpGlobals->curtime );
}

void CGEAmmoCrate::RefillThink( void )
{
	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
		m_pContents[i]->iAmmoTaken = 0;
}

bool CGEAmmoCrate::AddAmmoType( int weaponid, int amount )
{ 
	int newAmmoID = GetAmmoDef()->Index( GetAmmoForWeapon(weaponid) );

	if ( newAmmoID == -1 || amount == 0)
		return false; // No ammo to add.

	// Make sure we don't already have ammo registered under this weapon.
	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
	{
		if (m_pContents[i]->iWeaponID == weaponid)
			return false;
	}

	Ammo_t *pAmmo = GetAmmoDef()->GetAmmoOfIndex( newAmmoID ); // We know this exists because we just fetched it.
	Assert( pAmmo );

	int crateAmount = pAmmo->nCrateAmt;

	if (crateAmount <= 0)
		return false; 	// This weapon isn't meant to spawn with ammo.

	// Initalize our new ammo data.
	AmmoData *pNewAmmoData = new AmmoData;

	pNewAmmoData->iAmmoID = newAmmoID;
	pNewAmmoData->iWeaponID = weaponid;
	pNewAmmoData->iAmmoAmount = amount < 0 ? crateAmount : amount;
	pNewAmmoData->iAmmoTaken = 0;
	pNewAmmoData->iWeight = GEWeaponInfo[weaponid].strength;
	pNewAmmoData->iGlbIndex = -1; // Normal ammo does not exist in the global ammo registry.

	// Now that everything is ready, add our new entry to the ammo vector.
	if (m_pContents.AddToTail(pNewAmmoData) == 0) // Add it and check if the index it was assigned to is the first one.
	{
		m_nSkin = pAmmo->nCrateSkin;
		RespawnNow(); // First ammo type added spawns the crate.
	}

	return true;
}

bool CGEAmmoCrate::RemoveAmmoType( int weaponid )
{ 
	// Can't remove token ammo entries because these are global entries and should be removed that way.
	if (weaponid == WEAPON_TOKEN)
		return false;


	int targetIndex = -1;

	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
	{
		if (m_pContents[i]->iWeaponID == weaponid)
			targetIndex = i;
	}

	if (targetIndex == -1)
		return false; // Can't remove what we don't have.

	m_pContents.Remove(targetIndex);

	// Check our ammo status.
	if (!HasAmmo())
		BaseClass::Respawn(); // No ammo left in the crate now.

	if (m_pContents.Count() < 1)
		SetThink( NULL ); // And there's no more ammo types left to spawn back in, so don't spawn until we have some.
	else if (targetIndex == 0) // Otherwise we at least have a slot 0 so let's make sure that didn't get changed.
		m_nSkin =  GetAmmoDef()->CrateSkin(m_pContents[0]->iAmmoID); // And if it did, the skin is determined by new slot 0 ammo.


	return true;
}

bool CGEAmmoCrate::AddGlobalAmmoType( int index )
{ 
	int ammoID = GEMPRules()->GetTokenManager()->GetGlobalAmmoID(index);
	int globalAmmoCount = GEMPRules()->GetTokenManager()->GetGlobalAmmoCount(index);

	if (ammoID == -1 || globalAmmoCount <= 0)
	{
		DevWarning("Tried to add invalid global ammo type with id %d and amount %d!\n", ammoID, globalAmmoCount);
		return false;
	}

	// Make sure we don't already have this global index added.
	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
	{
		if (m_pContents[i]->iGlbIndex == index)
			return UpdateGlobalAmmoType(index);
	}

	// Initalize our new ammo data.
	AmmoData *pNewAmmoData = new AmmoData;

	pNewAmmoData->iAmmoID = ammoID;
	pNewAmmoData->iWeaponID = WEAPON_TOKEN; // Global ammo is not associated by weapon.
	pNewAmmoData->iAmmoAmount = globalAmmoCount;
	pNewAmmoData->iAmmoTaken = 0;
	pNewAmmoData->iWeight = 0; // Global ammo is in every single crate so who cares.
	pNewAmmoData->iGlbIndex = index; // Index of this ammo entry in the global ammo registry.  Used for updates/removals.

	// Now that everything is ready, add our new entry to the ammo vector.
	if (m_pContents.AddToTail(pNewAmmoData) == 0) // Add it and check if the index it was assigned to is the first one.
	{
		m_nSkin = GetAmmoDef()->CrateSkin(m_pContents[0]->iAmmoID);
		RespawnNow(); // First ammo type added spawns the crate.
	}

	return true;
}

bool CGEAmmoCrate::UpdateGlobalAmmoType( int index )
{
	int ammoID = GEMPRules()->GetTokenManager()->GetGlobalAmmoID( index );

	if (ammoID == -1)
	{
		DevWarning("Tried to update invalid global ammo type!\n");
		return false;
	}

	int targetIndex = -1;

	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
	{
		if (m_pContents[i]->iGlbIndex == index)
		{
			targetIndex = i;
			break;
		}
	}

	if (targetIndex == -1)
		return false;

	if (targetIndex == 0) // If we updated the first ammo index we need to update the skin.
		m_nSkin = GetAmmoDef()->CrateSkin(m_pContents[0]->iAmmoID);

	m_pContents[targetIndex]->iAmmoID = ammoID;
	m_pContents[targetIndex]->iAmmoAmount = GEMPRules()->GetTokenManager()->GetGlobalAmmoCount(index);
	m_pContents[targetIndex]->iAmmoTaken = 0;

	return true;
}

bool CGEAmmoCrate::RemoveGlobalAmmoType( int index )
{ 
	int targetIndex = -1;

	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
	{
		if (m_pContents[i]->iGlbIndex == index)
			targetIndex = i;
	}

	if (targetIndex == -1)
		return false; // Can't remove what we don't have.

	m_pContents.Remove(targetIndex);

	// Check our ammo status.
	if (!HasAmmo())
		BaseClass::Respawn(); // No ammo left in the crate now.

	if (m_pContents.Count() < 1)
		SetThink( NULL ); // And there's no more ammo types left to spawn back in, so don't spawn until we have some.
	else if (targetIndex == 0) // Otherwise we at least have a slot 0 so let's make sure that didn't get changed.
		m_nSkin =  GetAmmoDef()->CrateSkin(m_pContents[0]->iAmmoID); // And if it did, the skin is determined by new slot 0 ammo.


	return true;
}

void CGEAmmoCrate::RemoveAllGlobalAmmo()
{ 
	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
	{
		if (m_pContents[i]->iGlbIndex != -1)
			m_pContents.Remove(i); // Going backwards has another benefit here in that we don't need to worry about shifted elements.
	}
}

bool CGEAmmoCrate::HasAmmo( void )
{
	int amount = 0;

	for ( int i = m_pContents.Count() - 1; i >= 0; i-- )
		amount += max(m_pContents[i]->iAmmoAmount - m_pContents[i]->iAmmoTaken, 0);

	// We have ammo if any of our ammo types have ammo left.
	return amount > 0;
}

void CGEAmmoCrate::ItemTouch( CBaseEntity *pOther )
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
				// Might not need these, let's see if we don't so we can make this as straightforward as possible.
				// SetTouch( NULL );
				// SetThink( NULL );

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

bool CGEAmmoCrate::MyTouch( CBasePlayer *pPlayer )
{
	// All checks have already been done (called by ItemTouch)
	// so just give the ammo to the player and return true!
	if ( !HasAmmo() )
	{
		Warning( "[GEAmmoCrate] Crate touched with no ammo in it\n" );
		return true;
	}
	
	bool someAmmoTaken = false;
	int ammoTypeCount = m_pContents.Count();

	for ( int i = 0; i < ammoTypeCount; i++ )
	{
		// Try to give the player an ammo amount of each type equal to the amount that has not yet been taken from the crate.
		// Only play the pickup sound on the first ammo type taken.
		int ammoGiven = pPlayer->GiveAmmo( m_pContents[i]->iAmmoAmount - m_pContents[i]->iAmmoTaken, m_pContents[i]->iAmmoID, someAmmoTaken );
		m_pContents[i]->iAmmoTaken += ammoGiven;

		// Once ammo has been successfully given, take note of it so we no longer play the sound.
		// and can force pickup if ge_partialammopickups is 0.
		if ( ammoGiven )
			someAmmoTaken = true;
	}

	// The crate should vanish and start its respawn cycle if:
	// it has no ammo
	// we've just taken some ammo and don't allow partial pickups
	// the gamemode wants to force us to take it.
	if ( !HasAmmo() || (someAmmoTaken && !ge_partialammopickups.GetBool()) || GERules()->ShouldForcePickup( pPlayer, this ) )
		return true;
	
	// But if it's still around, make sure to refill all of the included ammo types after a little while.
	SetThink( &CGEAmmoCrate::RefillThink );
	SetNextThink( gpGlobals->curtime + g_pGameRules->FlItemRespawnTime( this ) );

	return false;
}

// To be removed when entity tracker is finished.
void CGEAmmoCrate::UpdateOnRemove(void)
{
	BaseClass::UpdateOnRemove();

	if ( GetScenario() && m_pContents.Count() )
	{
		GetScenario()->OnAmmoRemoved( this );
	}
}