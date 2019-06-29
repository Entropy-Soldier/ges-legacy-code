///////////// Copyright 2019 Team GoldenEye:Source. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ent_head.cpp
//   Description :
//      Custom entity for modular player heads that support eye shaders and flex effects.
//
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ent_head.h"
#include "ge_player.h"
#include "engine/ivmodelrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGE_Head : public CBaseFlex
{
public:
	DECLARE_CLASS( CGE_Head, CBaseFlex );
	DECLARE_DATADESC();

    CGE_Head();

	void Spawn( void );
	void Precache( void );

	void RemoveThink();
	void KnockHatOff();
	void Activate();
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

    virtual int UpdateTransmitState();

private:
	char *m_szModel;
    bool m_bIsAttached;
};

LINK_ENTITY_TO_CLASS( prop_head, CGE_Head );
BEGIN_DATADESC( CGE_Head )
	DEFINE_KEYFIELD(m_szModel, FIELD_STRING, "HeadModel"),
END_DATADESC()

CGE_Head::CGE_Head()
{
    m_bIsAttached = true;
}

void CGE_Head::Precache( void )
{
	if (m_szModel)
		PrecacheModel( m_szModel );
}

// Always update heads if they're attached to players or ragdolls.
int CGE_Head::UpdateTransmitState()
{
	if (m_bIsAttached)
		return SetTransmitState(FL_EDICT_PVSCHECK);
	else
		return BaseClass::UpdateTransmitState();
}

void CGE_Head::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	ApplyAbsVelocityImpulse(info.GetDamageForce());
	BaseClass::TraceAttack(info, vecDir, ptr);
}

void CGE_Head::Spawn( void )
{
	if (!m_szModel || m_szModel[0] == '\0')
	{
		Warning("Tried to spawn head with NULL Model.\n");
		UTIL_Remove(this);
		return;
	}

	Precache();
	SetModel( m_szModel );
    m_bIsAttached = true;

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
}

void CGE_Head::Activate()
{
	KnockHatOff();
}

void CGE_Head::KnockHatOff()
{
	StopFollowingEntity();

	VPhysicsDestroyObject();
	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	
	SetThink( &CGE_Head::RemoveThink );
	SetNextThink( gpGlobals->curtime + 300.0);

    m_bIsAttached = false;
}

void CGE_Head::RemoveThink()
{
	UTIL_Remove(this);
}

extern IVModelRender *modelrender;

CBaseEntity *CreateNewHead( const Vector &position, const QAngle &angles, const char* hatModel)
{
	if (!hatModel || hatModel[0] == '\0')
	{
		Warning("Trying to create a Null Head!!\n");
		return NULL;
	}

	CGE_Head *pHead = (CGE_Head *)CBaseFlex::CreateNoSpawn( "prop_head", position, angles, NULL );
	pHead->KeyValue("HeadModel", hatModel);
	DispatchSpawn( pHead );
	return pHead;
}
