//////////  Copyright � 2008, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_mapmanager.cpp
// Description: Manages all relevant map data
//
///////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "gemp_player.h"
#include "ge_spawner.h"
#include "ge_gameplayresource.h"

#include "ge_loadoutmanager.h"
#include "ge_tokenmanager.h"

#include "ge_utils.h"
#include "script_parser.h"
#include "filesystem.h"

#include "ge_playerspawn.h"
#include "ge_mapmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ge_mapchooser_avoidteamplay("ge_mapchooser_avoidteamplay", "0", FCVAR_GAMEDLL, "If set to 1, server will avoid choosing maps where the current playercount is above the team threshold.");
ConVar ge_mapchooser_mapbuffercount("ge_mapchooser_mapbuffercount", "5", FCVAR_GAMEDLL, "How many other maps need to be played since the last time a map was played before it can be selected randomly again.");
ConVar ge_mapchooser_usemapcycle("ge_mapchooser_usemapcycle", "0", FCVAR_GAMEDLL, "If set to 1, server will just use the mapcycle file and not choose randomly based on map script files.");
ConVar ge_mapchooser_resthreshold("ge_mapchooser_resthreshold", "14", FCVAR_GAMEDLL, "The mapchooser will do everything it can to avoid switching between maps with this combined resintensity.");


CGEMapManager::CGEMapManager(void)
{
	m_pCurrentSelectionData = NULL;
}

CGEMapManager::~CGEMapManager(void)
{
	m_pSelectionData.PurgeAndDeleteElements();
	m_pLoadoutBlacklist.PurgeAndDeleteElements();
	m_pMapGamemodes.PurgeAndDeleteElements();
	m_pMapTeamGamemodes.PurgeAndDeleteElements();

	m_pMapGamemodeWeights.RemoveAll();
	m_pMapTeamGamemodeWeights.RemoveAll();
}

// Based off of the parser in loadoutmanager.  Thanks KM!
void CGEMapManager::ParseMapSelectionData(void)
{
	const char *baseDir = "scripts/maps/";
	char filePath[256] = "";

	FileFindHandle_t finder;
	const char *filename = filesystem->FindFirstEx("scripts/maps/*.txt", "MOD", &finder);

	while (filename)
	{
		char mapname[32];
		Q_StrLeft(filename, -4, mapname, 32); // Get rid of the file extension.
		Q_strlower(mapname);

		// First see if we already have this.
		for (int i = 0; i < m_pSelectionData.Count(); i++)
		{
			if (!strcmp(m_pSelectionData[i]->mapname, mapname))
			{
				delete m_pSelectionData[i];
				m_pSelectionData.Remove(i);  // Get rid of it so we can update it.
				break;  // We're just looking for one thing so we don't need to shift everything over and keep scanning like we otherwise would.
			}
		}
		
		MapSelectionData *pMapSelectionData = new MapSelectionData;

		Q_strncpy(pMapSelectionData->mapname, mapname, 32); // We already have our map name.
		
		// Set the defaults for everything else
		
		int *valueptrs[5] = { &pMapSelectionData->baseweight, &pMapSelectionData->minplayers, &pMapSelectionData->maxplayers, &pMapSelectionData->teamthreshold, &pMapSelectionData->resintensity };
		char *keyvalues[5] = { "BaseWeight", "MinPlayers", "MaxPlayers", "TeamThreshold", "ResIntensity" };
		bool hasvalue[5] = { false, false, false, false, false };
		int valuedefaults[5] = { 500, 0, 16, 12, 4 };


		for (int v = 0; v < 5; v++)
		{
			*valueptrs[v] = valuedefaults[v]; // Set our default values incase the file is missing one.
		}
		
		// Load the file
		Q_strncpy(filePath, baseDir, 256);
		Q_strncat(filePath, filename, 256);
		char *contents = (char*)UTIL_LoadFileForMe(filePath, NULL);
		
		if (contents)
		{
			CUtlVector<char*> lines;
			CUtlVector<char*> data;
			Q_SplitString(contents, "\n", lines);

			bool skippingblock = false;

			for (int i = 0; i < lines.Count(); i++)
			{
				// Ignore comments
				if (!Q_strncmp(lines[i], "//", 2))
					continue;

				if (!skippingblock && Q_strstr(lines[i], "{")) // We've found an opening bracket, so we no longer care about anything until we hit the closing bracket.
					skippingblock = true;

				// If we're skipping over a bracketed section just ignore everything until the end.
				if (skippingblock)
				{
					if (!Q_strstr(lines[i], "}")) // We don't actually care about the datablocks right now, so just look for the terminator.
						continue; 
					else
						skippingblock = false;
				}

				// Otherwise try parsing the line: [field]\s+[data]
				if (Q_ExtractData(lines[i], data))
				{	
					for (int v = 0; v < 5; v++)
					{
						if (!Q_strcmp(data[0] , keyvalues[v]))
						{
							if (data.Count() > 1)
								*valueptrs[v] = atoi(data[1]);

							hasvalue[v] = true;

							if (hasvalue[0] && hasvalue[1] && hasvalue[2] && hasvalue[3] && hasvalue[4]) //If we got all of our values why are we still here?
								break;
						}
					}
				}

				// Clear the data set (do not purge!)
				data.RemoveAll();
			}

			m_pSelectionData.AddToTail(pMapSelectionData);

			if (!Q_strcmp("default", mapname)) // If this is our default map data, keep a record of it.
				m_pDefaultSelectionData = pMapSelectionData;

			// NOTE: We do not purge the data!
			ClearStringVector(lines);
		}

		delete[] contents;

		// Get the next one
		filename = filesystem->FindNext(finder);
	}
	
	filesystem->FindClose(finder);
}

void CGEMapManager::ParseLogData()
{
	char *contents = (char*)UTIL_LoadFileForMe("gamesetuprecord.txt", NULL);

	if (!contents)
	{
		Msg("No rotation log!\n");
		return;
	}

	CUtlVector<char*> lines;
	char linebuffer[64];
	Q_SplitString(contents, "\n", lines);

	bool readingmaps = false;

	for (int i = 0; i < lines.Count(); i++)
	{
		// Ignore comments
		if (!Q_strncmp(lines[i], "//", 2))
			continue;

		if (readingmaps)
		{
			if (!Q_strncmp(lines[i], "-", 1)) // Our symbol for the end of a block.
				break;

			Q_StrLeft(lines[i], -1, linebuffer, 64); // Take off the newline character.

			MapSelectionData *mapData = GetMapSelectionData(linebuffer);

			if (mapData)
				m_pRecentMaps.AddToTail(mapData);
		}

		if (!Q_strncmp(lines[i], "Maps:", 5))
			readingmaps = true;
	}

	// NOTE: We do not purge the data!
	ClearStringVector(lines);
	delete[] contents;
}

void CGEMapManager::ParseMapData(const char *mapname)
{
	if (!mapname) // We don't have a map yet somehow.
		return;

	m_pCurrentSelectionData = NULL; // Zero this out so we don't keep the pointer to the old map data.

	// We already parsed the selection data so just grab that now.
	for (int i = 0; i < m_pSelectionData.Count(); i++)
	{
		if (!strcmp(m_pSelectionData[i]->mapname, mapname))
		{
			m_pCurrentSelectionData = m_pSelectionData[i];
		}
	}

	if (!m_pCurrentSelectionData)
		m_pCurrentSelectionData = m_pDefaultSelectionData;

	// Find our map file.
	char filename[64];
	Q_snprintf(filename, 64, "scripts/maps/%s.txt", mapname);
	V_FixSlashes(filename);

	Msg("Parsing %s for map data\n", filename);

	char *contents = (char*)UTIL_LoadFileForMe(filename, NULL);

	if (!contents)
	{
		Warning("Failed to find map data scipt file!\n");
		return;
	}

	CUtlVector<char*> lines;
	CUtlVector<char*> data;
	Q_SplitString(contents, "\n", lines);

	enum ReadMode
	{
		RD_NONE = 0,
		RD_BLACKLIST,
		RD_FFAGAMEPLAY,
		RD_TEAMGAMEPLAY,
	};

	int Reading = RD_NONE;

	for (int i = 0; i < lines.Count(); i++)
	{
		// Ignore comments
		if (!Q_strncmp(lines[i], "//", 2))
			continue;

		if (Reading == RD_NONE)
		{
			if (!Q_strncmp(lines[i], "WeaponsetWeights", 12))
				Reading = RD_BLACKLIST;

			if (!Q_strncmp(lines[i], "GamemodeWeights", 15))
				Reading = RD_FFAGAMEPLAY;

			if (!Q_strncmp(lines[i], "TeamGamemodeWeights", 19))
				Reading = RD_TEAMGAMEPLAY;

			continue; // The most that should be after the identifier is the opening bracket.
		}

		if (!Q_strncmp(lines[i], "{", 1)) // Ignore the opening bracket if it's all by itself.  It doesn't actually do anything.
			continue;

		if (!Q_strncmp(lines[i], "}", 1)) // We hit the end of the block, right at the start of the line.
		{
			Reading = RD_NONE;
			continue;
		}

		if (Q_ExtractData(lines[i], data))
		{
			// Parse the line according to the data block we're in.
			if (Reading == RD_BLACKLIST)
			{
				if (data.Count() > 1)
				{
					int previndex = -1;

					for (int i = 0; i < m_pLoadoutBlacklist.Count(); i++)
					{
						if (!Q_strcmp(m_pLoadoutBlacklist[i], data[0]))
							previndex = i;
					}

					if (previndex != -1) // If we already have it, just replace the weight.
					{
						m_pLoadoutWeightOverrides[previndex] = atoi(data[1]);
					}
					else // Otherwise add it to the end.
					{
						m_pLoadoutBlacklist.AddToTail(data[0]);
						m_pLoadoutWeightOverrides.AddToTail(atoi(data[1]));
					}
				}
				else
					Warning("Weaponset Override of %s specified without weight, ignoring!\n", data[0]);
			}
			else if (Reading == RD_FFAGAMEPLAY)
			{
				if (data.Count() > 1)
				{
					int previndex = -1;

					for (int i = 0; i < m_pMapGamemodes.Count(); i++)
					{
						if (!Q_strcmp(m_pMapGamemodes[i], data[0]))
							previndex = i;
					}

					if (previndex != -1)
					{
						DevMsg("Matched %s, overwriting weight with %d\n", data[0], atoi(data[1]));
						m_pMapGamemodeWeights[previndex] = atoi(data[1]);
					}
					else
					{
						m_pMapGamemodes.AddToTail(data[0]);
						m_pMapGamemodeWeights.AddToTail(atoi(data[1]));
					}
				}
				else
					Warning("Gamemode %s specified without weight, ignoring!\n", data[0]);
			}
			else if (Reading == RD_TEAMGAMEPLAY)
			{
				if (data.Count() > 1)
				{
					int previndex = -1;

					for (int i = 0; i < m_pMapTeamGamemodes.Count(); i++)
					{
						if (!Q_strcmp(m_pMapTeamGamemodes[i], data[0]))
							previndex = i;
					}

					if (previndex != -1)
					{
						m_pMapTeamGamemodeWeights[previndex] = atoi(data[1]);
					}
					else
					{
						m_pMapTeamGamemodes.AddToTail(data[0]);
						m_pMapTeamGamemodeWeights.AddToTail(atoi(data[1]));
					}
				}
				else
					Warning("Team Gamemode %s specified without weight, ignoring!\n", data[0]);
			}
		}

		if (Q_strstr(lines[i], "}")) // We hit the end of the block, somewhere on this line.
			Reading = RD_NONE;

		// Clear the data set (do not purge!)
		data.RemoveAll();
	}

	// NOTE: We do not purge the data!
	ClearStringVector(lines);
	delete[] contents;
}

void CGEMapManager::ParseCurrentMapData(void)
{
	m_pLoadoutBlacklist.PurgeAndDeleteElements(); // Wipe all the lists so we can get a fresh one.
	m_pMapGamemodes.PurgeAndDeleteElements();
	m_pMapTeamGamemodes.PurgeAndDeleteElements();
	m_pMapGamemodeWeights.RemoveAll();
	m_pMapTeamGamemodeWeights.RemoveAll();

	ParseMapData("default"); // Parse this first so we can get all the default data, and then overwrite anything redundant with the current map's data.
	ParseMapData(gpGlobals->mapname.ToCStr());

	// Also parse our log data so we can add our current map to the rotation history.
	ParseLogData();

	// If it doesn't have a file it's not eligible for random rotation, no point in logging it.
	if (m_pCurrentSelectionData != m_pDefaultSelectionData)
		m_pRecentMaps.AddToHead(m_pCurrentSelectionData);
}

MapSelectionData* CGEMapManager::GetMapSelectionData(const char *mapname)
{
	for (int i = 0; i < m_pSelectionData.Count(); i++)
	{
		if (!strcmp(m_pSelectionData[i]->mapname, mapname))
			return m_pSelectionData[i];
	}

	return NULL; // We didn't find it, so they get nothing!
}

void CGEMapManager::GetMapGameplayList(CUtlVector<char*> &gameplays, CUtlVector<int> &weights, bool teamplay)
{
	gameplays.RemoveAll();
	weights.RemoveAll();

	if (teamplay)
	{
		if (m_pMapTeamGamemodes.Count())
		{
			for (int i = 0; i < m_pMapTeamGamemodes.Count(); i++)
			{
				if (GEGameplay()->IsValidGamePlay(m_pMapTeamGamemodes[i])) // Can't do this check when parsing because everything hasn't initialized yet.
				{
					gameplays.AddToTail(m_pMapTeamGamemodes[i]);
					weights.AddToTail(m_pMapTeamGamemodeWeights[i]);
				}
				else
					Warning("Tried to proccess invalid team gamemode from script file, %s\n", m_pMapTeamGamemodes[i]);
			}
		}
		else
			Warning("Map has no team gamemodes listed in script file!\n");
	}
	else
	{
		if (m_pMapGamemodes.Count())
		{
			for (int i = 0; i < m_pMapGamemodes.Count(); i++)
			{
				if (GEGameplay()->IsValidGamePlay(m_pMapGamemodes[i])) // Can't do this check when parsing because everything hasn't initialized yet.
				{
					gameplays.AddToTail(m_pMapGamemodes[i]);
					weights.AddToTail(m_pMapGamemodeWeights[i]);
				}
				else
					Warning("Tried to proccess invalid gamemode from script file, %s\n", m_pMapGamemodes[i]);
			}
		}
		else
			Warning("Map has no gamemodes listed in script file!\n");
	}
}

void CGEMapManager::GetSetBlacklist( CUtlVector<char*> &sets, CUtlVector<int> &weights )
{
	sets.RemoveAll();
	weights.RemoveAll();

	if (m_pLoadoutBlacklist.Count())
	{
		sets.AddVectorToTail(m_pLoadoutBlacklist); // Don't need to check validity because a listing is only relevant when it actually matches a real set.
		weights.AddVectorToTail(m_pLoadoutWeightOverrides);
	}
}

void CGEMapManager::GetRecentMaps(CUtlVector<const char*> &mapnames)
{
	mapnames.RemoveAll();

	int buffercount = min(ge_mapchooser_mapbuffercount.GetInt(), m_pRecentMaps.Count());

	for (int i = 0; i < buffercount; i++)
	{
		mapnames.AddToTail(m_pRecentMaps[i]->mapname);
	}
}

void CGEMapManager::GetViableMapList(int iNumPlayers, CUtlVector<char*> &mapnames, CUtlVector<int> &mapweights)
{
	if (!m_pSelectionData.Count()) // We don't actually have any maps to choose from.
		return;

	int currentResIntensity = 3;
	mapnames.RemoveAll();
	mapweights.RemoveAll();
	CUtlVector<const char*> recentmaps; // Vector of recently played maps that we would rather not play again.
	CUtlVector<int> mapintensities; // Vector of the resintensity scores of each map.
	int	currentmapindex = -1; // Index of the current map in the mapnames vector for easy removal later.
	int goodintensitycount = 0; // For keeping track of how many maps we have that won't crash the game on switch.
	int intensitythresh = ge_mapchooser_resthreshold.GetInt();
	GetRecentMaps(recentmaps);

	if (m_pCurrentSelectionData)
		currentResIntensity = m_pCurrentSelectionData->resintensity;

	DevMsg("---Choosing Map for playercount %d---\n", iNumPlayers);

	for (int i = 0; i < m_pSelectionData.Count(); i++)
	{
		int lowerbound = m_pSelectionData[i]->minplayers - 1; // Have to add and subtract one here so the range is inclusive.
		int upperbound = m_pSelectionData[i]->maxplayers + 1; // and also smoothly tapers off in weight with our formula.

		if (ge_mapchooser_avoidteamplay.GetBool()) // If we want to avoid teamplay our playercount has to come in below the teamthresh.
			upperbound = min(m_pSelectionData[i]->teamthreshold, m_pSelectionData[i]->maxplayers + 1);

		DevMsg("Looking at %s, with high %d, low %d\n", m_pSelectionData[i]->mapname, upperbound, lowerbound);
		if (lowerbound < iNumPlayers && upperbound > iNumPlayers && engine->IsMapValid(m_pSelectionData[i]->mapname)) //It's within range, calculate weight adjustment.
		{
			mapnames.AddToTail(m_pSelectionData[i]->mapname);

			int mapweight = m_pSelectionData[i]->baseweight;
			float playerradius = (upperbound - lowerbound) * 0.5;
			float centercount = (upperbound + lowerbound) * 0.5;
			float weightscale = 1 - abs(centercount - (float)iNumPlayers) / playerradius; // The closer we are to the center of our range the higher our weight is.
			int finalweight = round(mapweight * weightscale);

			if (m_pSelectionData[i] == m_pCurrentSelectionData)
				currentmapindex = mapnames.Count() - 1; // Record the current map index so we can remove it later if we have any other viable maps.

			DevMsg("Added %s with weight %d\n", m_pSelectionData[i]->mapname, finalweight);

			mapweights.AddToTail(finalweight);
			mapintensities.AddToTail(m_pSelectionData[i]->resintensity);

			if ( m_pSelectionData[i]->resintensity + currentResIntensity < intensitythresh && m_pSelectionData[i] != m_pCurrentSelectionData )
				goodintensitycount++;
		}
	}

	if (currentmapindex != -1 && mapnames.Count() > 1) // Our current map is in the list and we have another map to use.
	{
		DevMsg("Removing current map, which is %s\n", mapnames[currentmapindex]);
		mapnames.Remove(currentmapindex);
		mapweights.Remove(currentmapindex);
		mapintensities.Remove(currentmapindex);
	}

	// If resintensity is 15 then either one of these maps practically crashes on its own.
	// If there's a single map we can switch that doesn't have a resintensity of >15 then strip out any that do.
	// resintensity 15 is -really- high, and the amount of maps that will add up to be 15 is very low.  This mostly just
	// prevents caverns from getting switched to from any other remotely intensive map.

	// One day we'll upgrade engine versions or figure out a way to ameloriate this bizzare memory constraint, but for now
	// we're forced into this.

	if ( goodintensitycount > 0 )
	{
		for (int i = mapnames.Count() - 1; i >= 0; i--)
		{
			if ( mapintensities[i] + currentResIntensity >= intensitythresh )
			{
				DevWarning("Removing %s, because it has total resintensity %d\n", mapnames[i], mapintensities[i] + currentResIntensity);
				mapnames.Remove(i);
				mapweights.Remove(i);
				mapintensities.Remove(i);
			}

			DevMsg("Switching to %s will %s transistion, because it has total resintensity %d\n", mapnames[i], mapintensities[i] + currentResIntensity >= 10 ? "":" **not** ", mapintensities[i] + currentResIntensity);
		}
	}

	// Disqualify maps that we've played recently, but be sure to leave some for our mapchooser.
	if ( recentmaps.Count() > 0 )
	{
		int validmapcount = mapnames.Count();
		int recentmapcount = min(recentmaps.Count(), validmapcount - 2);

		for (int r = 0; r < recentmapcount; r++)
		{
			for (int i = 0; i < validmapcount; i++)
			{
				if (!Q_strcmp(mapnames[i], recentmaps[r]))
					mapweights[i] *= 0;
			}
		}
	}

	DevMsg("---Map list finished with %d maps total---\n", mapnames.Count());

	return;
}

const char* CGEMapManager::SelectNewMap( void )
{
	if (!m_pSelectionData.Count() || ge_mapchooser_usemapcycle.GetBool()) // We don't actually have any maps to choose from.
		return NULL;

	CUtlVector<char*> mapnames;
	CUtlVector<int> mapweights;
	int iNumPlayers = 0; // Get all the players currently in the game, spectators might decide they want to play the next map so we should count them too.

	FOR_EACH_MPPLAYER(pPlayer)
		if (pPlayer->IsConnected())
			iNumPlayers++;
	END_OF_PLAYER_LOOP()

	GetViableMapList( iNumPlayers, mapnames, mapweights );

	return GERandomWeighted<char*>(mapnames.Base(), mapweights.Base(), mapnames.Count());
}

void CGEMapManager::PrintMapSelectionData(void)
{
	for (int i = 0; i < m_pSelectionData.Count(); i++)
	{
		Msg("Mapname: %s\n", m_pSelectionData[i]->mapname);
		Msg("\tWeight: %d\n", m_pSelectionData[i]->baseweight);
		Msg("\tResIntensity: %d\n", m_pSelectionData[i]->resintensity);
		Msg("\tMinPlayers: %d\n", m_pSelectionData[i]->minplayers);
		Msg("\tMaxPlayers: %d\n", m_pSelectionData[i]->maxplayers);
		Msg("\tTeamThresh: %d\n", m_pSelectionData[i]->teamthreshold);
	}
}

void CGEMapManager::PrintMapDataLists(void)
{
	Msg("Mapname: %s\n", m_pCurrentSelectionData->mapname);
	Msg("Weight: %d\n", m_pCurrentSelectionData->baseweight);
	Msg("ResIntensity: %d\n", m_pCurrentSelectionData->resintensity);
	Msg("MinPlayers: %d\n", m_pCurrentSelectionData->minplayers);
	Msg("MaxPlayers: %d\n", m_pCurrentSelectionData->maxplayers);
	Msg("TeamThresh: %d\n", m_pCurrentSelectionData->teamthreshold);

	Msg("\nLoadout Weight Overrides\n");

	for (int i = 0; i < m_pLoadoutBlacklist.Count(); i++)
	{
		Msg("\t%s\t%d\n", m_pLoadoutBlacklist[i], m_pLoadoutWeightOverrides[i]);
	}

	Msg("\nGamemode Weights\n");

	for (int i = 0; i < m_pMapGamemodes.Count(); i++)
	{
		Msg("\t%s\t%d\n", m_pMapGamemodes[i], m_pMapGamemodeWeights[i]);
	}

	Msg("\nTeam Gamemode Weights\n");

	for (int i = 0; i < m_pMapTeamGamemodes.Count(); i++)
	{
		Msg("\t%s\t%d\n", m_pMapTeamGamemodes[i], m_pMapTeamGamemodeWeights[i]);
	}
}

static int mapWeightSort(MapWeightData* const *a, MapWeightData* const *b)
{
	if ((*a)->weight >(*b)->weight)
		return -1;
	else if ((*a)->weight == (*b)->weight)
		return 0;
	else return 1;
}

// Prints the map selection weights for all the viable maps for the given playercount.
void CGEMapManager::PrintMapSelectionWeights(int pcount, bool sorted)
{
	CUtlVector<char*> mapnames;
	CUtlVector<int> mapweights;
	CUtlVector<int> sortedmapweights;

	GetViableMapList( pcount, mapnames, mapweights );

	if (sorted)
	{
		CUtlVector<MapWeightData*> mapweightsgroup;

		for (int i = 0; i < mapnames.Count(); i++)
		{
			MapWeightData *pWeightNameGroup = new MapWeightData;

			pWeightNameGroup->weight = mapweights[i];
			Q_strncpy(pWeightNameGroup->mapname, mapnames[i], 32);

			mapweightsgroup.AddToTail(pWeightNameGroup);
		}

		mapweightsgroup.Sort(mapWeightSort);

		Msg("Map:Weight\n");
		for (int i = 0; i < mapnames.Count(); i++)
		{
			Msg("%s:%d\n", mapweightsgroup[i]->mapname, mapweightsgroup[i]->weight);
		}

		// Delete all those group objects because we do not need them now.
		mapweightsgroup.PurgeAndDeleteElements();
	}
	else
	{
		Msg("Map:Weight\n");
		for (int i = 0; i < mapnames.Count(); i++)
		{
			Msg("%s:%d\n", mapnames[i], mapweights[i]);
		}
	}
}

CON_COMMAND(ge_print_map_selection_data, "Prints the server's map selection data ")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
	{
		Msg("You must be a server admin to use this command\n");
		return;
	}

	if (GEMPRules())
		GEMPRules()->GetMapManager()->PrintMapSelectionData();
}

CON_COMMAND(ge_print_current_map_data, "Prints the current map's data ")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
	{
		Msg("You must be a server admin to use this command\n");
		return;
	}

	if (GEMPRules())
		GEMPRules()->GetMapManager()->PrintMapDataLists();
}

CON_COMMAND(ge_print_map_selection_weights, "Prints the map selection chance for given playercount, use 1 as second parameter to sort.")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
	{
		Msg("You must be a server admin to use this command\n");
		return;
	}

	if (!GEMPRules())
		return;

	int iNumPlayers = 0;
	bool bPrintSorted = false;

	if ( args.ArgC() < 2 )
	{
		FOR_EACH_MPPLAYER(pPlayer)
			if (pPlayer->IsConnected())
				iNumPlayers++;
		END_OF_PLAYER_LOOP()
	}
	else
		iNumPlayers = atoi(args[1]);

	if (args.ArgC() > 2 && atoi(args[2]) > 0)
		bPrintSorted = true;


	GEMPRules()->GetMapManager()->PrintMapSelectionWeights(iNumPlayers, bPrintSorted);
}