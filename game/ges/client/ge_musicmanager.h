///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_musicmanager.h
// Description:
//      Loads music from "scripts/level_music_LEVELNAME.txt" in KeyValue form
//		and controls the playback and looping throughout the GE:S experience
//
// Created On: 18 Sep 11
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_MUSICMANAGER_H
#define GE_MUSICMANAGER_H
#pragma once

#include "GameEventListener.h"
#include "tier0/threadtools.h"
#include "igamesystem.h"
#include "fmod.hpp"

#define GE_MUSIC_MENU		"_menu"
#define GE_MUSIC_DEFAULT	"_default"

class CGEMusicManager : public CThread, public CAutoGameSystem
{
public:
	CGEMusicManager();
	~CGEMusicManager();

	// Generic controls to the music player
	void SetVolume( float vol );
	void PauseMusic();
	void ResumeMusic();

	void SilenceMusic();
	void UnsilenceMusic();

	const char *CurrentPlaylist() { return m_szPlayList; }
	void LoadPlayList( const char *levelname );

	// Soundscape transition
	void SetSoundscape( const char *soundscape );

	// Game Event Functions (these drive the thread actions)
	void PostInit();
	void LevelInitPreEntity();

	// Debug functions
	void DEBUG_NextSong();

protected:
	// Thread controls
	int Run();
	void StopThread() { m_bRun = false; }
	void ClearPlayList();

	void StartFade( int type, float duration = 1.5 );
	void FadeThink();
	void PauseThink();
	void NextSong( float fadeDuration = 1.5 );
	void EndSong( FMOD::Channel *pChannel );
	void CheckWindowFocus();

	// Playlist functions
	void InternalLoadPlaylist();
	void EnforcePlaylist( bool force = false );
	void AddSong( const char* relative, const char* soundscape );

	// Soundscape functions
	void InternalLoadSoundscape();

	void InternalPlayXMusic();

	enum {
		STATE_STOPPED = 0,
		STATE_PAUSED,
		STATE_UNPAUSE,
		STATE_PLAYING,
	};

private:
	// Status flags (thread transient)
	bool	m_bRun;
	bool	m_bFMODRunning;
	bool	m_bLoadPlaylist;
	bool	m_bVolumeDirty;
	int		m_iState;
	bool	m_bSilenced;
	// Soundscape transitions
	char	m_szSoundscape[64];
	bool	m_bLoadSoundscape;

	bool	m_bPlayingXMusic;
	float	m_flNextXMusicCheck;

	// Music playlist controls
	char	m_szPlayList[64];
	int		m_nCurrSongIdx;
	bool	m_bNewPlaylist;

	// Playlist seperated by soundscape name
	CUtlDict<CUtlVector<char*>*, int>	m_Playlists;
	CUtlVector<char*>* m_CurrPlaylist;

	// Global sound controls
	float	m_fVolume;
	float	m_fNextSongTime;
	float	m_fFadeStartTime;
	float	m_fPauseTime;

	// Fade Status
	enum {
		FADE_NONE,
		FADE_IN,	// m_pCurrSong IN, m_pLastSong OUT
		FADE_OUT,	// m_pCurrSong and m_pLastSong OUT
	};

	// Direction we're fading in.  FADE_IN = increase volume, FADE_OUT = decrease volume
	int		m_iFadeMode;
	// Amount that m_fFadeMultiplier changes by every second.
	float	m_flFadeSpeed;
	// Track volume as controlled by fade status.  1.0 = no effect from fade and 0.0 = silent.
	float	m_flFadeMultiplier;
	// Last time we calculated and applied fade values.
	float	m_flLastFadeThink;

	// Track volume as controlled by fade status, for previous song.  1.0 = no effect from fade and 0.0 = silent.
	float	m_flLastSongFadeMultiplier;

	// Current song length in milliseconds
	unsigned int m_iCurrSongLength;

	// FMOD Variables
	FMOD::System		*m_pSystem;
	FMOD::Channel		*m_pLastSong;
	FMOD::Channel		*m_pCurrSong;
	FMOD::ChannelGroup	*m_pMasterChannel;
};

extern void StartGEMusicManager();
extern void StopGEMusicManager();
extern CGEMusicManager *GEMusicManager();

#endif