#include "pch.hpp"
#include <cmath>

// ------------------------------
// CONFIG
// ------------------------------
static constexpr int    START_MIN = 0;                      // start at 00:00 AM
static constexpr int    END_MIN = START_MIN + (60 * 6);     // 60 mins * 6 hours
static constexpr float  MIN_PER_SEC = 3.0f;                 // 3 in-game minutes per 1 real second
static float gGameMinutes = START_MIN;

static bool  gTimeUp = false;
static bool  gTimerPaused = false;
static float gAccum = 0.0f;
static int   gFontId = -1;                      // use int to avoid s8 issues
static int   bFontId = -1;                      // use int to avoid s8 issues

// ------------------------------
// DAY OVERLAY (FADE IN/OUT)
// ------------------------------
static bool  gDayOverlayActive  = false;
static float gDayOverlayAlpha   = 0.0f;   // 1 -> 0 fade out
static float gDayHoldTimer      = 0.0f;
static int   gOverlayDayNum     = 1;

static constexpr float DAY_HOLD_TIME    = 1.0f;  // BOOM hold
static constexpr float DAY_FADE_SPEED   = 1.0f;  // alpha per sec (1/2 sec fade)


// ------------------------------
// TIMER INTERFACE
// ------------------------------
// @brief: Loads the in-game timer
void Timer_Load()
{
    // load digital clock font
    gFontId = AEGfxCreateFont(Assets::Fonts::DigitalClock, 32);
    bFontId = AEGfxCreateFont(Assets::Fonts::BuggyInFontsDir, 30);
    Timer_Reset();
}

// @brief: 
// The game clock is stored as a floating-point value in minutes.


void Timer_Unload()
{
    if (gFontId >= 0)
    {
        AEGfxDestroyFont(gFontId);
        gFontId = -1;
    }

    if (bFontId >= 0)
    {
        AEGfxDestroyFont(bFontId);
        bFontId = -1;
    }
}

// Each frame, we advance it by minutesPerSecond × deltaTime.
// Time will move at an accelerated rate.
void Timer_Update(float dt)
{
    if (gTimeUp || gTimerPaused)
        return;

    gGameMinutes += MIN_PER_SEC * dt;

    if (gGameMinutes >= END_MIN)
    {
        gGameMinutes = END_MIN;
        gTimeUp = true;
    }
}


// @brief: Draws the current in-game time at the specified NDC position
void Timer_Draw(float ndcX, float ndcY)
{
    int totalMinutes = static_cast<int>(gGameMinutes);
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;

    char timeBuf[16];
    sprintf_s(timeBuf, "%02d:%02d AM", hours, minutes);
    AEGfxPrint(gFontId, timeBuf, ndcX, ndcY, 1, 1, 1, 1, 1);
}

// ------------------------------
// TIMER STATUS
// ------------------------------
// @brief: Returns the current in-game time in minutes since 3:00am
int Timer_GetGameMinutes()
{
    return gGameMinutes;
}

// @brief: Returns whether the in-game time is up
bool Timer_IsTimeUp()
{
    return gTimeUp;
}

// ------------------------------
// TIMER PAUSE WHEN PLAYER ENTERS ROOM
// ------------------------------
// @brief: Sets whether the timer is paused
void Timer_SetPaused(bool paused)
{
    gTimerPaused = paused;
}

// @brief: Returns whether the timer is paused
bool Timer_IsPaused()
{
    return gTimerPaused;
}

// ------------------------------
// TIMER RESET WHEN NEW DAY STARTS
// ------------------------------
// @brief: Resets the in-game timer
void Timer_Reset()
{
    gGameMinutes = START_MIN;
    gTimeUp = false;
    gAccum = 0.0f;
}

// ------------------------------
// DEBUG: Skip timer to 5:58 AM
// ------------------------------
void Timer_DebugSetTime(float minutes)
{
    gGameMinutes = minutes;
}

// ------------------------------
// DAY OVERLAY (FADE IN/OUT)
// ------------------------------
// @brief: Starts the day overlay for the specified day number
void Timer_StartDayOverlay(int dayNum)
{
    gOverlayDayNum = dayNum;
    gDayOverlayAlpha = 1.0f;      // BOOM full black instantly
    gDayHoldTimer = DAY_HOLD_TIME;
    gDayOverlayActive = true;
}

// @brief: Returns whether the day overlay is active
bool Timer_IsDayOverlayActive()
{
    return gDayOverlayActive;
}

// @brief: Appearing the day overlay
void Timer_UpdateDayOverlay(float dt)
{
    if (!gDayOverlayActive) return;

    // Hold full black first
    if (gDayHoldTimer > 0.0f)
    {
        gDayHoldTimer -= dt;
        if (gDayHoldTimer < 0.0f) gDayHoldTimer = 0.0f;
        return;
    }

    // Fade out only
    gDayOverlayAlpha -= DAY_FADE_SPEED * dt;
    if (gDayOverlayAlpha <= 0.0f)
    {
        gDayOverlayAlpha = 0.0f;
        gDayOverlayActive = false;
    }
}

// @brief: Draws the day overlay (dark screen + "DAY X" text)
void Timer_DrawDayOverlay(AEGfxVertexList* squareMesh)
{
    if (!gDayOverlayActive) return;

    // Black overlay (fade out)
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(gDayOverlayAlpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    AEMtx33 m;
    AEMtx33Scale(&m, 2000.0f, 2000.0f);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // Text (show while overlay is visible)
    if (bFontId >= 0)
    {
        std::string text = "DAY " + std::to_string(gOverlayDayNum);
        float textAlpha = gDayOverlayAlpha;
        AEGfxPrintCentered(bFontId, text, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, textAlpha);
    }

    // reset render state
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}


