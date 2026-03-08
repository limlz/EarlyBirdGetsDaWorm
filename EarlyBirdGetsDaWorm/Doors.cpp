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
static constexpr int   EVENT_CHANCE_PERCENT = 35;

// Ghost "+1 extra anomaly" tuning
static constexpr float GHOST_EXTRA_ROLL_COOLDOWN = 3.0f;
static constexpr int   GHOST_EXTRA_CHANCE_PERCENT = 18;

// ============================================================
// RENDER DATA
// ============================================================

static AEGfxVertexList* gQuadMesh = nullptr;
static s8               gDoorFontId = -1;

static AEGfxTexture* gDoorTex = nullptr;
static AEGfxTexture* gWindow0Tex = nullptr;
static AEGfxTexture* gWindow1Tex = nullptr;
static AEGfxTexture* gHandprintTex = nullptr;

// ============================================================
// EVENT STATE
// ============================================================

enum class DoorEvent
{
    None,
    HandprintSlam,
    LightChange,
    Knock
};

struct DoorEventState
{
    DoorEvent event = DoorEvent::None;

    float rollCooldown = 0.0f;
    float eventCooldown = 0.0f;
    float ghostRollCooldown = 0.0f;
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

bool Doors_TryDisposal(int floorNum, int doorIdx, bool& outDidJumpscare)
{
    outDidJumpscare = false;

    if (doorIdx < 0 || doorIdx >= NUM_DOORS) return false;

    BasementCheck& s = basementDoors[doorIdx];
    if (s.lockedThisDay) return false;

    s.lockedThisDay = true;

    if ((rand() % 100) < 30)
    {
        JumpScare_Start();
        outDidJumpscare = true;
        return false;
    }

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
    s.eventCooldown = 0.0f;
}

// ============================================================
// ANOMALY TRIGGERS
// ============================================================

static void TriggerKnock(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::Knock;
    s.eventCooldown = 1.0f;

    AudioManager_PlaySFX(SFX_DOOR_KNOCK, 0.35f, 1.0f, -1);
}

static void TriggerHandprintSlam(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::HandprintSlam;
    s.eventCooldown = 0.8f;
}

static void TriggerLightChange(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::LightChange;
    s.eventCooldown = 0.8f;
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
// ILLNESS -> DOOR EVENT POOLS
// ============================================================

static DoorEvent PickDoorEventFromPool(std::vector<DoorEvent> const& pool)
{
    if (pool.empty()) return DoorEvent::None;
    int r = rand() % (int)pool.size();
    return pool[r];
}

static DoorEvent GetDoorEventByIllness(ILLNESSES illness)
{
    // Pools to match dictionary / illness style
    static const std::vector<DoorEvent> paranoiaPool{ DoorEvent::Knock, DoorEvent::LightChange };
    static const std::vector<DoorEvent> maniaPool{ DoorEvent::HandprintSlam };
    static const std::vector<DoorEvent> depressionPool{ DoorEvent::Knock };
    static const std::vector<DoorEvent> dementiaPool{ DoorEvent::LightChange };
    static const std::vector<DoorEvent> schizophreniaPool{ DoorEvent::HandprintSlam, DoorEvent::LightChange };
    static const std::vector<DoorEvent> aiwPool{ DoorEvent::Knock };
    static const std::vector<DoorEvent> insomniaPool{ DoorEvent::Knock };
    static const std::vector<DoorEvent> ocdPool{ DoorEvent::HandprintSlam };
    static const std::vector<DoorEvent> scotophobiaPool{ DoorEvent::LightChange };

    switch (illness)
    {
    case ILLNESSES::PARANOIA:      return PickDoorEventFromPool(paranoiaPool);
    case ILLNESSES::MANIA:         return PickDoorEventFromPool(maniaPool);
    case ILLNESSES::DEPRESSION:    return PickDoorEventFromPool(depressionPool);
    case ILLNESSES::DEMENTIA:      return PickDoorEventFromPool(dementiaPool);
    case ILLNESSES::SCHIZOPHRENIA: return PickDoorEventFromPool(schizophreniaPool);
    case ILLNESSES::AIW_SYNDROME:  return PickDoorEventFromPool(aiwPool);
    case ILLNESSES::INSOMNIA:      return PickDoorEventFromPool(insomniaPool);
    case ILLNESSES::OCD:           return PickDoorEventFromPool(ocdPool);
    case ILLNESSES::SCOTOPHOBIA:   return PickDoorEventFromPool(scotophobiaPool);
    default:                       return DoorEvent::None;
    }
}

static ILLNESSES GetEffectiveIllnessForDoorEvents()
{
    ILLNESSES current = Player_GetCurrentIllness();
    if (current == ILLNESSES::GHOST)
        return Player_GetMimicIllness();
    return current;
}

// ============================================================
// LOAD / INIT / UNLOAD
// ============================================================

void Doors_Load()
{
    gDoorFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

    gDoorTex = LoadTextureChecked(Assets::Background::DoorBg);
    gWindow0Tex = LoadTextureChecked(Assets::Door_Anomaly::Window_0);
    gWindow1Tex = LoadTextureChecked(Assets::Door_Anomaly::Window_1);
    gHandprintTex = LoadTextureChecked(Assets::Door_Anomaly::Handprint);
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
    UnloadTextureSafe(gWindow0Tex);
    UnloadTextureSafe(gWindow1Tex);
    UnloadTextureSafe(gHandprintTex);

    if (gDoorFontId >= 0)
        AEGfxDestroyFont(gDoorFontId);

    gQuadMesh = nullptr;
    gDoorTex = gWindow0Tex = gWindow1Tex = gHandprintTex = nullptr;
    gDoorFontId = -1;
}

// ============================================================
// UPDATE: returns door index near the player (or -1)
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

void Doors_Animate(float dt, int doorNearPlayer, float /*unused*/)
{
    if (!gDoorsEnabled) return;
    if (!Player_HasPatient()) return;

    // Tick cooldowns / clear finished events
    for (int d = 0; d < NUM_DOORS; ++d)
    {
        DoorEventState& s = gDoorState[d];

        if (s.rollCooldown > 0.0f)      s.rollCooldown -= dt;
        if (s.eventCooldown > 0.0f)     s.eventCooldown -= dt;
        if (s.ghostRollCooldown > 0.0f) s.ghostRollCooldown -= dt;

        if (s.event != DoorEvent::None && s.eventCooldown <= 0.0f)
        {
            s.event = DoorEvent::None;
            s.eventCooldown = 0.0f;
        }
    }

    if (doorNearPlayer < 0 || doorNearPlayer >= NUM_DOORS)
        return;

    DoorEventState& s = gDoorState[doorNearPlayer];

    if (s.eventCooldown > 0.0f || s.rollCooldown > 0.0f)
        return;

    s.rollCooldown = EVENT_ROLL_COOLDOWN;

    if ((rand() % 100) >= EVENT_CHANCE_PERCENT)
        return;

    const ILLNESSES eff = GetEffectiveIllnessForDoorEvents();
    DoorEvent chosen = GetDoorEventByIllness(eff);

    switch (chosen)
    {
    case DoorEvent::HandprintSlam: TriggerHandprintSlam(doorNearPlayer); break;
    case DoorEvent::Knock:         TriggerKnock(doorNearPlayer); break;
    case DoorEvent::LightChange:   TriggerLightChange(doorNearPlayer); break;
    default: break;
    }

    const bool isGhost = (Player_GetCurrentIllness() == ILLNESSES::GHOST);
    const bool wantExtra = isGhost && (Player_GetGhostExtraAnomalies() >= 1);

    if (wantExtra && s.ghostRollCooldown <= 0.0f)
    {
        s.ghostRollCooldown = GHOST_EXTRA_ROLL_COOLDOWN;

        if ((rand() % 100) < GHOST_EXTRA_CHANCE_PERCENT)
        {
            DoorEvent extraEvent = GetDoorEventByIllness(eff);

            if (extraEvent == s.event)
            {
                for (int tries = 0; tries < 4; ++tries)
                {
                    DoorEvent re = GetDoorEventByIllness(eff);
                    if (re != s.event && re != DoorEvent::None)
                    {
                        extraEvent = re;
                        break;
                    }
                }
            }

            if (s.eventCooldown <= 0.0f)
            {
                switch (extraEvent)
                {
                case DoorEvent::HandprintSlam: TriggerHandprintSlam(doorNearPlayer); break;
                case DoorEvent::Knock:         TriggerKnock(doorNearPlayer); break;
                case DoorEvent::LightChange:   TriggerLightChange(doorNearPlayer); break;
                default: break;
                }
            }
        }
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
        DoorEventState& s = gDoorState[doorIdx];

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // =====================================================
        // DOOR
        // =====================================================
        if (gDoorTex)
            DrawTextureMesh(gQuadMesh, gDoorTex, doorX, DOOR_Y, DOOR_W, DOOR_H, 1.0f);

        // =====================================================
        // WINDOW
        // =====================================================
        float winX, winY;
        WindowWorldCenter(doorX, winX, winY);

        DrawSquareMesh(gQuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        AEGfxTexture* windowTex = gWindow0Tex;

        if (s.event == DoorEvent::LightChange)
            windowTex = gWindow1Tex;

        if (windowTex)
            DrawTextureMesh(gQuadMesh, windowTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

        // =====================================================
        // OVERLAY ANOMALY
        // =====================================================
        if (s.event == DoorEvent::HandprintSlam && gHandprintTex)
            DrawTextureMesh(gQuadMesh, gHandprintTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

        // =====================================================
        // LABEL
        // =====================================================
        if (gDoorFontId >= 0)
        {
            char text[32]{};

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