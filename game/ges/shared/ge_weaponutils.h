///////////// Copyright © 2018, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weaponutils.h
// Description:
//      Functions for easier handling of weapons.
//
/////////////////////////////////////////////////////////////////////////////'
#ifndef GE_WEAPONUTILS_H
#define GE_WEAPONUTILS_H

#include "takedamageinfo.h"
#include "ge_weapon_parse.h"

// Gets the GEWeaponInfo for a given weaponID.  Useful if you don't have an active instance of said weapon but still need the info.
const CGEWeaponInfo *WeaponInfoFromID( int id );

bool IsAmmoBasedWeapon( int id );

#ifdef GAME_DLL
// Infers the weaponID from a damageinfo struct.  Server-side Only.
int WeaponIDFromDamageInfo( CTakeDamageInfo *info );
#endif

#endif
