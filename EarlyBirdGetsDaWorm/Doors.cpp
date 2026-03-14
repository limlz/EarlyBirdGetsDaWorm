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
static constexpr float WINDOW_CENTER_V = 0.70f;

static constexpr float WINDOW_SIZE_U = 0.40f;
static constexpr float WINDOW_SIZE_V = 0.25f;

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
static AEGfxTexture* gDoorBoardedTex = nullptr; // NEW: Boarded Door Texture
static AEGfxTexture* gWindow0Tex = nullptr;
static AEGfxTexture* gWindow1Tex = nullptr;
static AEGfxTexture* gHandprintTex = nullptr;

// ============================================================
// EVENT STATE
// ============================================================

struct DoorState
{
    ANOMALYID anomaly = ANOMALYID::None;

    float rollCooldown = 0.0f;
    float eventCooldown = 0.0f;
    float ghostRollCooldown = 0.0f;
};

static DoorState gDoorState[NUM_DOORS]{};
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
            gDoorState[i] = DoorState{};
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
    DoorState& s = gDoorState[doorIdx];
    s.anomaly = ANOMALYID::None;
    s.eventCooldown = 0.0f;
}

// ============================================================
// ANOMALY TRIGGERS
// ============================================================

static void TriggerHandprintSlam(int doorIdx)
{
    DoorState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.anomaly = ANOMALYID::Door_HandSlam;
    s.eventCooldown = 0.8f;
}

static void TriggerLightChange(int doorIdx)
{
    DoorState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.anomaly = ANOMALYID::Door_LightChange;
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

static ANOMALYID PickFromPool(std::vector<ANOMALYID> const& pool)
{
    if (pool.empty()) return ANOMALYID::None;
    int r = rand() % (int)pool.size();
    return pool[r];
}

static ANOMALYID GetDoorEventByIllness(ILLNESSES illness)
{
    static const std::vector<ANOMALYID> paranoiaPool{ ANOMALYID::Door_LightChange };
    static const std::vector<ANOMALYID> schizophreniaPool{ ANOMALYID::Door_HandSlam };
    static const std::vector<ANOMALYID> insomniaPool{ ANOMALYID::Door_LightChange };
    static const std::vector<ANOMALYID> ocdPool{ ANOMALYID::Door_OCD };
    static const std::vector<ANOMALYID> scotophobiaPool{ ANOMALYID::Door_LightChange };

    switch (illness)
    {
    case ILLNESSES::PARANOIA:      return PickFromPool(paranoiaPool);
    case ILLNESSES::SCHIZOPHRENIA: return PickFromPool(schizophreniaPool);
    case ILLNESSES::INSOMNIA:      return PickFromPool(insomniaPool);
    case ILLNESSES::OCD:           return PickFromPool(ocdPool);
    case ILLNESSES::SCOTOPHOBIA:   return PickFromPool(scotophobiaPool);
    default:                       return ANOMALYID::None;
    }
}

static ILLNESSES GetEffectiveIllnessForDoorEvents()
{
    ILLNESSES current = Player_GetTrueDiagnosis();
    if (current == ILLNESSES::GHOST)
        return Player_GetAnomalyIllness();
    return current;
}

// ============================================================
// LOAD / INIT / UNLOAD
// ============================================================

void Doors_Load()
{
    gDoorFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

    gDoorTex = LoadTextureChecked(Assets::Background::DoorBg);
    gDoorBoardedTex = LoadTextureChecked(Assets::Background::DoorBoarded);
    gWindow0Tex = LoadTextureChecked(Assets::Door_Anomaly::Window_0);
    gWindow1Tex = LoadTextureChecked(Assets::Door_Anomaly::Window_1);
    gHandprintTex = LoadTextureChecked(Assets::Door_Anomaly::Handprint);
}

void Doors_Initialize()
{
    SeedRNGOnce();

    gQuadMesh = CreateSquareMesh(0xFFFFFFFF);

    for (int i = 0; i < NUM_DOORS; ++i)
        gDoorState[i] = DoorState{};
}

void Doors_Unload()
{
    FreeMeshSafe(gQuadMesh);

    UnloadTextureSafe(gDoorTex);
    UnloadTextureSafe(gDoorBoardedTex);
    UnloadTextureSafe(gWindow0Tex);
    UnloadTextureSafe(gWindow1Tex);
    UnloadTextureSafe(gHandprintTex);

    if (gDoorFontId >= 0)
        AEGfxDestroyFont(gDoorFontId);

    gQuadMesh = nullptr;
    gDoorTex = gDoorBoardedTex = gWindow0Tex = gWindow1Tex = gHandprintTex = nullptr;
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
    if (!CentralPool_IsSystemActive(AnomalyFiles::Doors_isActive)) return;
    if (!Player_HasPatient()) return;

    // Tick cooldowns / clear finished events
    for (int d = 0; d < NUM_DOORS; ++d)
    {
        DoorState& s = gDoorState[d];

        if (s.rollCooldown > 0.0f)      s.rollCooldown -= dt;
        if (s.eventCooldown > 0.0f)     s.eventCooldown -= dt;
        if (s.ghostRollCooldown > 0.0f) s.ghostRollCooldown -= dt;

        if (s.anomaly != ANOMALYID::None && s.eventCooldown <= 0.0f)
        {
            s.anomaly = ANOMALYID::None;
            s.eventCooldown = 0.0f;
        }
    }

    if (doorNearPlayer < 0 || doorNearPlayer >= NUM_DOORS)
        return;

    DoorState& s = gDoorState[doorNearPlayer];

    if (s.eventCooldown > 0.0f || s.rollCooldown > 0.0f)
        return;

    s.rollCooldown = EVENT_ROLL_COOLDOWN;

    if ((rand() % 100) >= EVENT_CHANCE_PERCENT)
        return;

    const ILLNESSES eff = GetEffectiveIllnessForDoorEvents();
    ANOMALYID chosen = GetDoorEventByIllness(eff);

    switch (chosen)
    {
    case ANOMALYID::Door_HandSlam:    TriggerHandprintSlam(doorNearPlayer); break;
    case ANOMALYID::Door_LightChange: TriggerLightChange(doorNearPlayer); break;
    default: break;
    }

    const bool isGhost = (Player_GetTrueDiagnosis() == ILLNESSES::GHOST);
    const bool wantExtra = isGhost && (Player_GetGhostExtraAnomalies() >= 1);

    if (wantExtra && s.ghostRollCooldown <= 0.0f)
    {
        s.ghostRollCooldown = GHOST_EXTRA_ROLL_COOLDOWN;

        if ((rand() % 100) < GHOST_EXTRA_CHANCE_PERCENT)
        {
            ANOMALYID extraEvent = GetDoorEventByIllness(eff);

            if (extraEvent == s.anomaly)
            {
                for (int tries = 0; tries < 4; ++tries)
                {
                    ANOMALYID re = GetDoorEventByIllness(eff);
                    if (re != s.anomaly && re != ANOMALYID::None)
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
                    case ANOMALYID::Door_HandSlam:    TriggerHandprintSlam(doorNearPlayer); break;
                    case ANOMALYID::Door_LightChange: TriggerLightChange(doorNearPlayer); break;
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
        DoorState& s = gDoorState[doorIdx];

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Check if this door is in the basement and has already been used today
        bool isBoarded = (floorNum == 0 && basementDoors[doorIdx].lockedThisDay);

        // =====================================================
        // DOOR
        // =====================================================

        DrawTextureMesh(gQuadMesh, gDoorTex, doorX, DOOR_Y, DOOR_W, DOOR_H, 1.0f);

        // =====================================================
        // WINDOW & OVERLAYS 
        // =====================================================
        float winX, winY;
        WindowWorldCenter(doorX, winX, winY);

        DrawSquareMesh(gQuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        AEGfxTexture* windowTex = gWindow0Tex;

        if (s.anomaly == ANOMALYID::Door_LightChange)
            windowTex = gWindow1Tex;

        if (windowTex)
            DrawTextureMesh(gQuadMesh, windowTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

        // =====================================================
        // OVERLAY ANOMALY
        // =====================================================
        if (s.anomaly == ANOMALYID::Door_HandSlam && gHandprintTex)
            DrawTextureMesh(gQuadMesh, gHandprintTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

		// Boarded doors get drawn on top of everything else to fully obscure the window and any anomalies behind it
        if (isBoarded) {
            DrawTextureMesh(gQuadMesh, gDoorBoardedTex, doorX, DOOR_Y, 300.0f, 200.0f, 1.0f);
        }


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