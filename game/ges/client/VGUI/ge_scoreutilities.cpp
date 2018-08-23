///////////// Copyright © 2018, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_scoreutilities.h
// Description:
//      Functions relating to scoreboard formatting, typically shared between ge_scoreboard and ge_roundreport.
//
/////////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "gemp_gamerules.h"
#include "ge_scoreutilities.h"

#include "hud.h"
#include <game/client/iviewport.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui/IVgui.h>

// Score is in seconds in this format.
void GetTimeFormatString( int score, char *formatString, int sizeofFormatString )
{
	int displayseconds = score % 60;
	int displayminutes = floor(score / 60);

	Q_snprintf(formatString, sizeofFormatString, "%d:%s%d", displayminutes, displayseconds < 10 ? "0" : "", displayseconds);
}

void GetLevelsFormatString( int score, char *formatString, int sizeofFormatString )
{
	int pointsPerLevel = GEMPRules()->GetScorePerScoreLevel();

	Q_snprintf(formatString, sizeofFormatString, "%d | %d", (score / pointsPerLevel) + 1, score % pointsPerLevel);
}

void UpdateTeamScorePanel( vgui::Panel *sourcePanel, vgui::Panel* targetPanel, int scoreValue, bool addPrefix, bool applyColor /*== false*/, Color color /*== Color(255, 255, 255)*/ )
{
    char scoreValueString[32];

    if (GEMPRules()->GetTeamScoreboardMode() == SCOREBOARD_POINTS_STANDARD)
		Q_snprintf( scoreValueString, sizeof(scoreValueString), "%i", scoreValue );
	else if (GEMPRules()->GetTeamScoreboardMode() == SCOREBOARD_POINTS_TIME)
		GetTimeFormatString( scoreValue, scoreValueString, sizeof(scoreValueString) );
	else if (GEMPRules()->GetTeamScoreboardMode() == SCOREBOARD_POINTS_LEVELS)
        GetLevelsFormatString( scoreValue, scoreValueString, sizeof(scoreValueString) );
	else
		Warning("Tried to display team score using invalid mode %d!\n", GEMPRules()->GetTeamScoreboardMode());

    wchar_t *scoreFmt;
    
    if (addPrefix)
    {
        scoreFmt = g_pVGuiLocalize->Find("#ScoreBoard_TeamScore");
        if (!scoreFmt)
            scoreFmt = L"Score: %s1";
    }
    else
    {
        scoreFmt = L"%s1";
    }

    wchar_t score[128], wszRounds[32];

	g_pVGuiLocalize->ConvertANSIToUnicode( scoreValueString, wszRounds, sizeof(wszRounds) );

	g_pVGuiLocalize->ConstructString( score, sizeof(score), scoreFmt, 1, wszRounds );
	sourcePanel->PostMessage( targetPanel, new KeyValues( "SetText", "text", score ) );

    if (applyColor)
    {
        targetPanel->SetFgColor( color );
    }
}