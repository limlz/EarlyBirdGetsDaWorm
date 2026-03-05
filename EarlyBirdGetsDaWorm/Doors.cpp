// ============================================================
// DOORS MODULE
// ============================================================

#include "pch.hpp"
#include <vector>
#include <ctime>
#include <cstdlib>

// ============================================================
// CONFIG
// ============================================================

static constexpr float DOOR_W = DOOR_WIDTH;
static constexpr float DOOR_H = DOOR_HEIGHT;

static float ComputeDoorY()
{
    const float floorLineY = (-FLOOR_CENTER_Y) + (FLOOR_HEIGHT * 0.5f);
    return floorLineY + (DOOR_H * 0.5f);
}

static const float DOOR_Y = ComputeDoorY();

// Window placement
static constexpr float WINDOW_CENTER_U = 0.50f;
static constexpr float WINDOW_CENTER_V = 0.62f;

static constexpr float WINDOW_SIZE_U = 0.45f;
static constexpr float WINDOW_SIZE_V = 0.18f;

static constexpr float WINDOW_W = DOOR_W * WINDOW_SIZE_U;
static constexpr float WINDOW_H = DOOR_H * WINDOW_SIZE_V;

// Event tuning
static constexpr float EVENT_ROLL_COOLDOWN = 2.0f;
static constexpr int EVENT_CHANCE_PERCENT = 35;

// ============================================================
// RENDER DATA
// ============================================================

static AEGfxVertexList* gQuadMesh = nullptr;
static s8 gDoorFontId = -1;

static AEGfxTexture* gDoorTex = nullptr;
static AEGfxTexture* gWindowBaseTex = nullptr;
static AEGfxTexture* gHandprintTex = nullptr;
static AEGfxTexture* gShadowTex = nullptr;

// ============================================================
// EVENT STATE
// ============================================================

enum class DoorEvent
{
    None,
    HandprintSlam,
    ShadowWalk,
    Knock
};

struct HandprintSlamState
{
    bool active = false;
    float t = 0.0f;
    float duration = 0.55f;
};

struct ShadowWalkState
{
    bool active = false;
    float offsetX = 0.0f;
    float speed = 220.0f;
};

struct DoorEventState
{
    DoorEvent event = DoorEvent::None;

    float rollCooldown = 0.0f;
    float eventCooldown = 0.0f;

    HandprintSlamState slam;
    ShadowWalkState shadow;
};

static DoorEventState gDoorState[NUM_DOORS]{};
static bool gDoorsEnabled = true;

// ============================================================
// BASEMENT SYSTEM
// ============================================================

struct BasementCheck
{
    bool lockedThisDay = false;
};

static BasementCheck basementDoors[NUM_DOORS]{};

void Doors_ResetAllLocks()
{
    for (int i = 0; i < NUM_DOORS; ++i)
        basementDoors[i].lockedThisDay = false;
}

bool Doors_TryDisposal(int floorNum, int doorIdx)
{
    if (doorIdx < 0 || doorIdx >= NUM_DOORS)
        return false;

    BasementCheck& s = basementDoors[doorIdx];

    if (s.lockedThisDay)
        return false;

    if ((rand() % 100) < 30)
    {
        JumpScare_Start();
        s.lockedThisDay = true;
        return false;
    }

    s.lockedThisDay = true;
    return true;
}

// ============================================================
// ENABLE TOGGLE
// ============================================================

void Doors_SetEnabled(bool enabled)
{
    gDoorsEnabled = enabled;

    if (!enabled)
    {
        for (int i = 0; i < NUM_DOORS; ++i)
            gDoorState[i] = DoorEventState{};
    }
}

// ============================================================
// HELPERS
// ============================================================

static int ClampFloor(int f)
{
    if (f < 0) return 0;
    if (f >= 10) return 9;
    return f;
}

static float DoorWorldX(int doorIndex, float camX)
{
    return DIST_BETWEEN_DOORS + camX + (DIST_BETWEEN_DOORS * doorIndex);
}

static void WindowWorldCenter(float doorX, float& x, float& y)
{
    const float offX = (WINDOW_CENTER_U - 0.5f) * DOOR_W;
    const float offY = (WINDOW_CENTER_V - 0.5f) * DOOR_H;

    x = doorX + offX;
    y = DOOR_Y + offY;
}

static void ClearDoorEvent(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];

    s.event = DoorEvent::None;
    s.slam = HandprintSlamState{};
    s.shadow = ShadowWalkState{};
}

static void TriggerKnock(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::Knock;

    AudioManager_PlaySFX(SFX_DOOR_KNOCK, 0.35f, 1.0f, -1);

    s.eventCooldown = 1.0f;
}

static void TriggerHandprintSlam(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::HandprintSlam;
    s.slam.active = true;
    s.slam.t = 0.0f;
}

static void TriggerShadowWalk(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::ShadowWalk;
    s.shadow.active = true;

    s.shadow.offsetX = -(WINDOW_W * 0.5f) - (WINDOW_W * 0.6f);
    s.shadow.speed = 200.0f + (rand() % 120);
}

static void SeedRNGOnce()
{
    static bool seeded = false;

    if (!seeded)
    {
        srand((unsigned)time(nullptr));
        seeded = true;
    }
}

// ============================================================
// ILLNESS EVENT POOLS
// ============================================================

static DoorEvent GetDoorEventByIllness(ILLNESSES illness)
{
    switch (illness)
    {
    case PARANOIA:      return (rand() % 2) ? DoorEvent::ShadowWalk : DoorEvent::Knock;
    case MANIA:         return DoorEvent::HandprintSlam;
    case DEPRESSION:    return DoorEvent::Knock;
    case DEMENTIA:      return DoorEvent::ShadowWalk;
    case SCHIZOPHRENIA: return (rand() % 2) ? DoorEvent::ShadowWalk : DoorEvent::HandprintSlam;
    case AIW_SYNDROME:  return DoorEvent::Knock;
    case INSOMNIA:      return DoorEvent::Knock;
    case OCD:           return DoorEvent::HandprintSlam;
    case SCOTOPHOBIA:   return DoorEvent::ShadowWalk;
    default:            return DoorEvent::None;
    }
}

// ============================================================
// LOAD / INIT / UNLOAD
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

    for (int i = 0; i < NUM_DOORS; ++i)
        gDoorState[i] = DoorEventState{};
}

void Doors_Unload()
{
    FreeMeshSafe(gQuadMesh);

    UnloadTextureSafe(gDoorTex);
    UnloadTextureSafe(gWindowBaseTex);
    UnloadTextureSafe(gHandprintTex);
    UnloadTextureSafe(gShadowTex);

    if (gDoorFontId >= 0)
        AEGfxDestroyFont(gDoorFontId);
}

// ============================================================
// UPDATE
// ============================================================

int Doors_Update(float camX)
{
    const float detectionRange = DOOR_W * 0.5f;

    for (int i = 0; i < NUM_DOORS; ++i)
    {
        float doorX = DoorWorldX(i, camX);

        if (doorX > -detectionRange + 50 && doorX < detectionRange + 50)
            return i;
    }

    return -1;
}

// ============================================================
// EVENTS
// ============================================================

void Doors_Animate(float dt, int doorNearPlayer, float)
{
    if (!gDoorsEnabled) return;
    if (!Player_HasPatient()) return;

    for (int d = 0; d < NUM_DOORS; ++d)
    {
        DoorEventState& s = gDoorState[d];

        if (s.rollCooldown > 0)  s.rollCooldown -= dt;
        if (s.eventCooldown > 0) s.eventCooldown -= dt;
    }

    if (doorNearPlayer < 0 || doorNearPlayer >= NUM_DOORS)
        return;

    DoorEventState& s = gDoorState[doorNearPlayer];

    if (s.eventCooldown > 0 || s.rollCooldown > 0)
        return;

    s.rollCooldown = EVENT_ROLL_COOLDOWN;

    if ((rand() % 100) >= EVENT_CHANCE_PERCENT)
        return;

    DoorEvent chosen = GetDoorEventByIllness(Player_GetCurrentIllness());

    switch (chosen)
    {
    case DoorEvent::HandprintSlam: TriggerHandprintSlam(doorNearPlayer); break;
    case DoorEvent::Knock:         TriggerKnock(doorNearPlayer); break;
    case DoorEvent::ShadowWalk:    TriggerShadowWalk(doorNearPlayer); break;
    default: break;
    }
}

// ============================================================
// DRAW
// ============================================================

void Doors_Draw(float camX, s8 floorNum, float textXoffset, float textY, bool dementia)
{
    if (!gQuadMesh) return;

    int maxDoors = dementia ? 1000 : NUM_DOORS;

    for (int i = 0; i < maxDoors; ++i)
    {
        float doorX = DoorWorldX(i, camX);

        if (doorX < -SCREEN_WIDTH_HALF - DOOR_W) continue;
        if (doorX > SCREEN_WIDTH_HALF + DOOR_W) continue;

        int doorIdx = i % NUM_DOORS;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        if (gDoorTex)
            DrawTextureMesh(gQuadMesh, gDoorTex, doorX, DOOR_Y, DOOR_W, DOOR_H, 1.0f);

        float winX, winY;
        WindowWorldCenter(doorX, winX, winY);

        DrawSquareMesh(gQuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        if (gWindowBaseTex)
            DrawTextureMesh(gQuadMesh, gWindowBaseTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

        DoorEventState& s = gDoorState[doorIdx];

        if (s.event == DoorEvent::ShadowWalk && s.shadow.active && gShadowTex)
        {
            float sx = winX + s.shadow.offsetX;

            DrawTextureMesh(
                gQuadMesh,
                gShadowTex,
                sx,
                winY,
                WINDOW_W * 0.95f,
                WINDOW_H * 0.95f,
                0.85f);
        }

        if (gDoorFontId >= 0)
        {
            char text[32];

            if (floorNum == 0)
                sprintf_s(text, "B1-%02d", i + 1);
            else
                sprintf_s(text, "%02d-%02d", floorNum, i + 1);

            float x = (doorX / SCREEN_WIDTH_HALF) - textXoffset;
            float y = textY / SCREEN_HEIGHT_HALF;

            AEGfxPrint(gDoorFontId, text, x, y, 1.0f, 1, 0, 0, 1);
        }
    }
}