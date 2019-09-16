//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_entitytracker.cpp
// Description: Contains easily accessible lists of various entity types.
//
///////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_entitytracker.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CGEEntityTracker::CGEEntityTracker(void)
{
}

CGEEntityTracker::~CGEEntityTracker(void)
{
}


CGEEntityTracker *g_GEEntityTracker = NULL;

CGEEntityTracker *GEEntityTracker()
{ 
	return g_GEEntityTracker;
}

void CreateEntityTracker()
{
	if ( GEEntityTracker() )
	{
		Warning( "Tried to make Entity Tracker when it already exists!\n" );
		return;
	}

	g_GEEntityTracker = new CGEEntityTracker;

	if ( !GEEntityTracker() )
		AssertFatal( "Failed to create entity tracker!" );
}

void DestroyEntityTracker()
{
	delete g_GEEntityTracker;
	g_GEEntityTracker = NULL;
}

bool CGEEntityTracker::AddItemToTracker( CBaseEntity *pEntity, char reason, char list /*== ET_LIST_ALL*/ )
{
	if (!pEntity)
	{
		DevWarning("Tried to add non-existant entity to entity tracker!");
		return false;
	}

	const char* targetClassName = pEntity->GetClassname();
	int targetlist = list;

	if (list == ET_LIST_ALL)
	{
		if (!Q_strncmp(targetClassName, "weapon_", 7))
			targetlist = ET_LIST_WEAPON; // There are no entities that start with weapon_ that are not CGEWeapons.
		else if (!Q_strcmp(targetClassName, "ge_ammocrate"))
			targetlist = ET_LIST_AMMO;
		else if (!Q_strcmp(targetClassName, "ge_armorvest"))
			targetlist = ET_LIST_ARMOR;
	}

	CUtlVector< CBaseEntity* >* targetlistPointer = NULL;

	switch(targetlist) 
	{
		case ET_LIST_WEAPON : targetlistPointer = &m_vWeaponList; break;
		case ET_LIST_AMMO : targetlistPointer = &m_vAmmoList; break;
		case ET_LIST_ARMOR : targetlistPointer = &m_vArmorList; break;
		case ET_LIST_MAP : targetlistPointer = &m_vMapEntList; break;
		case ET_LIST_GAMEMODE : targetlistPointer = &m_vGamemodeEntList; break;
		default: return false; // Invalid list.
	}

	// targetlistPointer MUST be initalized at this point, otherwise we would have returned already.
	if ( !targetlistPointer->HasElement(pEntity) )
	{
		targetlistPointer->AddToTail(pEntity);
	}
	else // If we track the same entity twice, our removal operations become more complicated.
	{
		Warning("Tried to add entity to entity tracker when it was already tracked!\n");
		return false;
	}

	//Warning("Added %s with pointer %x to list %d in item tracker, tracking count is now %d\n", pEntity->GetClassname(), pEntity, targetlist, m_vGamemodeEntList.Count() + m_vMapEntList.Count() + m_vArmorList.Count() + m_vAmmoList.Count() + m_vWeaponList.Count() );

	// Notify python of our newly tracked item.
	if ( GetScenario() )
		GetScenario()->OnItemTracked( pEntity, reason, targetlist );

	pEntity->NotifyOfTracking();
	return true;
}

bool CGEEntityTracker::RemoveItemFromTracker( CBaseEntity *pEntity, char reason, char list /* == ET_LIST_ALL */ )
{
	if (!pEntity)
	{
		DevWarning("Tried to remove non-existant entity from entity tracker!");
		return false;
	}

	// Success will increment for each successful search.
	int success = 0;
	int targetlist = list;
	int targetIndex = -1;

	if (list == ET_LIST_ALL)
	{
		const char* targetClassName = pEntity->GetClassname();

		// Find our target list.
		if (!Q_strncmp(targetClassName, "weapon_", 7) || !Q_strncmp(targetClassName, "token_", 6))
			targetlist = ET_LIST_WEAPON;
		else if (!Q_strcmp(targetClassName, "ge_ammocrate"))
			targetlist = ET_LIST_AMMO;
		else if (!Q_strcmp(targetClassName, "ge_armorvest"))
			targetlist = ET_LIST_ARMOR;

		targetIndex = m_vMapEntList.Find( pEntity ); targetIndex > -1 ? m_vMapEntList.FastRemove( targetIndex ) : success--; success++;
		targetIndex = m_vGamemodeEntList.Find( pEntity ); targetIndex > -1 ? m_vGamemodeEntList.FastRemove( targetIndex ) : success--; success++;
	}

	// No list to target, but we may have removed the entity from our map or gamemode lists.
	if ( targetlist == -1 )
		return success > 0;

	CUtlVector< CBaseEntity* >* targetlistPointer = NULL;

	switch(targetlist) 
	{
		case ET_LIST_WEAPON : targetlistPointer = &m_vWeaponList; break;
		case ET_LIST_AMMO : targetlistPointer = &m_vAmmoList; break;
		case ET_LIST_ARMOR : targetlistPointer = &m_vArmorList; break;
		case ET_LIST_MAP : targetlistPointer = &m_vMapEntList; break;
		case ET_LIST_GAMEMODE : targetlistPointer = &m_vGamemodeEntList; break;
		default: return false; // Invalid list.
	}

	targetIndex = targetlistPointer->Find( pEntity ); 
	
	if (targetIndex > -1)
	{
		targetlistPointer->FastRemove(targetIndex);
		success++;
	}

	// Notify Python about the item being removed from the tracker.
	if ( success > 0 && GetScenario() )
	{
		//Warning("Removed %s with pointer %x from list %d in item tracker, tracking count is now %d\n", pEntity->GetClassname(), pEntity, targetlist, m_vGamemodeEntList.Count() + m_vMapEntList.Count() + m_vArmorList.Count() + m_vAmmoList.Count() + m_vWeaponList.Count() );
		GetScenario()->OnItemUntracked(pEntity, reason, list);
	}

	// Successful if we found the entity to remove, 
	return success > 0;
}