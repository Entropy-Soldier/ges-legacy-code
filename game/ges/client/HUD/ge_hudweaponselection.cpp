///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudweaponselection.cpp
// Description:
//     Goldeneye's weapon selection HUD element
//
// Created On: 2/25/2008 1800
// Created By: Lodle and Others, Modified by Killer Monkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "weapon_selection.h"
#include "iclientmode.h"
#include "history_resource.h"
#include "input.h"
#include "weapon_parse.h"
#include "c_ge_player.h"
#include "in_buttons.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>

#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SELECTION_TIMEOUT_THRESHOLD  3.5f // Seconds
#define SELECTION_FADEOUT_TIME   2.0f
#define MAX_DISPLAYED_WEAPONS 30	// Max number of weapons that can be displayed at once.  Should always be less than MAX_WEAPONS.

using namespace vgui;

ConVar cl_ge_hud_fastswitchlist("cl_ge_hud_fastswitchlist", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Show all held weapons on fast switch.");
ConVar cl_ge_hud_noswitchlist("cl_ge_hud_noswitchlist", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Hide weaponlist entirely on switch.");
ConVar cl_ge_hud_uselastbucketitem("cl_ge_hud_uselastbucketitem", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "When first switching buckets with the number keys, select the last used weapon in that bucket.");

//-----------------------------------------------------------------------------
// Purpose: Selection of weapons for GE:Source
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, Panel );

public:
	CHudWeaponSelection(const char *pElementName );

	virtual void VidInit();
	virtual bool ShouldDraw();

	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );

	virtual C_BaseCombatWeapon *GetPrevActivePos( int iSlot, int iSlotPos, int iSubType );

	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot, int iSlotPos, int iSubType = 0 );
	virtual void SelectWeaponSlot( int iSlot );

	virtual C_BaseCombatWeapon *GetSelectedWeapon( void ) { return m_hSelectedWeapon; };

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void ProcessInput();

protected:
	virtual void OnThink();
	virtual void Paint();
	virtual void PaintFastWeaponSwitch();
	virtual void ApplySchemeSettings( IScheme *pScheme );

	// added function: find the number that will show up in selection
	int GetSelectionNumber(int iSlot,int iSlotPos);
	bool FirstOfBucket(int iSlot, int iSlotPos, int iSubType);

private:
	void FastWeaponSwitch( int iWeaponSlot );

	virtual void SetSelectedWeapon( C_BaseCombatWeapon *pWeapon, bool recordSelection = true );

	void DrawBox(int x, int y, int wide, int tall, Color color, int number = -1 );

    virtual int GetLastUsedSlotForBucket(int bucket)	{ return (bucket >= 0 && bucket < 10) ? m_iLastUsedSlotInBucket[bucket] : -1; }
    virtual int GetLastUsedSubtypeForBucket(int bucket) { return (bucket >= 0 && bucket < 10) ? m_iLastUsedSubTypeInBucket[bucket] : -1; }
    virtual void SetLastUsedWeaponForBucket(int bucket, int slot, int subtype)	{ if (bucket >= 0 && bucket < 10) { m_iLastUsedSlotInBucket[bucket] = slot; m_iLastUsedSubTypeInBucket[bucket] = subtype; } }

	int	m_iBoxWide;
	int m_iBoxTall;

	// Font dimensions
	int m_iNumberWidth;
	int m_iNumberHeight;
	int m_iTextHeight;

	bool m_bFadingOut;

    // Stores the weapon that was most recently used in each bucket.
	int m_iLastUsedSlotInBucket[MAX_WEAPON_SLOTS];
    int m_iLastUsedSubTypeInBucket[MAX_WEAPON_SLOTS];

	CPanelAnimationVar( HFont, m_hNumberFont, "NumberFont", "HudWeaponSelection" );
	CPanelAnimationVar( HFont, m_hTextFont, "Font", "HudWeaponSelection" );
	CPanelAnimationVarAliasType( float, m_iXPos, "xpos", "HudWeaponSelection", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iYPos, "ypos", "HudWeaponSelection", "proportional_float" );
	CPanelAnimationVar( float, m_iWide, "wide", "HudWeaponSelection" );
	CPanelAnimationVar( float, m_iTall, "tall", "HudWeaponSelection" );
	CPanelAnimationVarAliasType( float, m_flBoxGap, "BoxGap", "HudWeaponSelection", "proportional_float" );
	CPanelAnimationVar( float, m_flSelectionNumberXPos, "SelectionNumberXPos", "HudWeaponSelection" );
	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "255" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "SelectionTextFg" );
	CPanelAnimationVar( Color, m_UnselTextColor, "UnselTextColor", "SelectionUnselTextFg" );
	CPanelAnimationVar( Color, m_NumberColor, "NumberColor", "SelectionNumberFg" );
	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SelectedBoxColor, "SelectedBoxColor", "SelectionSelectedBoxBg" );
	CPanelAnimationVar( Color, m_UnselectableBoxColor, "UnselectableBoxColor", "SelectionUnselectableBoxBg" );
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : 
	CBaseHudWeaponSelection(pElementName), 
	BaseClass(NULL, "HudWeaponSelection")
{
	SetParent( g_pClientMode->GetViewport() );
	m_bFadingOut = false;
	m_iBoxWide = m_iBoxTall = 0;
	m_iNumberWidth = 0;

    memset(m_iLastUsedSlotInBucket, 0, 10);
    memset(m_iLastUsedSubTypeInBucket, 0, 10);
}

void CHudWeaponSelection::VidInit()
{
	CHudElement::VidInit();

	// Get our nominal widths/heights of text
	int tmp;
	surface()->GetTextSize( m_hNumberFont, L"3", m_iNumberWidth, m_iNumberHeight );
	surface()->GetTextSize( m_hTextFont, L"RCP90", tmp, m_iTextHeight );

	// Some padding
	m_iNumberWidth += XRES( 5 );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
	{
		if ( IsInSelectionMode() )
			HideSelection();
		return false;
	}

	if (cl_ge_hud_noswitchlist.GetBool())
		return false;

	if ( !CBaseHudWeaponSelection::ShouldDraw() )
		return false;

	return m_bSelectionVisible;
}

void CHudWeaponSelection::SetSelectedWeapon( C_BaseCombatWeapon *pWeapon, bool recordSelection /*== true*/ )
{
	m_hSelectedWeapon = pWeapon;

	if (recordSelection)
	{
        if (pWeapon)
        {
            SetLastUsedWeaponForBucket(pWeapon->GetSlot(), pWeapon->GetPosition(), pWeapon->GetSubType());
        }
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the next item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToNextWeapon()
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	C_BaseCombatWeapon *pCurrWeapon;
	C_BaseCombatWeapon *pNextWeapon = NULL;

	if ( IsInSelectionMode() )
		// Grab our current selection to start
		pCurrWeapon = GetSelectedWeapon();
	else
		// open selection at the current place
		pCurrWeapon = pPlayer->GetActiveWeapon();

	if (!pCurrWeapon)
		return;

	int i = pCurrWeapon->GetSlot();
	int j = pCurrWeapon->GetPosition();
    int k = pCurrWeapon->GetSubType();

	// Loop over all weapon slots starting at our current selection's slot
	// and our current selection's position. Try to find the next active position
	// for this weapon's slot. If you don't find anything jump to the next slot
	// at position 0. If nothing is found with this then we are at the end
	// and have to grab the first weapon!
	for ( ; i < MAX_WEAPON_SLOTS; i++, j=-1, k=-1 )
	{
		pNextWeapon = GetNextActivePos(i, j, k);
		if ( pNextWeapon )
			break;
	}

	// We didn't find a next one so we will return the FIRST one!
	if ( !pNextWeapon || pNextWeapon == pCurrWeapon )
	{
		for ( i=0,j=-1,k=-1; i < MAX_WEAPON_SLOTS; i++ )
		{
			pNextWeapon = GetNextActivePos(i, j, k);
			if ( pNextWeapon )
				break;
		}
	}
	
	if ( !pNextWeapon )
		pNextWeapon = pCurrWeapon;
	
	SetSelectedWeapon( pNextWeapon, hud_fastswitch.GetBool() );
	if ( !IsInSelectionMode() )
		OpenSelection();

	// Play the "cycle to next weapon" sound
	pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the previous item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToPrevWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	C_BaseCombatWeapon *pCurrWeapon;
	C_BaseCombatWeapon *pPrevWeapon = NULL;

	if ( IsInSelectionMode() )
		// Grab our current selection to start
		pCurrWeapon = GetSelectedWeapon();
	else
		// open selection at the current place
		pCurrWeapon = pPlayer->GetActiveWeapon();

	if (!pCurrWeapon)
		return;

	int i = pCurrWeapon->GetSlot();
	int j = pCurrWeapon->GetPosition();
    int k = pCurrWeapon->GetSubType();

	// Loop over all weapon slots starting at our current selection's slot
	// and our current selection's position. Try to find the previous active position
	// for this weapon's slot. If you don't find anything jump to the previous slot
	// at position MAX_WEAPON_POSITIONS. If nothing is found with this then we are at the end
	// and have to grab the first weapon!
	for ( ; i >= 0; i--, j = MAX_WEAPON_POSITIONS-1, k = MAX_WEAPON_SUBTYPES-1 )
	{
		pPrevWeapon = GetPrevActivePos(i, j, k);
		if ( pPrevWeapon )
			break;
	}

	// We didn't find a next one so we will return the LAST one!
	if ( !pPrevWeapon || pPrevWeapon == pCurrWeapon )
	{
		for ( i=MAX_WEAPON_SLOTS-1,j=MAX_WEAPON_POSITIONS-1,k=MAX_WEAPON_SUBTYPES-1; i >= 0; i-- )
		{
			pPrevWeapon = GetPrevActivePos(i, j, k);
			if ( pPrevWeapon )
				break;
		}
	}
	
	if ( !pPrevWeapon )
		pPrevWeapon = pCurrWeapon;

	
	SetSelectedWeapon( pPrevWeapon, hud_fastswitch.GetBool() );
	if ( !IsInSelectionMode() )
		OpenSelection();

	// Play the "cycle to next weapon" sound
	pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}

C_BaseCombatWeapon *CHudWeaponSelection::GetPrevActivePos( int iSlot, int iSlotPos, int iSubType )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	int iHighestPosition = -1;
    int iCurrentPosition = iSlotPos * MAX_WEAPON_SUBTYPES + iSubType;

	C_BaseCombatWeapon *pPrevWeapon = NULL;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( CanBeSelectedInHUD( pWeapon ) && pWeapon->GetSlot() == iSlot )
		{
            int weaponPos = pWeapon->GetPosition() * MAX_WEAPON_SUBTYPES + pWeapon->GetSubType();
			// If this weapon is higher in the slot then our previous highest it's the new winner
			if ( weaponPos > iHighestPosition && weaponPos < iCurrentPosition )
			{
				iHighestPosition = weaponPos;
				pPrevWeapon = pWeapon;
			}
		}
	}

	return pPrevWeapon;
}


//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::GetWeaponInSlot( int iSlot, int iSlotPos, int iSubType )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon(i);

		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() == iSlotPos && pWeapon->GetSubType() == iSubType )
			return pWeapon;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Moves selection to the specified slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeaponSlot( int iSlot )
{
	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	--iSlot;

	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Don't try and read past our possible number of slots
	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return;

	// do a fast switch if set
	if ( hud_fastswitch.GetBool() )
	{
		FastWeaponSwitch( iSlot );
		return;
	}

	// see where we should start selection
	int iPosition = -1;
    int iSubtype = -1;
	C_BaseCombatWeapon *pNextWeapon = NULL;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();

    if ( IsInSelectionMode() && GetSelectedWeapon() && GetSelectedWeapon()->GetSlot() == iSlot )
    {
        iPosition = GetSelectedWeapon()->GetPosition();
        iSubtype = GetSelectedWeapon()->GetSubType();
    }
    else if ( pActiveWeapon && pActiveWeapon->GetSlot() == iSlot )
    {
        iPosition = pActiveWeapon->GetPosition();
		iSubtype = pActiveWeapon->GetSubType();
    }
	else if ( cl_ge_hud_uselastbucketitem.GetBool() )
	{
        iPosition = GetLastUsedSlotForBucket(iSlot);
        iSubtype = GetLastUsedSubtypeForBucket(iSlot) - 1;
	}

	// search for the weapon after the current one
	pNextWeapon = GetNextActivePos( iSlot, iPosition, iSubtype );
	// make sure it's in the same bucket
	if ( !pNextWeapon || pNextWeapon->GetSlot() != iSlot )
	{
		// just look for any weapon in this slot
		pNextWeapon = GetNextActivePos( iSlot, -1, iSubtype );
	}

	if ( pNextWeapon )
	{
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
		if ( !IsInSelectionMode() )
			OpenSelection();

		SetSelectedWeapon( pNextWeapon, false );
	}
	else
		pPlayer->EmitSound( "Player.DenyWeaponSelection" );
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert(!IsInSelectionMode());

	CBaseHudWeaponSelection::OpenSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("OpenWeaponSelectionMenu");
}

//-----------------------------------------------------------------------------
// Purpose: Closes weapon selection control immediately
//-----------------------------------------------------------------------------
void CHudWeaponSelection::HideSelection( void )
{
	CBaseHudWeaponSelection::HideSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("CloseWeaponSelectionMenu");
	m_bFadingOut = false;
}

//-------------------------------------------------------------------------
// Purpose: Blocks mouse clicks from selecting if we are in fast switch mode
//-------------------------------------------------------------------------
void CHudWeaponSelection::ProcessInput( void )
{
	if ( hud_fastswitch.GetBool() )
		return;

	// Actually record the selection now for non-quickswitch users.
	if ( gHUD.m_iKeyBits & (IN_ATTACK | IN_ATTACK2) )
	{
		if (IsWeaponSelectable())
			SetSelectedWeapon(GetSelectedWeapon(), true);
	}
	CBaseHudWeaponSelection::ProcessInput();
}

//-----------------------------------------------------------------------------
// Purpose: updates animation status
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnThink( void )
{
	// Time out after awhile of inactivity
	if ( ( gpGlobals->curtime - m_flSelectionTime ) > SELECTION_TIMEOUT_THRESHOLD )
	{
		if ( !m_bFadingOut )
		{
			// start fading out
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FadeOutWeaponSelectionMenu" );
			m_bFadingOut = true;
		}
		else if ( gpGlobals->curtime - m_flSelectionTime > SELECTION_TIMEOUT_THRESHOLD + SELECTION_FADEOUT_TIME )
		{
			// finished fade, close
			HideSelection();
		}
	}
	else if ( m_bFadingOut )
	{
		// stop us fading out, show the animation again
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "OpenWeaponSelectionMenu" );
		m_bFadingOut = false;
	}
}

//-------------------------------------------------------------------------
// Purpose: draws the selection area
//-------------------------------------------------------------------------
void CHudWeaponSelection::Paint()
{
    Assert( MAX_DISPLAYED_WEAPONS < MAX_WEAPONS );
    
    if ( !ShouldDraw() )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// find and display our current selection
	C_BaseCombatWeapon *pSelectedWeapon = GetSelectedWeapon();
	if ( !pSelectedWeapon )
		return;

	// If we are using fast-switch we only draw our current weapon (exported methodology)
	if ( hud_fastswitch.GetBool() && !cl_ge_hud_fastswitchlist.GetBool())
	{
		PaintFastWeaponSwitch();
		return;
	}

	// calculate where to start drawing
	int x, y, w, h, screen_width, screen_height, i, num = 0;
	
	GetBounds( x, y, w, h );
	surface()->GetScreenSize( screen_width, screen_height );

	x = screen_width - m_iBoxWide - m_iXPos;
	y = (screen_height - m_iBoxTall*2) - m_iYPos;

	int text_y_pad = (m_iBoxTall / 2) - (m_iTextHeight / 2);

    int count_before = 0;
    int count_after = 0;
    bool foundSelected = false;

    for (i = MAX_WEAPON_SLOTS; i >= 0; i--)
    {
        for (int slotpos = MAX_WEAPON_POSITIONS; slotpos >= 0; slotpos--)
        {
            for (int subPos = MAX_WEAPON_SUBTYPES - 1; subPos >= 0; subPos--)
            {
                C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i, slotpos, subPos);

                if (!pWeapon)
                    continue;

                if (pWeapon == pSelectedWeapon)
                {
                    foundSelected = true;
                }
                else if (foundSelected)
                {
                    count_after++;
                }
                else
                {
                    count_before++;
                }
            }
        }
    }

    int total_count = count_before + count_after + 1;
    int ignore_before = 0;

    if ( total_count > MAX_DISPLAYED_WEAPONS )
    {
        ignore_before = clamp(count_before - (MAX_DISPLAYED_WEAPONS / 2), 0, total_count - MAX_DISPLAYED_WEAPONS);
    }

	for ( i=MAX_WEAPON_SLOTS; i >= 0; i-- )
	{
		bool bFirstItem = true; //First bucket item
		for (int slotpos = MAX_WEAPON_POSITIONS; slotpos >= 0; slotpos--)
		{
            for (int subPos = MAX_WEAPON_SUBTYPES - 1; subPos >= 0; subPos--)
            {
                if (num >= MAX_DISPLAYED_WEAPONS)
                    break;

                C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i, slotpos, subPos);

                if (!pWeapon)
                    continue;

                if (ignore_before > 0)
                {
                    ignore_before--;
                    continue;
                }

                // The default color of an unselected selectable weapon
                Color col = m_BoxColor;

                num++;

                if (!pWeapon->CanBeSelected())
                    // Make the box red if we can't select it
                    col = m_UnselectableBoxColor;

                // If this is the selected weapon then make it fully opaque
                if (pWeapon == pSelectedWeapon)
                    col[3] = m_flSelectionAlphaOverride;
                else
                    col[3] = m_flAlphaOverride;

                if (FirstOfBucket(pWeapon->GetSlot(), pWeapon->GetPosition(), pWeapon->GetSubType()))
                    DrawBox(x, y, m_iBoxWide, m_iBoxTall, col, pWeapon->GetSlot() + 1);
                else
                    DrawBox(x, y, m_iBoxWide, m_iBoxTall, col);

                // Draw the weapon's print name on the bucket

                const char *printName = pWeapon->GetPrintName();

                CGEWeapon *pGEWeapon = ToGEWeapon(pWeapon);
                if (pGEWeapon)
                    printName = pGEWeapon->GetCustomPrintName();

                wchar_t wszName[MAX_WEAPON_STRING];
                wchar_t *tempString = g_pVGuiLocalize->Find(printName);

                if (tempString)
                    _snwprintf(wszName, MAX_WEAPON_STRING, L"%s", tempString);
                else
                    g_pVGuiLocalize->ConvertANSIToUnicode(printName, wszName, MAX_WEAPON_STRING);

                if (!pWeapon->CanBeSelected())
                {
                    col = m_UnselTextColor;
                    col[3] = m_flSelectionAlphaOverride;
                    surface()->DrawSetTextColor(col);
                }
                else
                {
                    col = m_TextColor;
                    col[3] = m_flSelectionAlphaOverride;
                    surface()->DrawSetTextColor(col);
                }

                int tx = x + m_iNumberWidth;
                int ty = y + text_y_pad;

                surface()->DrawSetTextFont(m_hTextFont);
                surface()->DrawSetTextPos(tx, ty);
                surface()->DrawPrintText(wszName, wcslen(wszName));

                y -= (m_iBoxTall + m_flBoxGap);

                bFirstItem = false;
            }
		}
	}
}

void CHudWeaponSelection::PaintFastWeaponSwitch( void )
{
	// calculate where to start drawing
	int b[4], x, y;
	GetBounds( b[0], b[1], b[2], b[3] );
	surface()->GetScreenSize( x, y );

	b[1] = (y - m_iBoxTall*2) - m_iYPos;
	b[0] = x - m_iBoxWide - m_iXPos;

	C_BaseCombatWeapon *pSelectedWeapon = GetSelectedWeapon();
	if ( !pSelectedWeapon )
		return;

	// The default color of an unselected selectable weapon
	Color col = m_BoxColor;
	col[3] = m_flSelectionAlphaOverride;

	DrawBox( b[0], b[1], m_iBoxWide, m_iBoxTall, col );

    const char *selectedPrintName = pSelectedWeapon->GetPrintName();

    CGEWeapon *pGESelectedWeapon = ToGEWeapon(pSelectedWeapon);
    if (pGESelectedWeapon)
        selectedPrintName = pGESelectedWeapon->GetCustomPrintName();

	// Draw the weapon's print name on the bucket
	wchar_t wszName[MAX_WEAPON_STRING];
	wchar_t *tempString = g_pVGuiLocalize->Find(selectedPrintName);

	if ( tempString )
		_snwprintf(wszName, MAX_WEAPON_STRING, L"%s", tempString);
	else
		g_pVGuiLocalize->ConvertANSIToUnicode(selectedPrintName, wszName, MAX_WEAPON_STRING);

	col = m_TextColor;
	col[3] = m_flSelectionAlphaOverride;
	surface()->DrawSetTextColor( col );

	int tx = b[0] + 24;
	int ty = b[1] + 4;

	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextPos( tx, ty );
	surface()->DrawPrintText( wszName, wcslen(wszName) );
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ApplySchemeSettings( IScheme *pScheme )
{
	int x, y;
	surface()->GetScreenSize( x, y );
	SetBounds( 0, 0, x, y ); // temporary, take the screen up

	// Set our size to a ratio of the screen size
	m_iBoxWide = x * .12;  // 12% wide
	m_iBoxTall = y * .027;	// 2.7% tall

	Panel::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: check if this is their first weapon of a bucket
//-----------------------------------------------------------------------------
int CHudWeaponSelection::GetSelectionNumber( int iSlot, int iSlotPos )
{
	int num = 0;

	for ( int i=0; i < MAX_WEAPON_SLOTS; i++ )
	{
		for ( int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++ )
		{
			C_BaseCombatWeapon *pCurWeapon = GetWeaponInSlot(i,slotpos);
			C_BaseCombatWeapon *pGetWeapon = GetWeaponInSlot(iSlot,iSlotPos);

			if( !pCurWeapon || !pGetWeapon )
				continue;

			num++;

			if( pCurWeapon == pGetWeapon )
					return num;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: return the selection number
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::FirstOfBucket( int iSlot, int iSlotPos, int iSubType )
{
	C_BaseCombatWeapon *pGetWeapon = GetWeaponInSlot( iSlot, iSlotPos, iSubType );

	if( !pGetWeapon )
		return false;

	for ( int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++ )
	{
        for ( int subtype = 0; subtype < MAX_WEAPON_SUBTYPES; subtype++ )
        {
            C_BaseCombatWeapon *pCurWeapon = GetWeaponInSlot( iSlot, slotpos, subtype );
		    if( !pCurWeapon )
				    continue;

		    if( pCurWeapon == pGetWeapon )
			    return true;
		    else
			    return false;
        }
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::FastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// see where we should start selection
	int iPosition = -1;
    int iSubtype = -1;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();

	if ( pActiveWeapon && pActiveWeapon->GetSlot() == iWeaponSlot )
	{
		// start after this weapon
		iPosition = pActiveWeapon->GetPosition();
        iSubtype = pActiveWeapon->GetSubType();
	}
	else if ( cl_ge_hud_uselastbucketitem.GetBool() )
	{
        iPosition = GetLastUsedSlotForBucket(iWeaponSlot);
        iSubtype = GetLastUsedSubtypeForBucket(iWeaponSlot) - 1;
	}

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search for the weapon after the current one
	pNextWeapon = GetNextActivePos( iWeaponSlot, iPosition, iSubtype );
	// make sure it's in the same bucket
	if ( !pNextWeapon || pNextWeapon->GetSlot() != iWeaponSlot )
	{
		// just look for any weapon in this slot
		pNextWeapon = GetNextActivePos( iWeaponSlot, -1, iSubtype );
	}

	// see if we found a weapon that's different from the current and in the selected slot
	if ( pNextWeapon && pNextWeapon != pActiveWeapon && pNextWeapon->GetSlot() == iWeaponSlot )
	{
		// select the new weapon
		::input->MakeWeaponSelection( pNextWeapon );

		// Display this weapon's name
		if ( !IsInSelectionMode() )
			OpenSelection();

		SetSelectedWeapon( pNextWeapon );
	}
	else if ( pNextWeapon != pActiveWeapon )
	{
		// error sound
		pPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawBox( int x, int y, int wide, int tall, Color color, int number /* = -1 */ )
{
	BaseClass::DrawBox( x, y, wide, tall, color, 1.0f );

	// draw the number
	if ( number > 0 )
	{
		// This lets us fade the numbers properly
		Color col = m_NumberColor;
		col[3] = m_flSelectionAlphaOverride;

		surface()->DrawSetTextColor( col );
		surface()->DrawSetTextFont( m_hNumberFont );

		wchar_t wszNumber[1];
		_snwprintf(wszNumber, 1, L"%i", number);
		surface()->DrawSetTextPos( x + m_flSelectionNumberXPos, y + (tall / 2) - (m_iNumberHeight / 2) );
		surface()->DrawPrintText( wszNumber, 1 );
	}
}
