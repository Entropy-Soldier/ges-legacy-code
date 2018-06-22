///////////// Copyright © 2018, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weaponutils.cpp
// Description:
//      Functions for easier handling of weapons.
//
//////////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_shareddefs.h"
#include "ge_weapon_parse.h"
#include "ge_weapon.h"

#ifdef GAME_DLL
#include "grenade_gebase.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Given an alias, return the associated weapon ID
int AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i = WEAPON_NONE; i < WEAPON_MAX; ++i ) {
			if (!GEWeaponInfo[i].alias)
				continue;
			if (!Q_stricmp( GEWeaponInfo[i].alias, alias ))
				return i;
		}
	}

	return WEAPON_NONE;
}

// Given a weapon ID, return it's alias
//
const char *WeaponIDToAlias( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return "";

	return GEWeaponInfo[id].alias;
}

const char *GetAmmoForWeapon( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return "";

	return GEWeaponInfo[id].ammo;
}

const char *GetWeaponPrintName( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return "";

	return GEWeaponInfo[id].printname;
}

int GetRandWeightForWeapon( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return 0;

	return GEWeaponInfo[id].randweight;
}

int GetStrengthOfWeapon(int id)
{
	if ((id < 0) || (id >= WEAPON_MAX))
		return 0;

	return GEWeaponInfo[id].strength;
}

// Uses the given weapon's weaponID to determine its weaponinfo.
// Returns NULL if no valid info was found.
const CGEWeaponInfo *WeaponInfoFromID(int id)
{
	const char *name = WeaponIDToAlias(id);
	if (name && name[0] != '\0')
	{
		int h = LookupWeaponInfoSlot(name);
		if (h == GetInvalidWeaponInfoHandle())
			return NULL;

		return dynamic_cast<CGEWeaponInfo*>(GetFileWeaponInfoFromHandle(h));
	}

	Warning("Failed to look up info for ID %d\n", id);

	return NULL;
}

int WeaponMaxDamageFromID( int id )
{
	const CGEWeaponInfo *weap = WeaponInfoFromID( id );

	if (weap)
		return weap->m_iDamageCap;

	return 5000;
}

const char* WeaponPickupSoundFromID(int id)
{
	const CGEWeaponInfo *weap = WeaponInfoFromID( id );

	if (weap)
		return weap->aShootSounds[PICKUP];

	return NULL;
}

// End weapon helper functions

#ifdef GAME_DLL
// Extracts the ID of the weapon the attacker used from the given CTakeDamageInfo
int WeaponIDFromDamageInfo( CTakeDamageInfo *info )
{
	int weapid = WEAPON_NONE;

	CGEWeapon *pAttackerWep = NULL;

	if (info->GetWeapon())
		pAttackerWep = ToGEWeapon((CBaseCombatWeapon*)info->GetWeapon());

	if (pAttackerWep)
		weapid = pAttackerWep->GetWeaponID();
	else if (info->GetInflictor() && !info->GetInflictor()->IsNPC() && Q_stristr(info->GetInflictor()->GetClassname(), "npc_"))
		weapid = ToGEGrenade(info->GetInflictor())->GetWeaponID();
	else if (info->GetDamageType() & DMG_BLAST)
		weapid = WEAPON_EXPLOSION;

	return weapid;
}
#endif