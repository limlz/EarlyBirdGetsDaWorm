#include "pch.hpp"
#include <cmath>

// ------------------------------
// CONFIG
// ------------------------------
static constexpr int START_MIN  = 3 * 60;    // 03:00 AM
static constexpr int END_MIN    = 6 * 60;      // 06:00 AM
static int   gGameMinutes = START_MIN;      // store as int minutes
static bool  gTimeUp = false;
static int   gFontId = -1;                  // use int to avoid s8 issues
static int   bFontId = -1;                  // use int to avoid s8 issues

// ------------------------------
// TIMER INTERFACE
// ------------------------------
// @brief: Loads the in-game timer
void Timer_Load()
{
    // load digital clock font
    gFontId = AEGfxCreateFont("Assets/DS-DIGI.ttf", 32);
    bFontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
    Timer_Reset();
}

// @brief: Resets the in-game timer
void Timer_Reset()
{
    gGameMinutes = START_MIN;
    gTimeUp = false;
}

// @brief: Updates the in-game timer
void Timer_Update(float dt)
{
    if (gTimeUp) return;

    static float accum = 0.0f;
    accum += dt;

    // 1 real second = 1 in-game minute
    while (accum >= 1.0f)
    {
        accum -= 1.0f;
        gGameMinutes += 1;

        if (gGameMinutes >= END_MIN)
        {
            gGameMinutes = END_MIN;
            gTimeUp = true;
            break;
        }
    }
}

// @brief: Draws the current in-game time at the specified NDC position
void Timer_Draw(float ndcX, float ndcY)
{
    if (gFontId < 0) return;    

    int hours = gGameMinutes / 60;      
    int minutes = gGameMinutes % 60;

	// printing in AM format 
    char timeBuf[16];
    sprintf_s(timeBuf, "%02d:%02d AM", hours, minutes); 

	// draw time string
    AEGfxPrint(gFontId,timeBuf,ndcX, ndcY,1.0f, 1.0f, 1.0f, 1.0f,1.0f);
}

// @brief: Draws the "Shift Over" overlay when time is up
void Timer_DrawShiftOverOverlay(AEGfxVertexList* squareMesh)
{
    // dark overlay
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(0.85f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    AEMtx33 m;
    AEMtx33Scale(&m, 2000.0f, 2000.0f);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // RESET TRANSFORM so UI draws normally
    AEMtx33 identity;
    AEMtx33Identity(&identity);
    AEGfxSetTransform(identity.m);

    // reset tint
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1, 1, 1, 1);

    // boxes
    DrawSquareMesh(squareMesh, 0.0f, 100.0f, 300.0f, 60.0f, COLOR_WHITE);
    DrawSquareMesh(squareMesh, 0.0f, 0.0f, 300.0f, 60.0f, COLOR_WHITE);

    // labels (NDC centered)
    AEGfxPrintCentered(bFontId, std::string("SHIFT OVER"), 0.0f, 0.55f, 1.2f, 1, 1, 1, 1);  // title
	AEGfxPrintCentered(bFontId, std::string("PRESS H"), 0.0f, 0.2f, 1, 1, 0, 0, 1);         // main menu
    AEGfxPrintCentered(bFontId, std::string("USELESS"), 0.0f, -0.03f, 1, 1, 0, 0, 1);       // restart
}


// @brief: Returns whether the in-game time is up
bool Timer_IsTimeUp()
{
    return gTimeUp;
}

// @brief: Returns the current in-game time in minutes since 12:00am
int Timer_GetGameMinutes()
{
    return gGameMinutes;
}

// DEBUG: Skip timer to 5:58 AM
void Timer_DebugSetTime(float minutes)
{
    gGameMinutes = minutes;
}
