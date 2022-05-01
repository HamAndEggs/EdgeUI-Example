
#include "Graphics.h"
#include "Element.h"
#include "LayoutGrid.h"
#include "TinyTools.h"
#include "sgp30.h"
#include "MQTTData.h"
#include "TinyJson.h"

#include <thread>
#include <curl/curl.h>

#include <iostream>
#include <chrono>
#include <time.h>

const float CELL_PADDING = 0.03f;
static std::map<std::string,std::string> outsideData;
static std::time_t outsideTemperatureDelivered = 0;
static i2c::SGP30 indoorAirQuality;

static int CURLWriter(char *data, size_t size, size_t nmemb,std::string *writerData)
{
    if(writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}

class FetchBitcoinPrice
{
public:
    FetchBitcoinPrice()
    {
        mPriceUpdater.Tick(60*10,[this]()
        {
            std::string jsonData;
            try
            {
                const std::string url = "https://cex.io/api/ticker/BTC/GBP";
                if( DownloadReport(url,jsonData) )
                {
                    // We got it, now we need to build the weather object from the json.
                    // I would have used rapid json but that is a lot of files to add to this project.
                    // My intention is for someone to beable to drop these two files into their project and continue.
                    // And so I will make my own json reader, it's easy but not the best solution.
                    tinyjson::JsonProcessor json(jsonData);
                    const tinyjson::JsonValue price = json.GetRoot();

                    mLastPrice = (int)std::stoi(price["last"].GetString());
                    mPriceChange = (int)std::stoi(price["priceChange"].GetString());
                    m24HourLow = (int)std::stoi(price["low"].GetString());
                    m24HourHigh = (int)std::stoi(price["high"].GetString());
                }
            }
            catch(std::runtime_error &e)
            {
                std::cerr << "Failed to download bitcoin price: " << e.what() << "\n";
            }
        });
    }

    ~FetchBitcoinPrice()
    {
        mPriceUpdater.TellThreadToExitAndWait();
    }

    int mLastPrice = 0;
    int mPriceChange = 0;
    int m24HourLow = 0;
    int m24HourHigh = 0;

private:
    tinytools::threading::SleepableThread  mPriceUpdater;

    bool DownloadReport(const std::string& pURL,std::string& rJson)const
    {
        bool result = false;
        CURL *curl = curl_easy_init();
        if(curl)
        {
            char errorBuffer[CURL_ERROR_SIZE];
            errorBuffer[0] = 0;
            if( curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer) == CURLE_OK )
            {
                if( curl_easy_setopt(curl, CURLOPT_URL, pURL.c_str()) == CURLE_OK )
                {
                    if( curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLWriter) == CURLE_OK )
                    {
                        if( curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rJson) == CURLE_OK )
                        {
                            if( curl_easy_perform(curl) == CURLE_OK )
                            {
                                result = true;
                            }
                        }
                    }
                }
            }

            /* always cleanup */ 
            curl_easy_cleanup(curl);
        }

        return result;
    }
};
static FetchBitcoinPrice BitcoinPrice;

static void MakeClock(eui::Element* cell,int pBigFont,int pNormalFont,int pMiniFont)
{
    eui::Element* clock = eui::Element::Create();
        clock->SetPadding(0.05f);
        clock->GetStyle().mAlignment = eui::ALIGN_CENTER_TOP;
        clock->SetFont(pBigFont);
    cell->Attach(clock);

    eui::Element* dayName = eui::Element::Create();
        dayName->SetPadding(0.05f);
        dayName->GetStyle().mAlignment = eui::ALIGN_LEFT_BOTTOM;
        dayName->SetFont(pNormalFont);
    cell->Attach(dayName);

    eui::Element* dayNumber = eui::Element::Create();
        dayNumber->SetPadding(0.05f);
        dayNumber->GetStyle().mAlignment = eui::ALIGN_RIGHT_BOTTOM;
        dayNumber->SetFont(pNormalFont);
    cell->Attach(dayNumber);

    eui::Style s;
    s.mBackground = eui::COLOUR_BLACK;
    s.mBorderSize = 5.0f;
    s.mBorder = eui::COLOUR_WHITE;
    s.mRadius = 15.0f;
    cell->SetStyle(s);
    cell->SetPadding(CELL_PADDING);


    cell->SetOnUpdate([clock,dayName,dayNumber](eui::Element* pElem)
    {
        std::time_t result = std::time(nullptr);
        tm *currentTime = localtime(&result);
        if( currentTime )
        {
            clock->SetTextF("%02d:%02d",currentTime->tm_hour,currentTime->tm_min);

            const std::array<std::string,7> Days = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
            dayName->SetText(Days[currentTime->tm_wday]);

            // Do month day.
            const char* monthDayTag = "th";
            const int monthDay = currentTime->tm_mday;
            if( monthDay == 1 || monthDay == 21 || monthDay == 31 )
            {
                monthDayTag = "st";
            }
            else if( monthDay == 2 || monthDay == 22 )
            {
                monthDayTag = "nd";
            }
            else if( monthDay == 3 || monthDay == 23 )
            {
                monthDayTag = "rd";
            }

            dayNumber->SetTextF("%d%s",monthDay,monthDayTag);
        }
        else
        {
            clock->SetText("NO CLOCK");
            dayName->SetText("NO DAY");
            dayNumber->SetText("NO DAY");
        }
        return true;
    });
}

static void MakeSystemStatus(eui::Element* cell,int pBigFont,int pNormalFont,int pMiniFont)
{
    static std::map<int,tinytools::system::CPULoadTracking> trackingData;

    eui::Element* uptime = eui::Element::Create();
        uptime->SetPadding(0.05f);
        uptime->GetStyle().mAlignment = eui::ALIGN_CENTER_TOP;
        uptime->SetFont(pNormalFont);
        uptime->SetText("UP: XX:XX:XX");
    cell->Attach(uptime);

    eui::Element* localIP = eui::Element::Create();
        localIP->SetPadding(0.05f);
        localIP->GetStyle().mAlignment = eui::ALIGN_LEFT_CENTER;
        localIP->SetFont(pMiniFont);
        localIP->SetText("XX.XX.XX.XX");
    cell->Attach(localIP);

    eui::Element* hostName = eui::Element::Create();
        hostName->SetPadding(0.05f);
        hostName->GetStyle().mAlignment = eui::ALIGN_RIGHT_CENTER;
        hostName->SetFont(pMiniFont);
        hostName->SetText("--------");
    cell->Attach(hostName);

    eui::Element* cpuLoad = eui::Element::Create();
        cpuLoad->SetPadding(0.05f);
        cpuLoad->GetStyle().mAlignment = eui::ALIGN_LEFT_BOTTOM;
        cpuLoad->SetFont(pMiniFont);
        cpuLoad->SetText("XX.XX.XX.XX");
    cell->Attach(cpuLoad);

    eui::Element* ramUsed = eui::Element::Create();
        ramUsed->SetPadding(0.05f);
        ramUsed->GetStyle().mAlignment = eui::ALIGN_RIGHT_BOTTOM;
        ramUsed->SetFont(pMiniFont);
        ramUsed->SetText("--------");
    cell->Attach(ramUsed);

    eui::Style s;
    s.mBackground = eui::COLOUR_BLUE;
    s.mBorderSize = 5.0f;
    s.mBorder = eui::COLOUR_WHITE;
    s.mRadius = 15.0f;
    cell->SetStyle(s);
    cell->SetPadding(CELL_PADDING);

    std::map<int,int> CPULoads;
    int totalSystemLoad;
    tinytools::system::GetCPULoad(trackingData,totalSystemLoad,CPULoads);

    cell->SetOnUpdate([uptime,localIP,hostName,cpuLoad,ramUsed](eui::Element* pElem)
    {
    // Render the uptime
        uint64_t upDays,upHours,upMinutes;
        tinytools::system::GetUptime(upDays,upHours,upMinutes);

        uptime->SetTextF("UP: %lld:%02lld:%02lld",upDays,upHours,upMinutes);

        localIP->SetText(tinytools::network::GetLocalIP());

        hostName->SetText(tinytools::network::GetHostName());

        std::map<int,int> CPULoads;
        int totalSystemLoad;
        tinytools::system::GetCPULoad(trackingData,totalSystemLoad,CPULoads);
        if(CPULoads.size() > 0)
        {
            cpuLoad->SetTextF("CPU:%d%%",totalSystemLoad);
        }
        else
        {
            cpuLoad->SetText("CPU:--%%");
        }

        size_t memoryUsedKB,memAvailableKB,memTotalKB,swapUsedKB;
        if( tinytools::system::GetMemoryUsage(memoryUsedKB,memAvailableKB,memTotalKB,swapUsedKB) )
        {
            const std::string memory = "Mem:" + std::to_string(memoryUsedKB * 100 / memTotalKB) + "%"; 
            ramUsed->SetText(memory);
        }
        return true;
    });
}

void MakeEnvironmentStatus(eui::Element* cell,int pBigFont,int pNormalFont,int pMiniFont)
{
    eui::Element* eCO2 = eui::Element::Create();
        eCO2->SetPadding(0.05f);
        eCO2->GetStyle().mAlignment = eui::ALIGN_CENTER_TOP;
        eCO2->SetFont(pNormalFont);
        eCO2->SetText("UP: XX:XX:XX");
    cell->Attach(eCO2);
    
    eui::Element* tOC = eui::Element::Create();
        tOC->SetPadding(0.05f);
        tOC->GetStyle().mAlignment = eui::ALIGN_CENTER_CENTER;
        tOC->SetFont(pNormalFont);
        tOC->SetText("XX.XX.XX.XX");
    cell->Attach(tOC);

    eui::Element* outsideTemp = eui::Element::Create();
        outsideTemp->SetPadding(0.05f);
        outsideTemp->GetStyle().mAlignment = eui::ALIGN_CENTER_BOTTOM;
        outsideTemp->SetFont(pNormalFont);
        outsideTemp->SetText("XX.XC");
    cell->Attach(outsideTemp);

    eui::Style s;
    s.mBackground = eui::COLOUR_DARK_GREEN;
    s.mBorderSize = 5.0f;
    s.mBorder = eui::COLOUR_WHITE;
    s.mRadius = 15.0f;
    cell->SetStyle(s);
    cell->SetPadding(CELL_PADDING);

    uint16_t mECO2 = 0;
    uint16_t mTVOC = 0;
    int mResult = i2c::SGP30::READING_RESULT_WARM_UP;

    indoorAirQuality.Start([&mECO2,&mTVOC,&mResult](int pResult,uint16_t pECO2,uint16_t pTVOC)
    {
        mResult = pResult;
        if( pResult == i2c::SGP30::READING_RESULT_VALID )
        {
            mECO2 = pECO2;
            mTVOC = pTVOC;
        }
    });
    outsideData["/outside/temperature"] = "Waiting";// Make sure there is data.
    cell->SetOnUpdate([eCO2,tOC,outsideTemp,mECO2,mTVOC,mResult](eui::Element* pElem)
    {
        outsideTemp->SetText("Outside: " + outsideData["/outside/temperature"]);
        switch (mResult)
        {
        case i2c::SGP30::READING_RESULT_WARM_UP:
            eCO2->SetText("Start up");
            tOC->SetText("Please Wait");
            break;

        case i2c::SGP30::READING_RESULT_VALID:
            eCO2->SetTextF("eCO2: %d",mECO2);
            tOC->SetTextF("tVOC: %d",mTVOC);
            break;

        default:
            eCO2->SetText("Error");
            tOC->SetText("Disconnected");
            break;
        }
        return true;
    });
}

void MakeBitcoinPrice(eui::Element* cell,int pBigFont,int pNormalFont,int pMiniFont)
{
    eui::LayoutGrid* grid = eui::LayoutGrid::Create(cell,2,2);
    eui::Style s;
    s.mBackground = eui::COLOUR_DARK_GREY;
    s.mBorderSize = 5.0f;
    s.mBorder = eui::COLOUR_WHITE;
    s.mRadius = 15.0f;
    s.mAlignment = eui::ALIGN_CENTER_CENTER;

    cell = eui::Element::Create();
        cell->SetPadding(0.05f);
        cell->SetFont(pNormalFont);
        cell->SetText("£XXXXXX");
        cell->SetStyle(s);
        cell->SetPadding(CELL_PADDING);
        cell->SetOnUpdate([](eui::Element* pElem)
        {
            pElem->SetTextF("£%d",BitcoinPrice.mLastPrice);
            return true;
        });
    grid->ReplaceCell(0,0,cell);

    cell = eui::Element::Create();
        cell->SetPadding(0.05f);
        cell->SetFont(pNormalFont);
        cell->SetText("+XXXXXX");
        cell->SetStyle(s);
        cell->SetPadding(CELL_PADDING);
        cell->SetOnUpdate([](eui::Element* pElem)
        {
            std::string growth = std::to_string(BitcoinPrice.mPriceChange);
            if( BitcoinPrice.mPriceChange > 0 )
                growth = "+" + growth;

            pElem->SetText(growth);
            return true;
        });
    grid->ReplaceCell(1,0,cell);

    cell = eui::Element::Create();
        cell->SetPadding(0.05f);
        cell->SetFont(pNormalFont);
        cell->SetText("+XXXXXX");
        cell->SetStyle(s);
        cell->SetPadding(CELL_PADDING);
        cell->SetOnUpdate([](eui::Element* pElem)
        {
            pElem->SetTextF("£%d",BitcoinPrice.m24HourHigh);
            return true;
        });
    grid->ReplaceCell(0,1,cell);

    cell = eui::Element::Create();
        cell->SetPadding(0.05f);
        cell->SetFont(pNormalFont);
        cell->SetText("£XXXXXX");
        cell->SetStyle(s);
        cell->SetPadding(CELL_PADDING);
        cell->SetOnUpdate([](eui::Element* pElem)
        {
            pElem->SetTextF("£%d",BitcoinPrice.m24HourLow);
            return true;
        });
    grid->ReplaceCell(1,1,cell);
}

int main(int argc, char *argv[])
{
    eui::Graphics* graphics = eui::Graphics::Open();
//    graphics->FontSetMaximumAllowedGlyph(256);

    eui::Element* mainScreen = eui::Element::Create();
    mainScreen->SetID("mainScreen");

    int miniFont = graphics->FontLoad("./liberation_serif_font/LiberationSerif-Regular.ttf",20);
    int normalFont = graphics->FontLoad("./liberation_serif_font/LiberationSerif-Regular.ttf",35);
    int bigFont = graphics->FontLoad("./liberation_serif_font/LiberationSerif-Bold.ttf",130);
    mainScreen->SetFont(normalFont);
    mainScreen->GetStyle().mTexture = graphics->TextureLoadPNG("./bg-pastal-01.png");

    // Use dependency injection to pass events onto the controls.
    // This means that we don't need a circular header dependency that can make it hard to port code.
    // I do not want graphics.h including element.h as element.h already includes graphics.h
    auto touchEventHandler = [mainScreen](int32_t pX,int32_t pY,bool pTouched)
    {
        return mainScreen->TouchEvent(pX,pY,pTouched);
    };

    eui::LayoutGrid* grid = eui::LayoutGrid::Create(mainScreen,3,3);

    MakeClock(grid->GetCell(0,0),bigFont,normalFont,miniFont);
    MakeBitcoinPrice(grid->GetCell(1,0),bigFont,normalFont,miniFont);

    eui::LayoutGrid* status = eui::LayoutGrid::Create(grid->GetCell(2,0),1,2);
    MakeSystemStatus(status->GetCell(0,0),bigFont,normalFont,miniFont);
    MakeEnvironmentStatus(status->GetCell(0,1),bigFont,normalFont,miniFont);


    // MQTT data
    const std::vector<std::string>& topics = {"/outside/temperature","/outside/hartbeat"};
    MQTTData MQTT("server",1883,topics,[](const std::string &pTopic,const std::string &pData)
    {
        outsideData[pTopic] = pData;
        outsideTemperatureDelivered = std::time(nullptr);
    });
    
    while( graphics->ProcessSystemEvents(touchEventHandler) )
    {

        mainScreen->Update();

        graphics->BeginFrame();

        mainScreen->Draw(graphics);

        graphics->EndFrame();
    }

    indoorAirQuality.Stop();
    delete mainScreen;
    eui::Graphics::Close();
}
