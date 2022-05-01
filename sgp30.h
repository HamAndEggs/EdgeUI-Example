#include <functional>
#include <thread>

#include "i2c_device.h"

namespace i2c{
//////////////////////////////////////////////////////////////////////////
/*=========================================================================*/
// Lots of bits of code stolen from the adafruit Arduino library.
// Only put in what I need, nothing more.
// https://github.com/adafruit/Adafruit_SGP30/blob/master/Adafruit_SGP30.cpp
// Also based on the datasheet which is included in this repo.
class SGP30 : private Device
{
public:

    static const int READING_RESULT_FAILED = 0;
    static const int READING_RESULT_WARM_UP = 1;
    static const int READING_RESULT_VALID = 2;

	SGP30(int pAddress = 0x58,int pBus = 1);
    virtual ~SGP30();

    /**
     * @brief Starts the reading thread. Will call the callback when a reading is ready.
     * The sensor needs time to warm up so readings may not start imminently. 
     * @param pReadingCallback When is all working the call back will be called once a second.
     * @return true If all started ok.
     */
    bool Start(std::function<void(int pReadingResult,uint16_t pECO2,uint16_t pTVOC)> pReadingCallback);

    /**
     * @brief Stops the worker thread that takes the reading.
     */
    void Stop();

private:

    std::thread mWorker;    //<! This thread takes the readings.
    bool mRunWorker;

    bool SendCommand(const uint8_t pCommand[2]);
    std::vector<uint16_t> ReadResults(int pNumWords);
    uint8_t CalculateCRC(const uint8_t* pData,size_t pDataSize)const;
};

//////////////////////////////////////////////////////////////////////////
};//namespace i2c{

