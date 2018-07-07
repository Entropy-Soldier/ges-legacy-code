//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>

#include "ge_teammenu.h"

#include <networkstringtabledefs.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_controls/ImagePanel.h"
#include <KeyValues.h>
#include <FileSystem.h>

#include "IGameUIFuncs.h" // for key bindings
#include <c_playerresource.h>
#include "c_team.h"
#include <stdlib.h> // MAX_PATH define
#include <stdio.h>
#include "byteswap.h"

#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "ge_utils.h"

#include "c_ge_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details
extern INetworkStringTable *g_pStringTableInfoPanel;  // for MOTD

#define TEMP_HTML_FILE	"motd_temp.html"
#define OPT_BOOL	1
#define OPT_INT		2
#define OPT_STRING	3

extern ConVar ge_allowradar;
extern ConVar ge_allowjump;
extern ConVar ge_startarmed;
extern ConVar ge_velocity;
extern ConVar friendlyfire;

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGETeamMenu::CGETeamMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_TEAM )
{
	m_pViewPort = pViewPort;
	m_iJumpKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

	// load the new scheme early!!
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/TeamMenuScheme.res", "TeamMenuScheme"));
	SetMoveable(false);
	SetSizeable(false);
	SetProportional( true );

	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );
	SetTitleBarVisible( false );

	m_pHTMLMessage = new HTML(this, "MOTD");

	LoadControlSettings("resource/ui/TeamMenu.res");
	InvalidateLayout();

	// Used to load the fonts
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	m_pTeamButtonList = dynamic_cast<vgui::PanelListPanel*>( FindChildByName("TeamButtons") );
	m_pServerOptList = dynamic_cast<vgui::SectionedListPanel*>( FindChildByName("ServerInfo") );

	m_pTeamButtonList->SetFirstColumnWidth( 0 );
	m_pTeamButtonList->SetScrollBarVisible( false );
	m_pTeamButtonList->SetVerticalBufferPixels( scheme()->GetProportionalScaledValue(4) );

	m_pServerOptList->AddSection( 0, "" );
	m_pServerOptList->AddColumnToSection( 0, "name", "#TeamMenu_ServerVar", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), 105 ) );
	m_pServerOptList->AddColumnToSection( 0, "value", "#TeamMenu_Value", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), 125 ) );
	m_pServerOptList->SetSectionAlwaysVisible( 0 );
	m_pServerOptList->SetSectionFont( 0, pScheme->GetFont("TypeWriter_Small", true) );
	m_pServerOptList->SetHeaderFont( 0, pScheme->GetFont("TypeWriter", true) );

	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_disconnect" );
	
	m_szGameMode[0] = '\0';
	m_szWeaponSet[0] = '\0';
	m_szMapName[0] = '\0';

	m_bTeamplay = false;
	m_flNextUpdateTime = 0;

	m_iMI6Offset = 0;
	m_iJanusOffset = 0;
	m_iSpecOffset = 0;
}

void CGETeamMenu::OnGameplayDataUpdate( void )
{
	// Update Gameplay information
	wchar_t gamemode[64];
	GEUTIL_GetGameplayName( gamemode, sizeof(gamemode) );
	g_pVGuiLocalize->ConvertUnicodeToANSI( gamemode, m_szGameMode, sizeof(m_szGameMode) );

	// Update Weapon information
	Q_strncpy( m_szWeaponSet, GEGameplayRes()->GetLoadoutName(), sizeof(m_szWeaponSet) );

	// Update the display immediately
	if ( IsVisible() )
		Update();
}

void CGETeamMenu::FireGameEvent( IGameEvent *pEvent )
{
	if ( !Q_strcmp(pEvent->GetName(),"game_newmap"))
		SetMapName( pEvent->GetString("mapname") );
	else if ( !Q_strcmp(pEvent->GetName(),"player_team") ) // Someone joined a team so remake the team buttons.
	{
		m_iMI6Offset = 0; // Calculate offsets since the join event arives before the client team counters are updated.
		m_iJanusOffset = 0; // Don't really need to do this since they always reset at the end, but could be a bit of future-proofing.
		m_iSpecOffset = 0;

		int newTeam = pEvent->GetInt("team");
		int oldTeam = pEvent->GetInt("oldteam");

		// Seperating these two steps is important because spectator might be involved.
		if ( newTeam == TEAM_JANUS ) //switching to janus
			m_iJanusOffset++;
		else if ( newTeam == TEAM_MI6 ) //switching to MI6
			m_iMI6Offset++;
		else if ( newTeam == TEAM_SPECTATOR )
			m_iSpecOffset++;

		if ( oldTeam == TEAM_JANUS ) //switching away from janus
			m_iJanusOffset--;
		else if ( oldTeam == TEAM_MI6 ) //switching away from MI6
			m_iMI6Offset--;
		else if ( oldTeam == TEAM_SPECTATOR )
			m_iSpecOffset--;

		MakeTeamButtons();

		m_iMI6Offset = 0; // Reset the offsets now that we've finished updating the buttons.
		m_iJanusOffset = 0;
		m_iSpecOffset = 0;
	}
	else // Someone is disconnecting so remake the buttons.
	{
		// Can't do anything without our player resource.
		if ( !GEPlayerRes() )
			return;

		m_iMI6Offset = 0; // Calculate offsets since the disconnect event arives before the client team counters are updated.
		m_iJanusOffset = 0; // Don't really need to do this since they always reset at the end, but could be a bit of future-proofing.
		m_iSpecOffset = 0;

		int team = GEPlayerRes()->GetTeam(engine->GetPlayerForUserID(pEvent->GetInt("userid")));

		if ( team == TEAM_JANUS ) //switching away from janus
			m_iJanusOffset--;
		else if ( team == TEAM_MI6 ) //switching away from MI6
			m_iMI6Offset--;
		else if ( team == TEAM_SPECTATOR )
			m_iSpecOffset--;

		MakeTeamButtons();

		m_iMI6Offset = 0; // Reset the offsets now that we've finished updating the buttons.
		m_iJanusOffset = 0;
		m_iSpecOffset = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows the team menu
//-----------------------------------------------------------------------------
void CGETeamMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	// Never show the team menu in SourceTV
	if ( engine->IsHLTV() )
	{
		SetVisible( false );
		return;
	}

	if ( bShow )
	{
		Activate();

		SetMouseInputEnabled( true );

		m_bTeamplay = GEMPRules()->IsTeamplay();

		// Logically make our team buttons
		MakeTeamButtons();
		// Run through our functions
		Update();
		// Show the MOTD
		ShowMOTD();

		// get key bindings if shown
		if( m_iJumpKey == BUTTON_CODE_INVALID ) // you need to lookup the jump key AFTER the engine has loaded
		{
			m_iJumpKey = gameuifuncs->GetButtonCodeForBind( "jump" );
		}

		if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
		}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}

bool CGETeamMenu::NeedsUpdate()
{
	return gpGlobals->curtime > m_flNextUpdateTime;
}

//-------------------------------------------
// Purpose: updates the UI when visible
//-------------------------------------------
void CGETeamMenu::Update()
{
	if ( GEMPRules() && GEMPRules()->IsTeamplay() != m_bTeamplay )
	{
		m_bTeamplay = GEMPRules()->IsTeamplay();
		MakeTeamButtons();
	}

	// Update the server option text
	CreateServerOptText();

	// Update the board every 1 second
	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CGETeamMenu::OnCommand( const char *command )
{
	if ( Q_strstr( command, "jointeam" ) )
		engine->ClientCmd( command );
	
	ShowPanel(false);
	BaseClass::OnCommand(command);
}

//-----------------------------------------------------------------------------
// Purpose: sets the text on and displays the team buttons
//-----------------------------------------------------------------------------
void CGETeamMenu::MakeTeamButtons()
{
	// We can't make the team buttons until we have game resources!
	if ( !GameResources() || !GetNumberOfTeams() )
		return;

	if ( !m_pTeamButtonList )
		return;

	// First we clear out the list
	m_pTeamButtonList->DeleteAllItems();

	char szLabel[32];

	// Then create the Join Game or Auto Join Button
	if ( m_bTeamplay ) 
	{
		AddButton( "#TM_Auto_Join", MAX_GE_TEAMS );

		// Now create the buttons for the teams if we need them
		for (int i=FIRST_GAME_TEAM; i < MAX_GE_TEAMS; i++)
		{
			if ( !GetGlobalTeam(i) )
				continue;

			int offset = 0; // In case we need to predict team counts before they actually update.
			if (i == TEAM_JANUS)
				offset = m_iJanusOffset;
			else if (i == TEAM_MI6)
				offset = m_iMI6Offset;

			Q_snprintf( szLabel, 32, "%s (%i)", GameResources()->GetTeamName(i), GetGlobalTeam(i)->Get_Number_Players() + offset );
			AddButton( szLabel, i );
		}
	} 
	else 
	{
		AddButton( "#TM_Join", TEAM_UNASSIGNED );
	}

	// Lastly create the Spectator button (always last)
	if ( GetGlobalTeam(TEAM_SPECTATOR) )
	{
		wchar_t specs[64];
		GEUTIL_ParseLocalization(specs, 64, VarArgs("#TM_Spectator\r%i", GetGlobalTeam(TEAM_SPECTATOR)->Get_Number_Players() + m_iSpecOffset));
		AddButton(specs, TEAM_SPECTATOR);
	}
}

extern ConVar ge_allow_unbalanced_teamswitch;
extern ConVar ge_teamautobalance;

vgui::Button *CGETeamMenu::AddButton( const char* label, int iTeam )
{
	vgui::EditablePanel *pPanel = new vgui::EditablePanel( NULL, NULL );
	pPanel->SetTall( scheme()->GetProportionalScaledValue(28) );
	pPanel->SetWide( 100 );

	vgui::ImagePanel *pImage = new vgui::ImagePanel( pPanel, NULL );
	pImage->SetShouldScaleImage( true );
	pImage->SetBounds( 0, 0, pPanel->GetWide(), pPanel->GetTall() );
	pImage->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );

	int thisTeam = TEAM_UNASSIGNED;
	int thisTeamCount = 0;
	int otherTeam = TEAM_INVALID;
	int otherTeamCount = 0;
	int currentTeam = TEAM_INVALID;
	bool allowClick = true;

	// Setup our background based on the team
	if ( iTeam == TEAM_MI6 )
	{
		thisTeam = TEAM_MI6;
		thisTeamCount = GetGlobalTeam(TEAM_MI6)->Get_Number_Players() + m_iMI6Offset;
		otherTeam = TEAM_JANUS;
		otherTeamCount = GetGlobalTeam(TEAM_JANUS)->Get_Number_Players() + m_iJanusOffset;
		pImage->SetImage( "teamselect/button_mi6" );
	}
	else if ( iTeam == TEAM_JANUS )
	{
		thisTeam = TEAM_JANUS;
		thisTeamCount = GetGlobalTeam(TEAM_JANUS)->Get_Number_Players() + m_iJanusOffset;
		otherTeam = TEAM_MI6;
		otherTeamCount = GetGlobalTeam(TEAM_MI6)->Get_Number_Players() + m_iMI6Offset;
		pImage->SetImage( "teamselect/button_janus" );
	}
	else
		pImage->SetImage( "teamselect/button_normal" );

	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
		currentTeam = pPlayer->GetTeamNumber();

	if ( otherTeam != TEAM_INVALID && currentTeam != thisTeam && !ge_allow_unbalanced_teamswitch.GetBool() && ge_teamautobalance.GetBool() )
	{
		if (currentTeam == otherTeam) // Switching across team boundaries, should not be able to switch when teams are equal.
		{
			if ( thisTeamCount >= otherTeamCount )
				allowClick = false;
			else
				allowClick = true;
		}
		else // Joining from no team, can join when teams are equal.
		{
			if ( thisTeamCount > otherTeamCount )
				allowClick = false;
			else
				allowClick = true;
		}
	}

	char command[32];
	Q_snprintf( command, sizeof(command), "jointeam %i", iTeam );

	vgui::Button *pButton = new vgui::Button( pPanel, NULL, label );
	pButton->SetContentAlignment( vgui::Label::a_center );
	pButton->SetBounds( 0, 0, pPanel->GetWide(), pPanel->GetTall() );
	pButton->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );
	pButton->SetCommand(command);
	if (allowClick)
	{
		pButton->DisableMouseInputForThisPanel(false);
	}
	else
	{
		pButton->DisableMouseInputForThisPanel(true);
	}
	pButton->SetFont( scheme()->GetIScheme(GetScheme())->GetFont("TypeWriter", true) );
	pButton->SetPaintBackgroundEnabled( false );
	pButton->SetPaintBorderEnabled( false );
	pButton->AddActionSignalTarget( this );

	m_pTeamButtonList->AddItem( NULL, pPanel );

	return pButton;
}

vgui::Button *CGETeamMenu::AddButton( const wchar_t* label, int iTeam )
{
	vgui::EditablePanel *pPanel = new vgui::EditablePanel( NULL, NULL );
	pPanel->SetTall( scheme()->GetProportionalScaledValue(28) );
	pPanel->SetWide( 100 );

	vgui::ImagePanel *pImage = new vgui::ImagePanel( pPanel, NULL );
	pImage->SetShouldScaleImage( true );
	pImage->SetBounds( 0, 0, pPanel->GetWide(), pPanel->GetTall() );
	pImage->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );

	int thisTeam = TEAM_UNASSIGNED;
	int thisTeamCount = 0;
	int otherTeam = TEAM_INVALID;
	int otherTeamCount = 0;
	int currentTeam = TEAM_INVALID;
	bool allowClick = true;

	// Setup our background based on the team
	if ( iTeam == TEAM_MI6 )
	{
		thisTeam = TEAM_MI6;
		thisTeamCount = GetGlobalTeam(TEAM_MI6)->Get_Number_Players() + m_iMI6Offset;
		otherTeam = TEAM_JANUS;
		otherTeamCount = GetGlobalTeam(TEAM_JANUS)->Get_Number_Players() + m_iJanusOffset;
		pImage->SetImage( "teamselect/button_mi6" );
	}
	else if ( iTeam == TEAM_JANUS )
	{
		thisTeam = TEAM_JANUS;
		thisTeamCount = GetGlobalTeam(TEAM_JANUS)->Get_Number_Players() + m_iJanusOffset;
		otherTeam = TEAM_MI6;
		otherTeamCount = GetGlobalTeam(TEAM_MI6)->Get_Number_Players() + m_iMI6Offset;
		pImage->SetImage( "teamselect/button_janus" );
	}
	else
		pImage->SetImage( "teamselect/button_normal" );

	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
		currentTeam = pPlayer->GetTeamNumber();

	if ( otherTeam != TEAM_INVALID && currentTeam != thisTeam && !ge_allow_unbalanced_teamswitch.GetBool() && ge_teamautobalance.GetBool() )
	{
		if (currentTeam == otherTeam) // Switching across team boundaries, should not be able to switch when teams are equal.
		{
			if ( thisTeamCount >= otherTeamCount )
				allowClick = false;
			else
				allowClick = true;
		}
		else // Joining from no team, can join when teams are equal.
		{
			if ( thisTeamCount > otherTeamCount )
				allowClick = false;
			else
				allowClick = true;
		}
	}

	char command[32];
	Q_snprintf( command, sizeof(command), "jointeam %i", iTeam );

	vgui::Button *pButton = new vgui::Button( pPanel, NULL, label );
	pButton->SetContentAlignment( vgui::Label::a_center );
	pButton->SetBounds( 0, 0, pPanel->GetWide(), pPanel->GetTall() );
	pButton->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );
	pButton->SetCommand( command );
	if (allowClick)
	{
		pButton->DisableMouseInputForThisPanel(false);
	}
	else
	{
		pButton->DisableMouseInputForThisPanel(true);
	}
	pButton->SetFont( scheme()->GetIScheme(GetScheme())->GetFont("TypeWriter", true) );
	pButton->SetPaintBackgroundEnabled( false );
	pButton->SetPaintBorderEnabled( false );
	pButton->AddActionSignalTarget( this );

	m_pTeamButtonList->AddItem( NULL, pPanel );

	return pButton;
}

void CGETeamMenu::CreateServerOptText( void )
{
	// Clear our variables first
	m_pServerOptList->RemoveAll();

	char timelimit[6];
	GetMapTimeLeft(timelimit);

	bool radar = ge_allowradar.GetBool();
	bool startarmed = ge_startarmed.GetInt() > 1 ? true : false;
	bool ff = friendlyfire.GetBool();
	bool allowjump = ge_allowjump.GetBool();

	char turbo_mode[64] = "#TURBO_MODE_NORM";

	ConVar* ge_velocity = g_pCVar->FindVar( "ge_velocity" );
	if ( ge_velocity )
	{
		if ( ge_velocity->GetFloat() < 1.0f )
			Q_strncpy( turbo_mode, "#TURBO_MODE_SLOW", 64 );
		else if ( ge_velocity->GetFloat() > 1.5f )
			Q_strncpy( turbo_mode, "#TURBO_MODE_LIGHT", 64 );
		else if ( ge_velocity->GetFloat() > 1.0f )
			Q_strncpy( turbo_mode, "#TURBO_MODE_FAST", 64 );
	}
			
	ServerOptAdder( "#GE_GameMode", &m_szGameMode, OPT_STRING );
	ServerOptAdder( "#GE_WepSet", &m_szWeaponSet, OPT_STRING );
	ServerOptAdder( "#GE_MapTimeLeft", &timelimit, OPT_STRING );
	ServerOptAdder( "#GE_TurboMode", &turbo_mode, OPT_STRING );
	ServerOptAdder( "#GE_Radar", &radar, OPT_BOOL );
	ServerOptAdder( "#GE_StartArmed", &startarmed, OPT_BOOL );
	ServerOptAdder( "#GE_FriendlyFire", &ff, OPT_BOOL );
	ServerOptAdder( "#GE_Jumping", &allowjump, OPT_BOOL );
}

void CGETeamMenu::ServerOptAdder( const char* name, void *value, int type )
{
	// Used to load the color
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	char *color = "Black";

	KeyValues *data = new KeyValues("temp");
	data->SetString( "name", name );

	switch (type)
	{
	case OPT_BOOL:
		if ( *static_cast<bool*>(value) ) {
			data->SetString( "value", "#Enabled" );
			color = "true";
		} else {
			data->SetString( "value", "#Disabled" );
			color = "false";
		}
		break;

	case OPT_INT:
		data->SetInt( "value", *static_cast<int*>(value) );
		break;

	case OPT_STRING:
		data->SetString( "value", static_cast<char*>(value) );
		break;

	default:
		data->SetInt( "value", 0 );
		break;
	}

	int itemID = m_pServerOptList->AddItem( 0, data );

	m_pServerOptList->SetItemFgColor( itemID, pScheme->GetColor( color, Color(0,0,0,255) ) );
}

void CGETeamMenu::GetMapTimeLeft( char* szTime )
{
	if ( !GEMPRules() )
		return;

	float timeleft = GEMPRules()->GetMatchTimeRemaining();
	int mins = timeleft / 60;
	int secs = timeleft - (mins*60);

	Q_snprintf(szTime, 6, "%d:%.2d", mins, secs);
}

void CGETeamMenu::SetMapName( const char* name )
{
	vgui::Label *control = dynamic_cast<vgui::Label*>( FindChildByName("MapName") );
	if (control)
	{
		char temp[MAX_MAP_NAME], mapname[MAX_MAP_NAME + 5];
		Q_strncpy( temp, name, MAX_MAP_NAME );
		Q_strupr( temp );
		if ( Q_strlen(temp) >= 20 )
			control->SetFont( scheme()->GetIScheme(GetScheme())->GetFont("TypeWriter_Medium", true) );
		else
			control->SetFont( scheme()->GetIScheme(GetScheme())->GetFont("TypeWriter_Large", true) );

		Q_snprintf( mapname, MAX_MAP_NAME+5, "MAP: %s", temp );
		PostMessage( control, new KeyValues( "SetText", "text", mapname ) );
	}
}

// MOTD Functions
//
void CGETeamMenu::ShowURL( const char *URL)
{
	m_pHTMLMessage->SetVisible( true );
	m_pHTMLMessage->OpenURL( URL );
}

void CGETeamMenu::ShowMOTD( void )
{
	const char *data = NULL;
	int length = 0;

	if ( NULL == g_pStringTableInfoPanel )
		return;

	// Try to find the already loaded MOTD file
	int index = g_pStringTableInfoPanel->FindStringIndex( "motd" );
		
	if ( index != ::INVALID_STRING_INDEX )
		data = (const char *)g_pStringTableInfoPanel->GetStringUserData( index, &length );

	if ( !data || !data[0] )
		return; // nothing to show

	// is this a web URL ?
	if ( !Q_strncmp( data, "http://", 7 ) )
	{
		ShowURL( data );
		return;
	}

	// data is a HTML, we have to write to a file and then load the file
	FileHandle_t hFile = g_pFullFileSystem->Open( TEMP_HTML_FILE, "wb", "DEFAULT_WRITE_PATH" );

	if ( hFile == FILESYSTEM_INVALID_HANDLE )
		return;

	g_pFullFileSystem->Write( data, length, hFile );
	g_pFullFileSystem->Close( hFile );

	if ( g_pFullFileSystem->Size( TEMP_HTML_FILE ) != (unsigned int)length )
		return; // something went wrong while writing

	ShowFile( TEMP_HTML_FILE );
}

void CGETeamMenu::ShowFile( const char *filename )
{
	if  ( Q_stristr( filename, ".htm" ) || Q_stristr( filename, ".html" ) )
	{
		// it's a local HTML file
		char localURL[ _MAX_PATH + 7 ];
		Q_strncpy( localURL, "file://", sizeof( localURL ) );
		
		char pPathData[ _MAX_PATH ];
		g_pFullFileSystem->GetLocalPath( filename, pPathData, sizeof(pPathData) );
		Q_strncat( localURL, pPathData, sizeof( localURL ), COPY_ALL_CHARACTERS );

		ShowURL( localURL );
	}
}

void CGETeamMenu::OnKeyCodePressed(KeyCode code)
{
	if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

CON_COMMAND(teamselect, "Brings up the team menu.")
{
	gViewPortInterface->ShowPanel( PANEL_TEAM, true );
}
