///////////// Copyright © 2017, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_blacklist.cpp
//
// Logic for blacklisting servers
//
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_webkeys.h"
#include "ge_webrequest.h"
#include "vgui_controls/Frame.h"
#include "clientmode_shared.h"
#include "ge_popup.h"
#include "ge_utils.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Returns 0 if valid IP with port, -1 if completely invalid IP, otherwise returns amount of characters until port number.
int IsInvalidIpAddress(const char *address)
{
	int numCounter = 0;
	int bracketCount = 0;
	int i = 0; // Keeps track of offset, in case we need to report the location where the port number should go.

	while (address[i] != '\0')
	{
		if ( address[i] >= '0' && address[i] <= '9' )
		{
			if (numCounter >= 3) // Can't have more than 3 numbers in a row!
				return -1;

			numCounter++;
		}
		else if ( address[i] == '.' )
		{
			if (numCounter < 1) // IPv4 addresses don't have two decimal points in a row.
				return -1;

			numCounter = 0;
			bracketCount++;
		}
		else if ( address[i] == ':' )
		{
			if ( bracketCount == 3 && numCounter > 0 ) // We have 3 closed brackets and at least 1 number in the fourth one.
			{
				if ( address[i + 1] >= '0' && address[i + 1] <= '9' ) // Has valid port number!
					return 0;
				else
					return i; // Ends on colon or has nonsense port address.  Going to get set to default.
			}
			else
				return -1; // If we got to the port number without that, then it's an invalid IP.
		}
		else
			return -1; // IPv4 Addresses don't contain anything but numbers and decimal points.

		i++;
	}

	if ( bracketCount == 3 && numCounter > 0 ) // If we hit the null terminator here it means we have a valid IP with no port.
		return i;
	else
		return -1; // Otherwise it just means the address is incomplete.
}

#define MAX_BLACKLIST_REASON_SIZE 2048

char lastVisitedServerAddress[32];
char lastVisitedServerBlacklistReason[MAX_BLACKLIST_REASON_SIZE];
bool lastVisitedServerGood;

void DisplayBlacklistPopup()
{
	if (GetGEPopupBox() && (CGEPopupBox*)GetGEPopupBox()->GetPanel())
		((CGEPopupBox*)GetGEPopupBox()->GetPanel())->displayPopup("#GEUI_ServerBlacklisted", lastVisitedServerBlacklistReason);
}

void OnServerInvestigated( const char *result, const char *error, const char *internalData )
{
	if (!error || error[0] == '\0')
	{
		char responseBuffer[1024 + MAX_BLACKLIST_REASON_SIZE];
		char statusBuffer[MAX_BLACKLIST_REASON_SIZE + 64];
		char errorBuffer[512];

		Warning("got investigation result of %s\n", result);
		ExtractXMLTagSubstring(responseBuffer, sizeof(responseBuffer), result, "response");

		if (responseBuffer[0] != '\0') // We got a response!
		{
			ExtractXMLTagSubstring(statusBuffer, sizeof(statusBuffer), responseBuffer, "getServerStatus");

			Warning("statusBuffer = %s\n", statusBuffer);

			if (statusBuffer[0] == '\0')
			{
				ExtractXMLTagSubstring(errorBuffer, sizeof(errorBuffer), responseBuffer, "error");
				Warning("Failed to get server status with error %s\n", errorBuffer);
				return;
			}
		}
		else
		{
			Warning("Response did not have valid format!\n");
			return; // Don't write anything so we keep trying.
		}

		Q_strcpy(lastVisitedServerAddress, internalData);

		char statusString[64];
		
		// serverStatus=blacklisted&amp;statusReason=asdf

		const char *statusStart = Q_strstr(statusBuffer, "serverStatus=") + Q_strlen("serverStatus=");
		const char *statusEnd = Q_strstr(statusBuffer, "&amp;");
		const char *reasonStart = Q_strstr(statusBuffer, "statusReason=");

		// Make sure we actually found the reason tag before directing the pointer to a potentially out of bounds address.
		if (reasonStart)
			reasonStart += Q_strlen("statusReason=");

		if (!statusStart)
		{
			Warning("Failed to extract server status!\n");
			return;
		}

		if (!statusEnd && reasonStart)
		{
			Warning("Failed to identify end of status string!\n");
			return;
		}

		int statusLength;

		// If there's no ending marker, and no reason marker, the entire string is the status reason.
		if ( !statusEnd && !reasonStart )
		{
			statusLength = Q_strlen(statusStart) + 1;
		}
		else
			statusLength = (statusEnd + 1) - statusStart; // Otherwise be sure to extract only the part relevant to the status message.


		Q_strncpy(statusString, statusStart, min(statusLength, sizeof(statusString)));

		Warning("Status string = %s\n", statusString);
		Warning("Reason string = %s!\n", reasonStart);

		if (!Q_strcmp(statusString, "blacklisted"))
		{
			const char *reason = reasonStart;
			char firstDigit = '\0';

			int i;
			for (i = 0; i < MAX_BLACKLIST_REASON_SIZE - 1; i++)
			{
				if (*reason == '\0')
					break;

				if (*reason != '%')
				{
					lastVisitedServerBlacklistReason[i] = *reason;
				}
				else
				{
					reason++; // Skip % character since we've already gotten all the useful info out of it.
					firstDigit = *reason;
					reason++; // Move on to second character now that we've recorded the first one.

					// Combine both characters into one hex value, represented as a string.
					char buffer[8];
					Q_snprintf(buffer, sizeof(buffer), "%c%c", firstDigit, *reason);
					
					// Convert that string into a new, single character and record it to our reason buffer.
					lastVisitedServerBlacklistReason[i] = (char)strtol(buffer, NULL, 16);
				}

				reason++;
			}
			lastVisitedServerBlacklistReason[i] = '\0';


			Warning( "Server at address %s is blacklisted!\n%s\n", internalData, lastVisitedServerBlacklistReason );

			engine->ClientCmd_Unrestricted("disconnect");

			DisplayBlacklistPopup();

			lastVisitedServerGood = false;
		}
		else
		{
			lastVisitedServerGood = true;
			Warning("All clear!  Got result of %s when investigating %s\n", result, internalData);
		}
	}
	else
		Warning("Got error of %s when checking server status for %s.\n", error, internalData);
}

bool InvestigateGEServer(const char *address, bool abortOnInvalidIP)
{
	static bool needsInit = true;

	if ( needsInit )
	{
		Q_strcpy(lastVisitedServerAddress, ""); // Didn't visit any servers yet.
		Q_strcpy(lastVisitedServerBlacklistReason, "This Server is Blacklisted!\n");
		lastVisitedServerGood = true; // Innocent until proven guilty.  Haha just kidding the init value of this doesn't even matter.
		needsInit = false;
	}

	if ( address && Q_strcmp(address, "localhost:27015") && Q_strcmp(address, "loopback") ) // Never check localhost.
	{
		Warning("Got address of %s\n", address);

		int addressState = IsInvalidIpAddress(address);
		char adjustedAddress[32];

		if ( abortOnInvalidIP )
		{
			if (addressState < 0)
			{
				Warning("%s is an invalid address!\n", address);
				return true;
			}

			if (addressState > 0)
			{
				Warning("%s is an invalid address, attempting fix!\n", address);

				Q_strncpy(adjustedAddress, address, min(sizeof(adjustedAddress), addressState + 1));
				Q_strcat(adjustedAddress, ":27105", sizeof(adjustedAddress));

				address = adjustedAddress;

				Warning("Set address to %s!\n", address);
			}
		}

		if ( !Q_strcmp(address, lastVisitedServerAddress) ) // We remember this address!
		{
			if ( lastVisitedServerGood ) // It's perfectly fine.
				return true;
			else // It's a bad address!
			{
				DisplayBlacklistPopup(); // Display the ban reason again.
				return false; // Don't even bother trying to connect now.
			}
		}

		char webRequestURL[512];
		Q_snprintf( webRequestURL, sizeof(webRequestURL), "%s?getServerStatus&ip=", GetWebDomain() );

		new CGETempWebRequest( webRequestURL, OnServerInvestigated, address, address );
	}

	return true;
}