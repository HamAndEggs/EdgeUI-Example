#include <iostream>
#include <assert.h>
#include <chrono>

#include "sgp30.h"

// commands and constants
#define SGP30_FEATURESET 0x0020    ///< The required set for this library
#define SGP30_CRC8_POLYNOMIAL 0x31 ///< Seed for SGP30's CRC polynomial
#define SGP30_CRC8_INIT 0xFF       ///< Init value for CRC

static const uint8_t SGP30_CMD_IAQ_INIT[]     = { 0x20, 0x03 };
static const uint8_t SGP30_CMD_IAQ_MEASURE[]  = { 0x20, 0x08 };
static const uint8_t SGP30_CMD_SELFTEST[] = { 0x20, 0x32 };
static const uint8_t SGP30_SELFTEST_OK[] = { 0x00, 0xd4 };

namespace i2c{
//////////////////////////////////////////////////////////////////////////
SGP30::SGP30(int pAddress,int pBus) : Device(pAddress,pBus)
{
    if( !IsOpen() )
    {
        std::cerr << "Failed to open SGP30\n";
        return;
    }

}

SGP30::~SGP30()
{
    Stop();
}


bool SGP30::Start(std::function<void(int pReadingResult,uint16_t pECO2,uint16_t pTVOC)> pReadingCallback)
{
    assert(pReadingCallback);
    if( !pReadingCallback )
    {
        return false;
    }

    // This is the worker thread that deals with getting the sensor running and reading at one second intervals.
    // This is based on their documentation in the data sheet.
    // The SGP30 uses a dynamic baseline compensation algorithm and on-chip calibration parameters to provide two
    // complementary air quality signals. Based on the sensor signals a total VOC signal (TVOC) and a CO2 equivalent signal
    // (CO2eq) are calculated. Sending an “Init_air_quality” command starts the air quality measurement. After the “Init_air_quality”
    // command, a “Measure_air_quality” command has to be sent in regular intervals of 1s to ensure proper operation of the
    // dynamic baseline compensation algorithm. The sensor responds with 2 data bytes (MSB first) and 1 CRC byte for each of the
    // two preprocessed air quality signals in the order CO2eq (ppm) and TVOC (ppb). For the first 15s after the “Init_air_quality”
    // command the sensor is in an initialization phase during which a “Measure_air_quality” command returns fixed values of
    // 400 ppm CO2eq and 0 ppb TVOC.
    mWorker = std::thread([this,pReadingCallback]()
    {
        mRunWorker = true;
        if( SendCommand(SGP30_CMD_IAQ_INIT) ) // “Init_air_quality” command.
        {
            int warmUpCounter = 16;
            // Now take the readings.
            while( mRunWorker )
            {
                if( warmUpCounter > 0 )
                {
                    warmUpCounter--;
                    pReadingCallback(READING_RESULT_WARM_UP,0,0);
                }
                else if( SendCommand(SGP30_CMD_IAQ_MEASURE) ) // “Measure_air_quality” command.
                {
                    // Read results.
                    const std::vector<uint16_t> data = ReadResults(2);
                    if( data.size() == 2 )
                    {
                        pReadingCallback(READING_RESULT_VALID,data[0],data[1]);
                    }
                    else
                    {
                        pReadingCallback(READING_RESULT_FAILED,0,0);
                    }
                }
                else
                {
                    pReadingCallback(READING_RESULT_FAILED,0,0);
                }

                // Wait once second before going again. As per the data sheet.
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        else
        {
            pReadingCallback(READING_RESULT_FAILED,0,0);
        }
    });


    return true;
}

void SGP30::Stop()
{
     mRunWorker = false;
    if( mWorker.joinable() )
    {
        mWorker.join();
    }
}

bool SGP30::SendCommand(const uint8_t pCommand[2])
{
    const bool ret = WriteData(pCommand,2) == 2;
    std::this_thread::sleep_for(std::chrono::milliseconds(12));
    return ret;
}

std::vector<uint16_t> SGP30::ReadResults(int pNumWords)
{
    std::vector<uint16_t> data;

    // For each word of data sent the chip sends a CRC.
    const int numBytes = pNumWords * 3;
    uint8_t buffer[numBytes];

    if( ReadData(buffer,numBytes) == numBytes )
    {
        for( int n = 0 ; n < pNumWords ; n++ )
        {
            const int i = n * 3;
            uint8_t* reading = buffer + i;

            if( reading[2] == CalculateCRC(reading,2) )
            {
                std::swap(reading[0],reading[1]);// Correct for endian
                const uint16_t v = *((uint16_t*)reading);
                data.push_back(v);
            }
            else
            {
                std::cerr << "CRC failed on read data\n";
            }
        }
    }
    else
    {
        std::cerr << "Read data failed\n";
    }
    return data;
}

uint8_t SGP30::CalculateCRC(const uint8_t* pData,size_t pDataSize)const
{
    #define SGP30_CRC8_POLYNOMIAL 0x31 ///< Seed for SGP30's CRC polynomial
    #define SGP30_CRC8_INIT 0xFF       ///< Init value for CRC

    uint8_t crc = SGP30_CRC8_INIT;

    // Totally cut'n'paste from data fruit. :)
    for( size_t i = 0 ; i < pDataSize ; i++ )
    {
        crc ^= pData[i];
        for (uint8_t b = 0; b < 8; b++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ SGP30_CRC8_POLYNOMIAL;
            else
                crc <<= 1;
        }
    }
    return crc;
}

//////////////////////////////////////////////////////////////////////////
};//namespace i2c{
