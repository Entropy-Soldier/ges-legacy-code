///////////// Copyright © 2018, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_scoreutilities.h
// Description:
//      Functions relating to scoreboard formatting, typically shared between ge_scoreboard and ge_roundreport.
//
/////////////////////////////////////////////////////////////////////////////////////

#ifndef GE_SCOREUTILITIES_H
#define GE_SCOREUTILITIES_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/ImagePanel.h>

// Formats the given score value into formatString using the format MM:SS where each point is equal to 1 second.
void GetTimeFormatString( int score, char *formatString, int sizeofFormatString );

// Formats the given score value into formatString using the format L | K where L is the levels the player has and K is the kills.
void GetLevelsFormatString( int score, char *formatString, int sizeofFormatString );

// Updates the given score panel to the supplied score value and color.  Setting addPrefix to true will add "Score:" to the start of the score string.
void UpdateTeamScorePanel( vgui::Panel *sourcePanel, vgui::Panel* targetPanel, int scoreValue, bool addPrefix, bool applyColor = false, Color color = Color(255, 255, 255) );
#endif