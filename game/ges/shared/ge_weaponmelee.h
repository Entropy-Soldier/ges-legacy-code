///////////// Copyright � 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: weapon_gebasemelee.h
// Description:
//      see weapon_gebasemelee.cpp
//
// Created On: 3/19/2006 7:50:48 AM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_WEAPONMELEE
#define GE_WEAPONMELEE

#include "ge_weapon.h"
#include "gamestats.h"

#if defined( CLIENT_DLL )
#define CGEWeaponMelee C_GEWeaponMelee
#endif

class CGEWeaponMelee : public CGEWeapon
{
	DECLARE_CLASS( CGEWeaponMelee, CGEWeapon );
public:
	CGEWeaponMelee();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	//Attack functions
	virtual	void	PrimaryAttack( void );
//	virtual	void	SecondaryAttack( void );

	virtual void	FireWeapon();
	
	virtual void	ItemPostFrame( void );

	virtual void	AddViewKick( void );

#ifdef GAME_DLL
	virtual int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_MELEE_ATTACK1; }
	virtual int		WeaponMeleeAttack1Condition( float flDot, float flDist );
	virtual int		WeaponRangeAttack1Condition( float flDot, float flDist ) { return COND_NONE; }
#endif
	
	//Functions to select animation sequences 
	virtual Activity	GetPrimaryAttackActivity( void )	{	return	ACT_VM_HITCENTER;	}
	virtual Activity	GetSecondaryAttackActivity( void )	{	return	ACT_VM_HITCENTER2;	}

	virtual float	GetRange( bool modded = true )					{ return GetBaseRange() + (modded ? GetRangeOffset() : 0); }
    virtual float	GetBaseRange( void )							{ return 32.0f; }
	virtual	float	GetDamageForActivity( Activity hitActivity )	{ return GetWeaponDamage(); }
	virtual bool	DamageWorld()									{ return true; }
	virtual bool	SwingsInArc()									{ return true; }
	virtual int		GetStaticHitActivity()							{ return ACT_VM_MISSCENTER; }

	virtual void	MakeTracer(const Vector &vecTracerSrc, const trace_t &tr, int iTracerType)	{}; // Don't make tracers by default.

protected:
	virtual	void	ImpactEffect( trace_t &trace )					{}; // No impact effects by default.
	bool			ImpactWater( const Vector &start, const Vector &end );
	virtual void	Swing( int bIsSecondary );
	void			Hit( trace_t &traceHit );

private:
	CGEWeaponMelee( const CGEWeaponMelee & );
};

#endif //GE_WEAPONMELEE
