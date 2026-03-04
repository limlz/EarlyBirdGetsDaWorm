// ============================================================
// DOORS MODULE
// Handles all hallway door systems:
//
// - Renders door sprites + window base overlay
// - Tracks which door the player is standing near
// - Drives door anomalies (handprint slam, eyes blink, shadow walk)
// - Draws door label text (floor-door numbering)
// ============================================================

#include "pch.hpp"
#include <ctime>     // time
#include <cstdlib>   // rand, srand
#include <algorithm> // std::max (if needed later)

// ============================================================
// CONFIG
// ============================================================

// Door size in WORLD units
static constexpr float DOOR_W = DOOR_WIDTH;
static constexpr float DOOR_H = DOOR_HEIGHT;

// Compute door Y so door sits nicely on the floor line.
// DrawTextureMesh uses center position.
static float ComputeDoorY()
{
    const float floorLineY = (-FLOOR_CENTER_Y) + (FLOOR_HEIGHT * 0.5f);
    return floorLineY + (DOOR_H * 0.5f);
}
static const float DOOR_Y = ComputeDoorY();

// ------------------------------
// WINDOW PLACEMENT (normalized in door space)
// ------------------------------
// u = 0.5 means centered horizontally
// v = 0.5 means centered vertically
// v > 0.5 moves it UP, v < 0.5 moves it DOWN
static constexpr float WINDOW_CENTER_U = 0.50f;
static constexpr float WINDOW_CENTER_V = 0.62f;

// Window size relative to door size
static constexpr float WINDOW_SIZE_U = 0.45f;
static constexpr float WINDOW_SIZE_V = 0.18f;

// Derived world sizes
static constexpr float WINDOW_W = DOOR_W * WINDOW_SIZE_U;
static constexpr float WINDOW_H = DOOR_H * WINDOW_SIZE_V;

// ------------------------------
// EVENT TRIGGER TUNING
// ------------------------------
static constexpr float EVENT_ROLL_COOLDOWN = 2.0f; // seconds between "roll attempts"
static constexpr int   EVENT_CHANCE_PERCENT = 35;   // % chance to trigger per roll

// ============================================================
// RENDER DATA (Texture + Mesh + Font)
// ============================================================

static AEGfxVertexList* gQuadMesh = nullptr;
static s8               gDoorFontId = -1;

// Door base
static AEGfxTexture* gDoorTex = nullptr;

// Window base (always drawn)
static AEGfxTexture* gWindowBaseTex = nullptr;

// Anomaly overlays
static AEGfxTexture* gHandprintTex = nullptr;
static AEGfxTexture* gShadowTex = nullptr;

// ============================================================
// STATE
// ============================================================

enum class DoorEvent : int
{
    None = 0,
    HandprintSlam,
    EyesBlink,
    ShadowWalk
};

// --- Handprint "SLAM" ---
struct HandprintSlamState
{
    bool  active = false;
    float t = 0.0f;
    float duration = 0.55f;
};

// --- Eyes blink ---
// (Note: your current Animate() doesn't update this yet, so it won't show.
// We'll keep it as-is for now, but I can add the blink update/draw after.)
struct EyesBlinkState
{
    bool  active = false;
    float t = 0.0f;
    float duration = 1.20f;
    float blinkTimer = 0.0f;
    bool  eyesOn = false;
};

// --- Shadow walk ---
struct ShadowWalkState
{
    bool  active = false;
    float offsetX = 0.0f;    // relative to window center
    float speed = 220.0f;
};

struct DoorEventState
{
    DoorEvent event = DoorEvent::None;

    float rollCooldown = 0.0f; // time until allowed to roll again
    float eventCooldown = 0.0f; // time until a NEW event can start

    HandprintSlamState slam;
    EyesBlinkState     eyes;
    ShadowWalkState    shadow;
};

static DoorEventState gDoorState[NUM_DOORS]{};

// ============================================================

// Basement door array

struct BasementCheck
{
    bool lockedThisDay = false;
};

static BasementCheck basementDoors [10];

void Doors_ResetAllLocks()
{
    for (int d = 0; d < NUM_DOORS; ++d) {
        basementDoors[d].lockedThisDay = false;
    }

}

bool Doors_TryDisposal(int floorNum, int doorIdx)
{
    // Safety check for bounds
    if (doorIdx < 0 || doorIdx >= NUM_DOORS) return false;
    BasementCheck& s = basementDoors[doorIdx];

    // 1. Is it already locked?
    if (s.lockedThisDay) {
        // You could trigger a 'jammed' sound here
        return false;
    }

    if ((rand() % 100) < 30) { // 30% Chance
        // Trigger your existing JumpScare logic
        JumpScare_Start();
        s.lockedThisDay = true; // Lock the door even if it was just a scare
        return false;
    }

    // 3. If we got here, no jumpscare happened. 
    // Return true to tell Game.cpp to check the Patient Identity.
    s.lockedThisDay = true; // Lock the door for the day
    return true;
}

// ==================================================
// HELPERS
// ============================================================

static int ClampFloor(int f)
{
    if (f < 0)  return 0;
    if (f >= 10) return 9;
    return f;
}

static float DoorWorldX(int doorIndex, float camX)
{
    return DIST_BETWEEN_DOORS + camX + (DIST_BETWEEN_DOORS * doorIndex);
}

static void WindowWorldCenter(float doorX, float& outWinX, float& outWinY)
{
    const float winOffsetX = (WINDOW_CENTER_U - 0.5f) * DOOR_W;
    const float winOffsetY = (WINDOW_CENTER_V - 0.5f) * DOOR_H;

    outWinX = doorX + winOffsetX;
    outWinY = DOOR_Y + winOffsetY;
}

static void ClearDoorEvent(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];

    s.event = DoorEvent::None;
    s.slam = HandprintSlamState{};
    s.eyes = EyesBlinkState{};
    s.shadow = ShadowWalkState{};
}

static void TriggerHandprintSlam(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::HandprintSlam;
    s.slam.active = true;
    s.slam.t = 0.0f;
}

static void TriggerEyesBlink(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::EyesBlink;
    s.eyes.active = true;
    s.eyes.t = 0.0f;
    s.eyes.blinkTimer = 0.0f;
    s.eyes.eyesOn = false;
}

static void TriggerShadowWalk(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::ShadowWalk;
    s.shadow.active = true;

    // Start off left side of window
    s.shadow.offsetX = -(WINDOW_W * 0.5f) - (WINDOW_W * 0.6f);

    // Random speed
    s.shadow.speed = 200.0f + (rand() % 120);
}

static void SeedRNGOnce()
{
    static bool seeded = false;
    if (!seeded)
    {
        srand((unsigned)std::time(nullptr));
        seeded = true;
    }
}

// ============================================================
// LOAD / INITIALIZE / UNLOAD
// ============================================================

void Doors_Load()
{
    gDoorFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

    gDoorTex = LoadTextureChecked(Assets::Background::DoorBg);
    gWindowBaseTex = LoadTextureChecked(Assets::Door_Anomaly::WindowBase);
    gHandprintTex = LoadTextureChecked(Assets::Door_Anomaly::Handprint);
    gShadowTex = LoadTextureChecked(Assets::Door_Anomaly::Shadow);
}

void Doors_Initialize()
{
    SeedRNGOnce();

    gQuadMesh = CreateSquareMesh(0xFFFFFFFF);

    // Reset all per-door event states
    for (int d = 0; d < NUM_DOORS; ++d)
        gDoorState[d] = DoorEventState{};
}

void Doors_Unload()
{
    FreeMeshSafe(gQuadMesh);

    UnloadTextureSafe(gDoorTex);
    UnloadTextureSafe(gWindowBaseTex);
    UnloadTextureSafe(gHandprintTex);
    UnloadTextureSafe(gShadowTex);

    if (gDoorFontId >= 0)
    {
        AEGfxDestroyFont(gDoorFontId);
        gDoorFontId = -1;
    }
}

// ============================================================
// UPDATE
// - Returns the door index the player is in front of, else -1
// ============================================================

int Doors_Update(f32 camX)
{
    const float detectionRange = DOOR_W * 0.5f;

    for (int i = 0; i < NUM_DOORS; ++i)
    {
        const float doorX = DoorWorldX(i, camX);

        // Player is at screen center x=0; detect door centered near screen
        if (doorX > -detectionRange + 50 && doorX < detectionRange + 50)
            return i;
    }
    return -1;
}

// ============================================================
// ANIMATE / EVENTS
// - Updates cooldowns and runs/starts anomalies for the nearby door
// ============================================================

void Doors_Animate(float dt, int doorNearPlayer, float /*camX*/)
{
    // 1) Tick cooldown timers for all doors
    for (int d = 0; d < NUM_DOORS; ++d)
    {
        DoorEventState& s = gDoorState[d];
        if (s.rollCooldown > 0.0f)  s.rollCooldown -= dt;
        if (s.eventCooldown > 0.0f) s.eventCooldown -= dt;
    }

    // Only trigger/update events for the door you're near
    if (doorNearPlayer < 0 || doorNearPlayer >= NUM_DOORS)
        return;

    DoorEventState& s = gDoorState[doorNearPlayer];

    // --------------------------------------------------
    // 2) Update currently active event (if any)
    // --------------------------------------------------

    // A) Handprint SLAM
    if (s.event == DoorEvent::HandprintSlam && s.slam.active)
    {
        s.slam.t += dt;

        if (s.slam.t >= s.slam.duration)
        {
            ClearDoorEvent(doorNearPlayer);
            s.eventCooldown = 1.5f;
        }
        return; // do not start new event while active
    }

    // B) Shadow Walk
    if (s.event == DoorEvent::ShadowWalk && s.shadow.active)
    {
        s.shadow.offsetX += s.shadow.speed * dt;

        const float finish = (WINDOW_W * 0.5f) + (WINDOW_W * 0.6f);
        if (s.shadow.offsetX > finish)
        {
            ClearDoorEvent(doorNearPlayer);
            s.eventCooldown = 2.0f;
        }
        return;
    }

    // (EyesBlink is currently not animated in your original code — left untouched)

    // --------------------------------------------------
    // 3) No event active: attempt to roll a new one
    // --------------------------------------------------

    if (s.eventCooldown > 0.0f) return;
    if (s.rollCooldown > 0.0f) return;

    s.rollCooldown = EVENT_ROLL_COOLDOWN;

    const int roll = rand() % 100;
    if (roll >= EVENT_CHANCE_PERCENT)
        return;

    const int pick = rand() % 3;
    switch (pick)
    {
    case 0: TriggerHandprintSlam(doorNearPlayer); break;
    case 1: TriggerEyesBlink(doorNearPlayer);     break;
    case 2: TriggerShadowWalk(doorNearPlayer);    break;
    }
}

// ============================================================
// DRAW
// ============================================================

void Doors_Draw(f32 camX, s8 floorNum, f32 textXoffset, f32 textY, bool dementia)
{
    if (!gQuadMesh) return;

    const int f = ClampFloor((int)floorNum);
    (void)f; // reserved for future floor-based behavior

    const int max_doors = dementia ? 1000 : NUM_DOORS;

    for (int i = 0; i < max_doors; ++i)
    {
        const float doorX = DoorWorldX(i, camX);

        // Simple culling
        if (doorX < -SCREEN_WIDTH_HALF - DOOR_W) continue;
        if (doorX > SCREEN_WIDTH_HALF + DOOR_W) continue;

        const int doorIdx = (i % NUM_DOORS);

        // --------------------------------------------
        // 1) DOOR BASE
        // --------------------------------------------
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        if (gDoorTex)
            DrawTextureMesh(gQuadMesh, gDoorTex, doorX, DOOR_Y, DOOR_W, DOOR_H, 1.0f);
        else
            DrawSquareMesh(gQuadMesh, doorX, DOOR_Y, DOOR_W, DOOR_H, 0xFF5B3A20);

        // --------------------------------------------
        // 2) WINDOW CENTER (world space)
        // --------------------------------------------
        float winX{}, winY{};
        WindowWorldCenter(doorX, winX, winY);

        // --------------------------------------------
        // 3) WINDOW BASE (same for all doors)
        // --------------------------------------------
        DrawSquareMesh(gQuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        if (gWindowBaseTex)
            DrawTextureMesh(gQuadMesh, gWindowBaseTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

        // --------------------------------------------
        // 4) EVENT ANOMALIES (on top of window)
        // --------------------------------------------
        DoorEventState& s = gDoorState[doorIdx];

        // A) Handprint SLAM
        if (s.event == DoorEvent::HandprintSlam && s.slam.active && gHandprintTex)
        {
            const float t = s.slam.t;
            const float slamIn = 0.12f;
            const float fadeOut = 0.20f;

            float scale = 1.0f;
            float alpha = 1.0f;

            if (t < slamIn)
            {
                const float k = t / slamIn;           // 0..1
                scale = 1.35f - 0.35f * k;            // 1.35 -> 1.0
                alpha = 0.20f + 0.80f * k;            // 0.2  -> 1.0
            }
            else if (t > s.slam.duration - fadeOut)
            {
                const float k = (t - (s.slam.duration - fadeOut)) / fadeOut;
                alpha = 1.0f - k;
            }

            DrawTextureMesh(gQuadMesh, gHandprintTex,
                winX, winY,
                WINDOW_W * scale, WINDOW_H * scale,
                alpha);
        }

        // B) Shadow Walk
        if (s.event == DoorEvent::ShadowWalk && s.shadow.active && gShadowTex)
        {
            const float sx = winX + s.shadow.offsetX;

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            DrawTextureMesh(gQuadMesh, gShadowTex,
                sx, winY,
                WINDOW_W * 0.95f, WINDOW_H * 0.95f,
                0.85f);
        }

        // (EyesBlink draw is not present in your current code — can add after)

        // --------------------------------------------
        // 5) DOOR NUMBER TEXT
        // --------------------------------------------
        if (gDoorFontId >= 0)
        {
            char textBuffer[32]{};

            if (floorNum == 0)
                sprintf_s(textBuffer, "B1-%02d", i + 1);
            else
                sprintf_s(textBuffer, "%02d-%02d", floorNum, i + 1);

            const float textNDC_X = (doorX / SCREEN_WIDTH_HALF) - textXoffset;
            const float textNDC_Y = textY / SCREEN_HEIGHT_HALF;

            AEGfxPrint(gDoorFontId, textBuffer,
                textNDC_X, textNDC_Y,
                1.0f,
                1.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}