#include "pch.hpp"
#include <ctime>

// ==================================================
// CONFIG
// ==================================================

// --- Door PNG pixel size (your exported art size) ---
static constexpr float DOOR_W = DOOR_WIDTH;
static constexpr float DOOR_H = DOOR_HEIGHT;

// Door draw position (DrawTextureMesh uses CENTER position)
static float ComputeDoorY()
{
    const float floorLineY = (-FLOOR_CENTER_Y) + (FLOOR_HEIGHT * 0.5f);
    return floorLineY + (DOOR_H * 0.5f);
}

const float DOOR_Y = ComputeDoorY();

// ==================================================
// WINDOW PLACEMENT (normalized in door space)
// u = 0.5 means centered horizontally
// v = 0.5 means centered vertically
// v > 0.5 moves it UP, v < 0.5 moves it DOWN
// ==================================================
static constexpr float WINDOW_CENTER_U = 0.5f;   // middle of door
static constexpr float WINDOW_CENTER_V = 0.62f;  // slightly above middle

// window size relative to door size
static constexpr float WINDOW_SIZE_U = 0.45f;
static constexpr float WINDOW_SIZE_V = 0.18f;

// derived world sizes (auto scales with DOOR_SCALE)
static constexpr float WINDOW_W = DOOR_W * WINDOW_SIZE_U;
static constexpr float WINDOW_H = DOOR_H * WINDOW_SIZE_V;

// ==================================================
// EVENT TRIGGER TUNING
// ==================================================

// When you are near a door, we roll chance to trigger an event.
// Lower = rarer. 35% means: when cooldown allows, we frequently see events.
static constexpr float EVENT_ROLL_COOLDOWN = 2.0f;   // seconds between "roll attempts"
static constexpr int   EVENT_CHANCE_PERCENT = 35;    // % chance to trigger on a roll

// ==================================================
// RENDER DATA
// ==================================================

static AEGfxVertexList* gQuadMesh = nullptr;
static s8 doorFontId = -1;

// Door base
static AEGfxTexture* gDoorTex = nullptr;

// Window base (always drawn)
static AEGfxTexture* gWindowBaseTex = nullptr; // Window_0.png

// Anomaly textures you already have
static AEGfxTexture* gHandprintTex = nullptr;  // handprint overlay (transparent)
static AEGfxTexture* gShadowTex = nullptr;     // shadow silhouette overlay (transparent)

// ==================================================
// ANOMALY STATES (per door)
// ==================================================

enum class DoorEvent : int
{
    None = 0,
    HandprintSlam,
    EyesBlink,
    ShadowWalk
};

// --- Handprint "SLAM" ---
// We fake slam by scaling + alpha quickly, then fade out.
struct HandprintSlamState
{
    bool  active = false;
    float t = 0.0f;         // time since started
    float duration = 0.55f; // total lifetime (tweak)
};

// --- Eyes blink (no PNG) ---
// We draw 2 red squares inside window, blinking on/off for a short time.
struct EyesBlinkState
{
    bool  active = false;
    float t = 0.0f;           // time since started
    float duration = 1.20f;   // total lifetime (tweak)
    float blinkTimer = 0.0f;  // toggles on/off
    bool  eyesOn = false;
};

// --- Shadow walk (uses PNG) ---
struct ShadowWalkState
{
    bool  active = false;
    float offsetX = 0.0f;        // current shadow center X in world space
    float speed = 220.0f;
};

// Everything per door
struct DoorEventState
{
    DoorEvent event = DoorEvent::None;

    float rollCooldown = 0.0f; // time until we are allowed to roll again
    float eventCooldown = 0.0f; // time until we allow a NEW event after one finishes

    HandprintSlamState slam;
    EyesBlinkState     eyes;
    ShadowWalkState    shadow;
};

static DoorEventState gDoorState[NUM_DOORS];

// ==================================================
// HELPERS
// ==================================================

static int ClampFloor(int f)
{
    // you already use MAX_FLOORS in other files; doors doesn’t really need it for events
    // but keep this if you later want floor-based behavior.
    if (f < 0) return 0;
    if (f >= 10) return 9;
    return f;
}

static float DoorWorldX(int doorIndex, float camX)
{
    return DIST_BETWEEN_DOORS + camX + (DIST_BETWEEN_DOORS * doorIndex);
}

static void WindowWorldCenter(float doorX, float& outWinX, float& outWinY)
{
    // Convert normalized (u,v) into world offsets from the door center
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

static void TriggerShadowWalk(int doorIdx, float /*camX*/)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::ShadowWalk;
    s.shadow.active = true;

    // Start off the left of the window (relative space)
    s.shadow.offsetX = -(WINDOW_W * 0.5f) - (WINDOW_W * 0.6f);

    // Random speed
    s.shadow.speed = 200.0f + (rand() % 120);
}

// ==================================================
// PUBLIC API
// ==================================================

void Doors_Load()
{
    doorFontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);

    gDoorTex = LoadTextureChecked("Assets/Background/Door_bg.png");
    gWindowBaseTex = LoadTextureChecked("Assets/Door_Anomaly/Window_0.png");  // Window base always drawn
    gHandprintTex = LoadTextureChecked("Assets/Door_Anomaly/Handprint.png");  // Handprint overlay (transparent PNG)
    gShadowTex = LoadTextureChecked("Assets/Door_Anomaly/Shadow.png");        // Shadow silhouette overlay (transparent PNG)
}

void Doors_Initialize()
{
    gQuadMesh = CreateSquareMesh(0xFFFFFFFF);

    srand((unsigned)std::time(nullptr));

    // reset all door events
    for (int d = 0; d < NUM_DOORS; ++d)
        gDoorState[d] = DoorEventState{};
}

// returns door index player is in front of, else -1
int Doors_Update(f32 camX)
{
    const float detectionRange = DOOR_W * 0.5f;

    for (int i = 0; i < NUM_DOORS; ++i)
    {
        const float doorX = DoorWorldX(i, camX);

        // player is at screen center x=0; detect door centered on screen
        if (doorX > -detectionRange && doorX < detectionRange)
            return i;
    }

    return -1;
}

void Doors_Animate(float dt, int doorNearPlayer, float camX)
{
    // Tick cooldown timers for all doors
    for (int d = 0; d < NUM_DOORS; ++d)
    {
        DoorEventState& s = gDoorState[d];
        if (s.rollCooldown > 0.0f)  s.rollCooldown -= dt;
        if (s.eventCooldown > 0.0f) s.eventCooldown -= dt;
    }

    // We only trigger events for the door you're near (performance + design)
    if (doorNearPlayer < 0 || doorNearPlayer >= NUM_DOORS)
        return;

    DoorEventState& s = gDoorState[doorNearPlayer];

    // --------------------------------------------------
    // 1) Update currently playing anomaly (if any)
    // --------------------------------------------------
    // --- A) Handprint SLAM ---
    if (s.event == DoorEvent::HandprintSlam && s.slam.active)
    {
        s.slam.t += dt;
        if (s.slam.t >= s.slam.duration)
        {
            ClearDoorEvent(doorNearPlayer);
            s.eventCooldown = 1.5f; // after an event ends, wait a bit
        }
        return; // do not start another event while one is active
    }

    // --- B) Shadow walking (PNG silhouette) ---
    if (s.event == DoorEvent::ShadowWalk && s.shadow.active)
    {
        s.shadow.offsetX += s.shadow.speed * dt;

        // Finish when fully past right side (relative)
        const float finish = (WINDOW_W * 0.5f) + (WINDOW_W * 0.6f);
        if (s.shadow.offsetX > finish)
        {
            ClearDoorEvent(doorNearPlayer);
            s.eventCooldown = 2.0f; // after an event ends, wait a bit
        }
        return; // do not start another event while one is active
    }

    // --- C) Red glowing eyes (no PNG) ---
    //if (s.event == DoorEvent::EyesBlink && s.eyes.active)
    //{
    //    s.eyes.t += dt;

    //    // blink toggles (on/off) every ~0.12 sec
    //    s.eyes.blinkTimer += dt;
    //    if (s.eyes.blinkTimer >= 0.25f)
    //    {
    //        s.eyes.blinkTimer = 0.0f;
    //        s.eyes.eyesOn = !s.eyes.eyesOn;
    //    }

    //    if (s.eyes.t >= s.eyes.duration)
    //    {
    //        ClearDoorEvent(doorNearPlayer);
    //        s.eventCooldown = 1.5f; // after an event ends, wait a bit
    //    }
    //    return; // do not start another event while one is active
    //}

    // --------------------------------------------------
    // 2) If no event active: attempt to roll a new one
    // --------------------------------------------------

    // If we just finished an event, wait first
    if (s.eventCooldown > 0.0f)
        return;

    // We don't want to roll every frame; roll every X seconds
    if (s.rollCooldown > 0.0f)
        return;

    // Reset roll cooldown
    s.rollCooldown = EVENT_ROLL_COOLDOWN;

    // Roll the overall chance
    int roll = rand() % 100;
    if (roll >= EVENT_CHANCE_PERCENT)
        return; // nothing happens this time

    // Pick which anomaly to trigger (3 types)
    int pick = rand() % 3; 
    switch (pick)
    {
    case 0: TriggerHandprintSlam(doorNearPlayer);       break;
    case 1: TriggerEyesBlink(doorNearPlayer);           break;
    case 2: TriggerShadowWalk(doorNearPlayer, camX);    break;
    }
}

void Doors_Draw(f32 camX, s8 floorNum, f32 textXoffset, f32 textY, bool dementia)
{
    if (!gQuadMesh) return;

    const int f = ClampFloor((int)floorNum);
    (void)f; // currently unused, but kept if you want floor-based behavior later

    const int max_doors = dementia ? 1000 : NUM_DOORS;

    for (int i = 0; i < max_doors; ++i)
    {
        const float doorX = DoorWorldX(i, camX);

        // culling
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
        // Optional: draw black first to guarantee darkness behind the window art
        DrawSquareMesh(gQuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        if (gWindowBaseTex)
            DrawTextureMesh(gQuadMesh, gWindowBaseTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

        // --------------------------------------------
        // 4) EVENT ANOMALIES (on top of window)
        // --------------------------------------------
        DoorEventState& s = gDoorState[doorIdx];

        // --- A) Handprint SLAM ---
        if (s.event == DoorEvent::HandprintSlam && s.slam.active && gHandprintTex)
        {
            // slam curve:
            // first 0.12s: scale down from 1.35 -> 1.0, alpha up fast
            // remaining: hold then fade out at end
            const float t = s.slam.t;
            const float slamIn = 0.12f;
            const float fadeOut = 0.20f;

            float scale = 1.0f;
            float alpha = 1.0f;

            if (t < slamIn)
            {
                float k = t / slamIn; // 0..1
                scale = 1.35f - 0.35f * k;      // 1.35 -> 1.0
                alpha = 0.20f + 0.80f * k;      // 0.2  -> 1.0
            }
            else if (t > s.slam.duration - fadeOut)
            {
                float k = (t - (s.slam.duration - fadeOut)) / fadeOut; // 0..1
                alpha = 1.0f - k; // 1 -> 0
            }

            DrawTextureMesh(gQuadMesh, gHandprintTex, winX, winY,
                WINDOW_W * scale, WINDOW_H * scale, alpha);
        }

        // --- B) Shadow walking (PNG silhouette) ---
        if (s.event == DoorEvent::ShadowWalk && s.shadow.active && gShadowTex)
        {
            const float sx = winX + s.shadow.offsetX;

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            DrawTextureMesh(gQuadMesh, gShadowTex,
                sx, winY,
                WINDOW_W * 0.95f, WINDOW_H * 0.95f, 0.85f);
        }

        // --- C) Red glowing eyes (no PNG) ---
    //    if (s.event == DoorEvent::EyesBlink && s.eyes.active)
    //    {
    //        if (s.eyes.eyesOn)
    //        {
    //            // Eyes position relative to window (normalized)
    //            const float eyeY = winY + WINDOW_H * 0.10f; // slightly above window center
    //            const float eyeOffsetX = WINDOW_W * 0.18f;

				//// Eye size 
    //            const float eyeW = WINDOW_W * 0.14f;
    //            const float eyeH = WINDOW_H * 0.18f;

    //            // Glow layers: center bright + outer softer
    //            // Note: using ARGB colors (0xAARRGGBB)
    //            DrawSquareMesh(gQuadMesh, winX - eyeOffsetX, eyeY, eyeW, eyeH, 0xFFFF0000); 
    //            DrawSquareMesh(gQuadMesh, winX + eyeOffsetX, eyeY, eyeW, eyeH, 0xFFFF0000);

    //            // Glow layer (bigger + transparent)
    //            DrawSquareMesh(gQuadMesh, winX - eyeOffsetX, eyeY, eyeW * 2.2f, eyeH * 2.0f, 0x66FF0000);
    //            DrawSquareMesh(gQuadMesh, winX + eyeOffsetX, eyeY, eyeW * 2.2f, eyeH * 2.0f, 0x66FF0000);
    //        }
    //    }

        // --------------------------------------------
        // 5) DOOR NUMBER TEXT (unchanged)
        // --------------------------------------------
        if (doorFontId >= 0)
        {
            char textBuffer[32];

            if (floorNum == 0)
                sprintf_s(textBuffer, "B1-%02d", i + 1);
            else
                sprintf_s(textBuffer, "%02d-%02d", floorNum, i + 1);

            const float textNDC_X = (doorX / SCREEN_WIDTH_HALF) - textXoffset;
            const float textNDC_Y = textY / SCREEN_HEIGHT_HALF;

            AEGfxPrint(doorFontId, textBuffer, textNDC_X, textNDC_Y, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}

void Doors_Unload()
{
    FreeMeshSafe(gQuadMesh);

    UnloadTextureSafe(gDoorTex);
    UnloadTextureSafe(gWindowBaseTex);

    UnloadTextureSafe(gHandprintTex);
    UnloadTextureSafe(gShadowTex);

    if (doorFontId >= 0)
    {
        AEGfxDestroyFont(doorFontId);
        doorFontId = -1;
    }
}
