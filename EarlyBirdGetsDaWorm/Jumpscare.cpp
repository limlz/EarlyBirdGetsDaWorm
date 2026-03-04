// ============================================================
// JUMPSCARE MODULE
// Handles the full-screen jumpscare effect:
//
// - Triggers jumpscare (press J to test)
// - Updates screen shake + image shake + optional flash
// - Draws motion-blur trails + jumpscare texture (top layer)
// - Provides shake offset (optional camera offset use)
// ============================================================

#include "pch.hpp"
#include <cstdlib>   // rand, srand, RAND_MAX
#include <ctime>     // time
#include <algorithm> // std::max

// ============================================================
// CONFIG
// ============================================================

static constexpr float JUMPSCARE_DURATION = 1.0f;   // seconds
static constexpr float SCREEN_SHAKE_POWER = 35.0f;  // pixels (try 20-60)
static constexpr float IMAGE_SHAKE_POWER = 18.0f;   // pixels (try 10-30)
static constexpr float FLASH_DURATION = 0.08f;      // seconds (optional quick flash)

// ============================================================
// RENDER DATA (Texture + Mesh)
// ============================================================

static AEGfxVertexList* gQuadMesh = nullptr;    // quad mesh for drawing texture fullscreen
static AEGfxTexture* gJumpScareTex = nullptr;   // jumpscare image

// ============================================================
// STATE
// ============================================================

static bool  gActive = false;
static float gTimer = 0.0f;

static float gShakeX = 0.0f;   // screen/camera shake offset
static float gShakeY = 0.0f;

static float gFlashTimer = 0.0f;   // optional flash at start

// ============================================================
// HELPERS
// ============================================================

static float Clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float RandMinus1To1()
{
    // [-1, 1]
    return (float(rand()) / float(RAND_MAX)) * 2.0f - 1.0f;
}

// ============================================================
// LOAD / INIT / UNLOAD
// Call from Game_Load / Game_Initialize / Game_Unload
// ============================================================

void JumpScare_Load()
{
    LoadTextureChecked(Assets::Background::JumpScare);
}

void JumpScare_Initialize()
{
    // Seed random once
    static bool seeded = false;
    if (!seeded)
    {
        srand((unsigned)time(nullptr));
        seeded = true;
    }

    // Reset runtime state
    gActive = false;
    gTimer = 0.0f;
    gShakeX = 0.0f;
    gShakeY = 0.0f;
    gFlashTimer = 0.0f;
}

void JumpScare_Unload()
{
    UnloadTextureSafe(gJumpScareTex);
    FreeMeshSafe(gQuadMesh);

    gActive = false;
    gTimer = 0.0f;
    gShakeX = 0.0f;
    gShakeY = 0.0f;
    gFlashTimer = 0.0f;
}

// ============================================================
// TRIGGER
// ============================================================

void JumpScare_Start()
{
    gActive = true;
    gTimer = JUMPSCARE_DURATION;

    gShakeX = 0.0f;
    gShakeY = 0.0f;

    gFlashTimer = FLASH_DURATION; // optional quick flash at start
}

// ============================================================
// UPDATE
// Call inside Game_Update(dt)
// Return true if you want to freeze gameplay while active
// ============================================================

bool JumpScare_Update(float dt)
{
    // Press J to test (debug trigger)
    if (!gActive && AEInputCheckTriggered(AEVK_J))
        JumpScare_Start();

    if (!gActive)
        return false;

    // --------------------------------------------
    // 1) Countdown + End condition
    // --------------------------------------------
    gTimer -= dt;
    if (gTimer <= 0.0f)
    {
        gActive = false;
        gTimer = 0.0f;
        gShakeX = 0.0f;
        gShakeY = 0.0f;
        gFlashTimer = 0.0f;
        return false; // finished, resume gameplay
    }

    // --------------------------------------------
    // 2) Flash timer
    // --------------------------------------------
    if (gFlashTimer > 0.0f)
    {
        gFlashTimer -= dt;
        if (gFlashTimer < 0.0f) gFlashTimer = 0.0f;
    }

    // --------------------------------------------
    // 3) Screen shake with decay
    //    Strong at start, weaker near the end
    // --------------------------------------------
    float t = Clamp01(gTimer / JUMPSCARE_DURATION); // 1 -> 0
    float s = SCREEN_SHAKE_POWER * t;

    gShakeX = RandMinus1To1() * s;
    gShakeY = RandMinus1To1() * s;

    // If you want to fully freeze gameplay while the jumpscare runs:
    return true;
}

// ============================================================
// SHAKE OFFSET (Optional camera offset hook)
// Use if you want to apply screen shake to world/UI transforms
// ============================================================

void JumpScare_GetShakeOffset(float& outX, float& outY)
{
    outX = (gActive ? gShakeX : 0.0f);
    outY = (gActive ? gShakeY : 0.0f);
}

// ============================================================
// DRAW
// Call at the VERY END of Game_Draw() (top layer)
// ============================================================

void JumpScare_Draw()
{
    if (!gActive) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    const float W = SCREEN_WIDTH_HALF * 2.0f;
    const float H = SCREEN_HEIGHT_HALF * 2.0f;

    // Extra shake for the image itself (separate from screen shake)
    float imgX = RandMinus1To1() * IMAGE_SHAKE_POWER;
    float imgY = RandMinus1To1() * IMAGE_SHAKE_POWER;

    const float baseX = gShakeX + imgX;
    const float baseY = gShakeY + imgY;

    // If texture failed, show a clear fallback
    if (!gJumpScareTex || !gQuadMesh)
    {
        DrawSquareMesh(gQuadMesh, 0.0f, 0.0f, W, H, 0xFFFFFFFF);
        return;
    }

    // --------------------------------------------
    // 1) Fake motion blur trails
    // --------------------------------------------
    DrawTextureMesh(gQuadMesh, gJumpScareTex, baseX - 10.0f, baseY, W, H, 0.20f);
    DrawTextureMesh(gQuadMesh, gJumpScareTex, baseX + 10.0f, baseY, W, H, 0.20f);
    DrawTextureMesh(gQuadMesh, gJumpScareTex, baseX, baseY - 8.0f, W, H, 0.20f);

    // --------------------------------------------
    // 2) Main image
    // --------------------------------------------
    DrawTextureMesh(gQuadMesh, gJumpScareTex, baseX, baseY, W, H, 1.0f);

    // --------------------------------------------
    // 3) Optional flash overlay (quick white flash)
    // --------------------------------------------
    if (gFlashTimer > 0.0f)
    {
        float a = Clamp01(gFlashTimer / FLASH_DURATION); // 1 -> 0
        // If your DrawSquareMesh supports alpha in the color, use your own helper.
        // Otherwise you can draw a semi-transparent white using AEGfxSetTransparency.
        AEGfxSetTransparency(0.65f * a);
        DrawSquareMesh(gQuadMesh, 0.0f, 0.0f, W, H, 0xFFFFFFFF);
        AEGfxSetTransparency(1.0f);
    }
}

// ============================================================
// GETTERS
// ============================================================

bool JumpScare_IsActive()
{
    return gActive;
}