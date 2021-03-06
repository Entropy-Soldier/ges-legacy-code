///////////// Copyright � 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pybaseentity.cpp
//   Description :
//      [TODO: Write the purpose of ge_pybaseentity.cpp.]
//
//   Created On: 9/1/2009 10:19:15 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"
#include "baseentity.h"
#include "ge_player.h"
#include "ehandle.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int pyGetEntSerial( CBaseEntity *pEnt )
{
	if ( !pEnt ) return INVALID_EHANDLE_INDEX;
	return pEnt->GetRefEHandle().ToInt();
}

CBaseEntity *pyGetEntByUniqueId( int serial )
{
	if ( serial < 0 || serial == INVALID_EHANDLE_INDEX )
		return NULL;

	// Extract the ent index and serial number from the serial given
	int iEntity = serial & ((1 << MAX_EDICT_BITS) - 1);
	int iSerialNum = serial >> NUM_ENT_ENTRY_BITS;
	EHANDLE hEnt( iEntity, iSerialNum );

	return hEnt.Get();
}

bp::list pyGetEntitiesInBox( const char *classname, Vector origin, Vector mins, Vector maxs )
{
	CBaseEntity *pEnts[256];
	int found = UTIL_EntitiesInBox( pEnts, 256, origin + mins, origin + maxs, FL_OBJECT );

	bp::list ents;
	for ( int i=0; i < found; i++ )
	{
		if ( !classname || FClassnameIs( pEnts[i], classname ) )
			ents.append( bp::ptr(pEnts[i]) );
	}

	return ents;
}

void pyFireEntityInput(const char *target, const char *targetInput, bp::object value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller)
{
	bp::extract<char*> to_string(value);
	bp::extract<int> to_int(value);
	bp::extract<float> to_float(value);
	bp::extract<bool> to_bool(value);

	bp::extract<CBaseEntity*> to_ent(value);
	bp::extract<color32> to_color(value);
	bp::extract<Vector> to_vector(value);

	variant_t valueVariant;

	if (to_string.check())
	{
		valueVariant.SetString(AllocPooledString(to_string()));
	}
	else if (to_int.check())
	{
		valueVariant.SetInt(to_int());
	}
	else if (to_float.check())
	{
		valueVariant.SetFloat(to_float());
	}
	else if (to_bool.check())
	{
		valueVariant.SetBool(to_bool());
	}
	else if (to_ent.check())
	{
		valueVariant.SetEntity(to_ent());
	}
	else if (to_color.check())
	{
		valueVariant.SetColor32(to_color());
	}
	else if (to_vector.check())
	{
		valueVariant.SetVector3D(to_vector());
	}

	g_EventQueue.AddEvent(target, targetInput, valueVariant, fireDelay, pActivator, pCaller);
}

void pyDamageEnt(CBaseEntity *pEntity, CBaseEntity *pAttacker, float damage)
{
	if (!pEntity)
	{
		Warning("Tried to damage non-existant entity!\n");
		return;
	}

	CTakeDamageInfo damageInfo;

	damageInfo.SetDamage(damage);

	damageInfo.SetAttacker(pAttacker);

	damageInfo.SetInflictor(pAttacker);
	damageInfo.SetWeapon(NULL);

	damageInfo.SetDamageType(DMG_COMMAND);

	damageInfo.SetDamagePosition(pEntity->GetAbsOrigin());

	damageInfo.SetDamageForce(vec3_origin);

	pEntity->TakeDamage(damageInfo);
}

void pyAcceptEntityInput(CBaseEntity *pEntity, const char *targetInput, bp::object value, CBaseEntity *pActivator, CBaseEntity *pCaller)
{
	bp::extract<char*> to_string(value);
	bp::extract<int> to_int(value);
	bp::extract<float> to_float(value);
	bp::extract<bool> to_bool(value);

	bp::extract<CBaseEntity*> to_ent(value);
	bp::extract<color32> to_color(value);
	bp::extract<Vector> to_vector(value);

	variant_t valueVariant;

	if (to_string.check())
	{
		valueVariant.SetString(AllocPooledString(to_string()));
	}
	else if (to_int.check())
	{
		valueVariant.SetInt(to_int());
	}
	else if (to_float.check())
	{
		valueVariant.SetFloat(to_float());
	}
	else if (to_bool.check())
	{
		valueVariant.SetBool(to_bool());
	}
	else if (to_ent.check())
	{
		valueVariant.SetEntity(to_ent());
	}
	else if (to_color.check())
	{
		valueVariant.SetColor32(to_color());
	}
	else if (to_vector.check())
	{
		valueVariant.SetVector3D(to_vector());
	}

	pEntity->AcceptInput(targetInput, pActivator, pCaller, valueVariant, 0);
}

CBaseEntity *pyFindEntityByName(const char *szName, CBaseEntity *pStartEntity, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller)
{
	return gEntList.FindEntityByName(pStartEntity, szName, pSearchingEntity, pActivator, pCaller);
}

CBaseEntity *pyFindEntityByClassname(const char *szName, CBaseEntity *pStartEntity)
{
	return gEntList.FindEntityByClassname(pStartEntity, szName);
}

// The follow funcs are for simple virtualization
const QAngle &pyEyeAngles( CBaseEntity *pEnt )
{
	return pEnt->EyeAngles();
}

bool pyIsPlayer( CBaseEntity *pEnt )
{
	return pEnt->IsPlayer();
}

bool pyIsNPC( CBaseEntity *pEnt )
{
	return pEnt->IsNPC();
}

void pySetTargetName( CBaseEntity *pEnt, const char *name )
{
	if ( pEnt )
		pEnt->SetName( AllocPooledString( name ) );
}

const char *pyGetTargetName( CBaseEntity *pEnt )
{
	if ( pEnt )
		return pEnt->GetEntityName().ToCStr();
	return "";
}

CGEPlayer *pyGetOwner( CBaseEntity *pEnt )
{
	return ToGEPlayer( pEnt->GetOwnerEntity() );
}

CBaseEntity *pyGetGroundEntity( CBaseEntity *pEnt )
{
	return pEnt->GetGroundEntity();
}

static const char *EHANDLE_str( CHandle<CBaseEntity> &h )
{
	if ( h.Get() )
		return h->GetClassname();
	else
		return "Invalid Entity Handle";
}

// Compares CBaseEntity, EHANDLE to CBaseEntity, EHANDLE, and UID (int)
bool CBaseEntity_eq( bp::object a, bp::object b )
{
	CBaseEntity *a_cmp;
	bp::extract<CBaseEntity*> a_ent( a );
	bp::extract< EHANDLE > a_hent( a );

	if ( a_ent.check() )
		a_cmp = a_ent();
	else if ( a_hent.check() )
		a_cmp = a_hent().Get();
	else
		// Bad first param
		return false;

	bp::extract<CBaseEntity*>	b_ent( b );
	bp::extract< EHANDLE >		b_hent( b );
	bp::extract<int>			b_ient( b );

	if ( b_ent.check() )
		return a_cmp == b_ent();
	else if ( b_hent.check() )
		return a_cmp->GetRefEHandle() == b_hent();
	else
		return b_ient.check() && a_cmp->GetRefEHandle().ToInt() == b_ient();
}

// Anti-Compares CBaseEntity, EHANDLE to CBaseEntity, EHANDLE, and UID (int)
bool CBaseEntity_ne( bp::object a, bp::object b )
{
	CBaseEntity *a_cmp;
	bp::extract<CBaseEntity*> a_ent( a );
	bp::extract< EHANDLE > a_hent( a );

	if ( a_ent.check() )
		a_cmp = a_ent();
	else if ( a_hent.check() )
		a_cmp = a_hent().Get();
	else
		// Bad first param
		return false;

	bp::extract<CBaseEntity*>	b_ent( b );
	bp::extract< EHANDLE >		b_hent( b );
	bp::extract<int>			b_ient( b );

	if ( b_ent.check() )
		return a_cmp != b_ent();
	else if ( b_hent.check() )
		return a_cmp->GetRefEHandle() != b_hent();
	else
		return !b_ient.check() || a_cmp->GetRefEHandle().ToInt() != b_ient();
}

BOOST_PYTHON_MODULE(GEEntity)
{
	using namespace boost::python;

	def("GetUniqueId", pyGetEntSerial);
	def("GetUID", pyGetEntSerial);
	def("GetEntByUniqueId", pyGetEntByUniqueId, return_value_policy<reference_existing_object>());
	def("GetEntByUID", pyGetEntByUniqueId, return_value_policy<reference_existing_object>());

	def("GetEntitiesInBox", pyGetEntitiesInBox);

	def("FireEntityInput", pyFireEntityInput);

	def("FindEntityByName", pyFindEntityByName, return_value_policy<reference_existing_object>());
	def("FindEntityByClassname", pyFindEntityByClassname, return_value_policy<reference_existing_object>());

	class_< EHANDLE >("EntityHandle", init<CBaseEntity*>())
		.def("__str__", EHANDLE_str)
		.def("__eq__", CBaseEntity_eq)
		.def("__ne__", CBaseEntity_ne)
		.def("Get", &CHandle<CBaseEntity>::Get, return_value_policy<reference_existing_object>())
		.def("GetUID", &CBaseHandle::ToInt);

	class_<CBaseEntity, boost::noncopyable>("CBaseEntity", no_init)
		.def("__str__", &CBaseEntity::GetClassname)
		.def("__eq__", CBaseEntity_eq)
		.def("__ne__", CBaseEntity_ne)
		.def("GetParent", &CBaseEntity::GetParent, return_value_policy<reference_existing_object>())
		.def("GetPlayerOwner", pyGetOwner, return_value_policy<reference_existing_object>())
		.def("GetOwner", &CBaseEntity::GetOwnerEntity, return_value_policy<reference_existing_object>())
		.def("GetClassname", &CBaseEntity::GetClassname)
		.def("SetTargetName", pySetTargetName)
		.def("GetTargetName", pyGetTargetName)

		.def("Damage", pyDamageEnt)

		.def("FireInput", pyAcceptEntityInput)

		.def("SetModel", &CBaseEntity::SetModel)
		.def("IsAlive", &CBaseEntity::IsAlive)
        .def("GetGroundEntity", pyGetGroundEntity, return_value_policy<reference_existing_object>())
		.def("IsPlayer", pyIsPlayer)
		.def("IsNPC", pyIsNPC)
        .def("IsWorld", &CBaseEntity::IsWorld)
        .def("IsInWorld", &CBaseEntity::IsInWorld)
		.def("GetTeamNumber", &CBaseEntity::GetTeamNumber)
		.def("GetAbsOrigin", &CBaseEntity::GetAbsOrigin, return_value_policy<copy_const_reference>())
		.def("SetAbsOrigin", &CBaseEntity::SetAbsOrigin)
		.def("GetAbsAngles", &CBaseEntity::GetAbsAngles, return_value_policy<copy_const_reference>())
		.def("SetAbsAngles", &CBaseEntity::SetAbsAngles)
        .def("GetAbsVelocity", &CBaseEntity::GetAbsVelocity, return_value_policy<copy_const_reference>())
		.def("SetAbsVelocity", &CBaseEntity::SetAbsVelocity)
		.def("GetEyeAngles", pyEyeAngles, return_value_policy<copy_const_reference>())
		.def("GetUID", pyGetEntSerial)
		.def("GetIndex", &CBaseEntity::entindex);
}
