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

bool CGEEntityTracker::AddItemToTracker( CBaseEntity *pEntity, char list /*== ET_LIST_ALL*/ )
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
		else if (!Q_strcmp(targetClassName, "item_armorvest"))
			targetlist = ET_LIST_ARMOR;
	}

	switch(targetlist) 
	{
		case ET_LIST_WEAPON : m_vWeaponList.AddToTail( (CGEWeapon*)pEntity ); break;
		case ET_LIST_AMMO : m_vAmmoList.AddToTail( (CGEAmmoCrate*)pEntity ); break;
		case ET_LIST_ARMOR : m_vArmorList.AddToTail( (CGEArmorVest*)pEntity ); break;
		case ET_LIST_MAP : m_vMapEntList.AddToTail( pEntity ); break;
		case ET_LIST_GAMEMODE : m_vGamemodeEntList.AddToTail( pEntity ); break;
		default: return false; // Invalid list.
	}

//	Warning("Added %s to item tracker, tracking count is now %d\n", pEntity->GetClassname(), m_vGamemodeEntList.Count() + m_vMapEntList.Count() + m_vArmorList.Count() + m_vAmmoList.Count() + m_vWeaponList.Count() );

	// Notify python of our newly tracked item.
	if ( GetScenario() )
		GetScenario()->OnItemTracked( pEntity,  targetlist );

	pEntity->NotifyOfTracking();
	return true;
}

bool CGEEntityTracker::RemoveItemFromTracker( CBaseEntity *pEntity, char list /* == ET_LIST_ALL */ )
{
	if (!pEntity)
	{
		DevWarning("Tried to remove non-existant entity from entity tracker!");
		return false;
	}

	// Notify Python about the item being removed from the tracker.
	if ( GetScenario() )
		GetScenario()->OnItemUntracked( pEntity,  list );

	// Success will increment for each successful search.
	int success = 0;
	int targetList = list;
	int targetIndex = -1;

	if (list == ET_LIST_ALL)
	{
		const char* targetClassName = pEntity->GetClassname();

		// Find our target list.
		if (!Q_strncmp(targetClassName, "weapon_", 7))
			targetList = ET_LIST_WEAPON;
		else if (!Q_strcmp(targetClassName, "ge_ammocrate"))
			targetList = ET_LIST_AMMO;
		else if (!Q_strcmp(targetClassName, "item_armorvest"))
			targetList = ET_LIST_ARMOR;

		targetIndex = m_vMapEntList.Find( pEntity ); targetIndex > -1 ? m_vMapEntList.FastRemove( targetIndex ) : success--; success++;
		targetIndex = m_vGamemodeEntList.Find( pEntity ); targetIndex > -1 ? m_vGamemodeEntList.FastRemove( targetIndex ) : success--; success++;
	}

	if (targetList == -1)
		return success > 0;

	switch(targetList) 
	{
		case ET_LIST_WEAPON: targetIndex = m_vWeaponList.Find((CGEWeapon*)pEntity); targetIndex > -1 ? m_vWeaponList.FastRemove(targetIndex) : success--; success++; break;
		case ET_LIST_AMMO : targetIndex = m_vAmmoList.Find( (CGEAmmoCrate*)pEntity ); targetIndex > -1 ? m_vAmmoList.FastRemove(targetIndex) : success--; success++; break;
		case ET_LIST_ARMOR : targetIndex = m_vArmorList.Find( (CGEArmorVest*)pEntity ); targetIndex > -1 ? m_vArmorList.FastRemove(targetIndex) : success--; success++; break;
		case ET_LIST_MAP : targetIndex = m_vMapEntList.Find( pEntity ); targetIndex > -1 ? m_vMapEntList.FastRemove(targetIndex) : success--; success++; break;
		case ET_LIST_GAMEMODE : targetIndex = m_vGamemodeEntList.Find( pEntity ); targetIndex > -1 ? m_vGamemodeEntList.FastRemove(targetIndex) : success--; success++; break;
	}

//	Warning("Removed %s from item tracker, tracking count is now %d\n", pEntity->GetClassname(), m_vGamemodeEntList.Count() + m_vMapEntList.Count() + m_vArmorList.Count() + m_vAmmoList.Count() + m_vWeaponList.Count() );

	// Successful if we found the entity to remove, 
	return success > 0;
}