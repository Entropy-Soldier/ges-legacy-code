///////////// Copyright © 2016. All rights reserved. /////////////
// 
// File: weapon_watchlaser.cpp
// Description:
//      Melee weapon that fires super fast and does a ton of damage!
//
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_weaponmelee.h"
#include "ge_gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
#include "c_ge_player.h"
#else
#include "ge_player.h"
#include "gemp_player.h"
#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	WATCHLASER_MAX_RANGE	90.0f
#define	WATCHLASER_MIN_RANGE	70.0f
#define RECHARGE_INTERVAL		0.5f
#define POST_FIRE_CHARGE_DELAY	2.5f
#define RECHARGE_AMOUNT			4

#define MAX_RANGE_THRESH		190
#define MIN_RANGE_THRESH		50

#define MIN_RANGE_RATIO			0.7f

#ifdef CLIENT_DLL
#define CGEWeaponWatchLaser C_GEWeaponWatchLaser
#endif

//-----------------------------------------------------------------------------
// CGEWeaponWatchLaser
//-----------------------------------------------------------------------------

class CGEWeaponWatchLaser : public CGEWeaponMelee
{
public:
	DECLARE_CLASS(CGEWeaponWatchLaser, CGEWeaponMelee);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	DECLARE_ACTTABLE();

	CGEWeaponWatchLaser();

	float		GetRange(void);

	void		AddViewKick(void);
	virtual		void Precache(void);
	void		Swing(int bIsSecondary);

	virtual void	ItemPostFrame(void);

	virtual GEWeaponID GetWeaponID(void) const { return WEAPON_WATCHLASER; }
	virtual bool	   SwingsInArc()		   { return false; }

	virtual Activity GetDrawActivity(void);
	virtual void WeaponIdle(void);
	virtual int		GetStaticHitActivity()							{ return ACT_VM_WATCH_DETONATE; }

	virtual bool Deploy(void);

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	void HandleAnimEventMeleeHit(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif

protected:
	void ImpactEffect(trace_t &trace);

private:
	CGEWeaponWatchLaser(const CGEWeaponWatchLaser &);

	float m_flNextRecharge;
};

//-----------------------------------------------------------------------------
// CGEWeaponWatchLaser
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponWatchLaser, DT_GEWeaponWatchLaser )

BEGIN_NETWORK_TABLE( CGEWeaponWatchLaser, DT_GEWeaponWatchLaser )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CGEWeaponWatchLaser)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_watchlaser, CGEWeaponWatchLaser);
PRECACHE_WEAPON_REGISTER(weapon_watchlaser);

acttable_t	CGEWeaponWatchLaser::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_GES_IDLE_WATCH, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_SLAM, false },

	{ ACT_MP_RUN, ACT_GES_RUN_WATCH, false },
	{ ACT_MP_WALK, ACT_GES_WALK_WATCH, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_SLAM, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_GES_GESTURE_RANGE_ATTACK_WATCH, true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_GES_GESTURE_RANGE_ATTACK_WATCH, true },

	{ ACT_MP_JUMP, ACT_GES_JUMP_WATCH, false },
	{ ACT_GES_CJUMP, ACT_GES_CJUMP_WATCH, false },
};
IMPLEMENT_ACTTABLE(CGEWeaponWatchLaser);


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CGEWeaponWatchLaser::CGEWeaponWatchLaser(void)
{
	m_flNextRecharge = 1;
}

void CGEWeaponWatchLaser::Precache(void)
{
	PrecacheModel("models/weapons/mines/v_remotemine.mdl");
	PrecacheModel("models/weapons/mines/w_mine.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/mine_remote");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_remotemine");

	PrecacheScriptSound("weapon_mines.WatchBeep");

	//	m_iWatchModelIndex = CBaseEntity::PrecacheModel( "models/weapons/mines/w_watch.mdl" );
	BaseClass::Precache();
}

bool CGEWeaponWatchLaser::Deploy(void)
{
#ifdef GAME_DLL
#ifdef RECHARGE
	CGEMPPlayer *pOwner = ToGEMPPlayer(GetOwner());

	if (pOwner && gpGlobals->curtime > m_flNextRecharge)
	{
		pOwner->GiveAmmo((floor((gpGlobals->curtime - m_flNextRecharge) / RECHARGE_INTERVAL) + 1) * RECHARGE_AMOUNT, m_iPrimaryAmmoType, true);
		m_flNextRecharge = gpGlobals->curtime + RECHARGE_INTERVAL;
	}
#endif
#endif
	return BaseClass::Deploy();
}

void CGEWeaponWatchLaser::ItemPostFrame(void)
{
#ifdef GAME_DLL
#ifdef RECHARGE
	CGEMPPlayer *pOwner = ToGEMPPlayer(GetOwner());
	if (pOwner && m_flNextRecharge && m_flNextRecharge < gpGlobals->curtime)
	{
		m_flNextRecharge = gpGlobals->curtime + RECHARGE_INTERVAL;
		pOwner->GiveAmmo(4, m_iPrimaryAmmoType, true);

		if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) == AMMO_WATCHLASER_MAX)
			m_flNextRecharge = 0;
	}
#endif
#endif

	BaseClass::ItemPostFrame();
}

float CGEWeaponWatchLaser::GetRange(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return WATCHLASER_MAX_RANGE;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > MAX_RANGE_THRESH)
		return WATCHLASER_MAX_RANGE;
	else
		return WATCHLASER_MIN_RANGE;
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CGEWeaponWatchLaser::AddViewKick(void)
{
	return; // No watchlaser viewkick.
}

Activity CGEWeaponWatchLaser::GetDrawActivity(void)
{
	return ACT_VM_WATCH_DRAW;
}

void CGEWeaponWatchLaser::WeaponIdle(void)
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_WATCH_IDLE );
	}
}

// Have to check for ammo with watch laser.
void CGEWeaponWatchLaser::Swing(int bIsSecondary)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner || pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return;

	pOwner->RemoveAmmo(4, m_iPrimaryAmmoType);
	m_flNextRecharge = gpGlobals->curtime + POST_FIRE_CHARGE_DELAY;

	BaseClass::Swing(bIsSecondary);
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CGEWeaponWatchLaser::HandleAnimEventMeleeHit(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors(GetAbsAngles(), &vecDirection);

	Vector vecEnd;
	//	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	//	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
	//		Vector(-16,-16,-16), Vector(36,36,36), GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.75 );

	VectorMA(pOperator->Weapon_ShootPosition(), GetRange(), vecDirection, vecEnd);
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack(pOperator->Weapon_ShootPosition(), vecEnd, Vector(-16, -16, -16), Vector(36, 36, 36), GetGEWpnData().m_iDamage, DMG_CLUB);

	// did I hit someone?
	if (pHurt)
	{
		// play sound
		WeaponSound(MELEE_HIT);

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine(pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit);
		ImpactEffect(traceHit);
	}
	else
	{
		WeaponSound(MELEE_MISS);
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CGEWeaponWatchLaser::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_AR2:
		Swing(false);
		break;

	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit(pEvent, pOperator);
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif

void CGEWeaponWatchLaser::ImpactEffect(trace_t &traceHit)
{
	return; // Avoid doing melee impact effects for 5.0

	// See if we hit water (we don't do the other impact effects in this case)
	if (ImpactWater(traceHit.startpos, traceHit.endpos))
		return;

	if (traceHit.m_pEnt && traceHit.m_pEnt->MyCombatCharacterPointer())
		return;

	UTIL_ImpactTrace(&traceHit, DMG_SLASH);
}
