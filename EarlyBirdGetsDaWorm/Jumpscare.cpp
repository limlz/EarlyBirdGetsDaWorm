// ============================================================
// JUMPSCARE (Press J to test)
// - Screen shake (camera offset)
// - Popup image shakes more violently
// - Fake motion blur by drawing trails
// ============================================================

#include "pch.hpp"
#include <cstdlib>   // rand
#include <ctime>     // time
#include <algorithm> // std::max

// ------------------------------
// CONFIG
// ------------------------------
static constexpr float JUMPSCARE_DURATION = 1.0f;   // seconds
static constexpr float SCREEN_SHAKE_POWER = 35.0f;  // pixels (try 20-60)
static constexpr float IMAGE_SHAKE_POWER = 18.0f;  // pixels (try 10-30)

// ------------------------------
// STATE
// ------------------------------
static bool  gJumpScareActive = false;
static float gJumpScareTimer = 0.0f;

// shake offsets (applied as "camera offset")
static float gShakeX = 0.0f;
static float gShakeY = 0.0f;

// optional: flash effect at the start
static float gFlashTimer = 0.0f;

// meshes (assume you already have these elsewhere)
static AEGfxVertexList* gJumpQuadMesh = nullptr;   // local quad mesh for jumpscare
static AEGfxTexture* gJumpScareTex = nullptr;   // jumpscare image

// ------------------------------------------------------------
// small helpers
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// INIT / LOAD / UNLOAD
// call these from your Game_Load / Game_Unload
// ------------------------------------------------------------
void JumpScare_Init()
{
    // call once (safe if called multiple times)
    static bool seeded = false;
    if (!seeded)
    {
        srand((unsigned)time(nullptr));
        seeded = true;
    }
}

void JumpScare_Initialize()
{
    gJumpQuadMesh = CreateSquareMesh(0xFFFFFFFF);

    gJumpScareActive = false;
    gJumpScareTimer = 0.0f;
    gShakeX = gShakeY = 0.0f;
    gFlashTimer = 0.0f;
}

void JumpScare_Load()
{
    gJumpScareTex = LoadTextureChecked("Assets/Background/Jumpscare.png");
}

void JumpScare_Unload()
{
    UnloadTextureSafe(gJumpScareTex);
    FreeMeshSafe(gJumpQuadMesh);
}

// ------------------------------------------------------------
// TRIGGER
// ------------------------------------------------------------
void JumpScare_Start()
{
    gJumpScareActive = true;
    gJumpScareTimer = JUMPSCARE_DURATION;

    gShakeX = 0.0f;
    gShakeY = 0.0f;

    gFlashTimer = 0.08f; // quick flash at start (optional)
}

// ------------------------------------------------------------
// UPDATE
// call this inside Game_Update(dt)
// if it returns true -> freeze your normal gameplay update
// ------------------------------------------------------------
bool JumpScare_Update(float dt)
{
    // press J to test
    if (!gJumpScareActive && AEInputCheckTriggered(AEVK_J))
        JumpScare_Start();

    if (!gJumpScareActive)
        return false; // not active, do normal update

    // countdown
    gJumpScareTimer -= dt;
    if (gJumpScareTimer <= 0.0f)
    {
        gJumpScareActive = false;
        gJumpScareTimer = 0.0f;
        gShakeX = gShakeY = 0.0f;
        gFlashTimer = 0.0f;
        return false; // finished, resume normal update
    }

    // flash timer
    if (gFlashTimer > 0.0f)
    {
        gFlashTimer -= dt;
        if (gFlashTimer < 0.0f) gFlashTimer = 0.0f;
    }

    // screen shake with decay (strong at start, weaker near the end)
    float t = Clamp01(gJumpScareTimer / JUMPSCARE_DURATION); // 1 -> 0
    float s = SCREEN_SHAKE_POWER * t;

    gShakeX = RandMinus1To1() * s;
    gShakeY = RandMinus1To1() * s;

    return true; // active: freeze gameplay if you want
}

// ------------------------------------------------------------
// Get current "camera offset" to apply to ALL your drawings
// Use this for world + UI if you want the whole screen to shake.
// ------------------------------------------------------------
void JumpScare_GetShakeOffset(float& outX, float& outY)
{
    outX = gJumpScareActive ? gShakeX : 0.0f;
    outY = gJumpScareActive ? gShakeY : 0.0f;
}

// ------------------------------------------------------------
// DRAW
// call this at the VERY END of your Game_Draw() (top layer)
// ------------------------------------------------------------
void JumpScare_Draw()
{
    if (!gJumpScareActive) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    const float W = SCREEN_WIDTH_HALF * 2.0f;
    const float H = SCREEN_HEIGHT_HALF * 2.0f;

    float imgX = RandMinus1To1() * IMAGE_SHAKE_POWER;
    float imgY = RandMinus1To1() * IMAGE_SHAKE_POWER;

    const float baseX = 0.0f + gShakeX + imgX;
    const float baseY = 0.0f + gShakeY + imgY;

    if (gJumpQuadMesh && gJumpScareTex)
    {
        // blur trails
        DrawTextureMesh(gJumpQuadMesh, gJumpScareTex, baseX - 10.0f, baseY, W, H, 0.20f);
        DrawTextureMesh(gJumpQuadMesh, gJumpScareTex, baseX + 10.0f, baseY, W, H, 0.20f);
        DrawTextureMesh(gJumpQuadMesh, gJumpScareTex, baseX, baseY - 8.0f, W, H, 0.20f);

        // main
        DrawTextureMesh(gJumpQuadMesh, gJumpScareTex, baseX, baseY, W, H, 1.0f);
    }

    if (!gJumpScareTex)
    {
        // If you see a white screen when you press J, it means the texture path is WRONG.
        DrawSquareMesh(gJumpQuadMesh, 0.0f, 0.0f, W, H, 0xFFFFFFFF);
        return;
    }
}