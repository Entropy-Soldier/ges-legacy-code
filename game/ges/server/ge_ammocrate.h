//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.h
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 6/20/2011
// Created By: Killer Monkey
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_AMMOCRATE_H
#define GE_AMMOCRATE_H

#include "items.h"
#include "player.h"
#include "ge_shareddefs.h"

class CGEAmmoCrate : public CItem														
{																					
public:																				
	DECLARE_CLASS( CGEAmmoCrate, CItem );
	DECLARE_DATADESC();

	CGEAmmoCrate();
	~CGEAmmoCrate();

	// CBaseEntity functions
	virtual void Spawn();																																			
	virtual void Precache();

	// CItem functions
	virtual void ItemTouch( CBaseEntity *pEntity );
	virtual bool MyTouch( CBasePlayer *pPlayer );

	virtual void Materialize();
	virtual CBaseEntity *Respawn();

	// Triggers a respawn and makes the crate appear instantly.
	virtual void RespawnNow( void );

	// Special refill function if we only take partial ammo
	void RefillThink();
	
	// Ammo Crate specific functions

	// Add ammo type to the crate, the weaponid should be a weapon that uses this ammo and the amount is how much is given each respawn.
	// -1 for amount means use ammo crate default amount defined in shareddefs.h.  Returns false if weaponid already exists in crate.
	bool AddAmmoType( int weaponid, int amount );

	// Remove a specified weapon's ammo from the crate.  Returns false if weaponid not found.
	bool RemoveAmmoType( int weaponid );

	// Add a global ammo type, these follow special rules and are controlled by the gamemode.
	// Returns false if index already exists.
	bool AddGlobalAmmoType( int index );

	// Update a global ammo type already contained in the crate.
	// Returns false if index does not exist.
	bool UpdateGlobalAmmoType( int index );

	// Remove one of the global ammo types.  Returns false if index not found.
	bool RemoveGlobalAmmoType( int index );

	// Removes all global ammo from this crate.
	void RemoveAllGlobalAmmo( void );

	// Checks to see if the crate has any ammo types that have not been completely taken.
	bool HasAmmo( void );

	// Returns the AmmoID of the ammo at the given slot of this ammo crate.  Slot 0 is the base slot.
	int GetAmmoType( int slot = 0) { return ( slot < m_pContents.Count() && slot >= 0 ) ? m_pContents[slot]->iAmmoID : -1; };

	// Python Itemtracker Functionality
	int GetWeaponID( int slot = 0) { return ( slot < m_pContents.Count() && slot >= 0 ) ? m_pContents[slot]->iWeaponID : -1; }
	int GetWeight( int slot = 0) { return ( slot < m_pContents.Count() && slot >= 0 ) ? m_pContents[slot]->iWeight : -1; }

	// Entity Functions
	void InputInsertAmmo(inputdata_t &inputdata);
	void InputRemoveAmmo(inputdata_t &inputdata);

	void InputInsertSlotAmmo(inputdata_t &inputdata);
	void InputRemoveSlotAmmo(inputdata_t &inputdata);
private:

	struct AmmoData
	{
		int		iAmmoID; // What ammo occupies this slot.
		int		iAmmoAmount; // Max amount of ammo this slot holds.
		int		iAmmoTaken; // How much ammo has been taken out of this slot
		int		iWeaponID; // ID of weapon associated with this ammo type.
		int		iWeight; // Weight of weapon associated with this ammo type
		int		iGlbIndex; // Index of the ammo type in the global ammo listing.  -1 if not global ammo.
	};


	CUtlVector<AmmoData*> m_pContents;
};

#endif
