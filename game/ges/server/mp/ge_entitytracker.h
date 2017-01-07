//////////  Copyright © 2017, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_entitytracker.h
// Description:
//      Stores a list of important entities for quick access by relevant systems.
//		The main distinction between this and the entity list is that this is for gameplay relevant entities such as armor
//		and ammo that need functions called on every member of the class on a regular basis.  This is why they are all sorted by class.
//		It is also for special map entities, marked by the map, that gamemodes may want to interact with.
//
//		The lists are not expected to change often but are expected to be accessed frequently by certain systems.
//		To this end the tracker is set up to make these operations as quick as possible.
//
//		Adding a new entity type to the tracker should be pretty straightforward, 
//		just copy the code for one of the existing entities.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_ENTITYTRACKER_H
#define GE_ENTITYTRACKER_H

#include "ge_gameplay.h"
#include "ge_loadout.h"
#include "ge_spawner.h"
#include "ge_ammocrate.h"
#include "ge_triggers.h"
#include "ge_door.h"

// Find and add to relevant lists, search all lists, and remove from all lists.
#define ET_LIST_ALL 0

// Add, remove, and search using weapon list.
#define ET_LIST_WEAPON 1

// Add, remove, and search using ammo list.
#define ET_LIST_AMMO 2

// Add, remove, and search using armor list.
#define ET_LIST_ARMOR 3

// Add, remove, and search using gamemode list.
#define ET_LIST_GAMEPLAY 4

// Add, remove, and search using map list.
#define ET_LIST_MAP 10

// Add, remove, and search using gamemode list.
#define ET_LIST_GAMEMODE 11


class CGEEntityTracker
{
public:
	DECLARE_CLASS_NOBASE( CGEEntityTracker );

	CGEEntityTracker(void);
	~CGEEntityTracker(void);

	// Completely removes an entity from the given tracking lists and lets it know it's no longer being tracked.
	// ET_LIST_ALL will remove it from all lists.
	bool RemoveItemFromTracker( CBaseEntity *pEntity, char list = ET_LIST_ALL );

	// Adds an entity to the relevant list in the tracker and lets the entity know it's being tracked.
	// BE SURE TO SPECIFY THE CORRECT LIST IF NOT USING ET_LIST_ALL.
	bool AddItemToTracker( CBaseEntity *pEntity, char list = ET_LIST_ALL );

	// Passes a pointer to the list of tracked weapons, to avoid potentially copying a gigantic list a bunch of times.
	// DO NOT MANUALLY ADD ITEMS TO THIS.  Use AddItemToTracker and RemoveItemFromTracker to change it.
	CUtlVector< CGEWeapon* >* GetWeaponList() { return &m_vWeaponList; };

	// Passes a pointer to the list of tracked ammo crates, to avoid potentially copying a gigantic list a bunch of times.
	// DO NOT MANUALLY ADD ITEMS TO THIS.  Use AddItemToTracker and RemoveItemFromTracker to change it.
	CUtlVector< CGEAmmoCrate* >* GetAmmoList() { return &m_vAmmoList; };

	// Passes a pointer to the list of tracked armor vests, to avoid potentially copying a gigantic list a bunch of times.
	// DO NOT MANUALLY ADD ITEMS TO THIS.  Use AddItemToTracker and RemoveItemFromTracker to change it.
	CUtlVector< CGEArmorVest* >* GetArmorList() { return &m_vArmorList; };

	// Passes a pointer to the list of tracked armor vests, to avoid potentially copying a gigantic list a bunch of times.
	// DO NOT MANUALLY ADD ITEMS TO THIS.  Use AddItemToTracker and RemoveItemFromTracker to change it.
	CUtlVector< CBaseEntity* >* GetGamemodeEntList() { return &m_vGamemodeEntList; };

	// Passes a pointer to the list of tracked armor vests, to avoid potentially copying a gigantic list a bunch of times.
	// DO NOT MANUALLY ADD ITEMS TO THIS.  Use AddItemToTracker and RemoveItemFromTracker to change it.
	CUtlVector< CBaseEntity* >* GetMapEntList() { return &m_vMapEntList; };

private:
	// Entities tracked by the gamemode, any entity created by the gamemode goes in this list.
	// This way entities that should persist between rounds but not gamemodes can exist.
	CUtlVector< CBaseEntity* > m_vGamemodeEntList;

	// Items tracked by the map, for access by the gamemode.  Will not get deleted or untracked when the gamemode is changed.
	// for nearly all items this will happen anyway though since round change also reloads map ents.
	CUtlVector< CBaseEntity* > m_vMapEntList;


	// Entity Specific Lists, are all mutually exclusive.

	// All tracked armor vests.
	CUtlVector< CGEArmorVest* > m_vArmorList;

	// All tracked weapons.
	CUtlVector< CGEWeapon* > m_vWeaponList;

	// All tracked ammo crates.
	CUtlVector< CGEAmmoCrate* > m_vAmmoList;
};

extern void CreateEntityTracker();
extern CGEEntityTracker* GEEntityTracker();

#endif //GE_ENTITYTRACKER_H

