/*
 * @file TinyTools.h
 * @author Richard e Collins
 * @version 0.1
 * @date 2021-03-15
 * https://github.com/HamAndEggs/TinyTools

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

#ifndef TINY_TOOLS_H
#define TINY_TOOLS_H

#include <getopt.h>

#include <vector>
#include <string>
#include <map>
#include <set>
#include <stack>
#include <functional>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <ctime>
#include <iomanip>

/**
 * @brief Adds line and source file. There is a c++20 way now that is better. I need to look at that.
 */
#define TINYTOOLS_THROW(THE_MESSAGE__)	{throw std::runtime_error("At: " + std::to_string(__LINE__) + " In " + std::string(__FILE__) + " : " + std::string(THE_MESSAGE__));}


namespace tinytools{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace math{

inline float GetFractional(float pValue)
{
	return std::fmod(pValue,1.0f);
}

inline float GetInteger(float pValue)
{
	return pValue - GetFractional(pValue);
}

/**
 * @brief Rounds a floating point value into multiplies of 0.5
 * -1.2 -> -1.0
 * -1.0 -> -1.0
 * -0.8 -> -1.0
 * 2.3 -> 2.5
 * 2.8 -> 3.0
 */
inline float RoundToPointFive(float pValue)
{
	const float integer = GetInteger(pValue);
	const float frac = std::round(GetFractional(pValue)*2.0f) / 2.0f;
	return integer + frac;
}

// Taken from the site: https://tttapa.github.io/Pages/Mathematics/Systems-and-Control-Theory/Digital-filters/Simple%20Moving%20Average/C++Implementation.html
template <uint8_t N, class input_t = uint16_t, class sum_t = uint32_t>
// Simple Moving Average difference equation
class SimpleMovingAverage
{
public:
	input_t operator()(input_t input)
	{
		sum -= previousInputs[index];
		sum += input;
		previousInputs[index] = input;
		if (++index == N)
		index = 0;
		return (sum + (N / 2)) / N;
	}

	static_assert
	(
		sum_t(0) < sum_t(-1),  // Check that `sum_t` is an unsigned type
		"Error: sum data type should be an unsigned integer, otherwise, "
		"the rounding operation in the return statement is invalid."
	);

private:
	uint8_t index             = 0;
	input_t previousInputs[N] = {};
	sum_t sum                 = 0;
};

};//namespace math{
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// Common string types, not in string namespace as the types already start with String.
typedef std::vector<std::string> StringVec;
typedef std::set<std::string> StringSet;
typedef std::stack<std::string> StringStack;
typedef std::map<std::string,StringVec> StringVecMap;
typedef std::map<std::string,StringSet> StringSetMap;
typedef std::map<std::string,int> StringIntMap;

class StringMap : public std::map<std::string,std::string>
{
public:
	inline void GetKeys(StringVec& rKeys)const
	{
		for( const auto& key : *this )
			rKeys.push_back(key.first);
	}

	inline void GetValues(StringVec& rValues)const
	{
		for( const auto& value : *this )
			rValues.push_back(value.second);
	}
};

namespace string{

// If pNumChars == 0 then full length is used.
// assert(cppmake::CompareNoCase("onetwo","one",3) == true);
// assert(cppmake::CompareNoCase("onetwo","ONE",3) == true);
// assert(cppmake::CompareNoCase("OneTwo","one",3) == true);
// assert(cppmake::CompareNoCase("onetwo","oneX",3) == true);
// assert(cppmake::CompareNoCase("OnE","oNe") == true);
// assert(cppmake::CompareNoCase("onetwo","one") == true);	// Does it start with 'one'
// assert(cppmake::CompareNoCase("onetwo","onetwothree",6) == true);
// assert(cppmake::CompareNoCase("onetwo","onetwothreeX",6) == true);
// assert(cppmake::CompareNoCase("onetwo","onetwothree") == false); // sorry, but we're searching for more than there is... false...
// assert(cppmake::CompareNoCase("onetwo","onetwo") == true);
/**
 * @brief Does an ascii case insensitive test within the full string or a limited start of the string.
 * 
 * @param pA 
 * @param pB 
 * @param pLength If == 0 then length of second string is used. If first is shorter, will always return false.
 * @return true 
 * @return false 
 */
bool CompareNoCase(const char* pA,const char* pB,size_t pLength = 0);

inline bool CompareNoCase(const std::string& pA,const std::string& pB,size_t pLength = 0)
{
	return CompareNoCase(pA.c_str(),pB.c_str(),pLength);
}

char* CopyString(const char* pString, size_t pMaxLength);
inline char* CopyString(const std::string& pString){return CopyString(pString.c_str(),pString.size());}
StringVec SplitString(const std::string& pString,const char* pSeperator);
std::string ReplaceString(const std::string& pString,const std::string& pSearch,const std::string& pReplace);
std::string TrimWhiteSpace(const std::string &s);

inline bool Search(const StringVec& pVec,const std::string& pLost)
{
   return std::find(pVec.begin(),pVec.end(),pLost) != pVec.end();
}

};//namespace string{
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace timers{
class MillisecondTicker
{
public:
	MillisecondTicker() = default;
    MillisecondTicker(int pMilliseconds);


	/**
	 * @brief Sets the new timeout interval, resets internal counter.
	 * 
	 * @param pMilliseconds 
	 */
	void SetTimeout(int pMilliseconds);

	/**
	 * @brief Returns true if trigger ticks is less than now
	 */
    bool Tick(){return Tick(std::chrono::system_clock::now());}
    bool Tick(const std::chrono::system_clock::time_point pNow);

	/**
	 * @brief Calls the function if trigger ticks is less than now. 
	 */
    void Tick(std::function<void()> pCallback){Tick(std::chrono::system_clock::now(),pCallback);}
    void Tick(const std::chrono::system_clock::time_point pNow,std::function<void()> pCallback );


private:
    std::chrono::milliseconds mTimeout;
    std::chrono::system_clock::time_point mTrigger;
};

};//namespace timers{
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace network{

/**
 * @brief 
 * Used the excellent answer by SpectreVert from the topic below.
 * https://stackoverflow.com/questions/49335001/get-local-ip-address-in-c
 * A little restructure to minimise typo bugs. (e.g fogetting close on socket)
 * @return std::string 
 */
std::string GetLocalIP();

/**
 * @brief Get Host Name, handy little wrapper.
 */
std::string GetHostName();

/**
 * @brief Get the host name from the IP address
 * e.g. const std::string name = GetNameFromIPv4("192.168.1.100");
 */
std::string GetNameFromIPv4(const std::string& pAddress);

/**
 * @brief Get the host name from the IP address
 * e.g. const std::string name = GetNameFromIPv4(MakeIP4V(192,168,1,100));
 */
std::string GetNameFromIPv4(const uint32_t pAddress);

/**
 * @brief If the device with the host name has a IPv4 address returns that address.
 */
uint32_t GetIPv4FromName(const std::string& pHostName);

/**
 * @brief Scans from PI range to PI range. Broadcast IP's ignored.
 * Will return some intresting information handy for network monitoring.
 */
void ScanNetworkIPv4(uint32_t pFromIPRange,uint32_t pToIPRange,std::function<bool(const uint32_t pIPv4,const char* pHostName)> pDeviceFound);

/**
 * @brief Checks that the port on the device at IPv4 can be opened.
 */
bool IsPortOpen(uint32_t pIPv4,uint16_t pPort);

/**
 * @brief Creates an 32 bit value from the passed IPv4 address.
 * https://www.sciencedirect.com/topics/computer-science/network-byte-order
 */
inline uint32_t MakeIP4V(uint8_t pA,uint8_t pB,uint8_t pC,uint8_t pD)
{
	// Networking byte order is big endian, so most significan't byte is byte 0.
	return (uint32_t)((pA << 0) | (pB << 8) | (pC << 16) | pD << 24);
}

/**
 * @brief Makes a string, IE 192.168.1.1 from the passed in IPv4 value.
 * 
 * @param pIPv4 In big endian format https://www.sciencedirect.com/topics/computer-science/network-byte-order
 * @return std::string 
 */
inline std::string IPv4ToString(uint32_t pIPv4)
{
	int a = (pIPv4&0x000000ff)>>0;
	int b = (pIPv4&0x0000ff00)>>8;
	int c = (pIPv4&0x00ff0000)>>16;
	int d = (pIPv4&0xff000000)>>24;

	return std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + "." + std::to_string(d);
}

/**
 * @brief Encodes 8 bit data to a 7 bit data data stream. No data is lost, new buffer size will be bigger because of that.
 * Used for comunication protocals so that the most significant bit can be used for control bytes.
 * @param r7Bit A point to memory holding the converted data. You have to delete this after use with delete[].
 * @return size_t The size of the 7Bit data.
 */
size_t Encode7Bit(const uint8_t* p8Bit,size_t p8BitSize,uint8_t** r7Bit);

/**
 * @brief Converts 7 bit input data into the original 8 bit data.
 */
size_t Decode7Bit(const uint8_t* p7Bit,size_t p7BitSize,uint8_t** r8Bit);

};// namespace network

///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace system{

/**
 * @brief Returns seconds since 1970, the epoch. I put this is as I can never rememeber the correct construction using c++ :D
 */
inline int64_t SecondsSinceEpoch()
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

/**
 * @brief Get the local date time as string
 * 
 * @return std::string 
 */
std::string GetLocalDateTime();

/**
 * @brief Fetches the system uptime in a more human readable format
 */
bool GetUptime(uint64_t& rUpDays,uint64_t& rUpHours,uint64_t& rUpMinutes);

/**
 * @brief Return the uptime as a nice human readable string.
 */
std::string GetUptime();

/**
 * @brief USed to track the deltas from last time function is called. Needed to be done like this to beable to see instantaneous CPU load. Method copied from htop source.
 * https://www.linuxhowtos.org/manpages/5/proc.htm
 * https://github.com/htop-dev/htop/
 */
struct CPULoadTracking
{
	uint64_t mUserTime;		// <! Time spent in user mode and hosting virtual machines.
	uint64_t mTotalTime;	// <! Total time, used to create the percentage.
};

/**
 * @brief Will return N + 1 entries for each hardware thread, +1 for total avarage.
 * First call expects pTrackingData to be zero in length, and will initialise the vector. Don't mess with the vector as you're break it all. ;)
 * Index of map is core ID, The ID of the CPU core (including HW threading, so a 8 core 16 thread system will have 17 entries, 16 for threads and one for total)
 * The load is the load of user space and virtual guests on the system. Does not include system time load.
 */
bool GetCPULoad(std::map<int,CPULoadTracking>& pTrackingData,int& rTotalSystemLoad,std::map<int,int>& rCoreLoads);

/**
 * @brief Get the Memory Usage, all values passed back in 1K units because that is what the OS sends back.
 * Used https://gitlab.com/procps-ng/procps as reference as it's not as simple as reading the file. :-? Thanks Linus.....
 */
bool GetMemoryUsage(size_t& rMemoryUsedKB,size_t& rMemAvailableKB,size_t& rMemTotalKB,size_t& rSwapUsedKB);


/**
 * @brief Calls and waits for the command in pCommand with the arguments pArgs and addictions to environment variables in pEnv.
 * Uses the function ExecuteCommand below but first forks the process so that current execution can continue.
 * https://linux.die.net/man/3/execvp
 * 
 * Blocking. If you need a non blocking then just call in a worker thread of your own. This makes it cleaner and more flexable.
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 * @param pEnv Extra environment variables, string pair (name,value), to append to the current processes environment variables, maybe empty if you wish. An example use is setting LD_LIBRARY_PATH
 * @param rOutput The output from the executed command, if there was any.
 * @return true if calling the command worked, says nothing of the command itself.
 * @return false Something went wrong. Does not represent return value of command.
 */
extern bool ExecuteShellCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv, std::string& rOutput);

/**
 * @brief Calls and waits for the command in pCommand with the arguments pArgs.
 * Uses the function ExecuteCommand below but first forks the process so that current execution can continue.
 * https://linux.die.net/man/3/execvp
 * 
 * Blocking. If you need a non blocking then just call in a worker thread of your own. This makes it cleaner and more flexable.
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 * @param rOutput The output from the executed command, if there was any.
 * @return true if calling the command worked, says nothing of the command itself.
 * @return false Something went wrong. Does not represent return value of command.
 */
inline bool ExecuteShellCommand(const std::string& pCommand,const std::vector<std::string>& pArgs, std::string& rOutput)
{
    const std::map<std::string,std::string> empty;
    return ExecuteShellCommand(pCommand,pArgs,empty,rOutput);
}

/**
 * @brief Replaces the current process image with the command in pCommand with the arguments pArgs and addictions to environment variables in pEnv.
 * Uses the execvp command.
 * https://linux.die.net/man/3/execvp
 * 
 * Replaces the current process, so not returning from this function!
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 * @param pEnv Extra environment variables, string pair (name,value), to append to the current processes environment variables, maybe empty if you wish. An example use is setting LD_LIBRARY_PATH
 */
extern void ExecuteCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv);

/**
 * @brief Replaces the current process image with the command in pCommand with the arguments pArgs.
 * Uses the execvp command.
 * https://linux.die.net/man/3/execvp
 * 
 * Replaces the current process, so not returning from this function!
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 */
inline void ExecuteCommand(const std::string& pCommand,const std::vector<std::string>& pArgs)
{
    const std::map<std::string,std::string> empty;
    ExecuteCommand(pCommand,pArgs,empty);
}

};//namespace system{
///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace threading{

/**
 * @brief This class encapsulates a thread that can sleep for any amount of time but wake up when the ownering, main thread, needs to exit the application.
 */
class SleepableThread
{
public:

	/**
	 * @brief This will start the thread and return, pTheWork will be called 'ticked' by the thread after each pause, and exit when asked to.
	 * pPauseInterval is seconds.
	 */
	void Tick(int pPauseInterval,std::function<void()> pTheWork);

	/**
	 * @brief Called from another thread, will ask it to exit and then wait for it to do so.
	 * 
	 */
	void TellThreadToExitAndWait();

private:
    bool mKeepGoing = true;           	//!< A boolean that will be used to signal the worker thread that it should exit.
	std::thread mWorkerThread;			//!< The thread that will do your work and then sleep for a bit. 
    std::condition_variable mSleeper;   //!< Used to sleep for how long asked for but also wake up if we need to exit.
    std::mutex mSleeperMutex;			//!< This is used to correctly use the condition variable.
};

/**
 * @brief This is the ring buffer container.
 * As long as you can grantee that only one thread is writing and one thread is reading no locks are needed.
 * The reading and writing threads can be different.
 * The writing thread must not call ReadNext.
 * The reading thread must not call WriteNext.
 * Using volatile is to force the suppression of optimisations
 * that could otherwise occur on the counters that could break the lockless nature of the buffer.
 */
class LocklessRingBuffer
{
public:
    /**
     * @brief Allocates a buffer object.
     * 
     * @param pItemSizeof The size of each item being but into the buffer.
     * @param pItemCount The number of items in the buffer.
     */
    LocklessRingBuffer(size_t pItemSizeof,size_t pItemCount);

    ~LocklessRingBuffer();

	/**
	 * @brief Will state if there is data to be read or not.
	 * 
	 * @return true No data to be read.
	 * @return false There is data to read.
	 */
	bool Empty()const{return mCurrentReadPos == mNextWritePos;}
	
	/**
     * @brief Reads the next item that is in the buffer.
	 * 
	 * @param rItem The memory store to write the data too.
	 * @param pBufferSize The size of the buffer we're writing too.
	 * @return true If data was read.
	 * @return false If the buffer is empty.
	 */
    bool ReadNext(void* rItem,size_t pBufferSize);

    /**
     * @brief Writes an item to the buffer.
     * 
     * @param pItem The item to write.
	 * @param pBufferSize The size of the buffer we're reading.
     * @return true if there was room to write the item, false if the buffer is full and items can not be written.
     */
    bool WriteNext(const void* pItem,size_t pBufferSize);

private:
    volatile int mCurrentReadPos;//!< The current item READ index in the buffer. That is mBuffer + (mCurrentReadPos * mItemSizeof)    
    volatile int mNextWritePos;//!< The current item WRITE index in the buffer. That is mBuffer + (mNextWritePos * mItemSizeof)
    
    uint8_t *mBuffer;
    size_t mItemSizeof;
    size_t mItemCount;
};


};//namespace threading{

///////////////////////////////////////////////////////////////////////////////////////////////////////////
class CommandLineOptions
{
public:
	CommandLineOptions(const std::string& pUsageHelp);

	void AddArgument(char pArg,const std::string& pLongArg,const std::string& pHelp,int pArgumentOption = no_argument,std::function<void(const std::string& pOptionalArgument)> pCallback = nullptr);
	bool Process(int argc, char *argv[]);
	bool IsSet(char pShortOption)const;
	bool IsSet(const std::string& pLongOption)const;
	void PrintHelp()const;

private:
	struct Argument
	{
		Argument(const std::string& pLongArgument,const std::string& pHelp,const int pArgumentOption,std::function<void(const std::string& pOptionalArgument)> pCallback):
			mLongArgument(pLongArgument),
			mHelp(pHelp),
			mArgumentOption(pArgumentOption),
			mCallback(pCallback),
			mIsSet(false)
		{

		}

		const std::string mLongArgument;
		const std::string mHelp;
		const int mArgumentOption;
		std::function<void(const std::string& pOptionalArgument)> mCallback;
		bool mIsSet;	//!< This will be true if the option was part of the commandline. Handy for when you just want to know true or false.
	};
	
	const std::string mUsageHelp;
	std::map<char,Argument> mArguments;
	
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace file{
bool FileExists(const char* pFilename);	//Will return false if file name is a path! We want to know if the file exists!
bool DirectoryExists(const char* pDirname);	//Will return false if name name is a file! We want to know if the dir exists!

inline bool FileExists(const std::string& pFilename){return FileExists(pFilename.c_str());}
inline bool DirectoryExists(const std::string& pDirname){return DirectoryExists(pDirname.c_str());}

/**
 * @brief Checks if the path passed is absolute.
 * abstracted out like this in case it needs to be changed / ported.
 * Just checks first char.
 * 
 * @param pPath 
 * @return true 
 * @return false 
 */
inline bool GetIsPathAbsolute(const std::string& pPath)
{
   return pPath.size() > 0 && pPath.at(0) == '/';
}

// Splits string into list have names to be made into directories, so don't include any filenames as they will be made into a folder.
// Returns true if all was ok.
bool MakeDir(const std::string& pPath);

/**
 * @brief Takes the pathed file name and creates any missing folders that it needs.
 * 
 * @param pPathedFilename Can either be absolute or relative to current working directory. 
 * If folders already exist then nothing will change.
 * @return true The folders were made ok.
 * @return false There was an issue.
 */
bool MakeDirForFile(const std::string& pPathedFilename);
/**
 * @brief Get the Current Working Directory
 */
std::string GetCurrentWorkingDirectory();

/**
 * @brief Tries to clean the path passed in. So things lile /./ become /
 */
std::string CleanPath(const std::string& pPath);

/**
 * @brief Gets the File Name from the pathed filename string.
 */
std::string GetFileName(const std::string& pPathedFileName,bool RemoveExtension = false);

/**
 * @brief Gets the Path from the pathed filename string.
 */
std::string GetPath(const std::string& pPathedFileName);

/**
 * @brief Get the Extension of the passed filename,
 * E.G. hello.text will return text
 * 
 * @param pFileName 
 * @param pToLower 
 * @return std::string The extension found or a zero length string. The extension returned does not include the dot.
 */
std::string GetExtension(const std::string& pFileName,bool pToLower = true);

/**
 * @brief Find the files in path that match the filter. Does not enter child folders.
 * 
 * @param pPath 
 * @param pFilter 
 * @return StringVec 
 */
StringVec FindFiles(const std::string& pPath,const std::string& pFilter = "*");

/**
 * @brief Get the Relative Path based on the passed absolute paths.
 * In debug the following errors will result in an assertion.
 * Examples
 *     GetRelativePath("/home/richard/games/chess","/home/richard/games/chess/game1") == "./game1/"
 *     GetRelativePath("/home/richard/games","/home/richard/music") == "../music/"
 * @param pCWD The directory that you want the full path to be relative too. This path should also start with a / if it does not the function will just return the full path unchanged.
 * @param pFullPath The full path, if it does not start with a / it will be assumed to be already relative and so just returned.
 * @return std::string If either input string is empty './' will be returned.
 */
std::string GetRelativePath(const std::string& pCWD,const std::string& pFullPath);

/**
 * @brief Checks if the file for source is newer than of dest.
 * Also will pretend that it is newer if any of the stats fail.
 * This is a bespoke to my needs here, should not be exported to rest of the code.
 * 
 * @param pSourceFile 
 * @param pDestFile 
 * @return true 
 * @return false 
 */
bool CompareFileTimes(const std::string& pSourceFile,const std::string& pDestFile);

/**
 * @brief Loads the contents of the file into a string object.
 * Throws an exception if file not found.
 * @param pFilename 
 * @return std::string 
 */
std::string LoadFileIntoString(const std::string& pFilename);

};// namespace file
///////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace tinytools
	
#endif //TINY_TOOLS_H
