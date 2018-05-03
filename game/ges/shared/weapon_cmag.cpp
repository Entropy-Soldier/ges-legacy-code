///////////// Copyright © 2008, Anthony Iacono. All rights reserved. /////////////
// 
// File: weapon_cmag.cpp
// Description:
//      Implimentation of the Cougar Magnum
//
// Created On: 09/16/2008
// Created By: Killer Monkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
#else
	#include "ge_player.h"
#endif

#include "ge_weaponpistol.h"

#ifdef CLIENT_DLL
#define CWeaponCMag C_WeaponCMag
#endif

//-----------------------------------------------------------------------------
// CWeaponPP7
//-----------------------------------------------------------------------------

class CWeaponCMag : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponCMag, CGEWeaponPistol );

	CWeaponCMag(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_COUGAR_MAGNUM; }

	virtual void PrimaryAttack( void );
	// This function is purely for the CMag and encorporates primary attack functionality
	// but does not play the animation again!
	virtual void FireWeapon( void );
	virtual void HandleFireOnEmpty(void);

	virtual void Precache( void );

	DECLARE_ACTTABLE();

private:
	CWeaponCMag( const CWeaponCMag & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCMag, DT_WeaponCMag )

BEGIN_NETWORK_TABLE( CWeaponCMag, DT_WeaponCMag )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponCMag )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_cmag, CWeaponCMag );
PRECACHE_WEAPON_REGISTER( weapon_cmag );

acttable_t CWeaponCMag::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_CMAG,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_CMAG,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_CMAG,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_CMAG,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_CMAG,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_CMAG,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_PISTOL,			false },
	{ ACT_GES_DRAW_RUN,					ACT_GES_GESTURE_DRAW_PISTOL_RUN,		false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_CMAG,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_CMAG,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_CMAG,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_CMAG,						false },
};
IMPLEMENT_ACTTABLE( CWeaponCMag );


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponCMag::CWeaponCMag( void )
{
	m_fMaxRange1 = 3000;
}

void CWeaponCMag::Precache(void)
{
	PrecacheModel("models/weapons/cmag/v_cmag.mdl");
	PrecacheModel("models/weapons/cmag/w_cmag.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/cmag");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_magnum");

	PrecacheScriptSound("Weapon_cmag.Single");
	PrecacheScriptSound("Weapon_cmag.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}

void CWeaponCMag::PrimaryAttack( void )
{
	WeaponSound( SPECIAL1 ); // Play our chamber rotate sound.
	BaseClass::PrimaryAttack();
}

void CWeaponCMag::FireWeapon( void )
{
	// If my clip is empty dry fire instead, needed because of our adjusted HandleFireOnEmpty()
	if (!m_iClip1)
	{
		WeaponSound( EMPTY );
		return;
	}

	// Otherwise just do the standard weapon fire procedure.
	BaseClass::FireWeapon();
}

void CWeaponCMag::HandleFireOnEmpty()
{
	// Normally we wouldn't attempt to fire with no ammo, but for the revolver-type weapons we want to
	// still rotate the chamber and then do the dryfire effects.  So we need to pseudo-fire.
	m_bFireOnEmpty = true;

	if ( m_flNextEmptySoundTime <= gpGlobals->curtime )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		WeaponSound( SPECIAL1 );

		m_flShootTime = gpGlobals->curtime + GetFireDelay();
		m_flNextEmptySoundTime = gpGlobals->curtime + GetFireRate();
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}
}