///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ent_capturearea.cpp
// Description:
//      See header
//
// Created On: 01 Aug 11
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "gemp_gamerules.h"
#include "ge_tokenmanager.h"
#include "ge_gameplay.h"
#include "npc_gebase.h"
#include "ent_capturearea.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAPTURE_MODEL "models/gameplay/capturepoint.mdl"

LINK_ENTITY_TO_CLASS( ge_capturearea, CGECaptureArea );

PRECACHE_REGISTER( ge_capturearea );

IMPLEMENT_SERVERCLASS_ST( CGECaptureArea, DT_GECaptureArea )
	SendPropBool( SENDINFO(m_bEnableGlow) ),
	SendPropInt( SENDINFO(m_GlowColor), 32, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_GlowDist) ),
END_SEND_TABLE()

CGECaptureArea::CGECaptureArea()
{
	m_szGroupName[0] = '\0';
	m_fRadius = 32.0f;
    m_fZRadius = 32.0f;

	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
}

void CGECaptureArea::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	// Do a trace from the origin of the object straight down to find the floor
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,-1024), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid && tr.fraction < 1.0 )
		SetAbsOrigin( tr.endpos );
	
	AddFlag( FL_OBJECT );

	VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
	SetMoveType( MOVETYPE_NONE );
	SetBlocksLOS( false );
	SetCollisionGroup( COLLISION_GROUP_CAPAREA );
	
	// See if we are touching anything when we spawn
	ClearTouchingEntities();
	PhysicsTouchTriggers();

	GEMPRules()->GetTokenManager()->OnCaptureAreaSpawned( this );
}

void CGECaptureArea::Precache( void )
{
	PrecacheModel( CAPTURE_MODEL );
	BaseClass::Precache();
}

void CGECaptureArea::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
	if ( GEMPRules() )
		GEMPRules()->GetTokenManager()->OnCaptureAreaRemoved( this );
}

void CGECaptureArea::SetGroupName( const char* name )
{
	Q_strncpy( m_szGroupName, name, 32 );
}

void CGECaptureArea::SetCaptureBounds( float radius, float z_radius /*==-1.0f*/ )
{
	// Warn if we exceed our threshold radius
	if ( radius > 255.0f )
	{
		Warning( "[CAP AREA] The radius of a capture area cannot exceed 255 units, clipping!\n" );
		radius = 255.0f;
	}

    // Make sure our largest radius is within our prop's collision bounds as that's what determines if we do capture validity
    // checks in the first place.
    float biggest_radius = max( radius, z_radius );

	// Set the collision bloat, this makes sure we don't affect tracing!
    // If we have no radius set, just use the prop's standard collision bounds.
	if ( biggest_radius > 0 )
		CollisionProp()->UseTriggerBounds( true, biggest_radius );
	else
		CollisionProp()->UseTriggerBounds( false );

    // If radius isn't set, use the prop's bounding radius for it to make sure we have some sort of radius value.
    if ( radius > 0 )
        m_fRadius = radius;
    else
        m_fRadius = CollisionProp()->BoundingRadius2D();

    // If z_radius isn't set, just use 1/4 the standard radius for it to have good compatability with legacy values.
    if (z_radius < 0)
        m_fZRadius = m_fRadius / 4;
    else
        m_fZRadius = z_radius;

	// Recalculate our bounds
	CollisionProp()->MarkSurroundingBoundsDirty();

	// See if our new bounds encapsulates anyone nearby
	PhysicsTouchTriggers();
}

void CGECaptureArea::SetupGlow( bool state, Color glowColor /*=Color(255,255,255)*/, float glowDist /*=250.0f*/ )
{
	m_GlowColor.Set( glowColor.GetRawColor() );
	m_GlowDist.Set( glowDist );
	m_bEnableGlow = state;
}

void CGECaptureArea::ClearTouchingEntities( void )
{
	m_hTouchingEntities.RemoveAll();
}

void CGECaptureArea::Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() && !pOther->IsNPC() )
		return;

	if ( !GEMPRules()->GetTokenManager()->CanTouchCaptureArea( m_szGroupName, pOther ) )
		return;

	// Check if we are touching us already
	EHANDLE hPlayer = pOther->GetRefEHandle();
	if ( m_hTouchingEntities.Find( hPlayer ) != m_hTouchingEntities.InvalidIndex() )
		return;

	// We have passed all filters and this is a fresh touch, now enforce the radius.  First do a check in 2D.
	if ( (pOther->GetAbsOrigin() - GetAbsOrigin()).Length2D() <= m_fRadius )
	{
        // Resolve the player (or bot proxy)
		CGEPlayer *pPlayer = ToGEMPPlayer( pOther );
		if ( !pPlayer ) // We know we must be either a player or a bot.
			pPlayer = ToGEBotPlayer( pOther );

        float zdiff = pOther->GetAbsOrigin().z - GetAbsOrigin().z;

        // Get the difference between the player's eyes and their feet, so we have a good idea of their general height.  This will
        // account for crouching players, whereas using the bounds values directly would require a check for it.  
        float playerViewHeight = pPlayer->EyePosition().z - pPlayer->GetAbsOrigin().z;

        // Now make sure the player is within the Z radius, accounting for their height so tokens spawned
        // in the air with small radii don't require jumping.  The distance is measured from the player's absolute origin, which
        // is the bottom of their feet.  Extending the bounds downward by the player's height means that the distance is measured
        // both from the top of their head and the bottom of their feet, leading to more intuitive behavior.
        // This model creates a cylinder instead of a sphere, oriented along the Z axis.  It will also now extend through
        // the floor slightly, though any prospective capturing player will need to get their feet into said capture zone which
        // might prove difficult for most capture radius settings.
        if ( ( zdiff < -1 * (playerViewHeight + m_fZRadius) ) || ( zdiff > m_fZRadius ) )
            return;

		// Put us on the touching list
		m_hTouchingEntities.AddToTail( hPlayer );

		// Resolve our held token (note we must use the pOther)
		const char *szToken = GEMPRules()->GetTokenManager()->GetCaptureAreaToken(m_szGroupName);
		CGEWeapon *pToken = (CGEWeapon*) pOther->MyCombatCharacterPointer()->Weapon_OwnsThisType( szToken );

		// Notify the Gameplay
		GEGameplay()->GetScenario()->OnCaptureAreaEntered( this, pPlayer, pToken );
	}
}

void CGECaptureArea::EndTouch( CBaseEntity *pOther )
{
	EHANDLE hPlayer = pOther->GetRefEHandle();
	if ( m_hTouchingEntities.FindAndRemove( hPlayer ) )
	{
		// Resolve the player (or bot proxy)
		CGEPlayer *pPlayer = ToGEMPPlayer( pOther );
		if ( !pPlayer )
			pPlayer = ToGEBotPlayer( pOther );

		if ( GEGameplay() )
			GEGameplay()->GetScenario()->OnCaptureAreaExited( this, ToGEMPPlayer(pOther) );
	}
}
