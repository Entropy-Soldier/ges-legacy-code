///////////// Copyright © 2016, Goldeneye: Source. All rights reserved. /////////////
// 
// File: v5_achievements.cpp
// Description:
//      Goldeneye Achievements for Version 5.0
//
// Created On: 01 Oct 09
// Check github for contributors.
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "achievementmgr.h"
#include "util_shared.h"
#include "ge_achievement.h"

// Define our client's manager so that they can recieve updates when they achieve things
//#ifdef CLIENT_DLL

#include "clientmode_shared.h"
#include "c_ge_player.h"
#include "c_gemp_player.h"
#include "c_ge_playerresource.h"
#include "c_ge_gameplayresource.h"
#include "ge_weapon.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Torn Ambitions: In Gun Trade, remove any of the PP7s from play 100 times.
class CAchTornAmbitions : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(100);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "gt_weaponsub"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			// If we're the one that triggered the event, and the pp7 was swapped out, we did it!
			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && (!Q_strnicmp(event->GetString("value3"), "weapon_pp7", 10) || !Q_stricmp(event->GetString("value3"), "weapon_silver_pp7") || !Q_stricmp(event->GetString("value3"), "weapon_golden_pp7")) && IsScenario("guntrade", false))
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchTornAmbitions, ACHIEVEMENT_GES_TORNAMBITIONS, "GES_TORNAMBITIONS", 100, GE_ACH_UNLOCKED);


// Moon Landing:  Get a rocket launcher direct hit from 400 feet away.
class CAchMoonLanding : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the grenade launcher, got a direct hit, and the victim is over 300 feet(3600 inches) away, we did it!
		if (event->GetInt("weaponid") == WEAPON_ROCKET_LAUNCHER && event->GetBool("headshot") && event->GetInt("dist") > 3600)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchMoonLanding, ACHIEVEMENT_GES_MOONLANDING, "GES_MOONLANDING", 100, GE_ACH_UNLOCKED);
