///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weapon.h
// Description:
//      All Goldeneye Weapons derive from this class.
//
// Created On: 2/25/2008 1800
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_WEAPON_H
#define GE_WEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "ge_weapon_parse.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
	#include "ge_screeneffects.h"
#else
	#include "ge_player.h"
	#include "ai_basenpc.h"
#endif

#if defined( CLIENT_DLL )
	#define CGEWeapon C_GEWeapon
#endif

typedef enum
{
	Primary_Mode = 0,
	Secondary_Mode,
} GEWeaponMode;


class CGEWeapon : public CBaseHL2MPCombatWeapon
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS( CGEWeapon, CBaseHL2MPCombatWeapon );

public:
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CGEWeapon();
	
#ifdef GAME_DLL
	~CGEWeapon();
#endif

	virtual void	Precache();

	virtual GEWeaponID GetWeaponID() const { return WEAPON_NONE; }

	// For Python and NPC's
	virtual bool	IsExplosiveWeapon() { return false; }
	virtual bool	IsAutomaticWeapon() { return false; }
	virtual bool	IsThrownWeapon()	{ return false; }
	virtual bool	HasFireDelay()		{ return GetFireDelay() != 0.0f; }

	virtual void	PrimaryAttack();
	virtual void	FireWeapon();
	virtual bool	Reload();
	virtual void	OnReloadOffscreen();
	virtual void	DryFire();
	virtual void	PrepareFireBullets(int number, CBaseCombatCharacter *pOperator, Vector vecShootOrigin, Vector vecShootDir, bool haveplayer);
	virtual void	PreOwnerDeath(); //Fired when owner dies but before anything else.

#ifdef GAME_DLL
	virtual void	Spawn();
	virtual void	UpdateOnRemove();
	virtual int		UpdateTransmitState();

	virtual float	GetHeldTime() { return gpGlobals->curtime - m_flDeployTime; };
	virtual bool	CanEquip( CBaseCombatCharacter *pOther ) { return true; };

	virtual float	GetSecondsUntilPickup() { return (m_flPickupAllowedTime > gpGlobals->curtime) ? (m_flPickupAllowedTime - gpGlobals->curtime) : 0; }
	virtual void	SetSecondsUntilPickup( float timeDelay ) { if (timeDelay <= 0) m_flPickupAllowedTime = 0; else m_flPickupAllowedTime = gpGlobals->curtime + timeDelay; }
	virtual void	SetRoundSecondsUntilPickup( float timeDelay );

	// NPC Functions
	virtual void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual void 	DefaultTouch( CBaseEntity *pOther );

	virtual void	SetupGlow( bool state, Color glowColor = Color(255,255,255), float glowDist = 350.0f );

	Vector	GetOriginalSpawnOrigin( void ) { return m_vOriginalSpawnOrigin;	}
	QAngle	GetOriginalSpawnAngles( void ) { return m_vOriginalSpawnAngles;	}

    CGEPlayer *GetOriginalOwner( void );

    // Token Related Functions

    // Gets the value that determines the order token weapons are removed by the limit enforcer.
    unsigned int GetLimitEnforcementPriority()                        { return m_iLimitEnforcementPriority; };
    // Sets the value that determines the order token weapons are removed by the limit enforcer.
    void         SetLimitEnforcementPriority( unsigned int priority ) { m_iLimitEnforcementPriority = priority; };
#endif

	// Get GE weapon specific weapon data.
	// Get a pointer to the player that owns this weapon
	CGEWeaponInfo const	&GetGEWpnData() const;

	virtual float			GetFireRate( bool modded = true );
	virtual float			GetClickFireRate( bool modded = true );
    virtual int				GetMaxClip1( bool modded = true );
	virtual int				GetMaxClip2( bool modded = true );
	virtual const Vector&	GetBulletSpread( void );
	virtual int				GetGaussFactor( void );
	virtual float			GetFireDelay( void );
	virtual int				GetTracerFreq( void ) { return GetGEWpnData().m_iTracerFreq; };
	virtual const char*		GetSpecAttString(void) { return GetGEWpnData().m_szSpecialAttributes; };

	virtual int		GetDamageCap( bool modded = true ) { return GetGEWpnData().m_iDamageCap + (modded ? m_iDamageCapOffset : 0); };

	virtual float	GetMaxPenetrationDepth( bool modded = true ) { return GetGEWpnData().m_flMaxPenetration + (modded ? m_flPenetrationOffset : 0.0f); };
	virtual void	AddAccPenalty(float numshots);
	virtual void	UpdateAccPenalty( void );
	virtual float	GetAccPenalty(void) { return m_flAccuracyPenalty; };

    virtual Vector GetMinSpreadVec(bool modded = true) { return GetGEWpnData().m_vecSpread + (modded ? m_vMinSpreadOffset : Vector(0, 0, 0));  }
    virtual Vector GetMaxSpreadVec(bool modded = true) { return GetGEWpnData().m_vecMaxSpread + (modded ? m_vMaxSpreadOffset : Vector(0, 0, 0));  }

    virtual float GetAimBonus(bool modded = true) { return GetGEWpnData().m_flAimBonus + (modded ? m_flAimBonusOffset : 0.0f);  }
    virtual float GetJumpPenalty(bool modded = true) { return GetGEWpnData().m_flJumpPenalty + (modded ? m_flJumpPenaltyOffset : 0.0f);  }

	// Fixes for NPC's
	virtual bool	HasAmmo( void );
	virtual bool	HasShotsLeft( void );
	virtual bool	IsWeaponVisible( void );

	virtual void	ItemPreFrame( void );
	virtual void	ItemPostFrame( void );

	virtual void	Equip( CBaseCombatCharacter *pOwner );
	virtual void	Drop( const Vector &vecVelocity );
	virtual	bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );



	virtual void	SetViewModel();
	virtual void	SetSkin( int skin );
	virtual int		GetSkin()						{ return (int)m_nSkin; }

	virtual int		GetBodygroupFromName(const char* name);
	virtual void	SwitchBodygroup( int group, int value );
	
	virtual void	EnableAlternateViewmodel( bool enabled ) {}; // Does nothing on most weapons.

	virtual void	SetPickupTouch( void );
	virtual void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	virtual float	GetWeaponDamage( bool modded = true )    { return GetGEWpnData().m_iDamage + (modded ? m_iDamageOffset : 0); }
	virtual float	GetHitBoxDataModifierHead()		{ return GetGEWpnData().HitBoxDamage.fDamageHead;	}
	virtual float	GetHitBoxDataModifierChest()	{ return GetGEWpnData().HitBoxDamage.fDamageChest;	}
	virtual float	GetHitBoxDataModifierStomach()	{ return GetGEWpnData().HitBoxDamage.fDamageStomach;}
	virtual float	GetHitBoxDataModifierLeftArm()	{ return GetGEWpnData().HitBoxDamage.fLeftArm;		}
	virtual float	GetHitBoxDataModifierRightArm() { return GetGEWpnData().HitBoxDamage.fRightArm;		}
	virtual float	GetHitBoxDataModifierLeftLeg()	{ return GetGEWpnData().HitBoxDamage.fLefLeg;		}
	virtual float	GetHitBoxDataModifierRightLeg() { return GetGEWpnData().HitBoxDamage.fRightLeg;		}

    virtual float	GetWeaponDamageRadius( bool modded = true )	{ return GetGEWpnData().m_flDamageRadius + (modded ? m_flBlastRadiusOffset : 0.0f); }

    virtual float	GetWeaponPushForceMult( bool modded = true ) { return 1.0f + (modded ? m_flPushForceMultOffset : 0.0f); }

    virtual int     GetWeaponSoundPitchShift() { return m_iWeaponSoundPitchOffset; }
    virtual float   GetWeaponSoundVolumeShift() { return m_flWeaponSoundVolumeOffset; }

	virtual bool	CanBeSilenced( void ) { return false; };
	virtual bool	IsSilenced() { return m_bSilenced; };
	virtual bool	IsShotgun() { return false; };
	virtual void	ToggleSilencer( bool doanim = true );
	virtual void	SetAlwaysSilenced( bool set ) { m_bIsAlwaysSilent = set; };
	virtual bool	IsAlwaysSilenced() { return m_bIsAlwaysSilent; };
	virtual float	GetAccFireRate( bool modded = true ) { return GetGEWpnData().m_flAccurateRateOfFire + (modded ? m_flAccFireRateOffset : 0.0f); }
	virtual int		GetAccShots( bool modded = true ){ return GetGEWpnData().m_flAccurateShots + (modded ? m_iAccShotsOffset : 0); }

	virtual int		GetTracerAttachment( void );

#ifdef CLIENT_DLL
	virtual float CalcViewmodelBob( void );
	virtual void AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void UpdateVisibility();
#endif

	virtual bool	 PlayEmptySound( void );

	virtual Activity GetPrimaryAttackActivity( void );
	virtual Activity GetDrawActivity( void );
	virtual void	 WeaponIdle( void );

	virtual float GetZoomOffset( bool modded = true );

    virtual bool CanUseWeaponMods();

	// NPC Functions
	virtual float GetMinRestTime() { return GetClickFireRate(); }
	virtual float GetMaxRestTime() { return GetFireRate(); }

    virtual void SetOriginalOwnerID(int val) { m_iOriginalOwnerID = val; }
    virtual int  GetOriginalOwnerID() { return m_iOriginalOwnerID; }

	// Variable that stores our accuracy penalty time limit
	CNetworkVar( float,	m_flAccuracyPenalty );
	CNetworkVar( float, m_flCoolDownTime );

    CNetworkVar( int, m_iOriginalOwnerID );
    
    // Stat modifiers.
    // Adding a new variable to here naturally requires updating the network tables and
    // ge_weapon constructor with the new value.
#define GEWeaponStatModVar( type, prefix, name ) \
    type Get ## name ## ( void ) { return prefix ## name; } \
    void Set ## name ## ( type newVal ) { if (CanUseWeaponMods()) prefix ## name = newVal; else Warning("Gamemode tried to use weapon mods without enabling them!\n"); } \
    CNetworkVar( type, prefix ## name )

    GEWeaponStatModVar( int, m_i, DamageOffset );
    GEWeaponStatModVar( int, m_i, DamageCapOffset );
    GEWeaponStatModVar( float, m_fl, FireRateOffset );
    GEWeaponStatModVar( float, m_fl, ClickFireRateOffset );
    GEWeaponStatModVar( float, m_fl, AccFireRateOffset );

    GEWeaponStatModVar( Vector, m_v, MinSpreadOffset );
    GEWeaponStatModVar( Vector, m_v, MaxSpreadOffset );
    GEWeaponStatModVar( float, m_fl, JumpPenaltyOffset );
    GEWeaponStatModVar( float, m_fl, AimBonusOffset );

    GEWeaponStatModVar( int, m_i, MaxClip1Offset );
    GEWeaponStatModVar( int, m_i, AccShotsOffset );

    GEWeaponStatModVar( int, m_i, ZoomOffsetOffset );

    GEWeaponStatModVar( float, m_fl, BlastRadiusOffset );
    GEWeaponStatModVar( float, m_fl, RangeOffset );

    GEWeaponStatModVar( float, m_fl, PushForceMultOffset );

    GEWeaponStatModVar( float, m_fl, PenetrationOffset );

    GEWeaponStatModVar( int, m_i, WeaponSoundPitchOffset );
    GEWeaponStatModVar( float, m_fl, WeaponSoundVolumeOffset );

    // Returns the custom print name if set, otherwise returns the standard one.
    const char *GetCustomPrintName(void);
    void SetCustomPrintName( char *newVal ) { if (CanUseWeaponMods()) Q_strncpy(m_sPrintNameCustom.GetForModify(), newVal, 32); else Warning("Gamemode tried to use weapon mods without enabling them!\n"); }
    CNetworkString( m_sPrintNameCustom, 32 );

protected:
	bool			m_bLowered;			// Whether the viewmodel is raised or lowered
	float			m_flRaiseTime;		// If lowered, the time we should raise the viewmodel
	float			m_flHolsterTime;	// When the weapon was holstered

	// Used to control the barrel smoke (we don't care about synching these it is not necessary)
	void			RecordShotFired(int count = 1);
	float			m_flSmokeRate;
	int				m_iShotsFired;
	float			m_flLastShotTime;

	CNetworkVar( float, m_flShootTime );

private:
#ifdef GAME_DLL
	void SetEnableGlow( bool state );
	bool m_bServerGlow;
    unsigned int m_iLimitEnforcementPriority;
#else
	CEntGlowEffect *m_pEntGlowEffect;
	bool m_bClientGlow;
	float m_flLastBobCalc;
	float m_flLastSpeedrat;
	float m_flLastFallrat;
#endif

	CNetworkVar( bool, m_bEnableGlow );
	CNetworkVar( color32, m_GlowColor );
	CNetworkVar( float, m_GlowDist );
	CNetworkVar( bool,	m_bSilenced );
	bool m_bIsAlwaysSilent;

#ifdef GAME_DLL
	Vector			m_vOriginalSpawnOrigin;
	QAngle			m_vOriginalSpawnAngles;

	float			m_flDeployTime;

	float			m_flPickupAllowedTime;
#endif

	CGEWeapon( const CGEWeapon & );
};

inline CGEWeapon *ToGEWeapon( CBaseCombatWeapon *pWeapon )
{
#ifdef _DEBUG
	return dynamic_cast<CGEWeapon*>(pWeapon);
#else
	return static_cast<CGEWeapon*>(pWeapon);
#endif
}

#endif // WEAPON_GEBASE_H
