///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: weapon_gebasemelee.cpp
// Description:
//      All melee weapons will derive from this (such as knife/slappers).
//
// Created On: 3/9/2006 2:28:17 AM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_weaponmelee.h"
#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "animation.h"
#include "takedamageinfo.h"

#if defined( CLIENT_DLL )
	#include "c_ge_player.h"
#else
	#include "ge_player.h"
	#include "npc_gebase.h"
	#include "ndebugoverlay.h"
	#include "te_effect_dispatch.h"
	#include "ilagcompensationmanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponMelee, DT_GEWeaponMelee )

BEGIN_NETWORK_TABLE( CGEWeaponMelee, DT_GEWeaponMelee )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CGEWeaponMelee )
END_PREDICTION_DATA()

#ifdef GAME_DLL
	ConVar ge_debug_melee( "ge_debug_melee", "0", FCVAR_GAMEDLL | FCVAR_CHEAT );
#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CGEWeaponMelee::CGEWeaponMelee()
{
	m_bFiresUnderwater = true;
	// NPC Ranging
	m_fMinRange1	= 0;
	m_fMinRange2	= 0;
	m_fMaxRange1	= GetRange() * 0.9;
	m_fMaxRange2	= GetRange() * 0.9;
}

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CGEWeaponMelee::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	} 
	else 
	{
		WeaponIdle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add's a view kick, multiplied when you actually hit something!
//-----------------------------------------------------------------------------
void CGEWeaponMelee::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "gemeleepax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "gemeleepay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

#ifdef GAME_DLL
int CGEWeaponMelee::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	if ( flDist > GetRange() )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if ( flDot < 0.7 )
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}
#endif

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGEWeaponMelee::PrimaryAttack()
{
	// Make sure the bots don't go absolutely balastic with their attack speed.
	if (gpGlobals->curtime < m_flNextPrimaryAttack)
		return;

	// Get pointer to weapon owner.
	CGEPlayer *pGEPlayer = ToGEPlayer( GetOwner() );

	// Play animations and sounds
	SendWeaponAnim( GetStaticHitActivity() );

	if ( pGEPlayer )
		pGEPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	WeaponSound( SINGLE );

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();

	BaseClass::PrimaryAttack();
}

// Actually initate the attack.
void CGEWeaponMelee::FireWeapon()
{
#ifndef CLIENT_DLL
	CGEPlayer *pPlayer = ToGEPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#else
	CUtlVector<C_GEPlayer*> ents;
	for ( int i=1; i < gpGlobals->maxClients; i++ )
	{
		C_GEPlayer *pPlayer = ToGEPlayer( UTIL_PlayerByIndex(i) );
		if ( !pPlayer )
			continue;

		pPlayer->AdjustCollisionBounds( true );
		ents.AddToTail( pPlayer );
	}
#endif

	Swing( false );

#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#else
	for ( int i=0; i < ents.Count(); i++ )
		ents[i]->AdjustCollisionBounds( false );
#endif
}

//------------------------------------------------------------------------------
// Purpose : Starts the swing of the weapon and determines the animation
// Input   : bIsSecondary - is this a secondary attack?
//------------------------------------------------------------------------------
void CGEWeaponMelee::Swing( int bIsSecondary )
{

	// ------------------------------
	//	Perform initial attack trace
	// ------------------------------

	trace_t traceHit;

	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

#ifdef CLIENT_DLL
	Vector swingStart = pOwner->EyePosition();
#else
	Vector swingStart = pOwner->Weapon_ShootPosition();
#endif
	Vector forward;

	AngleVectors( pOwner->EyeAngles(), &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT | CONTENTS_GRATE, pOwner, COLLISION_GROUP_NONE, &traceHit );

#ifndef CLIENT_DLL
	// Like bullets, bludgeon traces have to trace against triggers.
	CTakeDamageInfo triggerInfo( pOwner, pOwner, GetWeaponDamage(), DMG_CLUB );
    triggerInfo.SetDamageCap( GetDamageCap() );
	TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin );
#endif

	// --------------------------------------------------------------------
	//	Attempt to redirect attack trace if target meets right conditions.
	// --------------------------------------------------------------------

	// If we swing in an arc and we didn't hit a player on the direct trace, do some additional checks.  This means that if the user was looking directly at a player
	// that player will always be hit at the location the user was aiming at, but if they weren't then they still
	// have a good chance to hit someone close to their crosshair.

	if ( SwingsInArc() && (traceHit.fraction == 1.0 || (!traceHit.m_pEnt->IsPlayer() || !traceHit.m_pEnt->IsNPC())) )
	{
		// This is just used to approximate the extra distance to add to the radius check to make sure we include
		// all eligible entities, so we can just pretend the player is a giant cube and calculate the max possible distance from that.

		// Distance from cube corner to center = sqrt(3dist^2) or dist * sqrt(3), sqrt(3) = 1.732
		// Using player height as that's the longest dimension

		float maxPlayerDist = GEMPRules()->GetViewVectors()->m_vHullMax.z * 1.732;
		
		CGEPlayer *targetPlayer = NULL;
		float targetDist = GetRange() + maxPlayerDist;

		// First identify potential victims, choose the closest one.
		for (int i = 1; i <= gpGlobals->maxClients; ++i) 
		{
			CGEPlayer *pPlayer = ToGEPlayer(UTIL_PlayerByIndex(i));

			if (pPlayer == NULL || !pPlayer->IsPlayer())
				continue;

			// Don't hit spectating players.
			if (pPlayer->IsObserver())
				continue;

			// If they're not alive don't go huge.
			if (!pPlayer->IsAlive())
				continue;

			// Don't pay attention to teammates.
			if (GEMPRules()->IsTeamplay() && pPlayer->GetTeamNumber() == pOwner->GetTeamNumber())
				continue;

			// Use the best of 2 distance measurements to figure out if this player is in range.
			Vector diff1 = pPlayer->EyePosition() - pOwner->EyePosition();
			Vector diff2 = pPlayer->GetAbsOrigin() - pOwner->GetAbsOrigin();

			// Only do one sqrt operation to save time.
			float dist = min( diff1.LengthSqr(), diff2.LengthSqr() );
			dist = sqrtf(dist);

			// Make sure they're in range or closer than the current best target.
			if ( dist > targetDist )
				continue;

			// Draw a line from the player to the target to use for direction calculations.
			// Use eye position to more effectively track crouching players.
			Vector vecToTarget = pPlayer->EyePosition() - pOwner->EyePosition();
			VectorNormalize(vecToTarget);

			float dot = vecToTarget.Dot(forward);

			// If we don't have a target player at all, we'll accept someone who's within 75 degrees of our crosshair.
			// If we do, we require at least 45 degrees to switch.  The chance of 3 people being within the short
			// reach of a melee weapon is pretty small, so it's probably fine to favor the center target even if it
			// would ultimately sacrifice a better side-target.
			if ( (!targetPlayer && dot < 0.26) || dot < 0.7 )
				continue;
			
			// We made it to the end, which means this player is better than the previous target, so make them the new target.
			targetPlayer = pPlayer;
			targetDist = dist;
		}

		// Make sure there's someone to hit, if not then allow the previous miss to occour.
		if (targetPlayer)
		{
			// Identify 2 points.  Face, and Chest.  We don't care about arm hits, they'll get treated like chest hits anyway.
			// They're calculated this way to compensate for crouching players and other states.

			Vector headPos = targetPlayer->EyePosition() + Vector(0, 0, 4); // Slightly higher than eye position as eye position is actually in the neck.
			Vector chestPos = (headPos * 3 + targetPlayer->GetAbsOrigin())/4 - Vector(0, 0, 4); //3 fourths the distance between the ground and the target's eyes

			//First check to see if we can get a headshot.
			Vector vecToheadPos = headPos - swingStart;
			Vector vecTochestPos = chestPos - swingStart;

			float vecToheadLength = vecToheadPos.Length();
			float vecTochestLength = vecTochestPos.Length();

			VectorNormalize(vecToheadPos);
			VectorNormalize(vecTochestPos);

			float dothead = vecToheadPos.Dot(forward);
			float dotchest = vecTochestPos.Dot(forward);

			int refireTraceTarget = -1;
			Vector targetloc;

			// The closer you are, the easier it is to get hits.  Ranges from 1 when at max distance to 0 when point blank.  Can never actually hit zero though due to player volume.
			
			float hullwidth = GEMPRules()->GetViewVectors()->m_vHullMax.x * 1.8;
			float effectiverange = GetRange() - hullwidth;

			// These can actually go slightly negative sometimes since hullwidth is only approximating the minimum distance.
			// Not a big deal, it just means the starting values in the hit detecting equations aren't the lowest.
			float rangefactor1 = (vecToheadLength - hullwidth) / effectiverange;
			float rangefactor2 = (vecTochestLength - hullwidth) / effectiverange;

			// First try for a headshot, the criteria for this is much more demanding 
			// so that someone can't just aim into the sky and get them every time.

			if ((dothead > dotchest || vecTochestLength > GetRange()) && dothead > 0.94 + 0.06 * rangefactor1 && vecToheadLength < GetRange())
			{
				targetloc = headPos;
				refireTraceTarget = HITGROUP_HEAD;
			}
			else if ( dotchest > 0.65 + 0.35 * rangefactor2 && vecTochestLength < GetRange() ) // Now try for a chest hit, they're much easier to get.
			{
				targetloc = chestPos;
				refireTraceTarget = HITGROUP_CHEST;
			}
			else //We can't hit anything, resign to missing.  If damageworld is false then set up the trace to fail, otherwise we can still damage entities we're looking straight at.
				DamageWorld() ? targetloc = swingEnd : targetloc = swingStart;

			if ( refireTraceTarget != -1 )
			{
				// Do a test trace to make sure we can actually hit the person before we overwrite parts of traceHit.
				trace_t testTrace;
				UTIL_TraceLine(swingStart, targetloc, MASK_SHOT | CONTENTS_GRATE, pOwner, COLLISION_GROUP_NONE, &testTrace);

				CBaseEntity *hitEntity = testTrace.m_pEnt;

				// Correct hitentity to the bot player controlling it if we hit an NPC.
#ifdef GAME_DLL
				if ( hitEntity && hitEntity->IsNPC() )
				{
					CNPC_GEBase *pNPC = (CNPC_GEBase*) hitEntity;
					if ( pNPC->GetBotPlayer() )
						hitEntity = pNPC->GetBotPlayer();
				}
#endif

				// Make sure we either hit them or it was possible to hit them.
				if ( testTrace.fraction == 1.0 || ( hitEntity && hitEntity == targetPlayer) )
				{
					UTIL_TraceLine(swingStart, targetloc, MASK_SHOT | CONTENTS_GRATE, pOwner, COLLISION_GROUP_NONE, &traceHit);

					if (!(traceHit.m_pEnt && traceHit.m_pEnt == targetPlayer && traceHit.hitgroup == refireTraceTarget))
					{
						traceHit.surface.name = "ForcedMeleeHit";

						if (refireTraceTarget == HITGROUP_HEAD)
							traceHit.hitbox = 8; // Head hitbox
						else
							traceHit.hitbox = 6; // Chest hitbox

						traceHit.hitgroup = refireTraceTarget; // Force a hit since it's possible we'll actually miss when at this point it should be impossible.
					}

					if (!traceHit.m_pEnt)
						traceHit.m_pEnt = targetPlayer;

					if (traceHit.fraction == 1.0)
						traceHit.fraction = 0.9; // Fudge the trace results and make it seem like we've hit them.
				}
			}
		}
	}

#ifdef GAME_DLL
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>( pOwner );

	if ( pPlayer )
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

	// -------------------------
	//	Handle Hit/Miss
	// -------------------------

	if ( traceHit.fraction == 1.0f )
	{
		// We want to test the first swing again
		Vector testEnd = swingStart + forward * GetRange();
		
		// See if we happened to hit water
		ImpactWater( swingStart, testEnd );

		// Make sure our line goes where we think it should.
#ifdef GAME_DLL
		if ( ge_debug_melee.GetBool() )
			debugoverlay->AddLineOverlay( traceHit.startpos, traceHit.endpos, 0, 255, 0, true, 3.5f );
#endif
	}
	else
	{
#ifdef GAME_DLL
		if ( pPlayer )
			gamestats->Event_WeaponHit( pPlayer, true, GetClassname(), triggerInfo );
#endif
		Hit( traceHit );

#ifdef GAME_DLL
	if ( ge_debug_melee.GetBool() )
		debugoverlay->AddLineOverlay( traceHit.startpos, traceHit.endpos, 255, 0, 0, true, 3.5f );
#endif
	}
	
	MakeTracer(swingStart, traceHit, TRACER_LINE);
}


//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CGEWeaponMelee::Hit( trace_t &traceHit )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;
	
	//Do view kick
	AddViewKick();

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if ( pHitEntity != NULL )
	{
		Vector hitDirection;
		//AngleVectors( pOwner->EyeAngles(), &hitDirection, NULL, NULL );

		hitDirection = traceHit.endpos - traceHit.startpos;
		VectorNormalize( hitDirection );

		// Take into account bots...
		CBaseCombatCharacter *pOwner = GetOwner();

	#ifdef GAME_DLL
		if ( pOwner->IsNPC() )
		{
			CNPC_GEBase *pNPC = (CNPC_GEBase*) pOwner;
			if ( pNPC->GetBotPlayer() )
				pOwner = pNPC->GetBotPlayer();
		}
	#endif

		CTakeDamageInfo info( pOwner, pOwner, GetDamageForActivity( GetActivity() ), DMG_CLUB );
		info.SetWeapon(this);
        info.SetDamageCap( GetDamageCap() );
		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos, 0.02 );
        info.ScaleDamageForce( GetWeaponPushForceMult() );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit );

	#ifndef CLIENT_DLL
		ApplyMultiDamage();
		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
	#endif

		WeaponSound( MELEE_HIT );
	}

	// Apply an impact effect
	ImpactEffect( traceHit );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &traceHit - 
//-----------------------------------------------------------------------------
bool CGEWeaponMelee::ImpactWater( const Vector &start, const Vector &end )
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...
	
	// We must start outside the water
	if ( UTIL_PointContents( start ) & (CONTENTS_WATER|CONTENTS_SLIME))
		return false;

	// We must end inside of water
	if ( !(UTIL_PointContents( end ) & (CONTENTS_WATER|CONTENTS_SLIME)))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine( start, end, (CONTENTS_WATER|CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace );

	if ( waterTrace.fraction < 1.0f )
	{
#ifndef CLIENT_DLL
		CEffectData	data;

		data.m_fFlags  = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = 8.0f;

		// See if we hit slime
		if ( waterTrace.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect( "watersplash", data );			
#endif
	}

	return true;
}