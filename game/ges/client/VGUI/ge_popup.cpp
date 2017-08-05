///////////// Copyright © 2017, Goldeneye: Source. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_popup.cpp
//   Description : Generic popup box with a message since I couldn't seem to find one that already existed.
//
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_popup.h"
#include "ge_shareddefs.h"
#include "ge_utils.h"
#include <vgui/ISurface.h>
#include "vgui_controls/Controls.h"
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/URLLabel.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGEPopupBox::CGEPopupBox( vgui::VPANEL parent, const char *title, const char *message ) : BaseClass( NULL, GetName() )
{
	SetParent( parent );

	SetScheme( vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme") );
	SetProportional( true );

	LoadControlSettings("resource/GESPopupBox.res");

	InvalidateLayout();
	MakePopup();

	SetSizeable( false );
	SetPaintBackgroundEnabled( true );
	SetPaintBorderEnabled( true );
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );

	displayPopup( title, message );
}

CGEPopupBox::~CGEPopupBox()
{
}

void CGEPopupBox::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBgColor( pScheme->GetColor("UpdateBGColor", Color(123,146,156,255)) );
}

void CGEPopupBox::displayPopup( const char *title, const char *message )
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

	vgui::Panel *pContent = FindChildByName( "message" );
	if ( pContent )
		PostMessage( pContent, new KeyValues( "SetText", "text", message ) );
	
	pContent = FindChildByName( "title" );
	if ( pContent )
		PostMessage( pContent, new KeyValues( "SetText", "text", title ) );

	SetVisible( true );
	SetZPos( 5 );
	RequestFocus();
	MoveToFront();

	//SetBounds(ScreenWidth()/2 - XRES(POPUP_SIZE)/2,ScreenHeight()/2 - YRES(POPUP_SIZE)/2,XRES(POPUP_SIZE), YRES(POPUP_SIZE));

	// Make sure our background is correct
	SetBgColor( pScheme->GetColor( "UpdateBGColor", Color(123,146,156,255) ) );
}