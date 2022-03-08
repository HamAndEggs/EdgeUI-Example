#include "Graphics.h"
#include "Element.h"

#include <iostream>

int main(int argc, char *argv[])
{
    eui::GraphicsPtr graphics = eui::Graphics::Open();

    eui::Element mainScreen(graphics);
    mainScreen.SetArea(graphics->GetDisplayRect());
    mainScreen.SetBackground(eui::COLOUR_GREY);
    mainScreen.SetPadding(10);

    mainScreen.AddButton(0,0,300,100,"Exit",
        [](eui::Element& pElement)
        {
            pElement.GetGraphics()->SetExitRequest();
            return true;
        })->SetRadius(40);

    // Use dependency injection to pass events onto the controls.
    // This means that we don't need a circular header dependency that can make it hard to port code.
    // I do not want graphics.h including element.h as element.h already includes graphics.h
    auto touchEventHandler = [&mainScreen](int32_t pX,int32_t pY,bool pTouched)
    {
        return mainScreen.TouchEvent(pX,pY,pTouched);
    };

    float rot = 0.0f;
    while( graphics->ProcessSystemEvents(touchEventHandler) )
    {
        rot += 0.01f;
		if( rot >= 360.0f )
		{
			rot -= 360.0f;
		}

        mainScreen.Update();

        graphics->BeginFrame();

        mainScreen.Draw();

        eui::Rectangle r(120,120,400,200);
        eui::Style s(eui::COLOUR_GREEN,25);
        s.mBorder = eui::SetAlpha(eui::COLOUR_BLACK,100);
        s.mBorderSize = 15;
        graphics->DrawRectangle(r,s);

        // Fat line
        {
			const int x = (graphics->GetDisplayWidth()*10)/14;
			const int y = (graphics->GetDisplayHeight()*1)/4;

            float angle = rot;
            const float angleStep = eui::DegreeToRadian(180.0f);

            int sx = x + ((int)(std::cos(angle) * 70.0f));
            int sy = y + ((int)(std::sin(angle) * 70.0f));

            angle += angleStep;

            int ex = x + ((int)(std::cos(angle) * 70.0f));
            int ey = y + ((int)(std::sin(angle) * 70.0f));

            graphics->DrawLine(sx,sy,ex,ey,eui::COLOUR_BLACK,11);
            graphics->DrawLine(sx,sy,ex,ey,eui::COLOUR_WHITE,5);
        }

        graphics->EndFrame();        
    }
}
