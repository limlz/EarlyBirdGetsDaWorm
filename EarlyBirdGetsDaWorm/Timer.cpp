// ============================================================================
// TIMER.cpp
// - Accelerated in-game clock (minutes stored as float)
// - Pause/Resume + Reset hooks for game states
// - Day transition overlay (hold black + fade out "DAY X")
// ============================================================================

#include "pch.hpp"

// ============================================================================
// CONFIG: In-game clock settings
// ============================================================================
static constexpr int   START_MIN = 0;                   // Start at 00:00 AM (minutes since midnight)
static constexpr int   END_MIN = START_MIN + 60 * 6;    // Shift length = 6 hours (in-game)
static constexpr float MIN_PER_SEC = 3.0f;              // 3 in-game minutes per 1 real second

// ============================================================================
// STATE: Clock runtime values
// ============================================================================
static float gGameMinutes = (float)START_MIN;   // Current time in minutes (float for smooth dt accumulation)
static bool  gTimeUp = false;                   // True when we reach END_MIN
static bool  gTimerPaused = false;              // True when gameplay wants to freeze time (boss room, etc.)
static float gAccum = 0.0f;                     // (Unused right now) could be removed if not needed

// UI fonts (int to avoid s8 handle issues)
static int   gClockFontId = -1;         // digital clock font (HH:MM)
static int   gOverlayFontId = -1;       // "DAY X" overlay font

// ============================================================================
// DAY OVERLAY: Full black hold, then fade out with "DAY X"
// ============================================================================
static bool  gDayOverlayActive = false;
static float gDayOverlayAlpha = 0.0f;       // 1 -> 0 fade out (used for both black overlay + text alpha)
static float gDayHoldTimer = 0.0f;          // seconds to hold full black before fading
static int   gOverlayDayNum = 1;            // which day number to display

static constexpr float DAY_HOLD_TIME = 1.0f; // seconds to hold full black
static constexpr float DAY_FADE_SPEED = 1.0f; // alpha decrease per second (1.0 = ~1 sec fade)

// ============================================================================
// LOAD / UNLOAD
// ============================================================================
void Timer_Load()
{
    // load digital clock font
    gOverlayFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 32);
    gClockFontId = AEGfxCreateFont(Assets::Fonts::DigitalClock, 32);
    Timer_Reset();
}

void Timer_Unload()
{
    // Destroy fonts safely
    if (gClockFontId >= 0)
    {
        AEGfxDestroyFont(gClockFontId);
        gClockFontId = -1;
    }

    if (gOverlayFontId >= 0)
    {
        AEGfxDestroyFont(gOverlayFontId);
        gOverlayFontId = -1;
    }
}

// ============================================================================
// CLOCK UPDATE: Frame-rate independent accelerated time
// ============================================================================
void Timer_Update(float dt)
{
    // Do not advance time if shift ended or timer is paused
    if (gTimeUp || gTimerPaused)
        return;

    // Accelerate game time: minutes-per-second * real delta time
    gGameMinutes += MIN_PER_SEC * dt;

    // Clamp + flag "time up"
    if (gGameMinutes >= (float)END_MIN)
    {
        gGameMinutes = (float)END_MIN;
        gTimeUp = true;
    }
}

// ============================================================================
// CLOCK DRAW: Converts minutes -> HH:MM and prints to screen (NDC coords)
// ============================================================================
void Timer_Draw(float ndcX, float ndcY)
{
    // Convert float minutes to integer minutes for display
    int totalMinutes = (int)gGameMinutes;

    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;

    char timeBuf[16]{};
    sprintf_s(timeBuf, "%02d:%02d AM", hours, minutes); // digital clock format

    if (gClockFontId >= 0)
        AEGfxPrint(gClockFontId, timeBuf, ndcX, ndcY, 1, 1, 1, 1, 1);
}

// ============================================================================
// STATUS / CONTROL (used by Game.cpp / other systems)
// ============================================================================
int Timer_GetGameMinutes()
{
    // Note: returns int minutes (float gets truncated)
    return (int)gGameMinutes;
}

bool Timer_IsTimeUp()
{
    return gTimeUp;
}

void Timer_SetPaused(bool paused)
{
    gTimerPaused = paused;
}

bool Timer_IsPaused()
{
    return gTimerPaused;
}

void Timer_Reset()
{
    gGameMinutes = (float)START_MIN;
    gTimeUp = false;
    gAccum = 0.0f; // currently unused
}

// Debug helper: set the current time in minutes (for fast testing)
void Timer_DebugSetTime(float minutes)
{
    gGameMinutes = minutes;
    if (gGameMinutes >= (float)END_MIN)
    {
        gGameMinutes = (float)END_MIN;
        gTimeUp = true;
    }
    else
    {
        gTimeUp = false;
    }
}

// ============================================================================
// DAY OVERLAY CONTROL
// ============================================================================
void Timer_StartDayOverlay(int dayNum)
{
    gOverlayDayNum = dayNum;

    // BOOM: start fully black instantly
    gDayOverlayAlpha = 1.0f;
    gDayHoldTimer = DAY_HOLD_TIME;
    gDayOverlayActive = true;
}

bool Timer_IsDayOverlayActive()
{
    return gDayOverlayActive;
}

// Overlay update:
// 1) hold full black for DAY_HOLD_TIME
// 2) fade alpha down to 0
void Timer_UpdateDayOverlay(float dt)
{
    if (!gDayOverlayActive)
        return;

    // Phase 1: hold full black
    if (gDayHoldTimer > 0.0f)
    {
        gDayHoldTimer -= dt;
        if (gDayHoldTimer < 0.0f) gDayHoldTimer = 0.0f;
        return; // still holding, no fading yet
    }

    // Phase 2: fade out
    gDayOverlayAlpha -= DAY_FADE_SPEED * dt;

    if (gDayOverlayAlpha <= 0.0f)
    {
        gDayOverlayAlpha = 0.0f;
        gDayOverlayActive = false;
    }
}

// ============================================================================
// DAY OVERLAY DRAW: Black screen + "DAY X" centered
// ============================================================================
void Timer_DrawDayOverlay(AEGfxVertexList* squareMesh)
{
    if (!gDayOverlayActive)
        return;

    // ----- 1) Draw full-screen black overlay (uses alpha) -----
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(gDayOverlayAlpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    AEMtx33 m;
    AEMtx33Scale(&m, 2000.0f, 2000.0f); // big enough to cover the screen
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // ----- 2) Draw "DAY X" centered (fades with same alpha) -----
    if (gOverlayFontId >= 0)
    {
        std::string text = "DAY " + std::to_string(gOverlayDayNum);
        AEGfxPrintCentered(gOverlayFontId, text, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f, gDayOverlayAlpha);
    }

    // ----- 3) Reset render state (avoid affecting later draws) -----
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}