
#include "Graphics.h"
#include "Element.h"
#include "TinyTools.h"
#include "sgp30.h"
#include "MQTTData.h"
#include "TinyJson.h"

#include <thread>
#include <curl/curl.h>

#include <iostream>
#include <chrono>
#include <time.h>

const float CELL_PADDING = 0.02f;
const float RECT_RADIUS = 0.2f;
const float BORDER_SIZE = 3.0f;

static int CURLWriter(char *data, size_t size, size_t nmemb,std::string *writerData)
{
    if(writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}

class TheClock : public eui::ElementExtension
{
    eui::ElementPtr root = nullptr;
    eui::ElementPtr clock = nullptr;
    eui::ElementPtr dayName = nullptr;
    eui::ElementPtr dayNumber = nullptr;

public:
    operator eui::ElementPtr(){return root;}

    TheClock(int pBigFont,int pNormalFont,int pMiniFont)
    {
        root = eui::Element::Create();
        root->SetID("clock");
        root->SetExtension(this);
        root->SetPos(0,0);

        eui::Style s;
        s.mBackground = eui::COLOUR_BLACK;
        s.mBorderSize = BORDER_SIZE;
        s.mBorder = eui::COLOUR_WHITE;
        s.mRadius = 0.1f;
        root->SetStyle(s);
        root->SetPadding(CELL_PADDING);

        clock = eui::Element::Create();
            clock->SetPadding(0.05f);
            clock->GetStyle().mAlignment = eui::ALIGN_CENTER_TOP;
            clock->SetFont(pBigFont);
        root->Attach(clock);

        dayName = eui::Element::Create();
            dayName->SetPadding(0.05f);
            dayName->GetStyle().mAlignment = eui::ALIGN_LEFT_BOTTOM;
            dayName->SetFont(pNormalFont);
        root->Attach(dayName);

        dayNumber = eui::Element::Create();
            dayNumber->SetPadding(0.05f);
            dayNumber->GetStyle().mAlignment = eui::ALIGN_RIGHT_BOTTOM;
            dayNumber->SetFont(pNormalFont);
        root->Attach(dayNumber);
    }

    virtual bool OnUpdate(eui::ElementPtr pElement)
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
    }
};

class SystemStatus : public eui::ElementExtension
{
    eui::ElementPtr root;
    eui::ElementPtr uptime;
    eui::ElementPtr localIP;
    eui::ElementPtr hostName;
    eui::ElementPtr cpuLoad;
    eui::ElementPtr ramUsed;

    std::map<int,tinytools::system::CPULoadTracking> trackingData;

public:
    operator eui::ElementPtr(){return root;}

    SystemStatus(int pBigFont,int pNormalFont,int pMiniFont)
    {
        root = eui::Element::Create();
        root->SetID("system status");
        root->SetExtension(this);
        root->SetPos(0,0);

        uptime = eui::Element::Create();
            uptime->SetPadding(0.05f);
            uptime->GetStyle().mAlignment = eui::ALIGN_CENTER_TOP;
            uptime->SetFont(pNormalFont);
            uptime->SetText("UP: XX:XX:XX");
        root->Attach(uptime);

        localIP = eui::Element::Create();
            localIP->SetPadding(0.05f);
            localIP->GetStyle().mAlignment = eui::ALIGN_LEFT_CENTER;
            localIP->SetFont(pMiniFont);
            localIP->SetText("XX.XX.XX.XX");
        root->Attach(localIP);

        hostName = eui::Element::Create();
            hostName->SetPadding(0.05f);
            hostName->GetStyle().mAlignment = eui::ALIGN_RIGHT_CENTER;
            hostName->SetFont(pMiniFont);
            hostName->SetText("--------");
        root->Attach(hostName);

        cpuLoad = eui::Element::Create();
            cpuLoad->SetPadding(0.05f);
            cpuLoad->GetStyle().mAlignment = eui::ALIGN_LEFT_BOTTOM;
            cpuLoad->SetFont(pMiniFont);
            cpuLoad->SetText("XX.XX.XX.XX");
        root->Attach(cpuLoad);

        ramUsed = eui::Element::Create();
            ramUsed->SetPadding(0.05f);
            ramUsed->GetStyle().mAlignment = eui::ALIGN_RIGHT_BOTTOM;
            ramUsed->SetFont(pMiniFont);
            ramUsed->SetText("--------");
        root->Attach(ramUsed);

        eui::Style s;
        s.mBackground = eui::COLOUR_BLUE;
        s.mBorderSize = BORDER_SIZE;
        s.mBorder = eui::COLOUR_WHITE;
        s.mRadius = RECT_RADIUS;
        root->SetStyle(s);
        root->SetPadding(CELL_PADDING);

        std::map<int,int> CPULoads;
        int totalSystemLoad;
        tinytools::system::GetCPULoad(trackingData,totalSystemLoad,CPULoads);
    }

    virtual bool OnUpdate(eui::ElementPtr pElement)
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
    }
};

class EnvironmentStatus : public eui::ElementExtension
{
    eui::ElementPtr root;
    eui::ElementPtr eCO2,tOC,outsideTemp;
    i2c::SGP30 indoorAirQuality;
    uint16_t mECO2 = 0;
    uint16_t mTVOC = 0;
    int mResult = i2c::SGP30::READING_RESULT_WARM_UP;
    std::map<std::string,std::string> outsideData;
    std::time_t outsideTemperatureDelivered = 0;

    // MQTT data
    const std::vector<std::string>& topics = {"/outside/temperature","/outside/hartbeat"};
    MQTTData OutsideWeather;

public:
    operator eui::ElementPtr(){return root;}

    EnvironmentStatus(int pBigFont,int pNormalFont,int pMiniFont) : 
        OutsideWeather("server",1883,topics,
            [this](const std::string &pTopic,const std::string &pData)
            {
                outsideData[pTopic] = pData;
                outsideTemperatureDelivered = std::time(nullptr);
            })
    {

        eui::Style s;
        s.mBackground = eui::COLOUR_DARK_GREEN;
        s.mBorderSize = BORDER_SIZE;
        s.mBorder = eui::COLOUR_WHITE;
        s.mRadius = RECT_RADIUS;

        root = eui::Element::Create(s);
        root->SetID("environment status");
        root->SetPadding(CELL_PADDING);
        root->SetExtension(this);
        root->SetPos(0,1);


        eCO2 = eui::Element::Create();
            eCO2->SetPadding(0.05f);
            eCO2->GetStyle().mAlignment = eui::ALIGN_CENTER_TOP;
            eCO2->SetFont(pNormalFont);
            eCO2->SetText("UP: XX:XX:XX");
        root->Attach(eCO2);
        
        tOC = eui::Element::Create();
            tOC->SetPadding(0.05f);
            tOC->GetStyle().mAlignment = eui::ALIGN_CENTER_CENTER;
            tOC->SetFont(pNormalFont);
            tOC->SetText("XX.XX.XX.XX");
        root->Attach(tOC);

        outsideTemp = eui::Element::Create();
            outsideTemp->SetPadding(0.05f);
            outsideTemp->GetStyle().mAlignment = eui::ALIGN_CENTER_BOTTOM;
            outsideTemp->SetFont(pNormalFont);
            outsideTemp->SetText("XX.XC");
        root->Attach(outsideTemp);

        indoorAirQuality.Start([this](int pResult,uint16_t pECO2,uint16_t pTVOC)
        {
            mResult = pResult;
            if( pResult == i2c::SGP30::READING_RESULT_VALID )
            {
                mECO2 = pECO2;
                mTVOC = pTVOC;
            }
        });

        outsideData["/outside/temperature"] = "Waiting";// Make sure there is data.
    }

    virtual bool OnUpdate(eui::ElementPtr pElement)
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
    }

    ~EnvironmentStatus()
    {
        indoorAirQuality.Stop();
    }
};

class BitcoinPrice : public eui::ElementExtension
{
    eui::ElementPtr root;

    int mLastPrice = 0;
    int mPriceChange = 0;
    int m24HourLow = 0;
    int m24HourHigh = 0;

    struct
    {
        eui::ElementPtr LastPrice;
        eui::ElementPtr PriceChange;
        eui::ElementPtr Low;
        eui::ElementPtr High;
    }mControls;

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

public:
    operator eui::ElementPtr(){return root;}

    BitcoinPrice(int pBigFont,int pNormalFont,int pMiniFont)
    {
        root = eui::Element::Create();
        root->SetID("bitcoin");
        root->SetExtension(this);
        root->SetPos(1,0);
        root->SetGrid(2,2);

        eui::Style s;
        s.mBackground = eui::COLOUR_DARK_GREY;
        s.mBorderSize = BORDER_SIZE;
        s.mBorder = eui::COLOUR_WHITE;
        s.mRadius = RECT_RADIUS;
        s.mAlignment = eui::ALIGN_CENTER_CENTER;

        mControls.LastPrice = eui::Element::Create(s);
            mControls.LastPrice->SetPadding(0.05f);
            mControls.LastPrice->SetFont(pNormalFont);
            mControls.LastPrice->SetText("£XXXXXX");
            mControls.LastPrice->SetPadding(CELL_PADDING);
            mControls.LastPrice->SetPos(0,0);
        root->Attach(mControls.LastPrice);

        mControls.PriceChange = eui::Element::Create(s);
            mControls.PriceChange->SetPadding(0.05f);
            mControls.PriceChange->SetFont(pNormalFont);
            mControls.PriceChange->SetText("+XXXXXX");
            mControls.PriceChange->SetPadding(CELL_PADDING);
            mControls.PriceChange->SetPos(0,1);
        root->Attach(mControls.PriceChange);

        mControls.High = eui::Element::Create(s);
            mControls.High->SetPadding(0.05f);
            mControls.High->SetFont(pNormalFont);
            mControls.High->SetText("+XXXXXX");
            mControls.High->SetPadding(CELL_PADDING);
            mControls.High->SetPos(1,0);
        root->Attach(mControls.High);

        mControls.Low = eui::Element::Create(s);
            mControls.Low->SetPadding(0.05f);
            mControls.Low->SetFont(pNormalFont);
            mControls.Low->SetText("£XXXXXX");
            mControls.Low->SetPadding(CELL_PADDING);
            mControls.Low->SetPos(1,1);
        root->Attach(mControls.Low);

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

    ~BitcoinPrice()
    {
        mPriceUpdater.TellThreadToExitAndWait();
    }

    virtual bool OnUpdate(eui::ElementPtr pElement)
    {
        mControls.LastPrice->SetTextF("£%d",mLastPrice);

        std::string growth = std::to_string(mPriceChange);
        if( mPriceChange > 0 )
            growth = "+" + growth;

        mControls.PriceChange->SetText(growth);

        mControls.High->SetTextF("£%d",m24HourHigh);
        mControls.Low->SetTextF("£%d",m24HourLow);
        return true;
    }

};

class WeatherTiles : public eui::ElementExtension
{
    eui::ElementPtr root;
    eui::ElementPtr icons[6];
    int tick = 0;
    float anim = 0;

    std::map<std::string,uint32_t>WeatherIcons;

    const std::vector<std::string> files =
    {
        "01d",
        "01n",
        "02d",
        "02n",
        "03d",
        "03n",
        "04d",
        "04n",
        "09d",
        "09n",
        "10d",
        "10n",
        "11d",
        "11n",
        "13d",
        "13n",
        "50d",
        "50n",
    }; 

    void LoadWeatherIcons(eui::Graphics* graphics)
    {
        for( std::string f : files )
        {
            WeatherIcons[f] = graphics->TextureLoadPNG("./icons/" + f + ".png");
        }
    }

    uint32_t GetRandomIcon()
    {
        return WeatherIcons[files[rand()%files.size()]];
    }


public:
    operator eui::ElementPtr(){return root;}

    WeatherTiles(eui::Graphics* graphics,int pBigFont,int pNormalFont,int pMiniFont)
    {
        LoadWeatherIcons(graphics);

        root = eui::Element::Create();
        root->SetExtension(this);
        root->SetPos(0,1);
        root->SetGrid(6,1);
        root->SetSpan(3,1);
        
        eui::Style s;
        s.mBackground = eui::MakeColour(200,200,200,160);
        s.mBorderSize = BORDER_SIZE;
        s.mBorder = eui::COLOUR_WHITE;
        s.mRadius = RECT_RADIUS;
        s.mAlignment = eui::ALIGN_CENTER_CENTER;
        for( int n = 0 ; n < 6 ; n++ )
        {
            eui::ElementPtr info = eui::Element::Create();
                info->SetStyle(s);
                info->SetPadding(CELL_PADDING);
                icons[n] = eui::Element::Create();
                icons[n]->SetPadding(0.05f);
                info->Attach(icons[n]);
                info->SetPos(n,0);
            root->Attach(info);
        }
    }

    virtual bool OnUpdate(eui::ElementPtr pElement)
    {
        tick++;
        if( tick > 60 )
        {
            tick = 0;
            for(int n = 0 ; n < 6 ; n++ )
            {
                icons[n]->GetStyle().mTexture = GetRandomIcon();
            }
        }

        anim += 0.1f;
        if( anim > eui::GetRadian() )
            anim -= eui::GetRadian();

        for(int n = 0 ; n < 6 ; n++ )
        {
            float Y = sin(anim + (n * 0.7f)) * 0.05f;
            eui::Rectangle r;
            r.left = 0.05f;
            r.right = 1.0f - 0.05f;

            r.top = 0.05f + Y;
            r.bottom = (1.0f - 0.05f) + Y;

            icons[n]->SetPadding(r);
        }

        return true;
    }
};

int main(int argc, char *argv[])
{
    eui::Graphics* graphics = eui::Graphics::Open();
//    graphics->FontSetMaximumAllowedGlyph(256);

    eui::ElementPtr mainScreen = eui::Element::Create();
    mainScreen->SetID("mainScreen");
    mainScreen->SetGrid(3,3);

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

    mainScreen->Attach(*(new TheClock(bigFont,normalFont,miniFont)));
    mainScreen->Attach(*(new BitcoinPrice(bigFont,normalFont,miniFont)));
    mainScreen->Attach(*(new WeatherTiles(graphics,bigFont,normalFont,miniFont)));

    eui::ElementPtr status = eui::Element::Create();
    status->SetGrid(1,2);
    status->SetPos(2,0);
    mainScreen->Attach(status);
    status->Attach(*(new SystemStatus(bigFont,normalFont,miniFont)));
    status->Attach(*(new EnvironmentStatus(bigFont,normalFont,miniFont)));
    
    float a = 0.0f;
    while( graphics->ProcessSystemEvents(touchEventHandler) )
    {
        a += 0.01f;
        const float anim = (sin(a)*0.5f)+0.5f;

        mainScreen->Update();

        graphics->BeginFrame();

        mainScreen->Draw(graphics);

        graphics->EndFrame();
    }
    eui::Graphics::Close();
}
