//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.h
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 3/22/2008 1200
// Created By: KillerMonkey
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_ARMORVEST_H
#define GE_ARMORVEST_H

#include "ge_pickupitem.h"

class CGEArmorVest : public CGEPickupItem
{																					
public:																				
	DECLARE_CLASS( CGEArmorVest, CGEPickupItem );
	DECLARE_DATADESC();

	CGEArmorVest();

	virtual void Spawn( void );																																			
	virtual void Precache( void );

	virtual void Materialize( void );
	virtual CBaseEntity *Respawn( void );
	virtual void RespawnThink( void );

	virtual bool MyTouch( CBasePlayer *pPlayer );

	int CalcSpawnProgress();
	void AddSpawnProgressMod(CBasePlayer *pPlayer, int amount);
	void ClearSpawnProgressMod(CBasePlayer *pPlayer);
	void ClearAllSpawnProgress();

	void DEBUG_ShowProgress(float duration, int progress);

	void SetAmount( int newAmount );
	void SetSpawnCheckRadius( int newRadius );

	int	m_iSpawnCheckRadius;
	int m_iSpawnCheckRadiusSqr;
	int m_iSpawnCheckHalfRadiusSqr;

	int m_iPlayerPointContribution[MAX_PLAYERS];

private:
	int		m_iAmount;
	int		m_iSpawnpoints;
	int		m_iSpawnpointsgoal;
};

#endif
