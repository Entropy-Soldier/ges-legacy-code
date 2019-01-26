//////////  Copyright © 2006, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_weaponspawner.h
// Description: Weapon spawner implementation.
//
// Created On: 3/12/2008
// Created By: Killer Monkey (With Help from Mario Cacciatore <mailto:mariocatch@gmail.com>)
///////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ge_shareddefs.h"
#include "gemp_gamerules.h"
#include "ge_loadoutmanager.h"
#include "ge_weapon.h"
#include "ge_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGEWeaponSpawner : public CGESpawner
{
public:
	DECLARE_CLASS( CGEWeaponSpawner, CGESpawner );
	DECLARE_DATADESC();

	CGEWeaponSpawner();

	virtual bool IsSpecial(void) { return ( m_bAllowSpecial || BaseClass::IsSpecial() ); };
	virtual bool IsVerySpecial() { return m_bAllowSpecial; } // Spawns we've marked are very special and will be used over ones that aren't marked.

	virtual bool OnEntSpawned( bool isOverrideEnt ) { return true; } // We don't need to do anything for weapons.

    virtual void SetSlot( int newSlot );

protected:
	virtual void OnInit( void );

	virtual int  ShouldRespawn( void );

private:
	int m_iSlotSkin;
	bool m_bAllowSpecial;		// Explicitly allow a special token to spawn here
};

LINK_ENTITY_TO_CLASS(ge_weaponspawner, CGEWeaponSpawner);

BEGIN_DATADESC( CGEWeaponSpawner )
	DEFINE_KEYFIELD( m_bAllowSpecial, FIELD_BOOLEAN, "allowgg" ),
	DEFINE_KEYFIELD( m_iSlotSkin, FIELD_INTEGER, "skin" ),
END_DATADESC();

CGEWeaponSpawner::CGEWeaponSpawner( void )
{
	m_bAllowSpecial = false;
}

void CGEWeaponSpawner::SetSlot( int newSlot )		
{ 
    BaseClass::SetSlot(newSlot);

    // Match our slotskin to the new slot as it can determine our slot assignment if set pre-initalization.
    if ( newSlot < MAX_WEAPON_SPAWN_SLOTS && newSlot >= 0 )
        m_iSlotSkin = newSlot - 1 >= 0 ? newSlot - 1 : 7;
}

void CGEWeaponSpawner::OnInit( void )
{
	if (m_iSlotSkin != 8)
	{
		// Use our hammer skin to determine what slot we are.
		SetSlot((m_iSlotSkin + 1) % 8);
	}

	int weapid = GEMPRules()->GetLoadoutManager()->GetWeaponInSlot( GetSlot() );
	if ( weapid != WEAPON_NONE )
		SetBaseEnt( WeaponIDToAlias( weapid ) );
}

int CGEWeaponSpawner::ShouldRespawn( void )
{
	int basestate = BaseClass::ShouldRespawn();

	if ( IsOverridden() && basestate > 1 )
		return 2;

	// Obey our game rules
	if ( !GEMPRules()->WeaponShouldRespawn( GetBaseClass() ) )
		return 0;

	return basestate;
}