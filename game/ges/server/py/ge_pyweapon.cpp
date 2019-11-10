///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyweapon.cpp
//   Description :
//      [TODO: Write the purpose of ge_pyweapon.cpp.]
//
//   Created On: 9/1/2009 10:27:58 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////
#include "ge_pyprecom.h"

#include "ammodef.h"

#include "ge_ai.h"
#include "ge_weapon.h"
#include "ge_weaponmelee.h"
#include "ge_weapon_parse.h"
#include "ge_loadoutmanager.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGEWeapon *pyToGEWeapon( CBaseEntity *pEnt )
{
	if ( !pEnt )
		return NULL;

	if ( Q_stristr( pEnt->GetClassname(), "weapon_" ) ||  Q_stristr( pEnt->GetClassname(), "token_" ) )
		return ToGEWeapon( (CBaseCombatWeapon*)pEnt );

	return NULL;
}

const char *pyGetAmmoType( CGEWeapon *pWeap )
{
	if ( pWeap )
	{
		int id = pWeap->GetPrimaryAmmoType();
		Ammo_t *ammo = GetAmmoDef()->GetAmmoOfIndex( id );
		return ammo ? ammo->pName : "";
	}

	return "";
}

int pyGetAmmoCount( CGEWeapon *pWeap )
{
	if ( !pWeap )
		return 0;

	CBaseCombatCharacter *pOwner = pWeap->GetOwner();
	if ( pOwner && (pOwner->IsPlayer() || pOwner->IsNPC()) )
		return pOwner->GetAmmoCount( pWeap->GetPrimaryAmmoType() );
	else
		return pWeap->GetDefaultClip1();
}

void pySetAmmoCount( CGEWeapon *pWeap, int amt )
{
	if ( pWeap )
	{
		CBaseCombatCharacter *pOwner = pWeap->GetOwner();
		if ( pOwner && (pOwner->IsPlayer() || pOwner->IsNPC()) )
		{
			int aid = pWeap->GetPrimaryAmmoType();
			pOwner->SetAmmoCount( amt, aid );
		}
	}
}

int pyGetMaxAmmoCount( CGEWeapon *pWeap )
{
	if ( !pWeap )
		return 0;

	if ( pWeap->UsesPrimaryAmmo() )
		return GetAmmoDef()->MaxCarry( pWeap->GetPrimaryAmmoType() );
	else
		return 999;
}

float pyGetMeleeWeaponRange( CGEWeapon *pWeap, bool modded = true )
{
    if ( !pWeap || !(pWeap->IsMeleeWeapon() || pWeap->GetWeaponID() == WEAPON_WATCHLASER) )
        return -1.0f;

    return static_cast<CGEWeaponMelee*>(pWeap)->GetRange(modded);
}

int pyGetWeaponSlot( CGEWeapon *pWeap )
{
	if ( !pWeap )
		return -1;

	// Return the highest slot position for this weapon id
	int weapid = pWeap->GetWeaponID();
	int slot = -1;
	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		if ( GEMPRules()->GetLoadoutManager()->GetWeaponInSlot(i) == weapid )
			slot = i;
	}

	return slot;
}

const char *pyWeaponAmmoType( bp::object weap )
{
	bp::extract<char*> to_name( weap );
	bp::extract<int> to_id( weap );

	if ( to_name.check() )
		return GetAmmoForWeapon( AliasToWeaponID( to_name() ) );
	else if ( to_id.check() )
		return GetAmmoForWeapon( to_id() );
	else
		return "";
}

const char* pyWeaponPrintName( bp::object weap )
{
	bp::extract<char*> to_name( weap );
	bp::extract<int> to_id( weap );

	if ( to_name.check() )
		return GetWeaponPrintName( AliasToWeaponID( to_name() ) );
	else if ( to_id.check() )
		return GetWeaponPrintName( to_id() );
	else
		return "";
}

const char* pyWeaponClassname( int id )
{
	// Only makes sense to accept an id
	const char *name = WeaponIDToAlias(id);
	return name != NULL ? name : "";
}

int pyWeaponID( const char *classname )
{
	return AliasToWeaponID( classname );
}

void pyWeaponSetAbsOrigin( CGEWeapon *weap, Vector origin )
{
	if ( !weap || weap->GetOwner() )
		return;

	weap->SetAbsOrigin( origin );
}

void pyWeaponSetAbsAngles( CGEWeapon *weap, QAngle angles )
{
	if ( !weap || weap->GetOwner() )
		return;

	weap->SetAbsAngles( angles );
}

CBasePlayer *pyWeaponGetOriginalOwner( CGEWeapon *weap )
{
	if ( !weap )
		return NULL;

    return ToGEMPPlayer(weap->GetOriginalOwner());
}

// Intended for setting the skins of tokens, was used in 5.0 to temporarily give weapon skin rewards out.

// But hey, giving strawberry rocket launchers to janus and blueberry to MI6 was a novel idea.
// So what the heck, people can do that if they want to, But the KF7 skin must be protected at all costs lest Luchador's legacy become tainted.
// Yes you can still do this other ways but dang nabit please respect our dinky skin system.
void pyWeaponSetSkin( CGEWeapon *pWeap, int skin )
{
	if ( pWeap )
	{
		int WhitelistedWeapons[] = { WEAPON_TOKEN, WEAPON_ROCKET_LAUNCHER }; // Whitelist weapons as needed.  But donchu ever whitelist the klobb or KF7, boy.

		int weaponID = pWeap->GetWeaponID();
		bool match = false;
		int whitelistLength = sizeof(WhitelistedWeapons) / sizeof(int);

		for (int i = 0; i < whitelistLength; i++)
		{
			if ( WhitelistedWeapons[i] == weaponID )
			{
				match = true; 
				break;
			}
		}

		if (match)
			pWeap->SetSkin(skin);
		else
			Warning("Tried to assign invalid skin through python!\n");
	}
}

bp::dict pyWeaponInfo( bp::object weap, CBaseCombatCharacter *pOwner = NULL )
{
	bp::dict info;
	bp::extract<char*> to_name( weap );
	bp::extract<int> to_id( weap );

	// Extract the weapon id
	int weap_id = WEAPON_NONE;
	if ( to_name.check() )
		weap_id = AliasToWeaponID( to_name() );
	else if ( to_id.check() )
		weap_id = to_id();

	// Get our info
	const char *name = WeaponIDToAlias( weap_id );
	if ( name && name[0] != '\0' )
	{
		int h = LookupWeaponInfoSlot( name );
		if ( h == GetInvalidWeaponInfoHandle() )
			return info;

		CGEWeaponInfo *weap = dynamic_cast<CGEWeaponInfo*>( GetFileWeaponInfoFromHandle( h ) );
		if ( weap )
		{
			info["id"] = weap_id;
			info["classname"] = name;
			info["printname"] = weap->szPrintName;
			info["weight"] = weap->iWeight;
			info["damage"] = weap->m_iDamage;
            info["damage_radius"] = weap->m_flDamageRadius;
			info["penetration"] = weap->m_flMaxPenetration > 1.5f ? weap->m_flMaxPenetration : 0;
			info["viewmodel"] = weap->szViewModel;
			info["worldmodel"] = weap->szWorldModel;
			info["melee"] = weap->m_bMeleeWeapon;
            info["explosive"] = weap->m_flDamageRadius > 0.0f;
			info["uses_clip"] = weap->iMaxClip1 != WEAPON_NOCLIP;
			info["clip_size"] = weap->iMaxClip1;
			info["clip_def"] = weap->iDefaultClip1;
			info["ammo_type"] = GetAmmoForWeapon( weap_id );

			int aid = GetAmmoDef()->Index( GetAmmoForWeapon( weap_id ) );
			Ammo_t *ammo = GetAmmoDef()->GetAmmoOfIndex( aid );
			if ( ammo )
			{
				info["ammo_max"] = ammo->pMaxCarry;

				if ( pOwner )
					info["ammo_count"] = pOwner->GetAmmoCount( aid );
			}			
			else
			{
				info["ammo_max"] = 0;
				info["ammo_count"] = 0;
			}
		}
	}

	return info;
}

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetWeaponDamage_overloads, CGEWeapon::GetWeaponDamage, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetDamageCap_overloads, CGEWeapon::GetDamageCap, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetMaxClip1_overloads, CGEWeapon::GetMaxClip1, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetAccShots_overloads, CGEWeapon::GetAccShots, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetWeaponFireRate_overloads, CGEWeapon::GetFireRate, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetWeaponClickFireRate_overloads, CGEWeapon::GetClickFireRate, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetWeaponAccFireRate_overloads, CGEWeapon::GetAccFireRate, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetMaxPenetrationDepth_overloads, CGEWeapon::GetMaxPenetrationDepth, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetWeaponDamageRadius_overloads, CGEWeapon::GetWeaponDamageRadius, 0, 1);
BOOST_PYTHON_FUNCTION_OVERLOADS(GetMeleeWeaponRange_overloads, pyGetMeleeWeaponRange, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetZoomOffset_overloads, CGEWeapon::GetZoomOffset, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetWeaponPushForceMult_overloads, CGEWeapon::GetWeaponPushForceMult, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetMinSpreadVec_overloads, CGEWeapon::GetMinSpreadVec, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetMaxSpreadVec_overloads, CGEWeapon::GetMaxSpreadVec, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetAimBonus_overloads, CGEWeapon::GetAimBonus, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GetJumpPenalty_overloads, CGEWeapon::GetJumpPenalty, 0, 1);

BOOST_PYTHON_FUNCTION_OVERLOADS(WeaponInfo_overloads, pyWeaponInfo, 1, 2);
BOOST_PYTHON_MODULE(GEWeapon)
{
	using namespace boost::python;

	def("ToGEWeapon", pyToGEWeapon, return_value_policy<reference_existing_object>());
	
	// Quick access to weapon information
	def("WeaponAmmoType", pyWeaponAmmoType);
	def("WeaponClassname", pyWeaponClassname);
	def("WeaponID", pyWeaponID);
	def("WeaponPrintName", pyWeaponPrintName);
	def("WeaponInfo", pyWeaponInfo, WeaponInfo_overloads());

	// Here for function calls only
	class_<CBaseCombatWeapon, bases<CBaseEntity>, boost::noncopyable>("CBaseCombatWeapon", no_init);

	// The weapon class definition
    class_<CGEWeapon, bases<CBaseCombatWeapon>, boost::noncopyable>("CGEWeapon", no_init)
        .def("GetWeight", &CGEWeapon::GetWeight)
        .def("GetPrintName", &CGEWeapon::GetPrintName)
        .def("GetSpawnerSlot", &CGEWeapon::GetWeaponSpawnerSlot)
        .def("IsMeleeWeapon", &CGEWeapon::IsMeleeWeapon)
        .def("IsExplosiveWeapon", &CGEWeapon::IsExplosiveWeapon)
        .def("IsAutomaticWeapon", &CGEWeapon::IsAutomaticWeapon)
        .def("IsThrownWeapon", &CGEWeapon::IsThrownWeapon)
        .def("CanHolster", &CGEWeapon::CanHolster)
        .def("HasAmmo", &CGEWeapon::HasPrimaryAmmo)
        .def("UsesAmmo", &CGEWeapon::UsesPrimaryAmmo)
        .def("GetAmmoType", pyGetAmmoType)
        .def("GetAmmoCount", pyGetAmmoCount)
        .def("SetAmmoCount", pySetAmmoCount)
        .def("GetMaxAmmoCount", pyGetMaxAmmoCount)
        .def("GetClip", &CGEWeapon::Clip1)
        .def("GetMaxClip", &CGEWeapon::GetMaxClip1, GetMaxClip1_overloads())
        .def("GetAccShots", &CGEWeapon::GetAccShots, GetAccShots_overloads())
        .def("GetDefaultClip", &CGEWeapon::GetDefaultClip1)
        .def("GetDamage", &CGEWeapon::GetWeaponDamage, GetWeaponDamage_overloads())
        .def("GetDamageCap", &CGEWeapon::GetDamageCap, GetDamageCap_overloads())
        .def("GetFireRate", &CGEWeapon::GetFireRate, GetWeaponFireRate_overloads())
        .def("GetClickFireRate", &CGEWeapon::GetClickFireRate, GetWeaponClickFireRate_overloads())
        .def("GetAccFireRate", &CGEWeapon::GetAccFireRate, GetWeaponAccFireRate_overloads())
        .def("GetWeaponPushForceMult", &CGEWeapon::GetWeaponPushForceMult, GetWeaponPushForceMult_overloads())

        .def("GetMaxPenetrationDepth", &CGEWeapon::GetMaxPenetrationDepth, GetMaxPenetrationDepth_overloads())
        .def("GetWeaponId", &CGEWeapon::GetWeaponID)
        .def("GetSecondsUntilPickupAllowed", &CGEWeapon::GetSecondsUntilPickup)
        .def("SetSecondsUntilPickupAllowed", &CGEWeapon::SetSecondsUntilPickup)
        .def("SetRoundSecondsUntilPickupAllowed", &CGEWeapon::SetRoundSecondsUntilPickup)
        .def("GetWeaponSlot", pyGetWeaponSlot)
        .def("SetSkin", pyWeaponSetSkin)

        .def("GetLimitEnforcementPriority", &CGEWeapon::GetLimitEnforcementPriority)
        .def("SetLimitEnforcementPriority", &CGEWeapon::SetLimitEnforcementPriority)
        .def("SetPlayerclipCollision", &CGEWeapon::SetPlayerclipCollision)
        .def("GetWeaponDamageRadius", &CGEWeapon::GetWeaponDamageRadius, GetWeaponDamageRadius_overloads())
        .def("GetWeaponMeleeRange", pyGetMeleeWeaponRange, GetMeleeWeaponRange_overloads())

        .def("GetMinSpreadVec", &CGEWeapon::GetMinSpreadVec, GetMinSpreadVec_overloads())
        .def("GetMaxSpreadVec", &CGEWeapon::GetMaxSpreadVec, GetMaxSpreadVec_overloads())
        .def("GetAimBonus", &CGEWeapon::GetAimBonus, GetAimBonus_overloads())
        .def("GetJumpPenalty", &CGEWeapon::GetJumpPenalty, GetJumpPenalty_overloads())

        .def("GetZoomOffset", &CGEWeapon::GetZoomOffset, GetZoomOffset_overloads())

        .def("GetOriginalOwner", &CGEWeapon::GetOriginalOwner, return_value_policy<reference_existing_object>())

        .def("GetDamageOffset", &CGEWeapon::GetDamageOffset)
		.def("SetDamageOffset", &CGEWeapon::SetDamageOffset)
        .def("GetDamageCapOffset", &CGEWeapon::GetDamageCapOffset)
		.def("SetDamageCapOffset", &CGEWeapon::SetDamageCapOffset)
		.def("GetFireRateOffset", &CGEWeapon::GetFireRateOffset)
		.def("SetFireRateOffset", &CGEWeapon::SetFireRateOffset)
        .def("GetClickFireRateOffset", &CGEWeapon::GetClickFireRateOffset)
		.def("SetClickFireRateOffset", &CGEWeapon::SetClickFireRateOffset)
        .def("GetAccFireRateOffset", &CGEWeapon::GetAccFireRateOffset)
		.def("SetAccFireRateOffset", &CGEWeapon::SetAccFireRateOffset)
        .def("GetMinSpreadOffset", &CGEWeapon::GetMinSpreadOffset)
		.def("SetMinSpreadOffset", &CGEWeapon::SetMinSpreadOffset)
		.def("GetMaxSpreadOffset", &CGEWeapon::GetMaxSpreadOffset)
		.def("SetMaxSpreadOffset", &CGEWeapon::SetMaxSpreadOffset)
        .def("GetJumpPenaltyOffset", &CGEWeapon::GetJumpPenaltyOffset)
		.def("SetJumpPenaltyOffset", &CGEWeapon::SetJumpPenaltyOffset)
        .def("GetAimBonusOffset", &CGEWeapon::GetAimBonusOffset)
		.def("SetAimBonusOffset", &CGEWeapon::SetAimBonusOffset)
        .def("GetMaxClip1Offset", &CGEWeapon::GetMaxClip1Offset)
		.def("SetMaxClip1Offset", &CGEWeapon::SetMaxClip1Offset)
        .def("GetAccShotsOffset", &CGEWeapon::GetAccShotsOffset)
		.def("SetAccShotsOffset", &CGEWeapon::SetAccShotsOffset)
        .def("GetZoomOffsetOffset", &CGEWeapon::GetZoomOffsetOffset)
		.def("SetZoomOffsetOffset", &CGEWeapon::SetZoomOffsetOffset)
        .def("GetBlastRadiusOffset", &CGEWeapon::GetBlastRadiusOffset)
		.def("SetBlastRadiusOffset", &CGEWeapon::SetBlastRadiusOffset)
        .def("GetPushForceMultOffset", &CGEWeapon::GetPushForceMultOffset)
		.def("SetPushForceMultOffset", &CGEWeapon::SetPushForceMultOffset)
        .def("GetRangeOffset", &CGEWeapon::GetRangeOffset)
		.def("SetRangeOffset", &CGEWeapon::SetRangeOffset)
        .def("GetPenetrationOffset", &CGEWeapon::GetPenetrationOffset)
		.def("SetPenetrationOffset", &CGEWeapon::SetPenetrationOffset)
        .def("GetWeaponSoundPitchOffset", &CGEWeapon::GetWeaponSoundPitchOffset)
		.def("SetWeaponSoundPitchOffset", &CGEWeapon::SetWeaponSoundPitchOffset)
        .def("GetWeaponSoundVolumeOffset", &CGEWeapon::GetWeaponSoundVolumeOffset)
		.def("SetWeaponSoundVolumeOffset", &CGEWeapon::SetWeaponSoundVolumeOffset)
        .def("GetCustomPrintName", &CGEWeapon::GetCustomPrintName)
		.def("SetCustomPrintName", &CGEWeapon::SetCustomPrintName)

		// The following override CBaseEntity to prevent movement when held
		.def("SetAbsOrigin", pyWeaponSetAbsOrigin)
		.def("SetAbsAngles", pyWeaponSetAbsAngles);
}
