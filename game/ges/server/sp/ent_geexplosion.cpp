///////////// Copyright � 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ent_geexplosion.cpp
//   Description :
//      [TODO: Write the purpose of ent_geexplosion.cpp.]
//
//   Created On: 12/30/2008 5:04:07 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
//	 Edited By: Jonathan White (Killermonkey)
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ent_envexplosion.h"
#include "particle_system.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "steamjet.h"
#include "ge_gamerules.h"
#include "ge_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GE_EXP_SMALL_SIZE	80.0f	// If you change this make sure you change gamerules.cpp line 466
#define GE_EXP_SPAWN_DELAY	0.25f
#define GE_EXP_DIE_DELAY	0.5f
#define GE_EXP_DMG_TIME		2.5f
#define GE_EXP_REFRESH_TIME	1.7f

extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion

#define PAINTBALL_BOMB "PaintballBombSplats"
extern ConVar ge_paintball;

class CGE_Explosion : public CParticleSystem
{
public:
	DECLARE_CLASS( CGE_Explosion, CParticleSystem );
	DECLARE_DATADESC();

	CGE_Explosion();

	void Precache();
	void Spawn();
	void Activate();

	void CreateInitialBlast();

	void Think();
	
	void SetDamage( float dmg );
	void SetDamageRadius( float dmgR );
    void SetDamageCap( int dmgC );
    void SetPushForceMult( float mult );
	void SetLifetimeMult(float mult);
	void SetOwner( CBaseEntity *owner );
	void SetActivator( CBaseEntity *activator);

	bool IsSmallExp() { return m_flDamageRadius <= GE_EXP_SMALL_SIZE; };

protected:
	void CreateHeatWave( void );
	void DestroyHeatWave( void );

private:
	float m_flDamage;
    int m_iDamageCap;
	float m_flDamageRadius;
    float m_flPushForceMult;

	float m_flLifeDuration;
	float m_flNextRefresh;

	float m_flDieTime;
	float m_flShakeTime;

	EHANDLE m_hOwner;
	EHANDLE m_hActivator;
	EHANDLE m_hHeatWave;
};

LINK_ENTITY_TO_CLASS( ge_explosion, CGE_Explosion );
BEGIN_DATADESC( CGE_Explosion )
	DEFINE_THINKFUNC(Think),
END_DATADESC()

CGE_Explosion::CGE_Explosion()
{
	m_flDamage = 320;
	m_flDamageRadius = 260;
    m_flPushForceMult = 1.0f;
	m_flShakeTime = 0;
	m_flDieTime = 0;
	m_flNextRefresh = 0.0f;

	m_flLifeDuration = GE_EXP_DMG_TIME;
}

void CGE_Explosion::SetDamage( float dmg )
{
	m_flDamage = dmg;
}

void CGE_Explosion::SetDamageRadius( float dmgR )
{
	m_flDamageRadius = dmgR;
}

void CGE_Explosion::SetDamageCap( int dmgC )
{
	m_iDamageCap = dmgC;
}

void CGE_Explosion::SetPushForceMult( float mult )
{
    m_flPushForceMult = mult;
}

void CGE_Explosion::SetLifetimeMult( float mult )
{
	float lifediff = mult * GE_EXP_DMG_TIME - m_flLifeDuration;

	m_flDieTime += lifediff;
	m_flLifeDuration = mult * GE_EXP_DMG_TIME;
}

void CGE_Explosion::SetActivator( CBaseEntity *activator)
{
	m_hActivator.Set( activator );
}

void CGE_Explosion::SetOwner( CBaseEntity *owner)
{
	m_hOwner.Set( owner );
}

void CGE_Explosion::Precache()
{
	PrecacheScriptSound( "Explosion.small" );
	PrecacheScriptSound( "Explosion.large" );

	BaseClass::Precache();
}

void CGE_Explosion::Spawn()
{
	m_takedamage = DAMAGE_NO;

	KeyValue( "start_active", "1" );

	if ( IsSmallExp() )
		KeyValue( "effect_name", "GES_Explosion_B_Tiny" );
	else
		KeyValue( "effect_name", "GES_Explosion_B" );

	BaseClass::Spawn();
}

void CGE_Explosion::Activate()
{
	BaseClass::Activate();

	m_flDieTime = gpGlobals->curtime + m_flLifeDuration;
	m_flNextRefresh = gpGlobals->curtime + GE_EXP_REFRESH_TIME;

	if ( !IsSmallExp() )
	{
		EmitSound("Explosion.large");
	}
	else
	{
		EmitSound("Explosion.small");
	}

	m_flShakeTime = gpGlobals->curtime;

	// Start off with a BANG ;-)
	CreateInitialBlast();

	SetThink( &CGE_Explosion::Think );
	SetNextThink( gpGlobals->curtime );
}

// Create a HL2 Generic Blast to start off the fireworks. This does NO DAMAGE and doesn't display smoke.
void CGE_Explosion::CreateInitialBlast( void )
{
	trace_t		tr, tr2;
	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents ( vecAbsOrigin );
	Vector dirs[5] = { Vector(0, 0, 48), Vector(0, 48, 0), Vector(0, -48, 0), Vector(48, 0, 0), Vector(-48, 0, 0) };


	// Trace along each axis to determine closest surface so explosion decals can also appear on walls and ceilings.

	// Start with a straight down trace, only colliding with the world.  Slightly longer so that the floor is favored when explosions hit corners.
	UTIL_TraceLine(vecAbsOrigin, vecAbsOrigin + Vector(0, 0, -64), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

	// Run through each direction comparing each trace length to the current longest in order to find the shortest one.
	// Then use that one for the rest of the code.

	for (int i = 0; i < 5; i++)
	{
		UTIL_TraceLine(vecAbsOrigin, vecAbsOrigin + dirs[i], MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr2);

		if (tr2.fraction < tr.fraction)
			tr = tr2;
	}

	if ( tr.fraction != 1.0 )
	{
		Vector vecNormal = tr.plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );	
		CPASFilter filter( vecAbsOrigin );

		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin,
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			min(m_flDamageRadius, 500) * 0.03f, 
			25,
			TE_EXPLFLAG_NOFIREBALLSMOKE | TE_EXPLFLAG_NOSOUND,
			min(m_flDamageRadius, 500) * 0.8f,
			0.0f,
			&vecNormal,
			(char) pdata->game.material );
	}
	else
	{
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin, 
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			min(m_flDamageRadius, 500) * 0.03f,
			25,
			TE_EXPLFLAG_NOFIREBALLSMOKE | TE_EXPLFLAG_NOSOUND,
			min(m_flDamageRadius, 500) * 0.8f,
			0.0f );
	}

	CSoundEnt::InsertSound( SOUND_DANGER, GetAbsOrigin(), min(m_flDamageRadius, 500)*1.5f, m_flLifeDuration, this );

	//Wreak paintball havok if paintball mode is on
	//otherwise just place a scorch mark down
	if( ge_paintball.GetBool() )
	{
		//Paint the big splat where the bomb went off
		UTIL_DecalTrace( &tr, PAINTBALL_BOMB );

		//Create a bunch of impacts around
		int splatCount =  random->RandomInt( 3, 5 );
		for( int i = 0; i < splatCount; i++ )
		{
			Vector	vecStart, vecStop, vecDir;

			// Snap our facing to where we are heading
			QAngle randAngle;
			randAngle.Random( 0, 360 );
			// get the vectors
			vecStart = GetAbsOrigin();
			vecStop = vecStart + vecDir * 150;
			AngleVectors( randAngle, &vecDir );

			UTIL_TraceLine( vecStart, vecStop, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

			// check to see if we hit something
			if ( tr.m_pEnt )
				if ( tr.m_pEnt->IsWorld() || tr.m_pEnt->IsPlayer() )
				{
					//Paint the big splat where the bomb went off
					UTIL_DecalTrace( &tr, PAINTBALL_BOMB );
				}
		}
	}
	else
		UTIL_DecalTrace( &tr, "Scorch" );

	// Generate the heat effects
	CreateHeatWave();
}


void CGE_Explosion::Think()
{ 
	if (gpGlobals->curtime > m_flDieTime + GE_EXP_DIE_DELAY * ( m_flLifeDuration / GE_EXP_DMG_TIME ))
	{
		StopParticleSystem();
		UTIL_Remove(this);
	}
	else if ( gpGlobals->curtime < m_flDieTime )
	{
		if ( !m_bActive )
		{
			StartParticleSystem();
			m_flNextRefresh = gpGlobals->curtime + GE_EXP_REFRESH_TIME;
		}
		else if ( gpGlobals->curtime > m_flNextRefresh && gpGlobals->curtime + 1.0f < m_flDieTime )
		{
			StopParticleSystem();
			m_flStartTime = gpGlobals->curtime;
			//SetNextThink(gpGlobals->curtime);
		}

		// Only apply damage if our owner and activator are still valid
		if ( m_hActivator.Get() && m_hOwner.Get() && m_flDamageRadius > 0 && m_flDamage > 0 )
		{
			CTakeDamageInfo info( m_hActivator.Get(), m_hOwner.Get(), vec3_origin, GetAbsOrigin(), m_flDamage, DMG_BLAST );
            info.SetDamageCap( m_iDamageCap );
			g_pGameRules->RadiusDamage( info, GetAbsOrigin(), m_flDamageRadius, CLASS_NONE, NULL, m_flPushForceMult );
		}

		if ( gpGlobals->curtime > m_flShakeTime && gpGlobals->curtime < m_flDieTime + 1.0f )
		{
			float radius = (m_flDamageRadius > 0) ? ( min(m_flDamageRadius, 300) * (IsSmallExp() ? 0.75f : 1.0f)) : 45.0f;
			float freq = IsSmallExp() ? 50.0f : 100.0f;
			float amp = IsSmallExp() ? 3.0 : 7.0;
			
			m_flShakeTime = gpGlobals->curtime + 1.0f;
			UTIL_ScreenShake( GetAbsOrigin(), amp, freq, 1.0f, radius, SHAKE_START );
		}

		// Apply another round of damage in 1/10 of a second
		SetNextThink( gpGlobals->curtime + GE_EXPLOSION_THINK_INTERVAL );
	}
	else
	{
		DestroyHeatWave();
		SetNextThink( gpGlobals->curtime + GE_EXPLOSION_THINK_INTERVAL );
	}
}


void CGE_Explosion::CreateHeatWave( void )
{
	return; // We can't afford to have this effect anymore, we're pushing things as it is.

	static Vector offset( -40.0f, 0, 70.0f );

	float scale = 1.0f;
	if ( m_flDamageRadius <= GE_EXP_SMALL_SIZE )
		scale = 0.4f;

	CSteamJet *pHeatWave = (CSteamJet*)CBaseEntity::CreateNoSpawn( "env_steam", GetAbsOrigin() + offset, vec3_angle, this );
	pHeatWave->m_nType = STEAM_HEATWAVE;
	pHeatWave->m_SpreadSpeed = 20.0f;
	pHeatWave->m_Speed = 120.0f;
	pHeatWave->m_StartSize = 30.0f * scale;
	pHeatWave->m_EndSize = 45.0f * scale;
	pHeatWave->m_Rate = 26.0f;
	pHeatWave->m_JetLength = 80.0f * scale;
	pHeatWave->m_InitialState = true;
	pHeatWave->m_bIsForExplosion = true;
	pHeatWave->m_flStartFadeTime = gpGlobals->curtime + m_flLifeDuration - 0.5f;
	pHeatWave->m_flFadeDuration = 0.5f;

	DispatchSpawn( pHeatWave );

	m_hHeatWave = pHeatWave;
}

void CGE_Explosion::DestroyHeatWave( void )
{
	return; // Can't destroy what we don't have.

	if ( m_hHeatWave.Get() )
	{
		m_hHeatWave->Remove();
		m_hHeatWave = NULL;
	}
}

CBaseEntity* Create_GEExplosion( CBaseEntity* owner, CBaseEntity* activator, const Vector pos, float dmg, float dmgRadius, int dmgCap, float pushScale, float lifetimeScale )
{
	// We must have an owner
	if ( !owner )
		return NULL;

	CGE_Explosion *pExp = (CGE_Explosion*)CreateEntityByName( "ge_explosion" );

	if ( pExp )
	{
		pExp->SetOwner( owner );
		pExp->SetActivator( activator == NULL ? owner : activator );
		pExp->SetDamage( dmg );
        pExp->SetDamageCap( dmgCap );
		pExp->SetDamageRadius( dmgRadius );
		pExp->SetAbsOrigin( pos );
        pExp->SetPushForceMult( pushScale );
		pExp->SetLifetimeMult( lifetimeScale );

		DispatchSpawn( pExp );
		pExp->Activate();
	}

	return pExp;
}

CBaseEntity* Create_GEExplosion( CBaseEntity* owner, const Vector pos, float dmg, float dmgRadius, int dmgCap, float pushScale, float lifetimeScale )
{
	return Create_GEExplosion( owner, NULL, pos, dmg, dmgRadius, dmgCap, pushScale, lifetimeScale );
}