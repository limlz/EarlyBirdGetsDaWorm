// ============================================================================
// TIMER MODULE
// Manages accelerated in-game time and the day transition overlay:
//
// - Accelerated in-game clock (minutes stored as float)
// - Pause/Resume + Reset hooks for game states
// - Day transition overlay (hold black + fade out "DAY X")
// ============================================================================

#include "pch.hpp"
#include <string>

// ============================================================================
// CONFIG
// ============================================================================

// In-game clock settings
static constexpr int   START_MIN = 0;                 // minutes since midnight
static constexpr int   END_MIN = START_MIN + 60 * 6; // 6 in-game hours
static constexpr float MIN_PER_SEC = 3.0f;              // 3 in-game minutes per 1 real second

// Day overlay settings
static constexpr float DAY_HOLD_TIME = 1.0f; // seconds to hold full black
static constexpr float DAY_FADE_SPEED = 1.0f; // alpha decrease per second (1.0 = ~1 sec fade)

// ============================================================================
// STATE
// ============================================================================

// Clock runtime values
static float gGameMinutes = (float)START_MIN; // float for smooth dt accumulation
static bool  gTimeUp = false;
static bool  gTimerPaused = false;

// (Currently unused, safe to remove if you want)
static float gAccum = 0.0f;

// Fonts (int to avoid s8 issues)
static int gClockFontId = -1; // digital clock font
static int gOverlayFontId = -1; // "DAY X" overlay font

// Day overlay state
static bool  gDayOverlayActive = false;
static float gDayOverlayAlpha = 0.0f;  // 1 -> 0 fade out
static float gDayHoldTimer = 0.0f;  // hold time before fade
static int   gOverlayDayNum = 1;     // which day number to display

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void ClampAndCheckTimeUp()
{
    if (gGameMinutes >= (float)END_MIN)
    {
        gGameMinutes = (float)END_MIN;
        gTimeUp = true;
    }
}

// ============================================================================
// LOAD / UNLOAD
// ============================================================================

void Timer_Load()
{
    // Fonts
    gOverlayFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 32);
    gClockFontId = AEGfxCreateFont(Assets::Fonts::DigitalClock, 32);

    // Start fresh
    Timer_Reset();
}

void Timer_Unload()
{
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
// CLOCK CONTROL
// ============================================================================

void Timer_Reset()
{
    gGameMinutes = (float)START_MIN;
    gTimeUp = false;
    gAccum = 0.0f;

    // Optional: you can choose to also clear overlay here, but leaving as-is
    // so Game.cpp can control overlay separately.
}

void Timer_SetPaused(bool paused)
{
    gTimerPaused = paused;
}

bool Timer_IsPaused()
{
    return gTimerPaused;
}

bool Timer_IsTimeUp()
{
    return gTimeUp;
}

int Timer_GetGameMinutes()
{
    // Returns truncated minutes
    return (int)gGameMinutes;
}

// Debug helper: set current time directly for testing
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
// CLOCK UPDATE
// ============================================================================

void Timer_Update(float dt)
{
    // Stop advancing if shift ended or game wants time frozen
    if (gTimeUp || gTimerPaused)
        return;

    // Accelerate game time (frame-rate independent)
    gGameMinutes += MIN_PER_SEC * dt;

    // Clamp + flag time up
    ClampAndCheckTimeUp();
}

// ============================================================================
// CLOCK DRAW
// ============================================================================

void Timer_Draw(float ndcX, float ndcY)
{
    const int totalMinutes = (int)gGameMinutes;

    const int hours = totalMinutes / 60;
    const int minutes = totalMinutes % 60;

    char timeBuf[16]{};

    // NOTE: This prints "AM" always (your original behavior).
    // If you want a real 12-hour clock + AM/PM, tell me and I’ll swap it.
    sprintf_s(timeBuf, "%02d:%02d AM", hours, minutes);

    if (gClockFontId >= 0)
        AEGfxPrint(gClockFontId, timeBuf, ndcX, ndcY, 1, 1, 1, 1, 1);
}

// ============================================================================
// DAY OVERLAY CONTROL
// ============================================================================

void Timer_StartDayOverlay(int dayNum)
{
    gOverlayDayNum = dayNum;

    // Start fully black instantly
    gDayOverlayAlpha = 1.0f;
    gDayHoldTimer = DAY_HOLD_TIME;
    gDayOverlayActive = true;
}

bool Timer_IsDayOverlayActive()
{
    return gDayOverlayActive;
}

// ============================================================================
// DAY OVERLAY UPDATE
// Phase 1: hold full black for DAY_HOLD_TIME
// Phase 2: fade alpha down to 0
// ============================================================================

void Timer_UpdateDayOverlay(float dt)
{
    if (!gDayOverlayActive)
        return;

    // Phase 1: hold full black
    if (gDayHoldTimer > 0.0f)
    {
        gDayHoldTimer -= dt;
        if (gDayHoldTimer < 0.0f) gDayHoldTimer = 0.0f;
        return;
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
// DAY OVERLAY DRAW
// Draws black screen + centered "DAY X" text
// ============================================================================

void Timer_DrawDayOverlay(AEGfxVertexList* squareMesh)
{
    if (!gDayOverlayActive)
        return;

    // 1) Full-screen black overlay
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(gDayOverlayAlpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    AEMtx33 m;
    AEMtx33Scale(&m, 2000.0f, 2000.0f);
    AEGfxSetTransform(m.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // 2) "DAY X" text (fades with same alpha)
    if (gOverlayFontId >= 0)
    {
        std::string text = "DAY " + std::to_string(gOverlayDayNum);
        AEGfxPrintCentered(gOverlayFontId, text, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f, gDayOverlayAlpha);
    }

    // 3) Reset render state so we don't affect later draws
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}