///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_mine.h
// Description:
//      This is what makes the mines go BOOM
//
// Created On: 8/2/2008
// Created By: Jonathan White <killermonkey01@gmail.com> (Some code taken from Scott Lloyd)
/////////////////////////////////////////////////////////////////////////////

#ifndef	GEMINE_H
#define	GEMINE_H

#ifdef _WIN32
#pragma once
#endif

#include "grenade_gebase.h"
#include "weapon_mines.h"

#define ATTACH_SOUND_REMOTE		"weapon_remotemine.Attach"
#define ATTACH_SOUND_TIMED		"weapon_timedmine.Attach"
#define ATTACH_SOUND_PROXIMITY	"weapon_proximitymine.Attach"

class CSoundPatch;
class CSprite;

class CGEMine : public CGEBaseGrenade
{
public:
	DECLARE_CLASS( CGEMine, CGEBaseGrenade );

	void	Spawn( void );
	void	Precache( void );
	void	MineTouch( CBaseEntity *pOther );
	void	MineAttach( CBaseEntity *pEntity );
	void	MineThink( void );

	bool	AlignToSurf( CBaseEntity *pSurface );
	
	// Mine Settings
	void SetMineType( GEWeaponID type );

	virtual GEWeaponID GetMineType( void ) { return m_iWeaponID; }
	virtual GEWeaponID GetWeaponID( void ) { return m_iWeaponID; }

	virtual bool		IsInAir( void ) { return m_bInAir; };
	virtual const char* GetPrintName( void );
	virtual const char* GetAttachSound( void );
	virtual int			GetCustomData( void ) { return m_bInAir ? 1 : 0; };
	
	// Only avoid timed mines, as the other two could otherwise be used to block spawns indefinitely.
	// Proxy mines don't come up in LTK/Arsenal and don't explode on spawn invulnerable players in the other modes.
	virtual bool	CanBlockPlayerSpawn() { return m_iWeaponID == WEAPON_TIMEDMINE; };

	// Input handlers
	void	InputExplode( inputdata_t &inputdata );

	bool	m_bInAir;
	bool	m_bPreExplode;
	Vector	m_vLastPosition;

	virtual int	OnTakeDamage( const CTakeDamageInfo &inputInfo );

	CGEMine();
	~CGEMine();

	DECLARE_DATADESC();

private:
	GEWeaponID		m_iWeaponID;
	float			m_flAttachTime;
	float			m_flSpawnTime;
	bool			m_bExploded;
};

#endif	//SATCHEL_H
