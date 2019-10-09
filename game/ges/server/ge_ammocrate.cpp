//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
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
#include "ge_loadoutmanager.h"

#include "ge_pickupitem.h"
#include "ge_ammocrate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( ge_ammocrate, CGEAmmoCrate );
PRECACHE_REGISTER( ge_ammocrate );

BEGIN_DATADESC( CGEAmmoCrate )
	// Function Pointers
	DEFINE_THINKFUNC( RefillThink ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InsertAmmo", InputInsertAmmo ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RemoveAmmo", InputRemoveAmmo ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "InsertSlotAmmo", InputInsertSlotAmmo ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveSlotAmmo", InputRemoveSlotAmmo ),
END_DATADESC();

ConVar ge_partialammopickups("ge_partialammopickups", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Allow players to pick up ammo from a crate without picking up all of it.");

CGEAmmoCrate::CGEAmmoCrate( void )
{
    m_iOriginSlot = -1;
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

    if (GetOwnerEntity() && (!Q_strcmp(GetOwnerEntity()->GetClassname(), "ge_weaponspawner") || !Q_strcmp(GetOwnerEntity()->GetClassname(), "ge_ammospawner")))
    {
        m_iOriginSlot = static_cast<CGESpawner*>(GetOwnerEntity())->GetSlot();
    }

	// Add us to the gameplay item tracker.
	GEEntityTracker()->AddItemToTracker( this, ET_START_ITEMSPAWNED, ET_LIST_AMMO );

	// Make sure we don't appear until we actually have ammo.
	BaseClass::Respawn();
	SetThink( NULL );
}

void CGEAmmoCrate::Precache( void )
{
	PrecacheModel( AMMOCRATE_MODEL );
	PrecacheModel( BABYCRATE_MODEL );
	BaseClass::Precache();
}

CBaseEntity *CGEAmmoCrate::Respawn( void )
{
	if ( !m_pContents.Count() ) // There's nothing for this crate to give so we shouldn't actually spawn it.
	{
		BaseClass::Respawn();
		SetThink( NULL );
		return this;
	}

	// Refill all the ammo slots.
	RefillThink();

	return BaseClass::Respawn();
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
	if ( !HasAmmo() || (someAmmoTaken && !ge_partialammopickups.GetBool()) )
		return true;
	
	// But if it's still around, make sure to refill all of the included ammo types after a little while.
	SetThink( &CGEAmmoCrate::RefillThink );
	SetNextThink( gpGlobals->curtime + g_pGameRules->FlItemRespawnTime( this ) );

	return false;
}


void CGEAmmoCrate::InputInsertAmmo(inputdata_t &inputdata)
{
	// Input format is "WEAPONID AMOUNT"

	const char* inputString = inputdata.value.String();
	CUtlVector<char*> data;

	Q_SplitString(inputString, " ", data);

	// AddAmmoType will take care of any invalid inputs.
	AddAmmoType(atoi(data[0]), atoi(data[1]));

	data.PurgeAndDeleteElements();
}

void CGEAmmoCrate::InputRemoveAmmo(inputdata_t &inputdata)
{
	// Input format is "WEAPONID"

	// RemoveAmmoType will take care of any invalid inputs.
	RemoveAmmoType( atoi(inputdata.value.String()) );
}

void CGEAmmoCrate::InputInsertSlotAmmo(inputdata_t &inputdata)
{
	// AddAmmoType will take care of any invalid inputs.
	AddAmmoType(GEMPRules()->GetLoadoutManager()->GetWeaponInSlot( inputdata.value.Int() ), -1);
}

void CGEAmmoCrate::InputRemoveSlotAmmo(inputdata_t &inputdata)
{
	// Input format is "WEAPONID"

	// RemoveAmmoType will take care of any invalid inputs.
	RemoveAmmoType( GEMPRules()->GetLoadoutManager()->GetWeaponInSlot( inputdata.value.Int() ) );
}