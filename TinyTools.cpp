/*
 * @file TinyTools.h
 * @author Richard e Collins
 * @version 0.1
 * @date 2021-03-15


   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include <getopt.h>
#include <assert.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <poll.h>

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <condition_variable>

#include "TinyTools.h"

/**
 * @brief Adds line and source file. There is a c++20 way now that is better. I need to look at that.
 */
#define TINYTOOLS_THROW(THE_MESSAGE__)	{throw std::runtime_error("At: " + std::to_string(__LINE__) + " In " + std::string(__FILE__) + " : " + std::string(THE_MESSAGE__));}


namespace tinytools{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace math
{
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace string{

bool CompareNoCase(const char* pA,const char* pB,size_t pLength)
{
    assert( pA != nullptr || pB != nullptr );// Note only goes pop if both are null.
// If either or both NULL, then say no. A bit like a divide by zero as null strings are not strings.
    if( pA == nullptr || pB == nullptr )
        return false;

// If same memory then yes they match, doh!
    if( pA == pB )
        return true;

    if( pLength == 0 )
        pLength = strlen(pB);

    while( (*pA != 0 || *pB != 0) && pLength > 0 )
    {
        // Get here are one of the strings has hit a null then not the same.
        // The while loop condition would not allow us to get here if both are null.
        if( *pA == 0 || *pB == 0 )
        {// Check my assertion above that should not get here if both are null. Note only goes pop if both are null.
            assert( pA != NULL || pB != NULL );
            return false;
        }

        if( tolower(*pA) != tolower(*pB) )
            return false;

        pA++;
        pB++;
        pLength--;
    };

    // Get here, they are the same.
    return true;
}


char* CopyString(const char* pString, size_t pMaxLength)
{
    char *newString = nullptr;
    if( pString != nullptr && pString[0] != 0 )
    {
        size_t len = strnlen(pString,pMaxLength);
        if( len > 0 )
        {
            newString = new char[len + 1];
            strncpy(newString,pString,len);
            newString[len] = 0;
        }
    }

    return newString;
}

StringVec SplitString(const std::string& pString, const char* pSeperator)
{
    StringVec res;
    for (size_t p = 0, q = 0; p != pString.npos; p = q)
	{
		const std::string part(pString.substr(p + (p != 0), (q = pString.find(pSeperator, p + 1)) - p - (p != 0)));
		if( part.size() > 0 )
		{
	        res.push_back(part);
		}
	}
    return res;
}

std::string ReplaceString(const std::string& pString,const std::string& pSearch,const std::string& pReplace)
{
    std::string Result = pString;
    // Make sure we can't get stuck in an infinite loop if replace includes the search string or both are the same.
    if( pSearch != pReplace && pSearch.find(pReplace) != pSearch.npos )
    {
        for( std::size_t found = Result.find(pSearch) ; found != Result.npos ; found = Result.find(pSearch) )
        {
            Result.erase(found,pSearch.length());
            Result.insert(found,pReplace);
        }
    }
    return Result;
}

std::string TrimWhiteSpace(const std::string &s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && isspace(*it))
        it++;

    std::string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && isspace(*rit))
        rit++;

    return std::string(it, rit.base());
}

};//namespace string{
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace timers{
MillisecondTicker::MillisecondTicker(int pMilliseconds)
{
	SetTimeout(pMilliseconds);
}


void MillisecondTicker::SetTimeout(int pMilliseconds)
{
	assert(pMilliseconds > 0 );
	mTimeout = std::chrono::milliseconds(pMilliseconds);
	mTrigger = std::chrono::system_clock::now() + mTimeout;
}

bool MillisecondTicker::Tick(const std::chrono::system_clock::time_point pNow)
{
	if( mTrigger < pNow )
	{
		mTrigger += mTimeout;
		return true;
	}
	return false;
}

void MillisecondTicker::Tick(const std::chrono::system_clock::time_point pNow,std::function<void()> pCallback )
{
	assert( pCallback != nullptr );
	if( mTrigger < pNow )
	{
		mTrigger += mTimeout;
		pCallback();
	}
}

};//namespace timers{
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace network{

std::string GetLocalIP()
{
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    sockaddr_in loopback;
 
	std::string ip = "Could not socket";
    if( sock > 0 )
	{
		std::memset(&loopback, 0, sizeof(loopback));
		loopback.sin_family = AF_INET;
		loopback.sin_addr.s_addr = 1;   // can be any IP address. Odd, but works. :/
		loopback.sin_port = htons(9);   // using debug port

		if( connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == 0 )
		{
			socklen_t addrlen = sizeof(loopback);
			if( getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) == 0 )
			{
				char buf[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0)
				{
					ip = "Could not inet_ntop";
				}
				else
				{
					ip = buf;
				}
			}
			else
			{
				ip = "Could not getsockname";
			}
		}
		else
		{
			ip = "Could not connect";
		}

		// All paths that happen after opening sock will come past here. Less chance of accidentally leaving it open.
		close(sock);
	}

	return ip;
}

std::string GetHostName()
{
    char buf[256];
    ::gethostname(buf,256);
	return std::string(buf);
}

std::string GetNameFromIPv4(const std::string& pAddress)
{
    sockaddr_in deviceIP;
	memset(&deviceIP, 0, sizeof deviceIP);

	const int ret = inet_pton(AF_INET,pAddress.c_str(),&deviceIP.sin_addr);
	 if (ret <= 0)
	 {
		if (ret == 0)
			return "Not in presentation format";
		return "inet_pton failed";
	}

	deviceIP.sin_family = AF_INET;

	char hbuf[NI_MAXHOST];
	hbuf[0] = 0;


	getnameinfo((struct sockaddr*)&deviceIP,sizeof(deviceIP),hbuf, sizeof(hbuf), NULL,0, NI_NAMEREQD);
	const std::string name = hbuf;
	return name;
}

std::string GetNameFromIPv4(const uint32_t pAddress)
{
    sockaddr_in deviceIP;
	memset(&deviceIP, 0, sizeof deviceIP);

	deviceIP.sin_addr.s_addr = pAddress;
	deviceIP.sin_family = AF_INET;

	char hbuf[NI_MAXHOST];
	hbuf[0] = 0;

	getnameinfo((struct sockaddr*)&deviceIP,sizeof(deviceIP),hbuf, sizeof(hbuf), NULL,0, NI_NAMEREQD);
	const std::string name = hbuf;
	return name;
}

uint32_t GetIPv4FromName(const std::string& pHostName)
{
	struct addrinfo hints;
    struct addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;    // only IPv4 please
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	uint32_t IPv4 = 0;

	if( getaddrinfo(pHostName.c_str(),NULL,&hints,&result) == 0 )
	{
		for( struct addrinfo *rp = result; rp != NULL && IPv4 == 0 ; rp = rp->ai_next)
		{
			if( rp->ai_addr->sa_family == AF_INET )
			{
				IPv4 = ((const sockaddr_in*)rp->ai_addr)->sin_addr.s_addr;
			}
		}

		// Clean up
		freeaddrinfo(result);
	}

	return IPv4;
}

void ScanNetworkIPv4(uint32_t pFromIPRange,uint32_t pToIPRange,std::function<bool(const uint32_t pIPv4,const char* pHostName)> pDeviceFound)
{
	// Because IP values are in big endian format I need disassemble to iterate over them.
	const int aFrom = (pFromIPRange&0x000000ff)>>0;
	const int bFrom = (pFromIPRange&0x0000ff00)>>8;
	const int cFrom = (pFromIPRange&0x00ff0000)>>16;
	const int dFrom = std::clamp(int((pFromIPRange&0xff000000)>>24),1,254);

	const int aTo = (pToIPRange&0x000000ff)>>0;
	const int bTo = (pToIPRange&0x0000ff00)>>8;
	const int cTo = (pToIPRange&0x00ff0000)>>16;
	const int dTo = std::clamp(int((pToIPRange&0xff000000)>>24),1,254);

	std::cout << "From " << aFrom << "." << bFrom << "." << cFrom << "." << dFrom << "\n";
	std::cout << "To " << aTo << "." << bTo << "." << cTo << "." << dTo << "\n";

	for( int a = aFrom ; a <= aTo ; a++ )
	{
		for( int b = bFrom ; b <= bTo ; b++ )
		{
			for( int c = cFrom ; c <= cTo ; c++ )
			{
				for( int d = dFrom ; d <= dTo ; d++ )
				{
					sockaddr_in deviceIP;
					memset(&deviceIP, 0, sizeof deviceIP);

					deviceIP.sin_addr.s_addr = MakeIP4V(a,b,c,d);
					deviceIP.sin_family = AF_INET;

					char hbuf[NI_MAXHOST];
					hbuf[0] = 0;

					if( getnameinfo((struct sockaddr*)&deviceIP,sizeof(deviceIP),hbuf, sizeof(hbuf), NULL,0, NI_NAMEREQD) == 0 )
					{
						if( pDeviceFound(deviceIP.sin_addr.s_addr,hbuf) == false )
						{
							return;// User asked to end.
						}
					}
				}
			}
		}
	}
}

bool IsPortOpen(uint32_t pIPv4,uint16_t pPort)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd < 0)
	{
		TINYTOOLS_THROW("ERROR opening socket");
		return false;
	}

    sockaddr_in deviceAddress;
	memset(&deviceAddress, 0, sizeof(deviceAddress));

	deviceAddress.sin_addr.s_addr = pIPv4;
	deviceAddress.sin_port = htons(pPort);
	deviceAddress.sin_family = AF_INET;

	const bool isOpen = connect(sockfd,(struct sockaddr *) &deviceAddress,sizeof(deviceAddress)) == 0;
	close(sockfd);
	return isOpen;
}

size_t Encode7Bit(const uint8_t* p8Bit,size_t p8BitSize,uint8_t** r7Bit)
{
    size_t newSize = ((p8BitSize+1) * 8) / 7;
    assert(newSize > 1);

    uint8_t* out = new uint8_t[newSize];
    *r7Bit = out;

    for( ; p8BitSize > 6 ; p8BitSize -=7 , out += 8, p8Bit += 7 )
    {
        assert( out + 7 < (*r7Bit) + newSize );
        out[0] = (                        (p8Bit[0]>>1) );
        out[1] = ( ((p8Bit[0]&0x01)<<6) | (p8Bit[1]>>2) );
        out[2] = ( ((p8Bit[1]&0x03)<<5) | (p8Bit[2]>>3) );
        out[3] = ( ((p8Bit[2]&0x07)<<4) | (p8Bit[3]>>4) );
        out[4] = ( ((p8Bit[3]&0x0F)<<3) | (p8Bit[4]>>5) );
        out[5] = ( ((p8Bit[4]&0x1F)<<2) | (p8Bit[5]>>6) );
        out[6] = ( ((p8Bit[5]&0x3F)<<1) | (p8Bit[6]>>7) );
        out[7] = ( ((p8Bit[6]&0x7F)<<0) );        
    }

    // And now for the trailing bytes.
    assert( p8BitSize < 7 );
    if( p8BitSize > 0 )
    {
        uint8_t padding[6] = {0,0,0,0,0,0};
        for( size_t n = 0 ; n < p8BitSize ; n++ )
        {
            padding[n] = p8Bit[n];
        }

        *out = (                          (padding[0]>>1) ); out++;
        *out = ( ((padding[0]&0x01)<<6) | (padding[1]>>2) ); out++;
        if( p8BitSize > 1 )
        {
            *out = ( ((p8Bit[1]&0x03)<<5) | (p8Bit[2]>>3) ); out++;
            if( p8BitSize > 2 )
            {
                *out = ( ((p8Bit[2]&0x07)<<4) | (p8Bit[3]>>4) ); out++;
                if( p8BitSize > 3 )
                {
                    *out = ( ((p8Bit[3]&0x0F)<<3) | (p8Bit[4]>>5) ); out++;
                    if( p8BitSize > 4 )
                    {
                        *out = ( ((p8Bit[4]&0x1F)<<2) | (p8Bit[5]>>6) ); out++;
                        if( p8BitSize > 5 )
                        {
                            *out = ((p8Bit[5]&0x3F)<<1); out++;
                        }
                    }
                }
            }
        }
    }
    assert( out <= (*r7Bit) + newSize );

    return out - *r7Bit;
}

size_t Decode7Bit(const uint8_t* p7Bit,size_t p7BitSize,uint8_t** r8Bit)
{
    size_t newSize = (p7BitSize+1) * 7 / 8;
    assert(newSize > 1);

    uint8_t* out = new uint8_t[newSize];
    *r8Bit = out;

    for( ; p7BitSize > 7 ; p7BitSize -= 8, p7Bit += 8, out += 7 )
    {
        assert( out + 6 < (*r8Bit) + newSize );
        out[0] = ( p7Bit[0] << 1 | (p7Bit[1]>>6));
        out[1] = ( p7Bit[1] << 2 | (p7Bit[2]>>5));
        out[2] = ( p7Bit[2] << 3 | (p7Bit[3]>>4));
        out[3] = ( p7Bit[3] << 4 | (p7Bit[4]>>3));
        out[4] = ( p7Bit[4] << 5 | (p7Bit[5]>>2));
        out[5] = ( p7Bit[5] << 6 | (p7Bit[6]>>1));
        out[6] = ( p7Bit[6] << 7 | (p7Bit[7]>>0));
    }

    if( p7BitSize > 0 )
    {
        *out = ( p7Bit[0] << 1 | (p7Bit[1]>>6)); out++;
        if( p7BitSize > 2 )
        {
            *out = ( p7Bit[1] << 2 | (p7Bit[2]>>5)); out++;
            if( p7BitSize > 3 )
            {
                *out = ( p7Bit[2] << 3 | (p7Bit[3]>>4)); out++;
                if( p7BitSize > 4 )
                {
                    *out = ( p7Bit[3] << 4 | (p7Bit[4]>>3)); out++;
                    if( p7BitSize > 5 )
                    {
                        *out = ( p7Bit[4] << 5 | (p7Bit[5]>>2)); out++;
                        if( p7BitSize > 6 )
                        {
                            *out = ( p7Bit[5] << 6 | (p7Bit[6]>>1)); out++;
                            if( p7BitSize > 7 )
                            {
                                *out = ( p7Bit[6] << 7 | (p7Bit[7]>>0)); out++;
                            }
                        }
                    }
                }
            }
        }
    }
    assert( out <= (*r8Bit) + newSize );

    return out - *r8Bit;
}

};// namespace network

///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace system{

std::string GetLocalDateTime()
{
	auto t = std::time(nullptr);
	char mbstr[100];
	std::strftime(mbstr,sizeof(mbstr), "%d-%m-%Y %H-%M-%S",std::localtime(&t));
    return mbstr;
}

/**
 * @brief Fetches the system uptime in a more human readable format
 */
bool GetUptime(uint64_t& rUpDays,uint64_t& rUpHours,uint64_t& rUpMinutes)
{
    rUpDays = 0;
    rUpHours = 0;
    rUpMinutes = 0;

    std::ifstream upTimeFile("/proc/uptime");
    if( upTimeFile.is_open() )
    {
        rUpDays = 999;
        char buf[256];
        buf[255] = 0;
        if( upTimeFile.getline(buf,255,' ') )
        {
            const uint64_t secondsInADay = 60 * 60 * 24;
            const uint64_t secondsInAnHour = 60 * 60;
            const uint64_t secondsInAMinute = 60;

            const uint64_t totalSeconds = std::stoull(buf);
            rUpDays = totalSeconds / secondsInADay;
            rUpHours = (totalSeconds - (rUpDays*secondsInADay)) / secondsInAnHour;
            rUpMinutes = (totalSeconds - (rUpDays*secondsInADay) - (rUpHours * secondsInAnHour) ) / secondsInAMinute;
            return true;
        }
    }
    return false;
}

std::string GetUptime()
{
	uint64_t upDays,upHours,upMinutes;
	GetUptime(upDays,upHours,upMinutes);
	std::string upTime;
	std::string space;
	if( upDays > 0 )
	{
		if( upDays == 1 )
		{
			upTime = "1 Day";
		}
		else
		{
			upTime = std::to_string(upDays) + " Days";
		}
		space = " ";
	}

	if( upHours > 0 )
	{
		if( upHours == 1 )
		{
			upTime += space + "1 Hour";
		}
		else
		{
			upTime += space + std::to_string(upHours) + " Hours";
		}
		space = " ";
	}

	if( upMinutes == 1 )
	{
		upTime += "1 Minute";
	}
	else
	{
		upTime += space + std::to_string(upMinutes) + " Minutes";
	}

	return upTime;
}

bool GetCPULoad(std::map<int,CPULoadTracking>& pTrackingData,int& rTotalSystemLoad,std::map<int,int>& rCoreLoads)
{
	// If pTrackingData is empty then we're initalising the state so lets build our starting point.
	const bool initalising = pTrackingData.size() ==  0;

	std::ifstream statFile("/proc/stat");
	if( statFile.is_open() )
	{
		// Written so that it'll read the lines in any order. Easy to do and safe.
		while( statFile.eof() == false )
		{
			std::string what;
			statFile >> what;
			if( string::CompareNoCase(what,"cpu") )
			{
				// The the rest of the bits we want.
				std::string usertimeSTR, nicetimeSTR, systemtimeSTR, idletimeSTR, ioWaitSTR, irqSTR, softIrqSTR, stealSTR, guestSTR, guestniceSTR;
				statFile >> usertimeSTR >>
								 nicetimeSTR >>
								 systemtimeSTR >>
								 idletimeSTR >>
								 ioWaitSTR >>
								 irqSTR >>
								 softIrqSTR >>
								 stealSTR >>
								 guestSTR >>
								 guestniceSTR;
				// Read rest of line.
				std::string eol;
				std::getline(statFile,eol,'\n');

				const uint64_t userTime = std::stoull(usertimeSTR);	
				const uint64_t niceTime = std::stoull(nicetimeSTR);
				const uint64_t idleTime = std::stoull(idletimeSTR);
				const uint64_t stealTime = std::stoull(stealSTR);
				
				const uint64_t systemAllTime = std::stoull(systemtimeSTR) + std::stoull(irqSTR) + std::stoull(softIrqSTR);
//				const uint64_t virtualTime = std::stoull(guestSTR) + std::stoull(guestniceSTR); // According to a comment in hot, guest and guest nice are already incorerated in user time.
				const uint64_t totalTime = userTime + niceTime + systemAllTime + idleTime + stealTime; // No need to add virtualTime, I want that to be part of user time. If not will not see load on CPU from virtual guest.

				int cpuID = -1; // This is the total system load.
				if( what.size() > 3 )
				{
					sscanf(what.data(),"cpu%d",&cpuID);
					if( cpuID < 0 || cpuID > 128 )
					{
						TINYTOOLS_THROW("Error in GetCPULoad, cpu Id read from /proc/stat has a massive index, not going ot happen mate... cpuID == " + std::to_string(cpuID));
					}
				}

				if( initalising )
				{
					if( pTrackingData.find(cpuID) != pTrackingData.end() )
					{
						TINYTOOLS_THROW("Error in GetCPULoad, trying to initalising and found a duplicate CPU id, something went wrong. cpuID == " + std::to_string(cpuID));
					}

					CPULoadTracking info;
					info.mUserTime		= userTime;
					info.mTotalTime		= totalTime;
					pTrackingData.emplace(cpuID,info);
				}
				else
				{
					// Workout the deltas to get cpu load in percentage.
					auto core = pTrackingData.find(cpuID);
					if( core == pTrackingData.end() )
					{
						TINYTOOLS_THROW("Error in GetCPULoad, trying to workout load but found a new CPU id, something went wrong. cpuID == " + std::to_string(cpuID));
					}

					// Copied from htop!
					// Since we do a subtraction (usertime - guest) and cputime64_to_clock_t()
					// used in /proc/stat rounds down numbers, it can lead to a case where the
					// integer overflow.
					#define WRAP_SUBTRACT(a,b) (a > b) ? a - b : 0					
					const uint64_t deltaUser = WRAP_SUBTRACT(userTime,core->second.mUserTime);
					const uint64_t deltaTotal = WRAP_SUBTRACT(totalTime,core->second.mTotalTime);
					#undef WRAP_SUBTRACT
					core->second.mUserTime = userTime;
					core->second.mTotalTime = totalTime;

					if( deltaTotal > 0 )
					{
						const uint64_t percentage = deltaUser * 100 / deltaTotal;
						if( cpuID == -1 )
						{
							rTotalSystemLoad = (int)percentage;
						}
						else
						{
							rCoreLoads[cpuID] = (int)percentage;
						}
					}
					else
					{
						if( cpuID == -1 )
						{
							rTotalSystemLoad = 0;
						}
						else
						{
							rCoreLoads[cpuID] = 0;
						}
					}
				}
			}
		}
		return true;
	}
	return false;
}

bool GetMemoryUsage(size_t& rMemoryUsedKB,size_t& rMemAvailableKB,size_t& rMemTotalKB,size_t& rSwapUsedKB)
{
	std::ifstream memFile("/proc/meminfo");
	if( memFile.is_open() )
	{
		// We'll build a map of the data lines that have key: value combination.
		std::map<std::string,size_t> keyValues;
		while( memFile.eof() == false )
		{
			std::string memLine;
			std::getline(memFile,memLine);
			if( memLine.size() > 0 )
			{
				std::vector<std::string> memParts = string::SplitString(memLine," ");// Have to do like this as maybe two or three parts. key: value [kb]
				if( memParts.size() != 2 && memParts.size() != 3 )
				{
					TINYTOOLS_THROW("Failed to correctly read a line from /proc/meminfo did the format change? Found line \"" + memLine + "\" and split it into " + std::to_string(memParts.size()) + " parts");
				}

				if( memParts[0].back() == ':' )
				{
					memParts[0].pop_back();
					keyValues[memParts[0]] = std::stoull(memParts[1]);
				}
			}
		}

		try
		{
			const size_t Buffers = keyValues.at("Buffers");
			const size_t Cached = keyValues["Cached"];
			const size_t SReclaimable = keyValues["SReclaimable"];
			const size_t MemAvailable = keyValues["MemAvailable"];
			const size_t MemTotal = keyValues["MemTotal"];
			const size_t MemFree = keyValues["MemFree"];
			const size_t SwapFree = keyValues["SwapFree"];
			const size_t SwapTotal = keyValues["SwapTotal"];

			const size_t mainCached = Cached + SReclaimable;
			// if kb_main_available is greater than kb_main_total or our calculation of
			// mem_used overflows, that's symptomatic of running within a lxc container
			// where such values will be dramatically distorted over those of the host.
			if (MemAvailable > MemTotal)
			{
				rMemAvailableKB = MemFree;
			}
			else
			{
				rMemAvailableKB = MemAvailable;
			}

			rMemoryUsedKB = MemTotal - MemFree - mainCached - Buffers;
			if (rMemoryUsedKB < 0)
			{
				rMemoryUsedKB = MemTotal - MemFree;
			}

			rMemTotalKB = MemTotal;
			rSwapUsedKB = SwapTotal - SwapFree;

			return true;
		}
		catch( std::out_of_range& e ){std::cerr << " failed to read all the data needed from /proc/meminfo " << e.what();}// We'll return false so they know something went wrong.
	}
	return false;
}

static char* CopyArg(const std::string& pString)
{
    char *newString = nullptr;
    const size_t len = pString.length();
    if( len > 0 )
    {
        newString = new char[len + 1];
        for( size_t n = 0 ; n < len ; n++ )
            newString[n] = pString.at(n);
        newString[len] = 0;
    }

    return newString;
}

static void BuildArgArray(char** pTheArgs, const std::vector<std::string>& pArgs)
{    
    for (const std::string& Arg : pArgs)
    {
        char* str = CopyArg(Arg);
        if(str)
        {
            //Trim leading white space.
            while(isspace(str[0]) && str[0])
                str++;

            if(str[0])
            {
                *pTheArgs = str;
                pTheArgs++;
            }
        }
    }
    *pTheArgs = nullptr;
}

bool ExecuteShellCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv, std::string& rOutput)
{
    const bool VERBOSE = false;
    if (pCommand.size() == 0 )
    {
        std::cerr << "ExecuteShellCommand Command name for was zero length! No command given!\n";
        return false;
    }

    int pipeSTDOUT[2];
    int result = pipe(pipeSTDOUT);
    if (result < 0)
    {
        perror("pipe");
        exit(-1);
    }

    int pipeSTDERR[2];
    result = pipe(pipeSTDERR);
    if (result < 0)
    {
        perror("pipe");
        exit(-1);
    }

    /* print error message if fork() fails */
    pid_t pid = fork();
    if (pid < 0)
    {
        std::cout << "ExecuteShellCommand Fork failed" << std::endl;
        return false;
    }

    /* fork() == 0 for child process */
    if (pid == 0)
    {
        dup2(pipeSTDOUT[1], STDOUT_FILENO ); /* Duplicate writing end to stdout */
        close(pipeSTDOUT[0]);
        close(pipeSTDOUT[1]);

        dup2(pipeSTDERR[1], STDERR_FILENO ); /* Duplicate writing end to stdout */
        close(pipeSTDERR[0]);
        close(pipeSTDERR[1]);

        ExecuteCommand(pCommand,pArgs,pEnv);
    }

    /*
     * parent process
     */

    /* Parent process */
    close(pipeSTDOUT[1]); /* Close writing end of pipes, don't need them */
    close(pipeSTDERR[1]); /* Close writing end of pipes, don't need them */

    size_t BufSize = 1000;
    char buf[BufSize+1];
    buf[BufSize] = 0;

    struct pollfd Pipes[] =
    {
        {pipeSTDOUT[0],POLLIN,0},
        {pipeSTDERR[0],POLLIN,0},
    };

    int NumPipesOk = 2;
    std::stringstream outputStream;
    do
    {
        int ret = poll(Pipes,2,1000);
        if( ret < 0 )
        {
            rOutput = "Error, pipes failed. Can't capture process output";
            NumPipesOk = 0;
        }
        else if(ret  > 0 )
        {
            for(int n = 0 ; n < 2 ; n++ )
            {
                if( (Pipes[n].revents&POLLIN) != 0 )
                {
                    ssize_t num = read(Pipes[n].fd,buf,BufSize);
                    if( num > 0 )
                    {
                        buf[num] = 0;
                        outputStream << buf;
                    }
                }
                else if( (Pipes[n].revents&(POLLERR|POLLHUP|POLLNVAL)) != 0 && Pipes[n].events != 0 )
                {
                    Pipes[n].fd = -1;
                    NumPipesOk--;
                }
            }
        }
    }while(NumPipesOk>0);
    rOutput = outputStream.str();

    int status;
    bool Worked = false;
    if( wait(&status) == -1 )
    {
        std::cout << "Failed to wait for child process." << std::endl;
    }
    else
    {
        if(WIFEXITED(status) && WEXITSTATUS(status) != 0)//did the child terminate normally?
        {
            if( VERBOSE )
                std::cout << (long)pid << " exited with return code " << WEXITSTATUS(status) << std::endl;
        }
        else if (WIFSIGNALED(status))// was the child terminated by a signal?
        {
            if( VERBOSE )
                std::cout << (long)pid << " terminated because it didn't catch signal number " << WTERMSIG(status) << std::endl;
        }
        else
        {// Get here, then all is ok.
            Worked = true;
        }
    }

    return Worked;
}

void ExecuteCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv)
{
    // +1 for the NULL and +1 for the file name as per convention, see https://linux.die.net/man/3/execlp.
    char** TheArgs = new char*[pArgs.size() + 2];
    TheArgs[0] = CopyArg(pCommand);
    BuildArgArray(TheArgs+1,pArgs);

    // Build the environment variables.
    for( const auto& var : pEnv )
    {
        setenv(var.first.c_str(),var.second.c_str(),1);
    }

    // This replaces the current process so no need to clean up the memory leaks before here. ;)
    execvp(TheArgs[0], TheArgs);

    const char* errorString = strerror(errno);

    std::cerr << "ExecuteCommand execvp() failure!\n" << "    Error: " << errorString << "\n    This print is after execvp() and should not have been executed if execvp were successful!\n";

    // Should never get here!
    throw std::runtime_error("Command execution failed! Should not have returned! " + pCommand + " Error: " + errorString);
    // Really make sure the process is gone...
    _exit(1);
}

};//namespace system{
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace threading{

void SleepableThread::Tick(int pPauseInterval,std::function<void()> pTheWork)
{
	if( pTheWork == nullptr )
	{
		TINYTOOLS_THROW("SleepableThread passed nullpoint for the work to do...");
	}

	mWorkerThread = std::thread([this,pPauseInterval,pTheWork]()
	{
		while(mKeepGoing)
		{
			pTheWork();
			std::unique_lock<std::mutex> lk(mSleeperMutex);
			mSleeper.wait_for(lk,std::chrono::seconds(pPauseInterval));
		};
	});
}

void SleepableThread::TellThreadToExitAndWait()
{
	mKeepGoing = false;
	mSleeper.notify_one();
	if( mWorkerThread.joinable() )
	{
		mWorkerThread.join();
	}
}

LocklessRingBuffer::LocklessRingBuffer(size_t pItemSizeof,size_t pItemCount)
{
    mBuffer = new uint8_t[pItemSizeof * pItemCount];
    mItemSizeof = pItemSizeof;
    mItemCount = pItemCount;
    mCurrentReadPos = 0;
    mNextWritePos = 0;    
}

LocklessRingBuffer::~LocklessRingBuffer()
{
    delete []mBuffer;
}

bool LocklessRingBuffer::ReadNext(void* rItem,size_t pBufferSize)
{
    void *item = NULL;
    volatile int NextPos = 0;

    // If we have caught up with the write index then we're blocked and return NULL.
    // Current read pos is allowed to become the same as the write pos.
    if( mCurrentReadPos == mNextWritePos )
        return false;

    // Get the items address
    item = mBuffer + (mCurrentReadPos * mItemSizeof);

    // Copy before advancing the read buffer so it can't get over written.
    const size_t numBytesToCopy = pBufferSize < mItemSizeof ? pBufferSize : mItemSizeof;
    memcpy(rItem,item,numBytesToCopy);

    // Advance to where we want to read from next time.
    // Also we don't write new index till we have calculated it. Makes the write a single instruction and so atomic.
    NextPos = (mCurrentReadPos+1)%mItemCount;

    // Write with one instuction.
    mCurrentReadPos = NextPos;
    
    return true;
}

bool LocklessRingBuffer::WriteNext(const void* pItem,size_t pBufferSize)
{
    // Notice how the logic for testing for overwrite is a 'little' different to the one above.
    // This is important. It deals with then there is no data in the buffer and we need to write.
    // The write pos is NOT allowed to become the same as the read pos.
    // Also we don't write result till we are done. Makes the write a single instruction and so atomic.
    volatile int NextPos = (mNextWritePos+1)%mItemCount;

    // If we have caught up with the read index then we're blocked, return false. Data not written
    if( NextPos == mCurrentReadPos )
        return false;

    const size_t numBytesToCopy = pBufferSize < mItemSizeof ? pBufferSize : mItemSizeof;
    memcpy(mBuffer + (mNextWritePos * mItemSizeof),
            pItem,
            numBytesToCopy);

    // Move to the place we'll next write too.
    mNextWritePos = NextPos;

    return true;// Written ok.
}

};//namespace threading{
///////////////////////////////////////////////////////////////////////////////////////////////////////////
CommandLineOptions::CommandLineOptions(const std::string& pUsageHelp):mUsageHelp(pUsageHelp)
{// Always add this, everyone does. BE rude not to. ;)
	AddArgument('h',"help","Display this help and exit");
}

void CommandLineOptions::AddArgument(char pArg,const std::string& pLongArg,const std::string& pHelp,int pArgumentOption,std::function<void(const std::string& pOptionalArgument)> pCallback)
{
	if( mArguments.find(pArg) != mArguments.end() )
	{
		TINYTOOLS_THROW(std::string("class CommandLineOptions: Argument ") + pArg + " has already been registered, can not contine");
	}
	
	mArguments.emplace(pArg,Argument(pLongArg,pHelp,pArgumentOption,pCallback));
}

bool CommandLineOptions::Process(int argc, char *argv[])
{
	std::vector<struct option> longOptions;
	std::string shortOptions;

	// Build the data for the getopt_long function that will do all the work for us.
	for( auto& opt : mArguments)
	{
		shortOptions += opt.first;
		if( opt.second.mArgumentOption == required_argument )
		{
			shortOptions += ":";
		}
		// Bit of messing about because mixing c code with c++
		struct option newOpt = {opt.second.mLongArgument.c_str(),opt.first,nullptr,opt.first};
		longOptions.emplace_back(newOpt);
	}
	struct option emptyOpt = {NULL, 0, NULL, 0};
	longOptions.emplace_back(emptyOpt);

	int c,oi;
	while( (c = getopt_long(argc,argv,shortOptions.c_str(),longOptions.data(),&oi)) != -1 )
	{
		auto arg = mArguments.find(c);
		if( arg == mArguments.end() )
		{// Unknow option, print help and bail.
			std::cout << "Unknown option \'" << c << "\' found.\n";
			PrintHelp();
			return false;
		}
		else
		{
			arg->second.mIsSet = true;
			std::string optionalArgument;
			if( optarg )
			{// optarg is defined in getopt_code.h 
				optionalArgument = optarg;
			}

			if( arg->second.mCallback != nullptr )
			{
				arg->second.mCallback(optionalArgument);
			}
		}
	};

	// See if help was asked for.
	if( IsSet('h') )
	{
		PrintHelp();
		return false;
	}

	return true;
}

bool CommandLineOptions::IsSet(char pShortOption)const
{
	return mArguments.at(pShortOption).mIsSet;
}

bool CommandLineOptions::IsSet(const std::string& pLongOption)const
{
	for( auto& opt : mArguments)
	{
		if( string::CompareNoCase(opt.second.mLongArgument,pLongOption) )
		{
			return opt.second.mIsSet;
		}
	}

	return false;
}

void CommandLineOptions::PrintHelp()const
{
	std::cout << mUsageHelp << "\n";

	std::vector<char> shortArgs;
	std::vector<std::string> longArgs;
	std::vector<std::string> descriptions;

	for( auto& opt : mArguments)
	{
		shortArgs.push_back(opt.first);
		descriptions.push_back(opt.second.mHelp);
		if( opt.second.mArgumentOption == required_argument )
		{
			longArgs.push_back("--" + opt.second.mLongArgument + "=arg");
		}
		else if( opt.second.mArgumentOption == optional_argument )
		{
			longArgs.push_back("--" + opt.second.mLongArgument + "[=arg]");
		}
		else
		{
			longArgs.push_back("--" + opt.second.mLongArgument);
		}
	}

	// Now do a load of formatting of the output.
	size_t DescMaxSpace = 0;
	for(auto lg : longArgs)
	{
		size_t l = 5 + lg.size(); // 5 == 2 spaces + -X + 1 for space for short arg.
		if( DescMaxSpace < l )
			DescMaxSpace = l;
	}

	DescMaxSpace += 4; // Add 4 spaces for formatting.
	for(size_t n=0;n<shortArgs.size();n++)
	{
		std::string line = "  -";
		line += shortArgs[n];
		line += " ";
		line += longArgs[n];
		line += " ";
		std::cout << line;

		size_t space = DescMaxSpace - line.size();
		const std::vector<std::string> lines = string::SplitString(descriptions[n],"\n");
		for(auto line : lines)
		{
			std::cout << std::string(space,' ') << line << '\n';
			space = DescMaxSpace + 2;// For subsequent lines.
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace file{
bool FileExists(const char* pFileName)
{
    struct stat file_info;
    return stat(pFileName, &file_info) == 0 && S_ISREG(file_info.st_mode);
}

bool DirectoryExists(const char* pDirname)
{
// Had and issue where a path had an odd char at the end of it. So do this to make sure it's clean.
	const std::string clean(string::TrimWhiteSpace(pDirname));

    struct stat dir_info;
    if( stat(clean.c_str(), &dir_info) == 0 )
    {
        return S_ISDIR(dir_info.st_mode);
    }
    return false;
}

bool MakeDir(const std::string& pPath)
{
    StringVec folders = string::SplitString(pPath,"/");

    std::string CurrentPath;
    const char* seperator = "";
    for(const std::string& path : folders )
    {
        CurrentPath += seperator;
        CurrentPath += path;
        seperator = "/";
        if(DirectoryExists(CurrentPath) == false)
        {
            if( mkdir(CurrentPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 )
            {
                std::cout << "Making folders failed for " << pPath << std::endl;
                std::cout << "Failed AT " << path << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool MakeDirForFile(const std::string& pPathedFilename)
{
    std::string path = pPathedFilename;

    if( GetIsPathAbsolute(pPathedFilename) )
    {
        path = GetRelativePath(GetCurrentWorkingDirectory(),path);
    }

    path = GetPath(path);

    return MakeDir(path);
}

std::string GetFileName(const std::string& pPathedFileName,bool RemoveExtension/* = false*/)
{
    std::string result = pPathedFileName;
    // If / is the last char then it is just a path, so do nothing.
    if(result.back() != '/' )
    {
            // String after the last / char is the file name.
        std::size_t found = result.rfind("/");
        if(found != std::string::npos)
        {
            result = result.substr(found+1);
        }

        if( RemoveExtension )
        {
            found = result.rfind(".");
            if(found != std::string::npos)
            {
                result = result.substr(0,found);
            }
        }
    }

    // Not found, and so is just the file name.
    return result;
}

std::string GetPath(const std::string& pPathedFileName)
{
    // String after the last / char is the file name.
    std::size_t found = pPathedFileName.find_last_of("/");
    if(found != std::string::npos)
    {
        std::string result = pPathedFileName.substr(0,found);
        result += '/';
        return CleanPath(result);
    }
    return "./";
}

std::string GetCurrentWorkingDirectory()
{
    char buf[PATH_MAX];
    std::string path = getcwd(buf,PATH_MAX);
    return path;
}

std::string CleanPath(const std::string& pPath)
{
    // Early out for daft calls... but valid.
    if( pPath == "./" || pPath == "." || pPath == "../" )
    {
        return pPath;
    }

    // First get rid of the easy stuff.
    return string::ReplaceString(string::ReplaceString(pPath,"/./","/"),"//","/");
}

std::string GuessOutputName(const std::string& pProjectName)
{
    if( pProjectName.size() > 0 )
    {
        // Lets see if assuming its a path to a file will give us something.
        std::string aGuess = GetFileName(pProjectName,true);
        if( aGuess.size() > 0 )
            return aGuess;

        // That was unexpected. So lets just removed all characters I don't like and see what we get.
        // At the end of the day if the user does not like it they can set it in the project file.
        // This code is here to allow for minimal project files.
        aGuess.reserve(pProjectName.size());
        for( auto c : pProjectName )
        {
            if( std::isalnum(c) )
                aGuess += c;
        }

        // What about that?
        if( aGuess.size() > 0 )
            return aGuess;

    }
    // We have nothing, so lets just use this.
    return "binary";
}

std::string GetExtension(const std::string& pFileName,bool pToLower)
{
    std::string result;
    std::size_t found = pFileName.rfind(".");
    if(found != std::string::npos)
    {
        result = pFileName.substr(found,pFileName.size()-found);
        if( pToLower )
            std::transform(result.begin(), result.end(), result.begin(), ::tolower);

        // Remove leading .
        if( result.at(0) == '.' )
            result = result.erase(0,1);
    }

    return result;
}

static bool FilterMatch(const std::string& pFilename,const std::string& pFilter)
{
    size_t wildcardpos=pFilter.find("*");
    if( wildcardpos != std::string::npos )
    {
        if( wildcardpos > 0 )
        {
            wildcardpos = pFilename.find(pFilter.substr(0,wildcardpos));
            if( wildcardpos == std::string::npos )
                return false;
        }
        return pFilename.find(pFilter.substr(wildcardpos+1)) != std::string::npos;
    }
    return pFilename.find(pFilter) != std::string::npos;
}

StringVec FindFiles(const std::string& pPath,const std::string& pFilter)
{
    StringVec FoundFiles;

    DIR *dir = opendir(pPath.c_str());
    if(dir)
    {
        struct dirent *ent;
        while((ent = readdir(dir)) != nullptr)
        {
            const std::string fname = ent->d_name;
            if( FilterMatch(fname,pFilter) )
                FoundFiles.push_back(fname);
        }
        closedir(dir);
    }
    else
    {
        std::cout << "FindFiles \'" << pPath << "\' was not found or is not a path." << std::endl;
    }

    return FoundFiles;
}

std::string GetRelativePath(const std::string& pCWD,const std::string& pFullPath)
{
    assert( pCWD.size() > 0 );
    assert( pFullPath.size() > 0 );
    assert( GetIsPathAbsolute(pCWD) );
    assert( GetIsPathAbsolute(pFullPath) );

    const std::string cwd = CleanPath(pCWD);
    const std::string fullPath = CleanPath(pFullPath);

    assert( GetIsPathAbsolute(cwd) );
    assert( GetIsPathAbsolute(fullPath) );

    if( cwd.size() == 0 )
        return "./";

    if( fullPath.size() == 0 )
        return "./";

    if( cwd.at(0) != '/' )
        return fullPath;

    if( fullPath.at(0) != '/' )
        return fullPath;

    // Now do the real work.
    // First substitute the parts of CWD that match FULL path. Starting at the start of CWD.
    const StringVec cwdFolders = string::SplitString(cwd,"/");
    const StringVec fullPathFolders = string::SplitString(fullPath,"/");

    const size_t count = std::min(cwdFolders.size(),fullPathFolders.size()); 

    size_t posOfFirstDifference = 0;
    for( size_t n = 0 ; n < count && string::CompareNoCase(cwdFolders[n].c_str(),fullPathFolders[n].c_str()) ; n++ )
    {
        posOfFirstDifference++;
    }

    // Now to start building the path.
    // For every dir left in CWD add a ../
    std::string relativePath = "";
    for( size_t n = posOfFirstDifference ; n < cwdFolders.size() ; n++ )
    {
        relativePath += "../";
    }

    // Now add the rest of the full path passed in.
    // If relativePath is still zero length, add a ./ to it.
    if( relativePath.size() == 0 )
    {
        relativePath += "./";
    }

    for( size_t n = posOfFirstDifference ; n < fullPathFolders.size() ; n++ )
    {
        relativePath += fullPathFolders[n] + "/";
    }

#ifdef DEBUG_BUILD
    std::cout << "GetRelativePath(" << pCWD << "," << pFullPath << ") == " << relativePath << std::endl;
#endif

    return relativePath;
}

bool CompareFileTimes(const std::string& pSourceFile,const std::string& pDestFile)
{
    struct stat Stats;
    if( stat(pDestFile.c_str(), &Stats) == 0 && S_ISREG(Stats.st_mode) )
    {
        timespec dstFileTime = Stats.st_mtim;

        if( stat(pSourceFile.c_str(), &Stats) == 0 && S_ISREG(Stats.st_mode) )
        {
            timespec srcFileTime = Stats.st_mtim;

            if(srcFileTime.tv_sec == dstFileTime.tv_sec)
                return srcFileTime.tv_nsec > dstFileTime.tv_nsec;

            return srcFileTime.tv_sec > dstFileTime.tv_sec;
        }
    }

    return true;
}

std::string LoadFileIntoString(const std::string& pFilename)
{
    std::ifstream jsonFile(pFilename);
    if( jsonFile.is_open() )
    {
        std::stringstream jsonStream;
        jsonStream << jsonFile.rdbuf();// Read the whole file in...

        return jsonStream.str();
    }

    std::throw_with_nested(std::runtime_error("Jons file not found " + pFilename));

    return "";
}

};//namespace file{
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace tinytools
	
