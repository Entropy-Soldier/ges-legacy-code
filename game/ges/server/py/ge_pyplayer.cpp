///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyplayer.cpp
//   Description :
//      [TODO: Write the purpose of ge_pyplayer.cpp.]
//
//   Created On: 8/24/2009 3:39:38 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////
#include "ge_pyprecom.h"

#include "ammodef.h"

#include "ge_player.h"
#include "gemp_player.h"
#include "gebot_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBaseEntity *pyGetEntByUniqueId( int );

bool pyIsValidPlayerIndex(int index)
{
	return UTIL_PlayerByIndex(index) != NULL;
}

CGEMPPlayer* pyGetMPPlayer(int index)
{
	return ToGEMPPlayer( UTIL_PlayerByIndex(index) );
}

CGEMPPlayer* pyToMPPlayer( bp::object ent_or_uid )
{
	bp::extract<CBaseEntity*> to_ent( ent_or_uid );
	bp::extract<int> to_uid( ent_or_uid );

	if ( to_ent.check() )
		return ToGEMPPlayer( to_ent() );
	else if ( to_uid )
		return ToGEMPPlayer( pyGetEntByUniqueId( to_uid() ) );
	else
		return NULL;
}

CBaseCombatCharacter *pyToCombatCharacter( CBaseEntity *pEnt )
{
	return pEnt ? pEnt->MyCombatCharacterPointer() : NULL;
}

CGEWeapon *pyGetActiveWeapon( CBaseCombatCharacter *pEnt )
{
	return pEnt ? (CGEWeapon*) pEnt->GetActiveWeapon() : NULL;
}

bool pyHasWeapon( CBaseCombatCharacter *pEnt, bp::object weap_or_id )
{
	bp::extract<CGEWeapon*> to_weap( weap_or_id );
	bp::extract<char*> to_name( weap_or_id );
	bp::extract<int> to_id( weap_or_id );

	if ( to_weap.check() )
		return pEnt->Weapon_OwnsThisType( to_weap()->GetClassname() ) != NULL;
	else if ( to_name.check() )
		return pEnt->Weapon_OwnsThisType( to_name() ) != NULL;
	else if ( to_id.check() )
		return pEnt->Weapon_OwnsThisType( WeaponIDToAlias( to_id() ) ) != NULL;

	return false;
}

bool pyWeaponSwitch( CBaseCombatCharacter *pEnt, bp::object weap_or_id )
{
	if ( !pEnt )
		return false;

	bp::extract<CGEWeapon*> to_weap( weap_or_id );
	bp::extract<char*> to_name( weap_or_id );
	bp::extract<int> to_id( weap_or_id );

	CBaseCombatWeapon *pWeap = NULL;

	if ( to_weap.check() )
		pWeap = to_weap();
	else if ( to_name.check() )
		pWeap = pEnt->Weapon_OwnsThisType( to_name() );
	else if ( to_id.check() )
		pWeap = pEnt->Weapon_OwnsThisType( WeaponIDToAlias( to_id() ) );
	
	if ( pWeap )
		return pEnt->Weapon_Switch( pWeap );

	return false;
}

bool pyRemoveWeapon( CBaseCombatCharacter *pEnt, bp::object weap_or_id )
{
	if ( !pEnt ) 
		return false;

	bp::extract<CGEWeapon*> to_weap( weap_or_id );
	bp::extract<char*> to_name( weap_or_id );
	bp::extract<int> to_id( weap_or_id );

	CBaseCombatWeapon *pWeap = NULL;

	if ( to_weap.check() )
		pWeap = to_weap();
	else if ( to_name.check() )
		pWeap = pEnt->Weapon_OwnsThisType( to_name() );
	else if ( to_id.check() )
		pWeap = pEnt->Weapon_OwnsThisType( WeaponIDToAlias( to_id() ) );

	if (!pWeap)
		return false;

	CGEBotPlayer *botPlayer = ToGEBotPlayer( pEnt );

	if ( botPlayer )
		return botPlayer->RemoveWeapon(pWeap);
	else
		return pEnt->RemoveWeapon( pWeap );
}

int pyGetAmmoCount( CBaseCombatCharacter *pEnt, bp::object weap_or_ammo )
{
	bp::extract<CGEWeapon*> to_weap( weap_or_ammo );
	bp::extract<int> to_weap_id( weap_or_ammo );
	bp::extract<char*> to_ammo( weap_or_ammo );

	if ( to_weap.check() )
		return pEnt->GetAmmoCount( to_weap()->GetPrimaryAmmoType() );
	else if ( to_weap_id.check() )
		return pEnt->GetAmmoCount( const_cast<char*>(GetAmmoForWeapon( to_weap_id() )) );
	else if ( to_ammo.check() )
		return pEnt->GetAmmoCount( to_ammo() );
	
	return 0;
}

bp::list pyGetHeldWeapons( CBaseCombatCharacter *pEnt )
{
	bp::list weaps;
	if ( pEnt )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CGEWeapon *pWeap = (CGEWeapon*) pEnt->GetWeapon( i );
			if ( pWeap )
				weaps.append( bp::ptr( pWeap ) );
		}
	}
	return weaps;
}

bp::list pyGetHeldWeaponIds( CBaseCombatCharacter *pEnt )
{
	bp::list weaps;
	if ( pEnt )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CGEWeapon *pWeap = (CGEWeapon*) pEnt->GetWeapon( i );
			if ( pWeap )
				weaps.append( pWeap->GetWeaponID() );
		}
	}
	return weaps;
}

int pyGetSkin( CBaseCombatCharacter *pEnt )
{
	if ( !pEnt ) return 0;
	return pEnt->m_nSkin;
}

void pyPlayerSetAbsOrigin( CBaseCombatCharacter *player, Vector origin )
{
	// DO NOTHING!!
}

void pyPlayerSetAbsAngles( CBaseCombatCharacter *player, QAngle angles )
{
	// DO NOTHING!!
}

void pyPlayerSetOrigin( CBaseCombatCharacter *player, Vector origin )
{
    player->SetAbsOrigin(origin);
}

Vector pyPlayerGetOrigin( CBaseCombatCharacter *player )
{
    return player->GetAbsOrigin();
}

void pyPlayerSetVelocity( CBaseCombatCharacter *player, Vector velocity )
{
    player->SetAbsVelocity(velocity);
}

Vector pyPlayerGetVelocity( CBaseCombatCharacter *player )
{
    return player->GetAbsVelocity();
}

// Floats don't cast to ints implicitly for these functions, requiring the gamemode coder to do an explicit conversion.
// This is easy to forget since even pure integer operations result in a conversion to a float, so let's make life easier
// with regards to the health/armor functions.
void pyPlayerSetFloatHealth( CBaseCombatCharacter *player, float health )
{
    player->SetHealth(health);
}

void pyPlayerSetFloatArmor( CGEPlayer *player, float armor )
{
    player->SetArmorValue(armor);
}

void pyPlayerSetFloatMaxHealth( CGEPlayer *player, float maxHealth )
{
    player->SetMaxHealth(maxHealth);
}

void pyPlayerSetFloatMaxArmor( CGEPlayer *player, float maxArmor )
{
    player->SetMaxArmor(maxArmor);
}

void pySetPlayerModel( CGEPlayer *pPlayer, const char *szChar, int nSkin )
{
	if ( pPlayer )
		pPlayer->SetPlayerModel( szChar, nSkin, true );
}

bool pyPlayerIsOnGround( CGEPlayer *player )
{
    return (player->GetGroundEntity() != NULL);
}

void pyResetTotalMaxArmorLimit( CGEPlayer *pPlayer )
{
	if ( pPlayer )
		pPlayer->SetTotalArmorPickup(0);
}

int pyGetRemainingArmorPickupLimit( CGEPlayer *pPlayer )
{
	if ( pPlayer )
		return pPlayer->GetMaxTotalArmor() - pPlayer->GetTotalArmorPickup();

	return -1;
}

void pyGiveAmmo( CGEMPPlayer *pPlayer, const char *ammo, int amt, bool supressSound = true )
{
	if ( pPlayer )
	{
		int aID = GetAmmoDef()->Index( ammo );
		pPlayer->GiveAmmo( amt, aID, supressSound );
	}	
}

Vector pyAimDirection( CGEPlayer *pEnt )
{
	Vector forward;
	pEnt->EyeVectors( &forward );
	return forward;
}

BOOST_PYTHON_FUNCTION_OVERLOADS(GiveAmmo_overloads, pyGiveAmmo, 3, 4);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GiveNamedWeapon_overloads, CGEPlayer::GiveNamedWeapon, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(BotGiveNamedWeapon_overloads, CGEBotPlayer::GiveNamedWeapon, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ChangeTeam_overloads, CGEMPPlayer::ChangeTeam, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(BotChangeTeam_overloads, CGEBotPlayer::ChangeTeam, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(KnockOffHat_overloads, CGEPlayer::KnockOffHat, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(BotKnockOffHat_overloads, CGEBotPlayer::KnockOffHat, 0, 1);

BOOST_PYTHON_MODULE(GEPlayer)
{
	using namespace boost::python;

	// Imports
	import( "GEEntity" );

	//because CommitSuicideFP is overloaded we must tell it which one to use
	void (CGEPlayer::*CommitSuicideFP)(bool, bool) = &CGEPlayer::CommitSuicide;

	def("IsValidPlayerIndex", pyIsValidPlayerIndex);
	def("ToCombatCharacter", pyToCombatCharacter, return_value_policy<reference_existing_object>());
	def("ToMPPlayer", pyToMPPlayer, return_value_policy<reference_existing_object>());
	def("GetMPPlayer", pyGetMPPlayer, return_value_policy<reference_existing_object>());

	// Functions used by NPC's
	class_<CBaseCombatCharacter, bases<CBaseEntity>, boost::noncopyable>("CBaseCombatCharacter", no_init)
		.def("GetHealth", &CBaseCombatCharacter::GetHealth)
		.def("SetHealth", &CBaseCombatCharacter::SetHealth)
        .def("SetHealth", pyPlayerSetFloatHealth)
		.def("GetSkin", pyGetSkin)
		.def("GetActiveWeapon", pyGetActiveWeapon, return_value_policy<reference_existing_object>())
		.def("HasWeapon", pyHasWeapon)
		.def("WeaponSwitch", pyWeaponSwitch)
		.def("StripWeapon", pyRemoveWeapon)
		.def("GetAmmoCount", pyGetAmmoCount)
		.def("GetHeldWeapons", pyGetHeldWeapons)
		.def("GetHeldWeaponIds", pyGetHeldWeaponIds)
		// Explicitly override these functions to prevent their use
		.def("SetAbsOrigin", pyPlayerSetAbsOrigin)
		.def("SetAbsAngles", pyPlayerSetAbsAngles);

	// Here for function call reasons only
	class_<CNPC_GEBase, bases<CBaseCombatCharacter>, boost::noncopyable>("CNPC_GEBase", no_init);
	class_<CBasePlayer, bases<CBaseCombatCharacter>, boost::noncopyable>("CBasePlayer", no_init);

	class_<CGEPlayer, bases<CBasePlayer>, boost::noncopyable>("CGEPlayer", no_init)
		.def("IncrementDeathCount", &CGEPlayer::IncrementDeathCount) // Deprecated
		.def("GetUserID", &CGEPlayer::GetUserID)	// Deprecated
		.def("GetFrags", &CGEPlayer::FragCount)
		.def("GetDeaths", &CGEPlayer::DeathCount)
		.def("AddDeathCount", &CGEPlayer::IncrementDeathCount)
		.def("GetArmor", &CGEPlayer::ArmorValue)
		.def("GetHealth", &CGEPlayer::GetHealth)
		.def("ResetDeathCount", &CGEPlayer::ResetDeathCount)
		.def("CommitSuicide", CommitSuicideFP)
		.def("GetPlayerName", &CGEMPPlayer::GetSafePlayerName)
		.def("GetPlayerID", &CGEPlayer::GetUserID)
		.def("GetSteamID", &CGEPlayer::GetNetworkIDString)
		.def("IsDead", &CGEPlayer::IsDead)
		.def("IsObserver", &CGEPlayer::IsObserver)
		.def("GetMaxArmor", &CGEPlayer::GetMaxArmor)
		.def("SetMaxArmor", &CGEPlayer::SetMaxArmor)
        .def("SetMaxArmor", pyPlayerSetFloatMaxArmor)
		.def("GetTotalAllowedArmorPickup", &CGEPlayer::GetMaxTotalArmor)
		.def("GetRemainingAllowedArmorPickup", pyGetRemainingArmorPickupLimit)
		.def("SetTotalAllowedArmorPickup", &CGEPlayer::SetMaxTotalArmor)
		.def("RenewArmorPickupLimit", pyResetTotalMaxArmorLimit)
		.def("GetMaxHealth", &CGEPlayer::GetMaxHealth)
		.def("SetMaxHealth", &CGEPlayer::SetMaxHealth)
        .def("SetMaxHealth", pyPlayerSetFloatMaxHealth)
		.def("SetArmor", &CGEPlayer::SetArmorValue)
        .def("SetArmor", pyPlayerSetFloatArmor)
		.def("GetPlayerModel", &CGEPlayer::GetCharIdent)
		.def("SetPlayerModel", pySetPlayerModel)
		.def("SetHat", &CGEPlayer::SpawnHat)
        .def("SetPosition", pyPlayerSetOrigin)
        .def("GetPosition", pyPlayerGetOrigin)
        .def("SetVelocity", pyPlayerSetVelocity)
        .def("GetVelocity", pyPlayerGetVelocity)
        .def("IsInAimMode", &CGEPlayer::IsInAimMode)
        .def("IsDucking", &CGEPlayer::IsDucking)
        .def("IsOnGround", pyPlayerIsOnGround)
		.def("KnockOffHat", &CGEPlayer::KnockOffHat, KnockOffHat_overloads())
		.def("MakeInvisible", &CGEPlayer::MakeInvisible)
		.def("MakeVisible", &CGEPlayer::MakeVisible)
		.def("SetDamageMultiplier", &CGEPlayer::SetDamageMultiplier)
		.def("SetSpeedMultiplier", &CGEPlayer::SetSpeedMultiplier)
        .def("SetJumpVelocityMultiplier", &CGEPlayer::SetJumpVelocityMult)
        .def("SetStrafeRunMultiplier", &CGEPlayer::SetStrafeRunMult)
        .def("SetSpeedMultiplier", &CGEPlayer::SetSpeedMultiplier)
        .def("SetMaxMidairJumps", &CGEPlayer::SetMaxMidairJumps)
		.def("SetScoreBoardColor", &CGEPlayer::SetHudColor)
		.def("StripAllWeapons", &CGEPlayer::StripAllWeapons)
		.def("GetAimDirection", pyAimDirection)
		.def("GetEyePosition", &CGEPlayer::EyePosition)
		.def("GetLastHitGroup", &CGEPlayer::LastHitGroup)
		.def("GiveNamedWeapon", &CGEPlayer::GiveNamedWeapon, GiveNamedWeapon_overloads());

	class_<CGEMPPlayer, bases<CGEPlayer>, boost::noncopyable>("CGEMPPlayer", no_init)
		.def("GetScore", &CGEMPPlayer::GetRoundScore)		// Deprecated
		.def("SetScore", &CGEMPPlayer::SetRoundScore)		// Deprecated
		.def("IncrementScore", &CGEMPPlayer::AddRoundScore) // Deprecated
		.def("ResetScore", &CGEMPPlayer::ResetRoundScore)	// Deprecated
		.def("IncrementMatchScore", &CGEMPPlayer::AddMatchScore) // Deprecated
		.def("GetRoundScore", &CGEMPPlayer::GetRoundScore)
		.def("SetRoundScore", &CGEMPPlayer::SetRoundScore)
		.def("AddRoundScore", &CGEMPPlayer::AddRoundScore)
		.def("ResetRoundScore", &CGEMPPlayer::ResetRoundScore)
		.def("GetMatchScore", &CGEMPPlayer::GetMatchScore)
		.def("SetMatchScore", &CGEMPPlayer::SetMatchScore)
		.def("AddMatchScore", &CGEMPPlayer::AddMatchScore)
		.def("SetDeaths", &CGEMPPlayer::SetDeaths)
		.def("ForceRespawn", &CGEMPPlayer::ForceRespawn)
		.def("GetLastWalkPosition", &CGEMPPlayer::GetLastWalkPosition)
        .def("WillGetStuckAtPosition", &CGEMPPlayer::WillGetStuckAtPosition)
		.def("ChangeTeam", &CGEMPPlayer::ChangeTeam, ChangeTeam_overloads())
		.def("GetCleanPlayerName", &CGEMPPlayer::GetSafeCleanPlayerName)
		.def("SetInitialSpawn", &CGEMPPlayer::SetInitialSpawn)
		.def("IsInitialSpawn", &CGEMPPlayer::IsInitialSpawn)
        .def("UpdateSpawnStatus", &CGEMPPlayer::UpdateGameplaySpawnStatus)
		.def("IsInRound", &CGEMPPlayer::IsInRound)
		.def("IsActive", &CGEMPPlayer::IsActive)
		.def("IsPreSpawn", &CGEMPPlayer::IsPreSpawn)
		.def("GiveDefaultWeapons", &CGEMPPlayer::GiveDefaultItems)
		.def("GiveAmmo", pyGiveAmmo, GiveAmmo_overloads());

	class_<CGEBotPlayer, bases<CGEMPPlayer>, boost::noncopyable>("CGEBotPlayer", no_init)
		.def("GiveNamedWeapon", &CGEBotPlayer::GiveNamedWeapon, BotGiveNamedWeapon_overloads())
		.def("ChangeTeam", &CGEBotPlayer::ChangeTeam, BotChangeTeam_overloads())
		.def("StripAllWeapons", &CGEBotPlayer::StripAllWeapons)
		.def("KnockOffHat", &CGEBotPlayer::KnockOffHat, BotKnockOffHat_overloads());
}
