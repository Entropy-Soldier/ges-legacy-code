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

// The Last Laugh:  As the Baron, win a match without winning any of its constituent rounds.
class CAchTheLastLaugh : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		VarInit();
	}

	virtual void VarInit()
	{
		m_bHasWonRound = false;
		m_bIsChar = false;
		m_bWasCharForRound = false;
		m_bHasChangedThisMatch = false;
		m_bHasPlayedRoundAsChar = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "round_end" );
		ListenForGameEvent( "player_changeident" );
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if (!pPlayer)
			return;

		if ( !Q_stricmp(event->GetName(), "round_end") )
		{
			if ( !event->GetBool("showreport") )
				return;

			// If we won a round, we can't get the achievement this match.
			if ( event->GetBool("isfinal") )
			{ 
				if ( m_bHasPlayedRoundAsChar && !m_bHasWonRound && event->GetBool("isfinal") && event->GetInt("winnerid") == pPlayer->entindex() )
					IncrementCount();
			}
			else
			{
				if ( event->GetInt("winnerid") == pPlayer->entindex() )
					m_bHasWonRound = true;

				// Make sure we played at least one normal round as our desired character and didn't switch off afterwards.
				if ( m_bWasCharForRound )
					m_bHasPlayedRoundAsChar = true;
				else
					m_bHasPlayedRoundAsChar = false;
			}

			if (m_bIsChar)
				m_bWasCharForRound = true;
		}
		else if ( !Q_strcmp( event->GetName(), "player_changeident" ) )
		{
			// This is not us, ignore
			if ( pPlayer->entindex() != event->GetInt("playerid") )
				return;

			// See if we changed to our required character
			if ( !Q_stricmp( event->GetString("ident"), "samedi" ) )
			{
				// If this is our first character change for the match then we set our flag
				if ( !m_bHasChangedThisMatch )
					m_bWasCharForRound = true;

				m_bIsChar = true;
			}
			else
			{
				m_bWasCharForRound = false;
				m_bIsChar = false;
			}

			m_bHasChangedThisMatch = true;
		}
	}

	private:
		bool m_bHasWonRound;
		bool m_bHasPlayedRoundAsChar;
		bool m_bIsChar;
		bool m_bWasCharForRound;
		bool m_bHasChangedThisMatch;
};
DECLARE_GE_ACHIEVEMENT( CAchTheLastLaugh, ACHIEVEMENT_GES_THELASTLAUGH, "GES_THELASTLAUGH", 100, GE_ACH_UNLOCKED );

// Golden Goose: Kill the MWGG 5 times in one round without ever picking up the golden gun yourself.
class CAchGoldenGoose : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_bPickedUpGG = false;
		m_iCarrierKills = 0;
		m_iGGCarrierID = -1;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("round_start");
		ListenForGameEvent("gameplay_event");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if (!IsScenario("mwgg"))
			return;

		if ( !Q_stricmp(event->GetName(), "round_start") )
			VarInit(); // Fresh start for each new round.
		else if (m_bPickedUpGG)
			return;
		else if (!Q_stricmp(event->GetName(), "player_death"))
		{
			if (event->GetInt("attacker") == pPlayer->GetUserID())
			{
				if (event->GetInt("userid") == m_iGGCarrierID)
				{
					m_iCarrierKills++;

					if (m_iCarrierKills >= 5)
						IncrementCount();
				}
			}
		}
		else if (!Q_stricmp(event->GetName(), "gameplay_event"))
		{
			if (!Q_stricmp(event->GetString("name"), "mwgg_ggpickup"))
			{
				m_iGGCarrierID = Q_atoi(event->GetString("value1"));

				if ( m_iGGCarrierID == pPlayer->GetUserID() )
					m_bPickedUpGG = true;
			}
			else if (!Q_stricmp(event->GetString("name"), "mwgg_ggdropped"))
			{
				// Make sure the person who dropped the golden gun is the person
				// we think has it, to protect against drop/pickup arriving out of order.
				if ( m_iGGCarrierID == Q_atoi(event->GetString( "value1" )) )
					m_iGGCarrierID = -1;
			}
		}
	}

private:
	bool m_bPickedUpGG;
	int m_iCarrierKills;
	int m_iGGCarrierID;
};
DECLARE_GE_ACHIEVEMENT(CAchGoldenGoose, ACHIEVEMENT_GES_GOLDENGOOSE, "GES_GOLDENGOOSE", 100, GE_ACH_UNLOCKED);