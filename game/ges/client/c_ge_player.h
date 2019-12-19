///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_player.h
// Description:
//      see c_ge_player.cpp
//
// Created On: 23 Feb 08, 17:06
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_PLAYER_H
#define GE_PLAYER_H
#pragma once

class CGEWeapon;

#include "c_basehlplayer.h"
#include "ge_player_shared.h"
#include "beamdraw.h"
#include "c_hl2mp_player.h"
#include "ge_shareddefs.h"
#include "GameEventListener.h"
#include "ge_weapon.h"

class C_GEPlayer : public C_HL2MP_Player, public CGameEventListener
{
public:
	DECLARE_CLASS( C_GEPlayer, C_HL2MP_Player );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_GEPlayer();
	~C_GEPlayer();

	virtual bool IsBotPlayer()	{ return false; }
	virtual bool IsMPPlayer()	{ return false; }
	virtual bool IsSPPlayer()	{ return false; }

	IMaterial *GetHeadLabelMaterial();
	virtual float GetHeadOffset();

	virtual void CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	virtual bool ShouldDraw();

	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );

	Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	// Used for soundmixer settings
	virtual bool CanSetSoundMixer();
	virtual void SetupSpecialMusic( float duration );
	virtual void FireGameEvent( IGameEvent *event );

	virtual void ItemPreFrame();
	virtual void ClientThink();
	virtual void PreThink();
	virtual void PostDataUpdate( DataUpdateType_t updateType );

	void ResetAimMode( bool forced=false );
	bool IsInAimMode();
	bool StartedAimMode()					{ return (m_flFullZoomTime > 0); }
	float GetFullyZoomedTime()				{ return m_flFullZoomTime; }
	void SetDesiredZoomOffset(float newoffset)				{ m_iNewZoomOffset = newoffset; }
	void CheckAimMode();
	int GetAimModeState();

	virtual void	RemoveAmmo(int iCount, int iAmmoIndex);

	void SetZoom( int zoom, bool forced=false ); 
	int GetZoomEnd();

	// Move the viewmodel closer to us if we are crouched
	virtual void  CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles);
	virtual void  CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual float GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );

	// Adjust collision bounds mid-frame for hit detection
	virtual void  AdjustCollisionBounds( bool inflate );

	virtual int GetArmor()		{ return m_ArmorValue; };

	// Get the maximum amount of armor that the player can currently pick up.  Might be lower than the value of iMaxArmor.
	virtual int GetMaxArmor();
	virtual int GetMaxHealth()	{ return m_iMaxHealth; };

    float GetJumpVelocityMult()	{ return m_flJumpHeightMult; }
    float GetStrafeRunMult()	{ return m_flStrafeRunMult; }

    void  SetMaxMidairJumps(int count)      { m_iMaxMidairJumps = count; }
    int   GetMaxMidairJumps()               { return m_iMaxMidairJumps; }

    // Resets remaining midair jumps to the max.
    void  RestoreMidairJumps()              { m_iRemainingMidairJumps = m_iMaxMidairJumps; }
    // Checks to see if we can perform a midair jump.  If so, removes one midair jump from our counter and returns true.  Returns false otherwise.
    int   TestMidairJump()                  { if (m_iRemainingMidairJumps > 0) { m_iRemainingMidairJumps--; return true; } else { return false; }; }

	C_BaseEntity *GetHat()		{ return m_hHat.Get(); };

	virtual float GetSweepTime()	{ return m_flSweepTime; }

	virtual void		FireBullets( const FireBulletsInfo_t &info );
	virtual bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = GE_RIGHT_HAND );

protected:
	IMaterial *m_pDM_VoiceHeadMaterial;
	IMaterial *m_pMI6_VoiceHeadMaterial;
	IMaterial *m_pJanus_VoiceHeadMaterial;

private:

	// This is to define the dualed weapon
	CHandle< C_BaseCombatWeapon > m_hActiveLeftWeapon;

	EHANDLE m_hHat;
    EHANDLE m_hHead;

	EHANDLE m_hActiveWeaponCache;

	// Local state tracking
	float m_flFullZoomTime;

	// Used to tell clientthink what to set the zoom to.
	int m_iNewZoomOffset;

	float m_flSweepTime;

	bool m_bInSpecialMusic;
	float m_flEndSpecialMusic;
	int m_ArmorValue;
	int m_iMaxArmor;
	int m_iMaxHealth;

	int m_iTotalMaxArmor;
	int m_iTotalArmorPickup;

    float m_flJumpHeightMult;
    float m_flStrafeRunMult;

    int m_iMaxMidairJumps;
    int m_iRemainingMidairJumps;

	bool m_bSentUnlockCode;
};

C_GEPlayer *ToGEPlayer( CBaseEntity *pEntity );

#endif //GE_PLAYER_H