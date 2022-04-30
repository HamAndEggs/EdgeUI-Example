#include "Graphics.h"
#include "Element.h"
#include "LayoutGrid.h"

#include <iostream>
#include <chrono>
#include <time.h>

eui::Element* MakeClock(eui::LayoutGrid* grid,int pFont)
{
    eui::Element* clock = eui::Element::Create();
    clock->SetPadding(0.05f);
    clock->GetStyle().mAlignment = eui::ALIGN_CENTER_TOP;
    clock->SetFont(pFont);

    clock->SetOnUpdate([](eui::Element* pElem)
    {
        std::time_t result = std::time(nullptr);
        tm *currentTime = localtime(&result);
        if( currentTime )
        {
            pElem->SetTextF("%02d:%02d",currentTime->tm_hour,currentTime->tm_min);
        }
        else
        {
            pElem->SetText("NO CLOCK");
        }
        return true;
    });


    grid->GetCell(0,0)->Attach(clock);

    eui::Style s;
    s.mBackground = eui::COLOUR_BLACK;
    s.mBorderSize = 5.0f;
    s.mBorder = eui::COLOUR_WHITE;
    s.mRadius = 15.0f;
    grid->GetCell(0,0)->SetStyle(s);

    return clock;
}

int main(int argc, char *argv[])
{
    eui::Graphics* graphics = eui::Graphics::Open();
//    graphics->FontSetMaximumAllowedGlyph(256);

    eui::Element* mainScreen = eui::Element::Create();
    mainScreen->SetID("mainScreen");

    int normalFont = graphics->FontLoad("./liberation_serif_font/LiberationSerif-Regular.ttf",52);
    int bigFont = graphics->FontLoad("./liberation_serif_font/LiberationSerif-Bold.ttf",130);
    mainScreen->SetFont(normalFont);

    // Use dependency injection to pass events onto the controls.
    // This means that we don't need a circular header dependency that can make it hard to port code.
    // I do not want graphics.h including element.h as element.h already includes graphics.h
    auto touchEventHandler = [mainScreen](int32_t pX,int32_t pY,bool pTouched)
    {
        return mainScreen->TouchEvent(pX,pY,pTouched);
    };

    eui::LayoutGrid* grid = eui::LayoutGrid::Create(mainScreen,3,3);
    grid->SetPadding(0.01f);
    eui::Element* clock = MakeClock(grid,bigFont);


//    eui::Element* theClock = Clock::Allocate(graphics);
//    grid->

/*        mainScreen.AddButton(0,0,300,100,"Exit",
        [](eui::Element& pElement)
        {
            pElement.GetGraphics()->SetExitRequest();
            return true;
        })->SetRadius(40);*/

    
    while( graphics->ProcessSystemEvents(touchEventHandler) )
    {

        mainScreen->Update();

        graphics->BeginFrame();

        mainScreen->Draw(graphics);

        graphics->EndFrame();
    }

    delete mainScreen;
    eui::Graphics::Close();
}
