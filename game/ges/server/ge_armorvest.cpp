//////////  Copyright � 2008, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.cpp
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 3/22/2008 1200
// Created By: KillerMonkey
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_armorvest.h"
#include "ge_shareddefs.h"
#include "gemp_gamerules.h"
#include "ge_player.h"
#include "gebot_player.h"
#include "ge_entitytracker.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( ge_armorvest, CGEArmorVest );
PRECACHE_REGISTER( ge_armorvest );

BEGIN_DATADESC( CGEArmorVest )
	DEFINE_THINKFUNC( RespawnThink ),
END_DATADESC();

ConVar ge_armorrespawntime("ge_armorrespawntime", "14", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum time in seconds before armor respawns.");
ConVar ge_armorrespawn_pc_scale("ge_armorrespawn_pc_scale", "15.0", FCVAR_REPLICATED, "Multiplier applied to playercount. ge_armorrespawntime * 10 - ge_armorrespawn_pc_scale * (playercount - 1)^ge_armorrespawn_pc_pow is the full equation.");
ConVar ge_armorrespawn_pc_pow("ge_armorrespawn_pc_pow", "0.5", FCVAR_REPLICATED, "Power applied to playercount. ge_armorrespawntime * 10 - ge_armorrespawn_pc_scale * (playercount - 1)^ge_armorrespawn_pc_pow is the full equation.");

ConVar ge_debug_armorspawns("ge_debug_armorspawns", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "Debug armor respawn progress.");

ConVar ge_limithalfarmorpickup("ge_limithalfarmorpickup", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Prevent players from getting more than half armor from a half armor pickup.");

CGEArmorVest::CGEArmorVest( void )
{
	m_iSpawnCheckRadius = 512;
	m_iSpawnCheckRadiusSqr = 262144;
	m_iSpawnCheckHalfRadiusSqr = 65536;
}

void CGEArmorVest::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	// Add us to the approperate entity tracker list
	GEEntityTracker()->AddItemToTracker( this, ET_START_ITEMSPAWNED, ET_LIST_ARMOR );
}

void CGEArmorVest::Precache( void )
{
	PrecacheModel( "models/weapons/armor/armor.mdl" );
	PrecacheModel( "models/weapons/halfarmor/halfarmor.mdl" );
	PrecacheScriptSound( PICKUP_SOUND_ARMOR );
	PrecacheScriptSound( PICKUP_SOUND_HALFARMOR );

	BaseClass::Precache();
}

void CGEArmorVest::Materialize( void )
{
	// Spawners handle most of our enabled/disabled logic,
	// but stagnate armor functionality used in certain gamemodes requires that the armor vest
	// not be disabled until it is picked up.

	if ( GEMPRules()->ArmorShouldRespawn() )
		BaseClass::Materialize();
	else
		Remove(); // Delete us so the spawner knows it is disabled.
}

CBaseEntity *CGEArmorVest::Respawn(void)
{
	BaseClass::Respawn();

	// Stops despawned armor from blocking attempts to detonate mines by shooting them.
	SetSolid( SOLID_NONE );
	RemoveSolidFlags( FSOLID_TRIGGER );

	m_iSpawnpointsgoal = (int)(ge_armorrespawntime.GetInt() * 10 - ge_armorrespawn_pc_scale.GetFloat() * pow(max((float)GEMPRules()->GetNumAlivePlayers() - 1, 0), ge_armorrespawn_pc_pow.GetFloat()));

	// Let interested developers know our new goal.
	if (ge_debug_armorspawns.GetBool())
	{
		Vector curPos = GetAbsOrigin();
		DevMsg("Spawnpoints goal for armor at location %f, %f, %f set to %d\n", curPos.x, curPos.y, curPos.z, m_iSpawnpointsgoal);
	}

	ClearAllSpawnProgress();

	SetThink(&CGEArmorVest::RespawnThink);
	SetNextThink(gpGlobals->curtime + 1);
	return this;
}

void CGEArmorVest::SetAmount(int newAmount)
{
	newAmount = clamp( newAmount, 0, MAX_ARMOR );

	m_iAmount = newAmount;

	if ( newAmount <= MAX_ARMOR / 2 )
		SetModel("models/weapons/halfarmor/halfarmor.mdl");
	else
		SetModel("models/weapons/armor/armor.mdl");
}

void CGEArmorVest::SetSpawnCheckRadius(int newRadius)
{
	newRadius = max( newRadius, 0 );

	m_iSpawnCheckRadius = newRadius;
	m_iSpawnCheckRadiusSqr = m_iSpawnCheckRadius * m_iSpawnCheckRadius;
	m_iSpawnCheckHalfRadiusSqr = m_iSpawnCheckRadiusSqr * 0.25;
}

void CGEArmorVest::RespawnThink(void)
{
	int newpoints;
	
	newpoints = CalcSpawnProgress();
	m_iSpawnpoints += newpoints;

	if (ge_debug_armorspawns.GetBool())
		DEBUG_ShowProgress(1, newpoints);

	if (m_iSpawnpointsgoal <= m_iSpawnpoints)
		Materialize();
	else
		SetNextThink(gpGlobals->curtime + 1);
}

bool CGEArmorVest::MyTouch( CBasePlayer *pPlayer )
{
	CGEPlayer *pGEPlayer = dynamic_cast<CGEPlayer*>( pPlayer );
	if ( !pGEPlayer )
		return false;

	return pGEPlayer->AddArmor(m_iAmount,  ge_limithalfarmorpickup.GetBool() ? m_iAmount : MAX_ARMOR);
}


// Finds two different values and uses them to compute a point total
// The first is a point value based on how many players are in range and how close they are.
// The second is the distance between the vest and the closest player.

// The closer the closest player is, the slower the armor will respawn.
// However, the weighted average of all the other nearby players can serve to counteract this.
// This means that heavily contested armor spawns still appear fairly frequently.
// A player within 64 units will completely freeze the timer, however.

int CGEArmorVest::CalcSpawnProgress()
{
	// On teamplay just ignore this fancy stuff, people clump together much more and it's not really as big of a deal.
	if ( GEMPRules()->IsTeamplay() )
		return 10;

	float length1 = m_iSpawnCheckRadiusSqr + 1; //Shortest length
	float length2 = m_iSpawnCheckRadiusSqr + 1; //Second shortest length
	float currentlength = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CGEMPPlayer *pPlayer = ToGEMPPlayer(UTIL_PlayerByIndex(i));
		if (pPlayer && pPlayer->IsConnected() && !pPlayer->IsObserver())
		{
			if (!pPlayer->IsAlive() || !pPlayer->CheckInPVS(this)) // We don't care about players who can't see us.
				continue;

			currentlength = (pPlayer->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
			if (currentlength < length1)
			{
				length2 = length1; //We have to shift length1 down one.  We already know it's lower than length2.
				length1 = currentlength;
			}
			else if (currentlength < length2)
				length2 = currentlength;
		}
	}

	// If the distances aren't low enough then we just tick down at max speed without calculating anything.
	if (length1 > m_iSpawnCheckHalfRadiusSqr || length2 > m_iSpawnCheckRadiusSqr)
		return 10;

	// Basically, 2 people being within the check distance of the armor with one of them within a quarter of it is the only way to get it to tick down at less than max rate.
	// Discourages standing on top of the armor spawn in the middle of a fight because "oh man i need 3 gauges of health"

	// It will never spawn if one of the players is standing right on top of it and the other player is within half the radius.
	if (length1 < 64 && length2 < m_iSpawnCheckHalfRadiusSqr)
		return 0;

	// Otherwise we only care about length 2.  Linear falloff, tick up faster the further away player2 is.
	float metric = 1 - sqrt(length2) / m_iSpawnCheckRadius;

	// If player2 is within 17% of the radius, the armor does not tick down at all.
	return (int)clamp(10 - 12 * metric, 0, 10);
}

void CGEArmorVest::AddSpawnProgressMod(CBasePlayer *pPlayer, int amount)
{
	int cappedamount = min(amount, m_iSpawnpoints * -1); // Can't put the armor below 0 spawn points.

	if (pPlayer)
	{
		int iPID = pPlayer->GetUserID();
		m_iPlayerPointContribution[iPID] += cappedamount;
	}

	m_iSpawnpoints += cappedamount;
}

void CGEArmorVest::ClearSpawnProgressMod(CBasePlayer *pPlayer)
{
	if (!pPlayer)
		return;

	int iPID = pPlayer->GetUserID();

	if (m_iSpawnpoints < m_iSpawnpointsgoal - 20) // Only do this next bit if we won't subtract points.
		m_iSpawnpoints = min(m_iSpawnpoints - m_iPlayerPointContribution[iPID], m_iSpawnpointsgoal - 20); // Add back all the spawn points that we stole but avoid having the armor respawn instantly after we died.

	m_iPlayerPointContribution[iPID] = 0;
}

void CGEArmorVest::ClearAllSpawnProgress()
{
	m_iSpawnpoints = 0;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_iPlayerPointContribution[i] = 0;
	}
}

// Just going to recycle the one from the playerspawns because it is so good.
void CGEArmorVest::DEBUG_ShowProgress(float duration, int progress)
{
	// Only show this on local host servers
	if (engine->IsDedicatedServer())
		return;

	int line = 0;
	char tempstr[64];

	int r = (int)RemapValClamped(progress, 0, 10.0f, 255.0f, 0);
	int g = (int)RemapValClamped(progress, 0, 10.0f, 0, 255.0f);

	Color c(r, g, 0, 200);
	Vector mins = {-12, -12, 0};
	Vector maxes = {12, 12, 4};
	debugoverlay->AddBoxOverlay2(GetAbsOrigin(), mins, maxes, GetAbsAngles(), c, c, duration);

	Q_snprintf(tempstr, 64, "%s", GetClassname());
	EntityText(++line, tempstr, duration);

	if (!IsEffectActive(EF_NODRAW))
	{
		// We've spawned
		Q_snprintf(tempstr, 64, "SPAWNED", progress);
		EntityText(++line, tempstr, duration);
	}
	else
	{
		// We are enabled, show our desirability and stats
		Q_snprintf(tempstr, 64, "Progress: %i/%i", m_iSpawnpoints, m_iSpawnpointsgoal);
		EntityText(++line, tempstr, duration);

		Q_snprintf(tempstr, 64, "Rate: %i", progress);
		EntityText(++line, tempstr, duration);

		Q_snprintf(tempstr, 64, "Radius: %i", m_iSpawnCheckRadius);
		EntityText(++line, tempstr, duration);
	}

	
}