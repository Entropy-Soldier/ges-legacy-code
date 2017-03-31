//////////  Copyright © 2017, Goldeneye Source. All rights reserved. ///////////
// 
// ge_pickupitem.h
//
// Description:
//      baseclass for items meant to be picked up by the player.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_PICKUPITEM_H
#define GE_PICKUPITEM_H

#include "items.h"
#include "player.h"
#include "ge_shareddefs.h"

class CGEPickupItem : public CItem													
{																					
public:																				
	DECLARE_CLASS( CGEPickupItem, CItem );
	DECLARE_DATADESC();

	CGEPickupItem();
	~CGEPickupItem();

	// CBaseEntity functions
	virtual void Spawn();

	// CItem functions
	virtual void ItemTouch( CBaseEntity *pEntity );
	virtual bool MyTouch( CBasePlayer *pPlayer );

	virtual void Materialize();

	// Triggers a respawn and makes the crate appear instantly.
	virtual void RespawnNow( void );
};

#endif
