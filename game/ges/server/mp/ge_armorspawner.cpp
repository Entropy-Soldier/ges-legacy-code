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
#include "ge_armorvest.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGEArmorSpawner : public CGESpawner
{
public:

	DECLARE_CLASS( CGEArmorSpawner, CGESpawner );
	DECLARE_DATADESC();

	CGEArmorSpawner();

	virtual bool IsSpecial(void) { return (m_iAmount != MAX_ARMOR); }; // Replace half armor first.

	int	m_iSpawnCheckRadius;
	bool m_bIsHalfArmor;

protected:
	virtual void OnInit( void );
	virtual int  ShouldRespawn( void );
	virtual bool OnEntSpawned( bool isOverrideEnt );

private:
	int		m_iAmount;
};

LINK_ENTITY_TO_CLASS(ge_armorspawner, CGEArmorSpawner);

LINK_ENTITY_TO_CLASS(item_armorvest, CGEArmorSpawner);
LINK_ENTITY_TO_CLASS(item_armorvest_half, CGEArmorSpawner);

BEGIN_DATADESC( CGEArmorSpawner )
	// Parameters
	DEFINE_KEYFIELD( m_iSpawnCheckRadius, FIELD_INTEGER, "CheckRadius"),
	DEFINE_KEYFIELD( m_bIsHalfArmor, FIELD_BOOLEAN, "IsHalfArmor"),
END_DATADESC();

CGEArmorSpawner::CGEArmorSpawner( void )
{
	m_iSpawnCheckRadius = 512;
	m_bIsHalfArmor = false;
	m_iAmount = MAX_ARMOR;
}

void CGEArmorSpawner::OnInit( void )
{
	// Make sure we're compatable with old naming convention.
	if ( !Q_strcmp(GetClassname(), "item_armorvest") )
	{
		m_bIsHalfArmor = false;
		Warning( "Map has %s in it when ge_armorspawner should be used!\n", GetClassname() );
		SetClassname( "ge_armorspawner" );
		return;
	}
	else if ( !Q_strcmp(GetClassname(), "item_armorvest_half") )
	{
		m_bIsHalfArmor = true;
		Warning( "Map has %s in it when ge_armorspawner should be used!\n", GetClassname() );
		SetClassname( "ge_armorspawner" );
		return;
	}

	if (m_bIsHalfArmor)
		m_iAmount = MAX_ARMOR / 2;

	SetBaseEnt( "ge_armorvest" );
}

int CGEArmorSpawner::ShouldRespawn( void )
{
	int basestate = BaseClass::ShouldRespawn();

	if ( IsOverridden() && basestate > 1 )
		return 2;

	if ( !GEMPRules()->ArmorShouldRespawn() )
		return 0;

	return basestate;
}

bool CGEArmorSpawner::OnEntSpawned( bool isOverrideEnt )
{
	// If we are not spawning an armor vest ignore
	if ( isOverrideEnt )
		return false;

	// Don't do 2 isOverrideEnt checks if we don't have to.
	if (BaseClass::OnEntSpawned(isOverrideEnt))
	{
		CGEArmorVest *pVest = (CGEArmorVest*)GetEnt();

		// Might as well check though I'm pretty much certain we can never get to this point without a valid pVest cast.
		if (!pVest)
			return false; // If we early out here somehow we'll end up with a non-armorvest entity with normal spawner settings somehow.

		pVest->SetAmount( m_iAmount );
		pVest->SetSpawnCheckRadius( m_iSpawnCheckRadius );

		return true;
	}

	return false;
}