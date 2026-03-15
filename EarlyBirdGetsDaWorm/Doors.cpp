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

    float ocdOffsetX = 0.0f;
    float ocdOffsetY = 0.0f;
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
    s.ocdOffsetX = 0.0f;
    s.ocdOffsetY = 0.0f;
}

static void InitializeRandomOnce()
{
    static bool seeded = false;

    if (!seeded)
    {
        srand((unsigned)time(nullptr));
        seeded = true;
    }
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

static void TriggerDoorOCD(int doorIdx)
{
    DoorState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.anomaly = ANOMALYID::Door_Position;
    s.eventCooldown = 1.2f;

    // Randomly shift the window around the door a little bit
    s.ocdOffsetX = (float)((rand() % 61) - 30); // -30 to +30
    s.ocdOffsetY = (float)((rand() % 41) - 20); // -20 to +20
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
    // Pools to match dictionary exactly
    static const std::vector<ANOMALYID> paranoiaPool{ ANOMALYID::Door_LightChange };
    static const std::vector<ANOMALYID> maniaPool{ ANOMALYID::Door_HandSlam };
    static const std::vector<ANOMALYID> depressionPool{ ANOMALYID::Door_LightChange };
    static const std::vector<ANOMALYID> dementiaPool{ ANOMALYID::Door_LightChange };
    static const std::vector<ANOMALYID> schizophreniaPool{ ANOMALYID::Door_HandSlam };
    static const std::vector<ANOMALYID> aiwPool{ ANOMALYID::Door_Position };
    static const std::vector<ANOMALYID> insomniaPool{ ANOMALYID::Door_LightChange };
    static const std::vector<ANOMALYID> ocdPool{ ANOMALYID::Door_Position };
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
    InitializeRandomOnce();

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
	// ------------------------------------------------
    // EARLY EXITS
    // Stop immediately if door anomalies are disabled
    // or if the player is not carrying a patient.
    // ------------------------------------------------
    if (!CentralPool_IsSystemActive(AnomalyFiles::Doors_isActive)) return;
    if (!Player_HasPatient()) return;


    // ------------------------------------------------
    // 1) UPDATE ALL DOOR TIMERS
    // - rollCooldown: prevents anomaly checks too often
    // - eventCooldown: controls how long the current anomaly stays active
    // - ghostRollCooldown: prevents ghost bonus anomaly checks too often
    //
    // Also clears expired anomalies once their cooldown ends.
    // ------------------------------------------------
    for (int d = 0; d < NUM_DOORS; ++d)
    {
        DoorState& s = gDoorState[d];

        if (s.rollCooldown > 0.0f)      s.rollCooldown -= dt;
        if (s.eventCooldown > 0.0f)     s.eventCooldown -= dt;
        if (s.ghostRollCooldown > 0.0f) s.ghostRollCooldown -= dt;

        // If the anomaly duration has ended, clear it
        if (s.anomaly != ANOMALYID::None && s.eventCooldown <= 0.0f)
        {
            s.anomaly = ANOMALYID::None;
            s.eventCooldown = 0.0f;
        }
    }


    // ------------------------------------------------
    // 2) VALIDATE THE DOOR NEAR THE PLAYER
    // If the player is not near any valid door, do nothing.
    // ------------------------------------------------
    if (doorNearPlayer < 0 || doorNearPlayer >= NUM_DOORS)
    {
        return;
    }

    DoorState& s = gDoorState[doorNearPlayer];


    // ------------------------------------------------
    // 3) CHECK IF THIS DOOR IS READY TO ROLL A NEW EVENT
    // If this door is already busy or still waiting on cooldown,
    // do not attempt a new anomaly yet.
    // ------------------------------------------------
    if (s.eventCooldown > 0.0f || s.rollCooldown > 0.0f)
    {
        return;
    }


    // ------------------------------------------------
    // 4) MAIN EVENT ROLL
    // Lock this door from rolling again for a short time.
    // Then perform a % chance roll to decide whether an anomaly happens.
    // ------------------------------------------------
    s.rollCooldown = EVENT_ROLL_COOLDOWN;

    if ((rand() % 100) >= EVENT_CHANCE_PERCENT)
    {
        return;
    }


    // ------------------------------------------------
    // 5) PICK AND TRIGGER THE MAIN DOOR ANOMALY
    // Choose the anomaly based on the patient's effective illness,
    // then activate the matching event.
    // ------------------------------------------------
    const ILLNESSES eff = GetEffectiveIllnessForDoorEvents();
    ANOMALYID chosen = GetDoorEventByIllness(eff);

    switch (chosen)
    {
    case ANOMALYID::Door_HandSlam:
        TriggerHandprintSlam(doorNearPlayer);
        break;

    case ANOMALYID::Door_LightChange:
        TriggerLightChange(doorNearPlayer);
        break;

    case ANOMALYID::Door_Position:
        TriggerDoorOCD(doorNearPlayer);
        break;

    default:
        break;
    }


    // ------------------------------------------------
    // 6) GHOST BONUS ANOMALY ROLL
    // If the patient is a ghost with +1 extra anomaly,
    // the game gets a second chance to apply an additional door event.
    // ------------------------------------------------
    const bool isGhost = (Player_GetTrueDiagnosis() == ILLNESSES::GHOST);
    const bool wantExtra = isGhost && (Player_GetGhostExtraAnomalies() >= 1);

    if (wantExtra && s.ghostRollCooldown <= 0.0f)
    {
        // Prevent the ghost bonus roll from happening too often
        s.ghostRollCooldown = GHOST_EXTRA_ROLL_COOLDOWN;

        // Roll for the ghost bonus event
        if ((rand() % 100) < GHOST_EXTRA_CHANCE_PERCENT)
        {
            ANOMALYID extraEvent = GetDoorEventByIllness(eff);


            // Avoid picking the exact same anomaly again
            // If it matches the current anomaly, reroll a few times
            // to try getting a different valid event.
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


            // Only apply the ghost extra if the door is currently free
            // (prevents overwriting an active event mid-duration).
            if (s.eventCooldown <= 0.0f)
            {
                switch (extraEvent)
                {
                case ANOMALYID::Door_HandSlam:
                    TriggerHandprintSlam(doorNearPlayer);
                    break;

                case ANOMALYID::Door_LightChange:
                    TriggerLightChange(doorNearPlayer);
                    break;

                case ANOMALYID::Door_Position:
                    TriggerDoorOCD(doorNearPlayer);
                    break;

                default:
                    break;
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

        // Report the anomaly to the journal
        switch (s.anomaly)
        {
        case ANOMALYID::Door_LightChange:
            Journal_ReportAnomaly(ANOMALYID::Door_LightChange);
            break;

        case ANOMALYID::Door_HandSlam:
            Journal_ReportAnomaly(ANOMALYID::Door_HandSlam);
            break;

        case ANOMALYID::Door_Position:
            Journal_ReportAnomaly(ANOMALYID::Door_Position);
            break;

        default:
            break;
        }

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        bool isBoarded = (floorNum == 0 && basementDoors[doorIdx].lockedThisDay);

        // ------------------------------------------------
        // DOOR BASE
        // ------------------------------------------------
        DrawTextureMesh(gQuadMesh, gDoorTex, doorX, DOOR_Y, DOOR_W, DOOR_H, 1.0f);

        // ------------------------------------------------
        // WINDOW POSITION
        // ------------------------------------------------
        float winX, winY;
        WindowWorldCenter(doorX, winX, winY);

        if (s.anomaly == ANOMALYID::Door_Position)
        {
            winX += s.ocdOffsetX;
            winY += s.ocdOffsetY;
        }

        // ------------------------------------------------
        // WINDOW TEXTURE
        // ------------------------------------------------
        AEGfxTexture* windowTex = gWindow0Tex;

        if (s.anomaly == ANOMALYID::Door_LightChange)
        {
            windowTex = gWindow1Tex;
        }

        // Window background
        DrawSquareMesh(gQuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        // Window image
        if (windowTex)
        {
            DrawTextureMesh(gQuadMesh, windowTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);
        }

        // ------------------------------------------------
        // WINDOW OVERLAYS
        // ------------------------------------------------
        if (s.anomaly == ANOMALYID::Door_HandSlam && gHandprintTex)
        {
            DrawTextureMesh(gQuadMesh, gHandprintTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);
        }

        // ------------------------------------------------
        // BOARDED OVERLAY
        // ------------------------------------------------
        if (isBoarded)
        {
            DrawTextureMesh(gQuadMesh, gDoorBoardedTex, doorX, DOOR_Y, 300.0f, 200.0f, 1.0f);
        }

        // ------------------------------------------------
        // LABEL
        // ------------------------------------------------
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