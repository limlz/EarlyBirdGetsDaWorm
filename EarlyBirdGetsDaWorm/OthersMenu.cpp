#include "pch.hpp"

static s8 gFont = -1;

void OthersMenu_Load()
{
    gFont = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
}

void OthersMenu_Initialize() 
{
	// blabla
}

void OthersMenu_Update()
{
    // Press H to go main menu
    if (AEInputCheckTriggered(AEVK_H))
        next = MAIN_MENU;

    // ESC quit
    if (AEInputCheckTriggered(AEVK_ESCAPE))
        next = GAME_STATE;
}

void OthersMenu_Draw()
{
    AEGfxSetBackgroundColor(0, 0, 0);

    if (gFont >= 0)
    {
        AEGfxPrint(gFont, "GAME PAUSED", -0.25f, 0.2f, 1, 1, 1, 1, 1);
        AEGfxPrint(gFont, "PRESS H: MAIN MENU", -0.33f, 0.0f, 1, 1, 1, 1, 1);
        AEGfxPrint(gFont, "PRESS ESC: QUIT", -0.20f, -0.2f, 1, 1, 1, 1, 1);
    }
}

void OthersMenu_Free() 
{
    // blabla
}

void OthersMenu_Unload() 
{
    // blabla
}
