#ifndef HL2_RESERVESPOT_H
#define HL2_RESERVESPOT_H
#pragma once

#include "cbase.h"

// Not my first choice to move this to its own special file...but it needs to be accessible from elsewhere to easily delete its instances.

// This is a simple helper class to reserver a player sized hull at a spot, owned by the current player so that nothing
// can move into this spot and cause us to get stuck when we get there
class CReservePlayerSpot : public CBaseEntity
{
	DECLARE_CLASS( CReservePlayerSpot, CBaseEntity )
public:
	static CReservePlayerSpot *ReserveSpot( CBasePlayer *owner, const Vector& org, const Vector& mins, const Vector& maxs, bool& validspot );

	virtual void Spawn();
};

#endif