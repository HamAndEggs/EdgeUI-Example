/*
   Copyright (C) 2021, Richard e Collins.

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

#include "Style.h"
#include "Graphics.h"
#include "Element.h"
#include "controls/Button.h"
#include "controls/CheckBox.h"
#include "controls/RadioButton.h"
#include "Application.h"

#include <unistd.h>
#include <filesystem>
#include <chrono>

class MyUI : public eui::Application
{
public:
    MyUI(const std::string& path);
    virtual ~MyUI();

    virtual void OnOpen(eui::Graphics* pGraphics);
    virtual void OnClose();
    virtual eui::ElementPtr GetRootElement(){return mRoot;}

private:
    const float CELL_PADDING = 0.02f;
    const float RECT_RADIUS = 0.2f;
    const float BORDER_SIZE = 3.0f;
    const std::string mPath;
    bool mAnimate = false;
    eui::ElementPtr mRoot = nullptr;

    eui::ElementPtr MakeBouncyIcon(eui::Graphics* pGraphics);
    eui::ElementPtr MakeOptions(int pFont);
    eui::ElementPtr MakeRadio(int pFont);

    uint32_t banana = 0;
    uint32_t cherry = 0;
    uint32_t grapes = 0;
    uint32_t orange = 0;

    uint32_t mCurrentImage = 0;
};

MyUI::MyUI(const std::string& path):mPath(path)
{

}

MyUI::~MyUI()
{

}

void MyUI::OnOpen(eui::Graphics* pGraphics)
{

    mRoot = new eui::Element;
    mRoot->SetID("mainScreen");
    mRoot->SetGrid(3,3);

    banana = pGraphics->TextureLoadPNG(mPath + "./icons/banana.png");
    cherry = pGraphics->TextureLoadPNG(mPath + "./icons/cherry.png");
    grapes = pGraphics->TextureLoadPNG(mPath + "./icons/grapes.png");
    orange = pGraphics->TextureLoadPNG(mPath + "./icons/orange.png");
    mCurrentImage = orange;

    int miniFont = pGraphics->FontLoad(mPath + "liberation_serif_font/LiberationSerif-Regular.ttf",25);
    int normalFont = pGraphics->FontLoad(mPath + "liberation_serif_font/LiberationSerif-Regular.ttf",40);
    int largeFont = pGraphics->FontLoad(mPath + "liberation_serif_font/LiberationSerif-Bold.ttf",100);

    mRoot->SetFont(normalFont);
    mRoot->GetStyle().mTexture = pGraphics->TextureLoadPNG(mPath + "TestImage.png");
    mRoot->GetStyle().mBackground = eui::COLOUR_WHITE;
 
    mRoot->Attach((new eui::Button("QUIT",largeFont))->SetOnTouched([this](eui::ElementPtr pElement,float pLocalX,float pLocalY,bool pTouched)
    {
        if( pTouched == false )
        {
            SetExit();
        }
        return true;
    })->SetPadding(0.05f));

    mRoot->Attach(MakeBouncyIcon(pGraphics)->SetPos(1,0));
    mRoot->Attach(MakeOptions(miniFont)->SetPos(2,0));
    mRoot->Attach(MakeRadio(miniFont)->SetPos(0,1));

    mRoot->Attach((new eui::Element)->SetOnUpdate([this](eui::ElementPtr e)
    {
        e->GetStyle().mTexture = mCurrentImage;
        return true;
    })->SetPos(1,1)->SetPadding(0.04f));

}

void MyUI::OnClose()
{
    delete mRoot;
    mRoot = nullptr;
}

eui::ElementPtr MyUI::MakeBouncyIcon(eui::Graphics* pGraphics)
{
    eui::Style s;
    s.mTexture = pGraphics->TextureLoadPNG("./icons/02d.png",true);

    return (new eui::Element(s))->SetOnUpdate([this](eui::ElementPtr e)
    {
        if( mAnimate )
        {
            static float anim = 0.0f;

            anim += 0.1f;

            float y = (std::sin(anim)+1.0f) * 0.1f;
            // Play around with the padding to make it move.
            e->SetPadding(0.0f,1.0f,y,1.0f - (0.2f - y));
        }

        return true;
    });
}

eui::ElementPtr MyUI::MakeOptions(int pFont)
{
    eui::Style s;
    s.mBackground = eui::COLOUR_GREY;
    eui::ElementPtr root = new eui::Element(s);

    root->SetGrid(1,4)->SetPadding(0.05f);
    root->Attach((new eui::Checkbox("Animate Cloud",pFont))->Bind(&mAnimate)->SetPadding(0.05f)->SetPos(0,0));

    root->Attach((new eui::Checkbox("Option 2",pFont))->SetPadding(0.05f)->SetPos(0,1));
    root->Attach((new eui::Checkbox("Option 3",pFont))->SetPadding(0.05f)->SetPos(0,2));
    root->Attach((new eui::Checkbox("Option 4",pFont))->SetPadding(0.05f)->SetPos(0,3));

    return root;
}

eui::ElementPtr MyUI::MakeRadio(int pFont)
{
    eui::Style s;
    s.mBackground = eui::COLOUR_GREY;
   
    eui::RadioButtonGroupPtr group = new eui::RadioButtonGroup;
    group->SetStyle(s);
    group->Add("Banana",banana);
    group->Add("Cherry",cherry);
    group->Add("Grapes",grapes);
    group->Add("Orange",orange);
    group->SetOnPressed([this](eui::RadioButtonPtr pButton,uint32_t pIndex)
    {
        mCurrentImage = pIndex;
    });

    group->SetPressed(banana);

    return group;
}

int main(const int argc,const char *argv[])
{
// Crude argument list handling.
    std::string path = "./";
    if( argc == 2 && std::filesystem::directory_entry(argv[1]).exists() )
    {
        path = argv[1];
        if( path.back() != '/' )
            path += '/';
    }

    MyUI* theUI = new MyUI(path); // MyUI is your derived application class.
    eui::Application::MainLoop(theUI);
    delete theUI;

    return EXIT_SUCCESS;
}
