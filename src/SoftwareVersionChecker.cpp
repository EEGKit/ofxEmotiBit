#include "SoftwareVersionChecker.h"


void SoftwareVersionChecker::checkLatestVersion()
{
	bool isNetworkAvailable = false;
	// -n specifies number of tries. we are going to try to detect internet connection by sending 1 packet
	// -w specifies timeout period in mS. We are going to wait for a maximum of 3 seconds for a ping response.
	// the IP 8.8.8.8 is google's public DNS. We are using ping with a specific IP as that removes undefined wait periods associated with DNS resolution. More details: https://developers.google.com/speed/public-dns
	std::string command = "ping -n 1 -w 3000 8.8.8.8";
	std::string pingResponse = "";
	char buffer[200];
	ofLogNotice() << "Trying to detect internet access";
	try
	{
#if defined (TARGET_OSX) || defined (TARGET_LINUX)
		FILE* pipe = popen(command.c_str(), "r"); // returns NULL on fail. refer: https://man7.org/linux/man-pages/man3/popen.3.html#RETURN_VALUE
#else
		FILE* pipe = _popen(command.c_str(), "r"); // returns NULL on fail. refer: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen?view=msvc-170#return-value
#endif		
		if (pipe != NULL)
		{
			while (fgets(buffer, sizeof buffer, pipe) != NULL)
			{
				pingResponse += buffer;
			}
#if defined (TARGET_OSX) || defined (TARGET_LINUX)
			pclose(pipe);
#else
			_pclose(pipe);
#endif
		}
		else
		{
			ofLogNotice() << "Failed to open pipe. ping failed";
	}
}
	catch (...)
	{
		ofLogError() << "Ping failed";
	}
	ofLogNotice() << pingResponse.c_str();

	std::string testString =
		"PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.\n"
		"64 bytes from 8.8.8.8: icmp_seq = 1 ttl = 54 time = 18.2 ms\n"
		"64 bytes from 8.8.8.8 : icmp_seq = 2 ttl = 54 time = 25.7 ms\n"
		"64 bytes from 8.8.8.8 : icmp_seq = 3 ttl = 54 time = 14.5 ms\n"
		"-- - 8.8.8.8 ping statistics-- -\n"
		"3 packets transmitted, 3 received, 0 % packet loss, time 2003ms\n"
		"rtt min / avg / max / mdev = 14.503 / 19.471 / 25.685 / 4.649 ms";

	/*testString =
		"Pinging 8.8.8.8 with 32 bytes of data:\n"
		"Reply from 8.8.8.8: bytes = 32 time = 20ms TTL = 54\n"
		"Reply from 8.8.8.8 : bytes = 32 time = 17ms TTL = 54\n"
		"Reply from 8.8.8.8 : bytes = 32 time = 15ms TTL = 54\n"
		"Ping statistics for 8.8.8.8 :\n"
		"Packets : Sent = 3, Received = 3, Lost = 0 (0 % loss),\n"
		"Approximate round trip times in milli - seconds :\n"
		"Minimum = 15ms, Maximum = 20ms, Average = 17ms";*/

	isNetworkAvailable = SoftwareVersionChecker::testPingresponse(testString);
	//isNetworkAvailable = testPingResponse(pingResponse);
	if (isNetworkAvailable)
	{
		bool newVersionAvailable = false;
		// system call to curl
		std::string latestReleaseUrl = "https://github.com/EmotiBit/ofxEmotiBit/releases/latest";
		std::string latestReleaseApiRequest = "https://api.github.com/repos/EmotiBit/ofxEmotiBit/releases/latest";
		// Calling curl with a 3 second timeout. Timeout tested working with curl version v8.0.1. Older versions of curl have shown a "disregard" for the timeout.
		std::string command = "curl --connect-timeout 3 " + latestReleaseApiRequest;
		std::string response = "";
		char buffer[200];
		bool exceptionOccured = false;
		try
		{
#if defined (TARGET_OSX) || defined (TARGET_LINUX)
			FILE* pipe = popen(command.c_str(), "r"); // returns NULL on fail. refer: https://man7.org/linux/man-pages/man3/popen.3.html#RETURN_VALUE
#else
			FILE* pipe = _popen(command.c_str(), "r"); // returns NULL on fail. refer: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen?view=msvc-170#return-value
#endif
			if (pipe != NULL)
			{
				try
				{
					while (fgets(buffer, sizeof buffer, pipe) != NULL)
					{
						response += buffer;
					}
				}
				catch (...) {
					try
					{
#if defined (TARGET_OSX) || defined (TARGET_LINUX)
						pclose(pipe);
#else
						_pclose(pipe);
#endif
					}
					catch (...)
					{
						ofLogError("Failed to close pipe");
					}
					ofLog(OF_LOG_ERROR, "An exception occured while executing curl");
					exceptionOccured = true;
				}
				try
				{
#if defined (TARGET_OSX) || defined (TARGET_LINUX)
					pclose(pipe);
#else
					_pclose(pipe);
#endif
				}
				catch (...)
				{
					ofLogError("Failed to close pipe");
				}
			}
			else
			{
				ofLog(OF_LOG_ERROR, "Failed to check for latest version. Failed to open pipe");
				exceptionOccured = true;
			}
		}
		catch (...)
		{
			ofLogError("Failed to open pipe");
			exceptionOccured = true;
		}
		try
		{
			if (!exceptionOccured & response != "")
			{
				ofxJSONElement jsonResponse;
				if (jsonResponse.parse(response))
				{
					//ofLog(OF_LOG_NOTICE, jsonResponse.getRawString(true));  // uncomment to print curl output
					std::string latestAvailableVersion = jsonResponse["tag_name"].asString();
					ofLogNotice("Latest version") << latestAvailableVersion;
					int swVerPrefixLoc = latestAvailableVersion.find(SOFTWARE_VERSION_PREFIX);
					if (swVerPrefixLoc != std::string::npos)
					{
						latestAvailableVersion.erase(swVerPrefixLoc, 1);  // remove leading version char "v"
					}
					// compare with ofxEmotiBitVersion
					std::vector<std::string> latestVersionSplit = ofSplitString(latestAvailableVersion, ".");
					std::vector<std::string> currentVersionSplit = ofSplitString(ofxEmotiBitVersion, ".");
					int versionLength = latestVersionSplit.size() < currentVersionSplit.size() ? latestVersionSplit.size() : currentVersionSplit.size();
					if (versionLength)
					{
						for (int i = 0; i < versionLength; i++)
						{
							int latest = ofToInt(latestVersionSplit.at(i));
							int current = ofToInt(currentVersionSplit.at(i));
							if (latest == current)
							{
								// need to check minor version
								continue;
							}
							else if (latest < current)
							{
								// current version higher than latest available
								break;
							}
							else // latest > current
							{
								// new version available
								newVersionAvailable = true;
								break;
							}
						}
					}
					else
					{
						ofLogError("Failed to parse version string");
					}
				}
				else
				{
					ofLog(OF_LOG_ERROR, "unexpected curl response");
				}
				// If newer version available, display alert message
				if (newVersionAvailable)
				{
					// create alert dialog box
					ofSystemAlertDialog("A new version of EmotiBit Software is available!");
					// open browser to latest version
					try
					{
#ifdef TARGET_WIN32
						std::string command = "start " + latestReleaseUrl;
						system(command.c_str());
#else
						std::string command = "open " + latestReleaseUrl;
						system(command.c_str());
#endif
					}
					catch (...)
					{
						ofLogError("Failed to open browser");
					}
				}
	}
		}
		catch (...)
		{
			ofLogError("An exception occured while checking latest version of EmotiBit software");
		}
	}
	else
	{
		ofLogNotice() << "Internet access not detected. Skipping version check.";
	}
}

bool SoftwareVersionChecker::testPingresponse(std::string pingResponse)
{
	// Parse ping response

	// LINUX/MACOS
	/* A successful ping response
	-----------------------------
	PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
	64 bytes from 8.8.8.8: icmp_seq=1 ttl=54 time=18.2 ms
	64 bytes from 8.8.8.8: icmp_seq=2 ttl=54 time=25.7 ms
	64 bytes from 8.8.8.8: icmp_seq=3 ttl=54 time=14.5 ms
	--- 8.8.8.8 ping statistics ---
	3 packets transmitted, 3 received, 0% packet loss, time 2003ms
	rtt min/avg/max/mdev = 14.503/19.471/25.685/4.649 ms
	*/

	// WINDOWS
	// A successful ping response contains a sumary of round trip times. We are using that do determine if ping was successful
	/* A successful ping response
	------------------------------
	Pinging 8.8.8.8 with 32 bytes of data:
	Reply from 8.8.8.8: bytes = 32 time = 18ms TTL = 119
		Ping statistics for 8.8.8.8 :
		Packets : Sent = 1, Received = 1, Lost = 0 (0 % loss),
		Approximate round trip times in milli - seconds :
		Minimum = 18ms, Maximum = 18ms, Average = 18ms
	*/
	bool isNetworkAvailable = false;
	std::string searchKey("Ping statistics");
#if not defined (TARGET_OSX) || defined (TARGET_LINUX)
	searchKey = "ping statistics";  // define search key for linux/macOS
#endif
	int keyLoc = pingResponse.find(searchKey);
	const char statDelimiter = '\n';

	// extract statistsics from ping response
	int statStartLoc = pingResponse.find(statDelimiter, keyLoc) + 1;
	int statEndLoc = pingResponse.find(statDelimiter, statStartLoc) + 1;
	std::string stats = pingResponse.substr(statStartLoc, (statEndLoc - statStartLoc));
	const char commaDelimiter = ',';
	int commaLoc = stats.find(commaDelimiter);
	const char equalDelimiter = '=';  // used only in Windows parsing
	struct ParsedStats {
		int packetsSent = 0;
		int packetsRec = 0;
		bool gotSentStats = false;
	}parsedStats;

	

	while (commaLoc != string::npos)
	{
#if not defined (TARGET_OSX) || defined (TARGET_LINUX)
		std::string numExtract = stats.substr(0, stats.find("packets"));
		if (!parsedStats.gotSentStats)
		{
			parsedStats.packetsSent = atoi(numExtract.c_str());
			parsedStats.gotSentStats = true; // extract packetsRec next

		}
		else
		{
			parsedStats.packetsRec = atoi(numExtract.c_str());
			break;
		}
#else
		int equalLoc = stats.find(equalDelimiter);
		std::string numExtract = stats.substr(equalLoc + 1, commaLoc);
		if (parsedStats.gotSentStats == 0)
		{
			parsedStats.packetsSent = atoi(numExtract.c_str());
			parsedStats.gotSentStats = true; // extract packetsRec next

		}
		else
		{
			parsedStats.packetsRec = atoi(numExtract.c_str());
			parsedStats.gotSentStats = true; // extract packetsRec next
			break;
		}
#endif
		stats.erase(stats.begin(), stats.begin() + commaLoc + 1);
		commaLoc = stats.find(commaDelimiter);
	}

	if (parsedStats.packetsSent > 0 && (parsedStats.packetsRec == parsedStats.packetsSent))
	{
		isNetworkAvailable = true;
	}
	return isNetworkAvailable;
}