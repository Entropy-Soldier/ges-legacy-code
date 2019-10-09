//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_ammospawner.cpp
// Description: See Header
//
// Created On: 7-19-11
// Created By: KillerMonkey
///////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ammodef.h"
#include "ge_utils.h"

#include "ge_ammocrate.h"
#include "ge_loadoutmanager.h"
#include "gemp_gamerules.h"
#include "ge_tokenmanager.h"
#include "ge_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGEAmmoSpawner : public CGESpawner
{
public:
	DECLARE_CLASS( CGEAmmoSpawner, CGESpawner );
	DECLARE_DATADESC();

protected:
	virtual void OnInit( void );
	virtual bool OnEntSpawned( bool isOverrideEnt );
	
	virtual int  ShouldRespawn( void );
	virtual float GetRespawnInterval( void );

	void InputInsertAmmo(inputdata_t &inputdata);
	void InputRemoveAmmo(inputdata_t &inputdata);

	void InputInsertSlotAmmo(inputdata_t &inputdata);
	void InputRemoveSlotAmmo(inputdata_t &inputdata);
};

LINK_ENTITY_TO_CLASS( ge_ammospawner, CGEAmmoSpawner );

BEGIN_DATADESC( CGEAmmoSpawner )
	DEFINE_INPUTFUNC( FIELD_STRING, "InsertAmmo", InputInsertAmmo ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RemoveAmmo", InputRemoveAmmo ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "InsertSlotAmmo", InputInsertSlotAmmo ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveSlotAmmo", InputRemoveSlotAmmo ),
END_DATADESC();

void CGEAmmoSpawner::OnInit( void )
{
	SetBaseEnt( "ge_ammocrate" );
}

bool CGEAmmoSpawner::OnEntSpawned( bool isOverrideEnt )
{
	// If we are not spawning an ammo crate ignore
	if ( isOverrideEnt )
		return false;

	// Don't do 2 isOverrideEnt checks if we don't have to.
	if (BaseClass::OnEntSpawned(isOverrideEnt))
	{
		CGEAmmoCrate *pCrate = dynamic_cast<CGEAmmoCrate *>(GetEnt());

		// Might as well check though I'm pretty much certain we can never get to this point without a valid pCrate cast.
		if (!pCrate)
			return false; // If we early out here somehow we'll end up with a non-ammo crate entity with normal spawner settings somehow.

		// Load us up with the appropriate ammo
		int weapid = GEMPRules()->GetLoadoutManager()->GetWeaponInSlot(GetSlot()); // If this isn't a valid weapon or slot AddAmmoType will not do anything.
		pCrate->AddAmmoType(weapid, -1); // Adding the first ammo type spawns the crate.
		GEMPRules()->GetTokenManager()->InsertGlobalAmmo(pCrate); // Insert global ammo if there is any.

		return true;
	}

	return false;
}

int CGEAmmoSpawner::ShouldRespawn( void )
{
	int basestate = BaseClass::ShouldRespawn();

	if ( IsOverridden() && basestate > 1 )
		return 2;

	if ( !GEMPRules()->AmmoShouldRespawn() )
		return 0;

	return basestate;
}

float CGEAmmoSpawner::GetRespawnInterval( void )
{
	return IsOverridden() ? 0 : GERules()->FlItemRespawnTime(NULL);
}

void CGEAmmoSpawner::InputInsertAmmo(inputdata_t &inputdata)
{
	// Input format is "WEAPONID AMOUNT"

	// Make sure we have a crate.
	CGEAmmoCrate *pCrate = (CGEAmmoCrate*) GetEnt();
	if ( !pCrate )
		return;

	const char* inputString = inputdata.value.String();
	CUtlVector<char*> data;

	Q_SplitString(inputString, " ", data);

	// AddAmmoType will take care of any invalid inputs.
	pCrate->AddAmmoType(atoi(data[0]), atoi(data[1]));

	data.PurgeAndDeleteElements();
}

void CGEAmmoSpawner::InputRemoveAmmo(inputdata_t &inputdata)
{
	// Input format is "WEAPONID"

	// Make sure we have a crate.
	CGEAmmoCrate *pCrate = (CGEAmmoCrate*) GetEnt();
	if ( !pCrate )
		return;

	// RemoveAmmoType will take care of any invalid inputs.
	pCrate->RemoveAmmoType( atoi(inputdata.value.String()) );
}

void CGEAmmoSpawner::InputInsertSlotAmmo(inputdata_t &inputdata)
{
	// Make sure we have a crate.
	CGEAmmoCrate *pCrate = (CGEAmmoCrate*) GetEnt();
	if ( !pCrate )
		return;

	// AddAmmoType will take care of any invalid inputs.
	pCrate->AddAmmoType(GEMPRules()->GetLoadoutManager()->GetWeaponInSlot( inputdata.value.Int() ), -1);
}

void CGEAmmoSpawner::InputRemoveSlotAmmo(inputdata_t &inputdata)
{
	// Make sure we have a crate.
	CGEAmmoCrate *pCrate = (CGEAmmoCrate*) GetEnt();
	if ( !pCrate )
		return;

	// RemoveAmmoType will take care of any invalid inputs.
	pCrate->RemoveAmmoType( GEMPRules()->GetLoadoutManager()->GetWeaponInSlot( inputdata.value.Int() ) );
}