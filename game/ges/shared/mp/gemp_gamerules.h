///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: gemp_gamerules.h
// Description:
//      GoldenEye game rules Multiplayer version
//
// Created On: 28 Feb 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#ifndef GEMP_GAMERULES_H
#define GEMP_GAMERULES_H

#include "ge_gamerules.h"

#ifdef CLIENT_DLL
	#define CGEMPRules			C_GEMPRules
	#define CGEMPGameRulesProxy C_GEMPGameRulesProxy
	#include "c_gemp_player.h"
#else
	#include "ge_gameplay.h"
	#include "gemp_player.h"

	class CGETokenManager;
	class CGELoadoutManager;
	class CGEMapManager;
#endif

class CGEGameTimer;
class CGEMPGameRulesProxy;

// Teamplay desirability
enum GES_TEAMPLAY
{
	TEAMPLAY_NOT = 0,
	TEAMPLAY_ONLY,
	TEAMPLAY_TOGGLE
};

// Scoreboard modes
enum GES_SCOREBOARD_POINT_MODES
{
	SCOREBOARD_POINTS_STANDARD,
	SCOREBOARD_POINTS_TIME,
	SCOREBOARD_POINTS_LEVELS
};

class CGEMPRules : public CGERules, public CGEGameplayEventListener
{
public:
	CGEMPRules();
	~CGEMPRules();

	DECLARE_CLASS( CGEMPRules, CGERules );

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.
#else
	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif

	// ------------------------------
	// Server Only -- GES Functions
#ifdef GAME_DLL
	// Gameplay event listener
	virtual void OnGameplayEvent( GPEvent event );

	// Round Setup Interface
	void SetupRound();

protected:
	// Gameplay event handlers
	void OnScenarioInit();
	void OnMatchStart();
	void OnRoundStart();
	void OnRoundEnd();

public:
	// This is used to call functions right before the client spawns into the server
	virtual void ClientActive( CBasePlayer *pPlayer );
	virtual void CalculateCustomDamage( CBasePlayer *pPlayer, CTakeDamageInfo &info, float &health, float &armor );
	
	// Timer control
	void StartMatchTimer( float time_sec=-1 );
	void ChangeMatchTimer( float new_time_sec );
	void SetMatchTimerPaused( bool state );
	void StopMatchTimer();

	void SetRoundTimerEnabled( bool state );
	void StartRoundTimer( float time_sec=-1 );
	// Change total length of round timer, increase current time by difference between the new value and the old one.
	void ChangeRoundTimer( float new_time_sec, bool announce = true );
	// Set current and total value of round timer.
	void SetRoundTimer(float new_time_sec, bool announce = true);
	// Add time to current value of round timer.
	void AddToRoundTimer(float new_time_sec, bool announce = true );
	void SetRoundTimerPaused( bool state );
	void StopRoundTimer();
	
	// Just used to keep track of the highest score for now, but could potentially be expanded in the future.
	void RegisterPlayerScoreChange(CGEMPPlayer* player, int newScore);
	// Change score that must be reached to instantly end the match.
	void ChangeGoalScore( int new_score, bool team_score, bool announce = true );

    int GetGoalScore()  { return m_iGoalScore; }
    int GetTeamGoalScore()  { return m_iGoalTeamScore; }
    int GetHighestRoundScore()  { return m_iHighestRoundScore; }
    int GetHighestTeamRoundScore();

	void SetSpawnInvulnInterval(float duration);
	void SetSpawnInvulnCanBreak(bool canbreak);
	float GetSpawnInvulnInterval();
	bool GetSpawnInvulnCanBreak();

	void AddTrapToList(CBaseEntity* pEnt);
	void RemoveTrapFromList(CBaseEntity* pEnt);

	bool AmmoShouldRespawn();
	bool ArmorShouldRespawn();
	bool ItemShouldRespawn( const char *name );
	bool WeaponShouldRespawn( const char *name );

	CGELoadoutManager *GetLoadoutManager()  { return m_pLoadoutManager; }
	CGETokenManager   *GetTokenManager()	{ return m_pTokenManager;	}
	CGEMapManager	  *GetMapManager()		{ return m_pMapManager; }

	int	  GetSpawnPointType( CGEPlayer *pPlayer );
	float GetSpeedMultiplier( CGEPlayer *pPlayer );

	int GetNumActivePlayers();
	int GetNumAlivePlayers();
	int GetNumInRoundPlayers();

    // Gets the player with the highest score in the round.  In the event of a tie, one of the leader's IDs will be returned.
    void GetRoundLeaderAndScore( int &leaderID, int &leaderScore );

    // Gets the round winner according to the gamerules.
	int GetRoundWinner();
	int GetRoundTeamWinner();
	void SetRoundWinner( int winner )		{ m_iPlayerWinner = winner; }
	void SetRoundTeamWinner( int winner )	{ m_iTeamWinner = winner; }

	void GetRankSortedPlayers(CUtlVector<CGEMPPlayer*> &sortedplayers, bool matchRank = false);

	void ResetPlayerScores( bool resetmatch = false );
	void ResetTeamScores( bool resetmatch = false );

	void NotifyOfPromoSkinOffer() { m_bPromoStatus = true; };
	bool IsCurrentlyPromoSkinOffer() { return m_bPromoStatus; };

	void BalanceTeams();
	void EnforceTeamplay();
	void EnforceBotCount();
	void RemoveWeaponsFromWorld( const char *szClassName = NULL );

	void SetWeaponSpawnState( bool state )	{ m_bEnableWeaponSpawns = state; }
	void SetAmmoSpawnState( bool state )	{ m_bEnableAmmoSpawns = state; }
	void SetArmorSpawnState( bool state )	{ m_bEnableArmorSpawns = state; }

	void SetSuperfluousAreasState(bool state)	{ m_bAllowSuperfluousAreas = state; }
	bool SuperfluousAreasEnabled()				{ return m_bAllowSuperfluousAreas; }

	// These accessors control the use of team spawns (if available and team play enabled)
	bool IsTeamSpawn()				{ return m_bUseTeamSpawns; }
	bool IsTeamSpawnSwapped()		{ return m_bSwappedTeamSpawns; }
	void SwapTeamSpawns()			{ m_bSwappedTeamSpawns = !m_bSwappedTeamSpawns; }
	void SetTeamSpawn( bool state )	{ m_bUseTeamSpawns = state; }

	// Used by Python to set the teamplay mode
	void SetTeamplay( bool state, bool force = false );
	void SetTeamplayMode( int mode ) { m_iTeamplayMode = mode; }

	// Static functions for bots
	static bool CreateBot();
	static bool RemoveBot( const char *name );
#endif

	// ------------------------------
	// Client-Server Shared
public:
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

	// Match Timing
	bool  IsMatchTimeRunning();
	bool  IsMatchTimePaused();
	float GetMatchTimeRemaining();
	float GetTimeSinceMatchStart();

	// Round Timing
	bool  IsRoundTimeEnabled();
	bool  IsRoundTimeRunning();
	bool  IsRoundTimePaused();
	float GetRoundTimeRemaining();
	float GetTimeSinceRoundStart();

	bool  AllowXMusic() { return m_bAllowXMusic; };
	void  SetAllowXMusic( bool newstate ) { m_bAllowXMusic = newstate; };

	void SetShouldShowHUDTimer( bool newState );
	bool ShouldShowHUDTimer();

	int   GetTeamplayMode() { return m_iTeamplayMode; }

    // Weaponmods allow individual weapon stats to be changed because it's fun okay.
    // However, they obviously break a lot of assumptions made by acheivements and general player expectations and
    // as such need to be clearly advertised.  When enabled they modify the gamemode title's displayed color and turn off
    // acheivements.  Do not allow weaponmods to be disabled by anything other than a gamemode reload as this would allow circumvention
    // of some of these protective measures.  It's like cheats.
    void  EnableWeaponMods()  { m_bWeaponModsEnabled = true; } 
	bool  WeaponModsEnabled()  { return m_bWeaponModsEnabled; };

	void  SetGlobalInfAmmoState( bool newstate )  { m_bGlobalInfAmmo = newstate; };
	void  SetGamemodeInfAmmoState( bool newstate )  { m_bGamemodeInfAmmo = newstate; };
	bool  InfAmmoEnabled()  { return m_bGlobalInfAmmo || m_bGamemodeInfAmmo; };

	int  GetScoreboardMode()  { return m_iScoreboardMode; };
	void  SetScoreboardMode( int newmode )  { m_iScoreboardMode = newmode; };

    int  GetTeamScoreboardMode()  { return m_iTeamScoreboardMode; };
	void  SetTeamScoreboardMode( int newmode )  { m_iTeamScoreboardMode = newmode; };

	int  GetScorePerScoreLevel()  { return m_iScoreboardScorePerLevel; };
	void  SetScorePerScoreLevel( int newscorethresh )  { m_iScoreboardScorePerLevel = newscorethresh; };

	bool IsMultiplayer() { return true; }
	bool IsTeamplay();
	bool IsIntermission();

	void SetMapFloorHeight(float height);
	float GetMapFloorHeight();

	int   GetRandomSeedOffset() { return m_iRandomSeedOffset; }

	int	  GetSpecialEventCode() { return m_iAwardEventCode; };
	void  SetSpecialEventCode(int newcode)  { m_iAwardEventCode = newcode; };

	// ------------------------------
	// Server Only -- Inherited Functions
#ifdef GAME_DLL
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void CreateStandardEntities();
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );

	virtual float FlItemRespawnTime( CItem *pItem );
	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon );
	virtual bool  FPlayerCanRespawn( CBasePlayer *pPlayer );

	virtual const char *GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer );
	virtual const char *GetGameDescription();
	virtual void		GetTaggedConVarList( KeyValues *pCvarTagList );

	// Get the data associated with a player's particular account on the GE:S online database.
	// First attempts to retreive the data from the server's webinfo cache, and if that fails it queries the
	// online database itself.  Will fire the OnWebDataRetreived callback on the target player object when successful.
	// Will fire OnWebDataRetreivalFromWebServerFailed if not successful.
	virtual void GetPlayerWebData( CGEMPPlayer *pPlayer );

	virtual void HandleTimeLimitChange( void );

	virtual void SetupChangeLevel( const char *next_level = NULL );
	virtual void GoToIntermission();

	virtual bool OnPlayerSay(CBasePlayer* player, const char* text);
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	
	// Used to initialize Python managers. They are shutdown when GameRules is destroyed
	virtual void LevelInitPreEntity();

	virtual void FrameUpdatePreEntityThink();
	virtual void Think();

	// Make sure we don't affect the "frag" count (fav weapon)
	bool UseSuicidePenalty() { return false; }
	
	virtual Vector	VecItemRespawnSpot( CItem *pItem );
	virtual QAngle	VecItemRespawnAngles( CItem *pItem );
#endif


private:

#ifdef GAME_DLL
	virtual void ChangeLevel();

	CGELoadoutManager	*m_pLoadoutManager;
	CGETokenManager		*m_pTokenManager;
	CGEMapManager		*m_pMapManager;

	ConVar				*m_pAllTalkVar;

	bool m_bEnableAmmoSpawns;
	bool m_bEnableWeaponSpawns;
	bool m_bEnableArmorSpawns;

	bool m_bAllowSuperfluousAreas;

	float m_flIntermissionEndTime;
	float m_flNextTeamBalance;
	float m_flNextIntermissionCheck;
	float m_flChangeLevelTime;

	float m_flSpawnInvulnDuration;
	bool m_bSpawnInvulnCanBreak;

	float m_flNextBotCheck;
	CUtlVector<EHANDLE> m_vBotList;

	CUtlVector<CBaseEntity*> m_vTrapList; //List of traps that needs to be checked when a potential trap owner disconnects.

	char  m_szNextLevel[64];
	char  m_szGameDesc[32];

	bool  m_bPromoStatus;

	bool  m_bUseTeamSpawns;
	bool  m_bSwappedTeamSpawns;
	bool  m_bInTeamBalance;

	int	  m_iPlayerWinner;
	int   m_iTeamWinner;

    // ID of player responsible for the current m_iHighestScore value.
    int	  m_iHighestScoreHolderID;

	// Per frame update variables
	// -- updates on first call to the count functions per frame
	// Player counts 
	int   m_iNumAlivePlayers;
	int   m_iNumActivePlayers;
	int   m_iNumInRoundPlayers;
#endif

	CNetworkVar( int, m_iRandomSeedOffset );
	CNetworkVar( float, m_flMapFloorHeight );
	CNetworkVar( bool,	m_bTeamPlayDesired );
	CNetworkVar( bool,  m_bGlobalInfAmmo );
	CNetworkVar( bool,  m_bGamemodeInfAmmo );
	CNetworkVar( bool,  m_bAllowXMusic );
	CNetworkVar( bool,  m_bShouldShowHUDTimer );
	CNetworkVar( int,	m_iHighestRoundScore );
    CNetworkVar( int,	m_iHighestTeamRoundScore );
	CNetworkVar( int,	m_iGoalScore );
    CNetworkVar( int,	m_iGoalTeamScore );
	CNetworkVar( int,	m_iTeamplayMode );
	CNetworkVar( int, m_iScoreboardMode );
	CNetworkVar( int, m_iTeamScoreboardMode );
	CNetworkVar( int, m_iScoreboardScorePerLevel );
	CNetworkVar( int, m_iAwardEventCode );

    CNetworkVar( bool,  m_bWeaponModsEnabled );

	CNetworkVar( float, m_flRoundStartTime );
	CNetworkVar( float, m_flMatchStartTime );

	CNetworkHandle( CGEGameTimer, m_hMatchTimer );
	CNetworkHandle( CGEGameTimer, m_hRoundTimer );
};


class CGEMPGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CGEMPGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

extern int g_iLastPlayerCount;

inline CGEMPRules* GEMPRules()
{
	return (CGEMPRules*) g_pGameRules;
}

#endif //MC_GEMP_GAMERULES_H
