///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weapon.cpp
// Description:
//      All Goldeneye Weapons derive from this class.
//
// Created On: 2/25/2008 1800
// Created By: Jonathan White <killermonkey01>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "npcevent.h"

#ifdef CLIENT_DLL
	// This is for the override to FireEvent
	#include "eventlist.h"
	#include "cl_animevent.h"
	#include "view.h"
	#include "c_te_legacytempents.h"

	#include "c_ge_player.h"
	#include "c_gemp_player.h"
	#include "ge_screeneffects.h"
#else
	#include "ge_player.h"
	#include "gemp_player.h"
	#include "npc_gebase.h"
	#include "particle_parse.h"
	#include "ge_tokenmanager.h"
	#include "ge_entitytracker.h"
#endif

#include "gemp_gamerules.h"
#include "ge_weapon.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

#ifdef CLIENT_DLL
ConVar cl_ge_nomuzzlesmoke("cl_ge_nomuzzlesmoke", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
#endif

// ----------------------------------------------------------------------------- //
// CGEWeapon tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeapon, DT_GEWeapon )

BEGIN_NETWORK_TABLE( CGEWeapon, DT_GEWeapon )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bSilenced ) ),
	RecvPropFloat( RECVINFO( m_flAccuracyPenalty ) ),
	RecvPropFloat( RECVINFO( m_flCoolDownTime) ),
    RecvPropInt( RECVINFO( m_iOriginalOwnerID ) ),

    RecvPropInt( RECVINFO( m_iDamageOffset ) ),
    RecvPropInt( RECVINFO( m_iDamageCapOffset ) ),
	RecvPropFloat( RECVINFO( m_flFireRateOffset ) ),
    RecvPropFloat( RECVINFO( m_flClickFireRateOffset ) ),
    RecvPropFloat( RECVINFO( m_flAccFireRateOffset ) ),

	RecvPropVector( RECVINFO( m_vMinSpreadOffset ) ),
    RecvPropVector( RECVINFO( m_vMaxSpreadOffset ) ),
    RecvPropFloat( RECVINFO( m_flJumpPenaltyOffset ) ),
    RecvPropFloat( RECVINFO( m_flAimBonusOffset ) ),

    RecvPropInt( RECVINFO( m_iMaxClip1Offset ) ),
    RecvPropInt( RECVINFO( m_iAccShotsOffset ) ),

    RecvPropInt( RECVINFO( m_iZoomOffsetOffset ) ),
    
    RecvPropFloat( RECVINFO( m_flBlastRadiusOffset ) ),
	RecvPropFloat( RECVINFO( m_flBlastLifetimeOffset ) ),

    RecvPropFloat( RECVINFO( m_flRangeOffset ) ),
    RecvPropFloat( RECVINFO( m_flPushForceMultOffset ) ),
    RecvPropFloat( RECVINFO( m_flPenetrationOffset ) ),

    RecvPropInt( RECVINFO( m_iWeaponSoundPitchOffset ) ),
    RecvPropFloat( RECVINFO( m_flWeaponSoundVolumeOffset ) ),

	RecvPropInt( RECVINFO( m_iWeaponSwitchWeightOffset ) ),

    RecvPropString( RECVINFO( m_sPrintNameCustom ) ),

	RecvPropBool( RECVINFO(m_bEnableGlow) ),
	RecvPropInt( RECVINFO(m_GlowColor), 0, RecvProxy_IntToColor32 ),
	RecvPropFloat( RECVINFO(m_GlowDist) ),
	RecvPropFloat( RECVINFO(m_flShootTime) ),
#else
	SendPropBool( SENDINFO( m_bSilenced ) ),
	SendPropFloat( SENDINFO( m_flAccuracyPenalty ) ),
	SendPropFloat( SENDINFO( m_flCoolDownTime) ),
    SendPropInt( SENDINFO( m_iOriginalOwnerID ) ),

    SendPropInt( SENDINFO( m_iDamageOffset ) ),
    SendPropInt( SENDINFO( m_iDamageCapOffset ) ),
	SendPropFloat( SENDINFO( m_flFireRateOffset ) ),
    SendPropFloat( SENDINFO( m_flClickFireRateOffset ) ),
    SendPropFloat( SENDINFO( m_flAccFireRateOffset ) ),

    SendPropInt( SENDINFO( m_iMaxClip1Offset ) ),
	SendPropInt( SENDINFO( m_iAccShotsOffset ) ),

    SendPropInt( SENDINFO( m_iZoomOffsetOffset ) ),

	SendPropVector( SENDINFO( m_vMinSpreadOffset ) ),
    SendPropVector( SENDINFO( m_vMaxSpreadOffset ) ),
	SendPropFloat( SENDINFO( m_flJumpPenaltyOffset ) ),
	SendPropFloat( SENDINFO( m_flAimBonusOffset ) ),

	SendPropFloat( SENDINFO( m_flBlastRadiusOffset ) ),
	SendPropFloat( SENDINFO( m_flBlastLifetimeOffset ) ),
	SendPropFloat( SENDINFO( m_flRangeOffset ) ),
    SendPropFloat( SENDINFO( m_flPushForceMultOffset ) ),
    SendPropFloat( SENDINFO( m_flPenetrationOffset ) ),

    SendPropInt( SENDINFO( m_iWeaponSoundPitchOffset ) ),
    SendPropFloat( SENDINFO( m_flWeaponSoundVolumeOffset ) ),

	SendPropInt(SENDINFO( m_iWeaponSwitchWeightOffset )),

    SendPropString( SENDINFO( m_sPrintNameCustom ) ),

	SendPropBool( SENDINFO(m_bEnableGlow) ),
	SendPropInt( SENDINFO(m_GlowColor), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropFloat( SENDINFO(m_GlowDist) ),
	SendPropFloat( SENDINFO(m_flShootTime) ),
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )

#include "globalstate.h"
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CGEWeapon )

	DEFINE_FIELD( m_bSilenced,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLowered,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flRaiseTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flHolsterTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAccuracyPenalty,	FIELD_TIME ),
	DEFINE_FIELD( m_flCoolDownTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flShootTime,		FIELD_TIME ),

    DEFINE_FIELD( m_iDamageOffset,	    FIELD_INTEGER ),
    DEFINE_FIELD( m_iDamageCapOffset,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flFireRateOffset,	FIELD_FLOAT ),
    DEFINE_FIELD( m_flClickFireRateOffset,	FIELD_FLOAT ),
    DEFINE_FIELD( m_flAccFireRateOffset,	FIELD_FLOAT ),

    DEFINE_FIELD( m_iMaxClip1Offset,	FIELD_INTEGER ),
    DEFINE_FIELD( m_iAccShotsOffset,	FIELD_INTEGER ),

    DEFINE_FIELD( m_iZoomOffsetOffset,	FIELD_INTEGER ),

	DEFINE_FIELD( m_vMinSpreadOffset,		FIELD_VECTOR ),
    DEFINE_FIELD( m_vMaxSpreadOffset,		FIELD_VECTOR ),
    DEFINE_FIELD( m_flJumpPenaltyOffset,	FIELD_FLOAT ),
    DEFINE_FIELD( m_flAimBonusOffset,	FIELD_FLOAT ),

    DEFINE_FIELD( m_flBlastRadiusOffset,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flBlastLifetimeOffset, FIELD_FLOAT ),

    DEFINE_FIELD( m_flRangeOffset,	FIELD_FLOAT ),
    DEFINE_FIELD( m_flPenetrationOffset,	FIELD_FLOAT ),

    DEFINE_FIELD( m_iWeaponSoundPitchOffset,	FIELD_INTEGER ),
    DEFINE_FIELD( m_flWeaponSoundVolumeOffset,	FIELD_FLOAT ),

	DEFINE_FIELD( m_iWeaponSwitchWeightOffset, FIELD_INTEGER ),

	DEFINE_THINKFUNC( OnReloadOffscreen ),
END_DATADESC()
#endif

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGEWeapon )
	DEFINE_PRED_FIELD( m_bSilenced, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCoolDownTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flShootTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flShootTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

// ----------------------------------------------------------------------------- //
// CWeaponCSBase implementation. 
// ----------------------------------------------------------------------------- //
CGEWeapon::CGEWeapon()
{
	m_flHolsterTime = 0.0f;
	m_bSilenced = false;
	m_bIsAlwaysSilent = false;
	m_flAccuracyPenalty = m_flCoolDownTime = 0.0f;
	// Our weapons are water proof by default, because why not.
	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;
	m_bEnableGlow = false;

	m_flSmokeRate = m_flLastShotTime = 0;
	m_iShotsFired = 0;

	m_flShootTime = 0;

    m_iOriginalOwnerID = -1;

    m_iDamageOffset = 0;
    m_iDamageCapOffset = 0;
    m_flFireRateOffset = 0.0f;
    m_flClickFireRateOffset = 0.0f;
    m_flAccFireRateOffset = 0.0f;

    m_iMaxClip1Offset = 0;
    m_iAccShotsOffset = 0;

    m_iZoomOffsetOffset = 0;

    m_vMinSpreadOffset = Vector(0, 0, 0);
    m_vMaxSpreadOffset = Vector(0, 0, 0);
    m_flJumpPenaltyOffset = 0.0f;
    m_flAimBonusOffset = 0.0f;

    m_flBlastRadiusOffset = 0.0f;
	m_flBlastLifetimeOffset = 0.0f;

    m_flRangeOffset = 0.0f;
    m_flPenetrationOffset = 0.0f;

    m_flPushForceMultOffset = 0.0f;

    m_iWeaponSoundPitchOffset = 0;
    m_flWeaponSoundVolumeOffset = 0.0f;

	m_iWeaponSwitchWeightOffset = 0;

    m_sPrintNameCustom.GetForModify()[0] = '\0';

#ifdef GAME_DLL
	m_flDeployTime = 0.0f;
	m_flPickupAllowedTime = 0.0f;
    m_iLimitEnforcementPriority = 0; // By default we have minimum priority.

	color32 col32 = { 255, 255, 255, 100 };
	m_GlowColor.Set( col32 );
	m_GlowDist.Set( 250.0f );

    m_vLastPhysPos = vec3_origin;
    m_vLastPhysAngle = vec3_angle;
    m_bDoPlayerclipCheck = false;
    m_iOriginSlot = -1; // Not from spawner by default.
#else
	m_flLastBobCalc = 0;
	m_flLastSpeedrat = 0;
	m_flLastFallrat = 0;
	m_bClientGlow = false;
	m_pEntGlowEffect = (CEntGlowEffect*)g_pScreenSpaceEffects->GetScreenSpaceEffect("ge_entglow");
#endif
}

#ifdef GAME_DLL

CGEWeapon::~CGEWeapon()
{
}

void CGEWeapon::Spawn()
{
	AddSpawnFlags(SF_NORESPAWN);
	// So NPC's will "see" us
	AddFlag( FL_OBJECT );

	BaseClass::Spawn();

	RegisterThinkContext("HalfReload");

	m_vOriginalSpawnOrigin = GetAbsOrigin();
	m_vOriginalSpawnAngles = GetAbsAngles();

    if (GetOwnerEntity() && (!Q_strcmp(GetOwnerEntity()->GetClassname(), "ge_weaponspawner") || !Q_strcmp(GetOwnerEntity()->GetClassname(), "ge_ammospawner")))
    {
        m_iOriginSlot = static_cast<CGESpawner*>(GetOwnerEntity())->GetSlot();
    }

	// Notify the token manager we are arriving
	if (GEMPRules()->GetTokenManager()->OnTokenSpawned( this )) // If we're actually allowed to spawn...
		GEEntityTracker()->AddItemToTracker( this, ET_START_ITEMSPAWNED, ET_LIST_WEAPON ); // Add us to the approperate entity tracker list
}

void CGEWeapon::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
	// Notify the token manager we are going away
	if ( GEMPRules() && GEMPRules()->GetTokenManager() )
		GEMPRules()->GetTokenManager()->OnTokenRemoved( this );
}

#endif //GAME_DLL

void CGEWeapon::Precache( void )
{
	BaseClass::Precache();

	// Best place to initialize this on client/server...
	m_flSmokeRate = GetClickFireRate() * GetMaxClip1() * 0.4f;

	PrecacheParticleSystem( "tracer_standard" );
	PrecacheParticleSystem( "muzzle_smoke" );

	// These are common enough to precache here.
	PrecacheScriptSound( "Weapon.Empty" );
	PrecacheScriptSound( "Weapon.Reload" );
}

void CGEWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
    CGEPlayer *pGEOwner = ToGEPlayer(pOwner);

    if ( pGEOwner && m_iOriginalOwnerID == -1 ) // We don't have an original owner yet.
        m_iOriginalOwnerID = pGEOwner->GetUserID();

#ifdef GAME_DLL
	GEMPRules()->GetTokenManager()->OnTokenPicked( this, ToGEPlayer( pOwner ) );

	// Disable Glowing (but preserve our set color)
	SetEnableGlow( false );

	// Don't track us while someone is carrying us around, since we're no longer a pickup.
	GEEntityTracker()->RemoveItemFromTracker( this, ET_END_ITEMPICKED, ET_LIST_WEAPON );
#endif

	// Fill this bad boy up with ammo if we have any for it to use!
	FinishReload();

	if ( GEMPRules()->InfAmmoEnabled() )
	{
		m_iClip1 = GetMaxClip1();
#ifdef GAME_DLL
		CGEMPPlayer *pGEPlayer = ToGEMPPlayer(pOwner);
		pGEPlayer->GiveAmmo(800, GetPrimaryAmmoType(), true);
#endif
	}
}

extern CBasePlayer* UTIL_PlayerByUserId(int userID);

#ifdef GAME_DLL
CGEPlayer *CGEWeapon::GetOriginalOwner()
{
    return ToGEPlayer(UTIL_PlayerByUserId(m_iOriginalOwnerID));
}
#endif

void CGEWeapon::Drop( const Vector &vecVelocity )
{
#ifdef GAME_DLL
	// Call this first so we can capture who owned the entity before the drop
	GEMPRules()->GetTokenManager()->OnTokenDropped( this, ToGEPlayer( GetOwner() ) );

	SetEnableGlow( m_bServerGlow );

	// We're a pickup again.
	GEEntityTracker()->AddItemToTracker( this, ET_START_ITEMDROPPED, ET_LIST_WEAPON );

    m_vLastPhysPos = vec3_origin; // Reset phys tracker state
#endif

	CGEMPPlayer *pGEMPPlayer = ToGEMPPlayer( GetOwner() );

	if (pGEMPPlayer && GetWeaponID() < WEAPON_RANDOM)
		m_nSkin = pGEMPPlayer->GetUsedWeaponSkin(GetWeaponID()); // Just in case the player never pulled it out.

	BaseClass::Drop( vecVelocity );

	// Dropped weapons don't collide with eachother
	SetCollisionGroup(COLLISION_GROUP_DROPPEDWEAPON);
}

void CGEWeapon::SetPickupTouch( void )
{
#ifndef CLIENT_DLL
	SetTouch(&CBaseCombatWeapon::DefaultTouch);

	if ( gpGlobals->maxClients > 1 )
	{
		if ( GetSpawnFlags() & SF_NORESPAWN )
		{
			SetThink( &CBaseEntity::SUB_Remove );
			float delay = 30.0f;
			GEMPRules()->GetTokenManager()->GetTokenRespawnDelay( GetClassname(), &delay );

			SetNextThink( gpGlobals->curtime + delay );
		}
	}
#endif
}

bool CGEWeapon::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );
	
	return 0;
}

void CGEWeapon::RecordShotFired(int count /*=1*/)
{
	m_iShotsFired += count;
	m_flLastShotTime = gpGlobals->curtime;
}

void CGEWeapon::PrimaryAttack(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if(!pPlayer)
		return;

	// Send the animation event to the client/server
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	float fireDelay = GetFireDelay();

	if (fireDelay == 0.0f)
	{
		// We have no fire delay so just shoot right now.
		FireWeapon();
		//Warning("Instant Fire!\n");
	}
	else
	{
		// We have a fire delay so get ready to shoot when it's time.
		m_flShootTime = gpGlobals->curtime + fireDelay;
		//Warning("Delayed Fire!\n");
	}
}

void CGEWeapon::FireWeapon()
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound( SINGLE );

	pPlayer->DoMuzzleFlash();

	Vector	vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	RecordShotFired();

	// Knock the player's view around
	AddViewKick();

	// Prepare to fire the bullets
	PrepareFireBullets( 1, pPlayer, vecSrc, vecAiming, true );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	if (!m_iClip1)
	{
		m_bFireOnEmpty = true;

		// Subtract out fire delay here so we keep the expected fire pace.
		m_flNextEmptySoundTime = gpGlobals->curtime + max(GetClickFireRate() - GetFireDelay(), 0.25);
	}
}

void CGEWeapon::OnReloadOffscreen(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return;

	if ( pOwner->GetActiveWeapon() == this && GetWeaponID() < WEAPON_RANDOM ) // Only switch skins if we still have the weapon equipped!
		SetSkin(ToGEMPPlayer(pOwner)->GetUsedWeaponSkin(GetWeaponID()));
}

bool CGEWeapon::Reload(void)
{
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return false;

	bool bRet = false;

	bRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);

	if ( bRet && GetOwner() && GetOwner()->IsPlayer() )
	{
		CGEPlayer *pPlayer = ToGEPlayer( GetOwner() );

	#ifdef CLIENT_DLL
		IGameEvent *event = gameeventmanager->CreateEvent( "weapon_event" );
		if ( event )
		{
			event->SetInt( "userid", ToBasePlayer(GetOwner())->GetUserID() );
			event->SetInt( "weaponid", GetWeaponID() );
			event->SetInt( "eventid", WEAPON_EVENT_RELOAD );
			gameeventmanager->FireEventClientSide( event );
		}
	#endif

		m_iShotsFired = 0;
		SetContextThink(&CGEWeapon::OnReloadOffscreen, gpGlobals->curtime + 0.5f, "HalfReload");
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
	}

	return bRet;
}

void CGEWeapon::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CGEWeapon::ToggleSilencer( bool doanim /* = true */ )
{
	if ( CanBeSilenced() )
	{
		if ( m_bSilenced && doanim )
		{
			SendWeaponAnim( ACT_VM_DETACH_SILENCER );
			WeaponSound(SPECIAL1);
		}
		else if ( doanim )
		{
			SendWeaponAnim( ACT_VM_ATTACH_SILENCER );
			WeaponSound(SPECIAL2);
		}

		m_bSilenced = !m_bSilenced;
	}
}

int CGEWeapon::GetTracerAttachment( void )
{
	return BaseClass::GetTracerAttachment();
}

#ifdef CLIENT_DLL

#define GE_BOB_W		0.65f
#define GE_BOB_H		0.3f
#define GE_BOB_TIME		1.25f
#define GE_FALL_H		0.5f
#define GE_FALL_S		500.0f //Speed the player needs to fall at to have max viewmodel offset

extern float	g_lateralBob;
extern float	g_verticalBob;

float CGEWeapon::CalcViewmodelBob( void )
{
	float	cycle;
	float	framelength = gpGlobals->curtime - m_flLastBobCalc;
	
	CGEMPPlayer *player = ToGEMPPlayer( GetOwner() );
	if ( !gpGlobals->frametime || player == NULL )
		return 0;

	// Find the speed ratio vs max speed
	float speedrat = player->GetLocalVelocity().Length2D() / (float)GE_NORM_SPEED;

	speedrat = clamp( speedrat, m_flLastSpeedrat - 6 * framelength, m_flLastSpeedrat + 6 * framelength ); // A little bit of interpolation to prevent flickering.
	speedrat = clamp( speedrat, 0, 1.5f );

	m_flLastSpeedrat = speedrat;

	// Calculate the width/height based on our speed ratio
	float w = GE_BOB_W * speedrat;
	float h = GE_BOB_H * speedrat;

	cycle = gpGlobals->curtime / GE_BOB_TIME;
	
	g_lateralBob = w * sin( M_TWOPI * cycle );
	g_verticalBob = h * sin ( 2.0f * M_TWOPI * cycle );

	// Now let's add an offset for how fast we're falling

	float fallspeed = clamp(player->GetLocalVelocity().z, -GE_FALL_S, GE_FALL_S);
	float fallrat = -(fallspeed / GE_FALL_S) - (clamp(player->GetJumpPenalty(), 0, 50) / 200);

	fallrat = clamp(fallrat, m_flLastFallrat - 10 * framelength, m_flLastFallrat + 10 * framelength);

	m_flLastFallrat = fallrat;

	g_verticalBob += RemapValClamped( fallrat, -1, 1, -GE_FALL_H, GE_FALL_H);

	m_flLastBobCalc = gpGlobals->curtime;

	return 0;
}

void CGEWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{

	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	CalcViewmodelBob();

	// Apply bob
	VectorMA( origin, g_verticalBob, up, origin );
	VectorMA( origin, g_lateralBob, right, origin );
	VectorMA( origin, g_lateralBob * 0.5f, forward, origin );
	
	// bob the angles
	angles[ ROLL ]	+= g_lateralBob;
	angles[ PITCH ]	-= g_verticalBob * 0.8f;
	angles[ YAW ]	-= g_lateralBob  * 0.6f;

}

void CGEWeapon::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	color32 col = m_GlowColor.Get();
	if ( m_bClientGlow != m_bEnableGlow )
	{
		if ( m_bEnableGlow )
		{
			m_pEntGlowEffect->RegisterEnt( this, Color(col.r, col.g, col.b, col.a), m_GlowDist.Get() );
		}
		else
		{
			m_pEntGlowEffect->DeregisterEnt( this );
		}

		m_bClientGlow = m_bEnableGlow;
	}
	else
	{
		m_pEntGlowEffect->SetEntColor( this, Color(col.r, col.g, col.b, col.a) );
		m_pEntGlowEffect->SetEntMaxGlowDist( this, m_GlowDist.Get() );
	}
}

void CGEWeapon::UpdateVisibility( void )
{
	// If we are glowing always make us visible so that we can draw the glow event
	if ( m_bClientGlow )
		AddToLeafSystem();
	else
		BaseClass::UpdateVisibility();
}

#endif

Activity CGEWeapon::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

Activity CGEWeapon::GetDrawActivity( void )
{
	return ACT_VM_DRAW;
}

void CGEWeapon::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}

#ifdef GAME_DLL

void CGEWeapon::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	if ( gpGlobals->curtime < m_flNextPrimaryAttack )
		DevWarning( 2, "NPC attempted fire too early (%0.2f) with %s!\n", m_flNextPrimaryAttack - gpGlobals->curtime, GetClassname() );

	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	WeaponSound( SINGLE_NPC );
	pOperator->DoMuzzleFlash();
	// Reduce our ammo count
	m_iClip1 = m_iClip1 - 1;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

		// NOTE: CBaseCombatCharacter::FindMissTarget( void ) is jacked up

		PrepareFireBullets(1, pOperator, vecShootOrigin, vecShootDir, false);
	}
}

void CGEWeapon::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_AR2:
			FireNPCPrimaryAttack( pOperator, false );
			break;
			
		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

void CGEWeapon::DefaultTouch( CBaseEntity *pOther )
{
	// Can't pick up if we're not allowed yet.
	if ( gpGlobals->curtime < m_flPickupAllowedTime )
		return;

	// Handle NPC's touching us
	if ( pOther->IsNPC() )
	{
		CNPC_GEBase *pNPC = (CNPC_GEBase*) pOther;

		if( HasSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP) )
			return;

		if ( pNPC->BumpWeapon(this) )
			OnPickedUp( pNPC );
	}
	else
	{
		BaseClass::DefaultTouch( pOther );
	}
}

void CGEWeapon::SetupGlow( bool state, Color glowColor /*=Color(255,255,255)*/, float glowDist /*=250.0f*/ )
{
	m_bServerGlow = state;
	color32 col32 = { glowColor.r(), glowColor.g(), glowColor.b(), glowColor.a() };
	m_GlowColor.Set( col32 );
	m_GlowDist.Set( glowDist );

	if ( !GetOwner() || (GetOwner() && !GetOwner()->IsPlayer()) )
		SetEnableGlow( state );

	UpdateTransmitState();
}

void CGEWeapon::SetEnableGlow( bool state )
{
	m_bEnableGlow = state;
}

int CGEWeapon::UpdateTransmitState()
{
	if (m_bServerGlow)
		return SetTransmitState(FL_EDICT_ALWAYS);
	else
		return BaseClass::UpdateTransmitState();
}

void CGEWeapon::SetRoundSecondsUntilPickup( float timeDelay )
{ 
	if (timeDelay - GEMPRules()->GetTimeSinceRoundStart() <= 0)
		m_flPickupAllowedTime = 0;
	else
		m_flPickupAllowedTime = gpGlobals->curtime + timeDelay - GEMPRules()->GetTimeSinceRoundStart();
}
#endif

void CGEWeapon::PreOwnerDeath()
{
	m_flAccuracyPenalty += GetAccShots(); //Max penalty for anything fired after death.
}

// Consolidate the fire preps into one function since all the custom primary attack functions have to do their own otherwise,
// And updating them all every time it needs to be changed kind of sucks.
void CGEWeapon::PrepareFireBullets(int number, CBaseCombatCharacter *pOperator, Vector vecShootOrigin, Vector vecShootDir, bool haveplayer)
{
	UpdateAccPenalty(); //Update the penalty before doing any of this nonsense, as it affects both gauss factor and spread cone.

	int tracerfreq = GetTracerFreq();
	FireBulletsInfo_t info(number, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
	info.m_iGaussFactor = GetGaussFactor(); //Calculates the gauss factor and adds it to the fire info.

	if (m_flAccuracyPenalty < 1.0 && tracerfreq > 0)
		tracerfreq = 1; //Always try to draw tracers on the first shot
	
	info.m_iTracerFreq = tracerfreq; //Otherwise use the weapon's tracerfreq

    // Fun little feature for custom gamemodes.  We have no blast radius by default so our offset is our radius.
    info.m_flBlastRadius = max(GetBlastRadiusOffset(), 0);
    info.m_flBlastRadiusDamage = max(GetRangeOffset(), 0); // Might as well recycle this too since hitscan weapons don't use it.  Yeah this feature is kind of tacked-on.

    info.m_flDamageForceScale = GetWeaponPushForceMult();

	if (haveplayer)
		info.m_pAttacker = pOperator;

	pOperator->FireBullets(info); //Actually fires the bullets this time.  For real!

	m_flAccuracyPenalty += number; //Adds an innaccuracy penalty equal to shots fired.  File settings determine how fast it decays and how much it affects.
	m_flCoolDownTime = gpGlobals->curtime; // Update our cool down time
}

const Vector& CGEWeapon::GetBulletSpread( void )
{
	static Vector cone;
	Vector minSpread, maxSpread;
	float aimbonus, jumppenalty;

    minSpread = GetMinSpreadVec();
    maxSpread = GetMaxSpreadVec();
    aimbonus = GetAimBonus();
    jumppenalty = GetJumpPenalty();

	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner && pOwner->IsPlayer() )
	{
		CGEPlayer *pPlayer = ToGEPlayer( pOwner );

		// Give penalty for being in the air (jumping/falling)
		// (ladder check: pPlayer->GetMoveType() == MOVETYPE_LADDER)

		if (pPlayer->GetGroundEntity() == NULL)
		{
			minSpread.x += jumppenalty;
			minSpread.y += jumppenalty;
			maxSpread.x += jumppenalty * 2;
			maxSpread.y += jumppenalty * 2;
		}
		else if (pPlayer->IsInAimMode()) //Aim bonus only affects min spread, meaning full auto gets no benefits.
		{
			minSpread.x -= min(aimbonus, minSpread.x);
			minSpread.y -= min(aimbonus, minSpread.y);
		}
	}

	// Map our penalty time from 0-MaxPenalty  to  0-1
	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, GetAccShots(), 0.0f, 1.0f);

	// We lerp from very accurate to inaccurate over time
	VectorLerp(minSpread, maxSpread, ramp, cone);

	// Valve says spread vector input on firebullets assumes sin(degrees/2), adjust to that.
	// Except that's just what valve says, after some digging and testing it's actually tan(degrees/2)

	cone.x = tanf(DEG2RAD(cone.x));
	cone.y = tanf(DEG2RAD(cone.y));

	return cone;
}

void CGEWeapon::UpdateAccPenalty()
{
	// Reduce accuracy penalty based on time of last attack and minimum attack delay
	float timeratio = (gpGlobals->curtime - m_flCoolDownTime - GetClickFireRate()) / (GetAccFireRate() - GetClickFireRate());
	
	m_flAccuracyPenalty -= timeratio*timeratio; //Time squared so waiting longer yeilds much greater benefits.
	m_flAccuracyPenalty = max(m_flAccuracyPenalty, 0.0f);
	m_flAccuracyPenalty = min(m_flAccuracyPenalty, (float)GetAccShots());
}

void CGEWeapon::AddAccPenalty(float numshots)
{
	UpdateAccPenalty(); //Because we're resetting the cooldown time we have to take off what should already be gone.

	m_flAccuracyPenalty += numshots;
	m_flCoolDownTime = gpGlobals->curtime;
}

int CGEWeapon::GetGaussFactor()
{
	int gaussfactor = GetGEWpnData().m_flGaussFactor;
	int gausspenalty = GetGEWpnData().m_flGaussPenalty;

	// Jumping lowers gauss to 1, or keeps it at 0.
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner && pOwner->IsPlayer())
	{
		CGEPlayer *pPlayer = ToGEPlayer(pOwner);

		if (pPlayer->GetGroundEntity() == NULL)
		{
			gaussfactor = min(gaussfactor, 1);
			gausspenalty = 0;
		}
	}

	// Calculate ramp as it plays into gauss penalty.
	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, GetAccShots(), 0.0f, 1.0f);

	return (int)(gaussfactor - ramp * gausspenalty);
}

#ifdef CLIENT_DLL
extern ConVar cl_ge_weapon_switchempty;
#endif

bool CGEWeapon::HasAmmo( void )
{
	// Weapons with no ammo types can always be selected
	if ( m_iPrimaryAmmoType == -1 && m_iSecondaryAmmoType == -1  )
		return true;
	if ( GetWeaponFlags() & ITEM_FLAG_SELECTONEMPTY )
	{
		int clAutoswitch;

#ifdef GAME_DLL
		clAutoswitch = atoi( engine->GetClientConVarValue( GetOwner()->entindex(), "cl_ge_weapon_switchempty" ) );
#else
		clAutoswitch = cl_ge_weapon_switchempty.GetInt();
#endif
		if ( clAutoswitch < 1 || GetWeaponID() == WEAPON_REMOTEMINE) // Remote mines are the only weapon that needs this but it should still be a flag.
			return true;
	}

	CBaseCombatCharacter *owner = GetOwner();
	if ( !owner )
		return false;

	return ( m_iClip1 > 0 || owner->GetAmmoCount( m_iPrimaryAmmoType ) || m_iClip2 > 0 || owner->GetAmmoCount( m_iSecondaryAmmoType ) );
}

bool CGEWeapon::HasShotsLeft( void )
{
	if ( m_iPrimaryAmmoType == -1 && m_iSecondaryAmmoType == -1  )
		return true;

	CBaseCombatCharacter *owner = GetOwner();
	if ( !owner )
		return false;

	return (UsesClipsForAmmo1() && m_iClip1 > 0) || owner->GetAmmoCount( m_iPrimaryAmmoType );
}

bool CGEWeapon::IsWeaponVisible( void )
{
	if ( GetOwner() && GetOwner()->IsNPC() )
		return !IsEffectActive( EF_NODRAW );
	else
		return BaseClass::IsWeaponVisible();
}

void CGEWeapon::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
#ifdef GAME_DLL
    trace_t	playerClipTrace;

    Vector physPos;
    QAngle physAngles;
    pPhysics->GetPosition(&physPos, &physAngles);

    // Only do this if we have a previous physics frame.
    if (m_bDoPlayerclipCheck && m_vLastPhysPos != vec3_origin)
    {
	    UTIL_TraceLine( m_vLastPhysPos, physPos, (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP), this, COLLISION_GROUP_NONE, &playerClipTrace );

        if (!playerClipTrace.startsolid && playerClipTrace.fraction < 1.0f)
        {
            if (playerClipTrace.plane.normal.z == 0)
            {
                pPhysics->SetPosition(m_vLastPhysPos, m_vLastPhysAngle, true);

                Vector physVelocity;
                AngularImpulse physAngVelocity;
                pPhysics->GetVelocity(&physVelocity, &physAngVelocity);
                pPhysics->SetVelocityInstantaneous(&vec3_origin, &vec3_origin);
            }
        }
    }

    m_vLastPhysPos = physPos;
    m_vLastPhysAngle = physAngles;
#endif

    BaseClass::VPhysicsUpdate(pPhysics);
}

void CGEWeapon::ItemPreFrame( void )
{
	// If we are waiting for our fire delay to expire, and it has, fire the weapon!
	if ( m_flShootTime && gpGlobals->curtime >= m_flShootTime) 
	{
		FireWeapon();
		m_flShootTime = 0;
	}

	BaseClass::ItemPreFrame();
}

void CGEWeapon::ItemPostFrame( void )
{
	// Reset shots fired if we go past our reset time (do this before any possible call to Primary Attack)
	if ( m_iShotsFired && (gpGlobals->curtime - m_flSmokeRate) > m_flLastShotTime )
		m_iShotsFired = 0;

	BaseClass::ItemPostFrame();

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	if ( pOwner->m_afButtonPressed & IN_ATTACK )
	{
	#ifdef CLIENT_DLL
		if ( pOwner->IsLocalPlayer() && pOwner->GetViewModel() )
			pOwner->GetViewModel()->ParticleProp()->StopParticlesNamed("muzzle_smoke");
	#else
		StopParticleEffects( this );
	#endif
	}
	else if ( (pOwner->m_nButtons & IN_ATTACK) == false && m_iShotsFired > 0 )
	{
		float shotlimit = 0.4f*GetMaxClip1();
	#ifdef CLIENT_DLL
		bool doSmoke = !cl_ge_nomuzzlesmoke.GetBool() && m_iShotsFired > shotlimit;
	#else
		bool doSmoke = m_iShotsFired > shotlimit;
	#endif

		if ( doSmoke && !(IsSilenced() || IsAlwaysSilenced()) )
		{
			float prob = RemapValClamped( m_iShotsFired, shotlimit, GetMaxClip1(), 0.4f, 0.05f );
			if ( GERandom<float>(1.0f) > prob )
			{
			#ifdef CLIENT_DLL
				if ( pOwner->IsLocalPlayer() && pOwner->GetViewModel() )
					pOwner->GetViewModel()->ParticleProp()->Create( "muzzle_smoke", PATTACH_POINT_FOLLOW, "muzzle" );
			#else
					DispatchParticleEffect( "muzzle_smoke", PATTACH_POINT_FOLLOW, this, "muzzle" );
			#endif
			}

			m_iShotsFired = 0;
		}
	}
}

bool CGEWeapon::Deploy( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return false;

	if ( BaseClass::Deploy() )
	{
		m_flShootTime = 0;

#ifdef GAME_DLL
		m_flDeployTime = gpGlobals->curtime;
#else
		if ( pOwner && pOwner->IsPlayer() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "weapon_event" );
			if ( event )
			{
				event->SetInt( "userid", ToBasePlayer(pOwner)->GetUserID() );
				event->SetInt( "weaponid", GetWeaponID() );
				event->SetInt( "eventid", WEAPON_EVENT_DRAW );
				gameeventmanager->FireEventClientSide( event );
			}
		}
#endif
		if ( pOwner && pOwner->IsPlayer() )
		{
			if (GetWeaponID() < WEAPON_RANDOM)
				SetSkin(ToGEMPPlayer(pOwner)->GetUsedWeaponSkin(GetWeaponID()));
			ToGEPlayer(pOwner)->DoAnimationEvent(PLAYERANIMEVENT_GES_DRAW);
			ToGEPlayer(pOwner)->ResetAimMode();
		}

		return true;
	}

	return false;
}

bool CGEWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return false;

	if ( BaseClass::Holster( pSwitchingTo ) )
	{
	#ifdef CLIENT_DLL
		if ( pOwner->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pOwner );

			// Stop emitting all particles!
			CBaseViewModel *pVM = pPlayer->GetViewModel();
			if ( pVM && pVM->ParticleProp() )
				pVM->ParticleProp()->StopEmission();

			IGameEvent *event = gameeventmanager->CreateEvent( "weapon_event" );
			if ( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );
				event->SetInt( "weaponid", GetWeaponID() );
				event->SetInt( "eventid", WEAPON_EVENT_HOLSTER );
				gameeventmanager->FireEventClientSide( event );
			}
		}
	#else
		StopParticleEffects( this );
	#endif
		
		if ( pOwner->IsPlayer() )
		{
			CGEPlayer *pPlayer = ToGEPlayer( pOwner );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_GES_HOLSTER );
		}

		return true;
	}

	return false;
}

void CGEWeapon::SetViewModel()
{
	BaseClass::SetViewModel();

	CGEPlayer *pGEPlayer = ToGEPlayer(GetOwner());
	if (!pGEPlayer)
		return;
	CBaseViewModel *vm = pGEPlayer->GetViewModel(m_nViewModelIndex);
	if (vm == NULL)
		return;

	vm->m_nSkin = m_nSkin;
}

void CGEWeapon::SetSkin( int skin )
{
	m_nSkin = skin;

	CGEPlayer *pGEPlayer = ToGEPlayer( GetOwner() );
	if ( !pGEPlayer )
		return;

	CBaseViewModel *vm = pGEPlayer->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
		return;

	vm->m_nSkin = m_nSkin;
}

int CGEWeapon::GetBodygroupFromName(const char* name)
{
	CGEPlayer *pGEPlayer = ToGEPlayer(GetOwner());
	if (!pGEPlayer)
		return -1;

	CBaseViewModel *vm = pGEPlayer->GetViewModel(m_nViewModelIndex);
	if (vm == NULL)
		return -1;

	return vm->FindBodygroupByName(name);
}

void CGEWeapon::SwitchBodygroup(int bodygroup, int value)
{
	CGEPlayer *pGEPlayer = ToGEPlayer(GetOwner());
	if (!pGEPlayer)
		return;

	CBaseViewModel *vm = pGEPlayer->GetViewModel(m_nViewModelIndex);
	if (vm == NULL)
		return;

	vm->SetBodygroup(bodygroup, value); // viewmodel
	SetBodygroup(bodygroup, value); // worldmodel
}

void CGEWeapon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	CBaseEntity *pOwner = GetOwner();

	if ( pOwner == NULL )
	{
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		return;
	}

	Vector vNewSrc = vecTracerSrc;
	int iEntIndex = pOwner->entindex();

	if ( g_pGameRules->IsMultiplayer() )
	{
		iEntIndex = entindex();
	}

	int iAttachment = GetTracerAttachment();

	switch ( iTracerType )
	{
	case TRACER_LINE:
		UTIL_ParticleTracer( "tracer_standard", vNewSrc, tr.endpos, iEntIndex, iAttachment, false );
		break;

	case TRACER_LINE_AND_WHIZ:
		UTIL_ParticleTracer( "tracer_standard", vNewSrc, tr.endpos, iEntIndex, iAttachment, true );
		break;
	}
}

bool CGEWeapon::CanUseWeaponMods()
{
    return GEMPRules() && GEMPRules()->WeaponModsEnabled();
}

const char *CGEWeapon::GetCustomPrintName()
{
    if ( m_sPrintNameCustom.Get()[0] == '\0' ) // No custom name, just use the normal one.
        return GetPrintName();
    else
        return m_sPrintNameCustom.Get();
}

float CGEWeapon::GetZoomOffset( bool modded /*== true*/ )
{
	return GetGEWpnData().m_iZoomOffset + (modded ? m_iZoomOffsetOffset : 0.0f);
}

// GE Weapon Data accessor functions
const CGEWeaponInfo &CGEWeapon::GetGEWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CGEWeaponInfo *pGEInfo;

	#ifdef _DEBUG
		pGEInfo = dynamic_cast< const CGEWeaponInfo* >( pWeaponInfo );
		Assert( pGEInfo );
	#else
		pGEInfo = static_cast< const CGEWeaponInfo* >( pWeaponInfo );
	#endif

	return *pGEInfo;
}

float CGEWeapon::GetFireRate( bool modded /*== true*/ )
{
	return GetGEWpnData().m_flRateOfFire + (modded ? m_flFireRateOffset : 0.0f);
}
float CGEWeapon::GetClickFireRate( bool modded /*== true*/ )
{
	return GetGEWpnData().m_flClickRateOfFire + (modded ? m_flClickFireRateOffset : 0.0f);
}
float CGEWeapon::GetFireDelay( void )
{
	return GetGEWpnData().m_flFireDelay;
}

int CGEWeapon::GetMaxClip1( bool modded /*== true*/ )
{
	return GetWpnData().iMaxClip1 + (modded ? m_iMaxClip1Offset : 0);
}

int CGEWeapon::GetMaxClip2( bool modded /*== true*/ )
{
	return GetWpnData().iMaxClip2;
}

// END Accessor functions
