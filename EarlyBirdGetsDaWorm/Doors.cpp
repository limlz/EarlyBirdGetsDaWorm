// ============================================================
// DOORS MODULE (class-based singleton)
// ============================================================

#include "pch.hpp"
#include <vector>
#include <ctime>
#include <cstdlib>

// ============================================================
// FILE-SCOPE CONSTANTS
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
// DOORS CLASS
// ============================================================

class Doors {
public:
    // Nested structs
    struct DoorState
    {
        ANOMALYID anomaly = ANOMALYID::None;

        float rollCooldown = 0.0f;
        float eventCooldown = 0.0f;
        float ghostRollCooldown = 0.0f;

        float ocdOffsetX = 0.0f;
        float ocdOffsetY = 0.0f;
    };

    struct BasementCheck
    {
        bool lockedThisDay = false;
    };

    // Render data
    struct Textures {
        AEGfxTexture* Door = nullptr;
        AEGfxTexture* DoorBoarded = nullptr;
        AEGfxTexture* Window0 = nullptr;
        AEGfxTexture* Window1 = nullptr;
        AEGfxTexture* Handprint = nullptr;
    } DoorTextures;

    AEGfxVertexList* QuadMesh = nullptr;
    s8               FontId = -1;

    // Event state
    DoorState DoorStates[NUM_DOORS]{};
    bool      Enabled = true;

    // Basement system
    BasementCheck BasementDoors[NUM_DOORS]{};

public:
    void Load();
    void Initialize();
    void Unload();
    int  Update(float camX);
    void Animate(float dt, int doorNearPlayer, float unused);
    void Draw(float camX, s8 floorNum, float textXoffset, float textY, bool dementia);
    void ResetAllLocks();
    bool TryDisposal(int floorNum, int doorIdx, bool& outDidJumpscare);

private:
    float DoorWorldX(int doorIndex, float camX);
    void  WindowWorldCenter(float doorX, float& x, float& y);
    void  ClearDoorEvent(int doorIdx);
    void  InitializeRandomOnce();
    void  TriggerHandprintSlam(int doorIdx);
    void  TriggerLightChange(int doorIdx);
    void  TriggerDoorOCD(int doorIdx);
    ANOMALYID PickFromPool(std::vector<ANOMALYID> const& pool);
    ANOMALYID GetDoorEventByIllness(ILLNESSES illness);
    ILLNESSES GetEffectiveIllnessForDoorEvents();
};

// ============================================================
// SINGLETON + WRAPPERS
// ============================================================

static Doors TheDoors;

void Doors_Load()                                                                   { TheDoors.Load(); }
void Doors_Initialize()                                                             { TheDoors.Initialize(); }
void Doors_Unload()                                                                 { TheDoors.Unload(); }
int  Doors_Update(float camX)                                                       { return TheDoors.Update(camX); }
void Doors_Animate(float dt, int doorNearPlayer, float camX)                        { TheDoors.Animate(dt, doorNearPlayer, camX); }
void Doors_Draw(float camX, s8 floorNum, float textXoffset, float textY, bool dementia) { TheDoors.Draw(camX, floorNum, textXoffset, textY, dementia); }
void Doors_ResetAllLocks()                                                          { TheDoors.ResetAllLocks(); }
bool Doors_TryDisposal(int floorNum, int doorIdx, bool& outDidJumpscare)            { return TheDoors.TryDisposal(floorNum, doorIdx, outDidJumpscare); }

// ============================================================
// BASEMENT SYSTEM
// ============================================================

void Doors::ResetAllLocks()
{
    for (int i = 0; i < NUM_DOORS; ++i)
        BasementDoors[i].lockedThisDay = false;
}

bool Doors::TryDisposal(int floorNum, int doorIdx, bool& outDidJumpscare)
{
    outDidJumpscare = false;

    if (doorIdx < 0 || doorIdx >= NUM_DOORS) return false;

    BasementCheck& s = BasementDoors[doorIdx];
    if (s.lockedThisDay) return false;

    s.lockedThisDay = true;

    // Presentation mode: force exactly 1 jumpscare on the first attempt
    if (Game_IsPresentationMode()) {
        static bool presentationScared = false;
        if (!presentationScared) {
            presentationScared = true;
            JumpScare_Start();
            outDidJumpscare = true;
            return false;
        }
        return true;
    }

    if (RandInt(0, 99) < 30)
    {
        JumpScare_Start();
        outDidJumpscare = true;
        return false;
    }

    return true;
}

// ============================================================
// PRIVATE HELPERS
// ============================================================

float Doors::DoorWorldX(int doorIndex, float camX)
{
    return DIST_BETWEEN_DOORS + camX + (DIST_BETWEEN_DOORS * doorIndex);
}

void Doors::WindowWorldCenter(float doorX, float& x, float& y)
{
    const float offX = (WINDOW_CENTER_U - 0.5f) * DOOR_W;
    const float offY = (WINDOW_CENTER_V - 0.5f) * DOOR_H;

    x = doorX + offX;
    y = DOOR_Y + offY;
}

void Doors::ClearDoorEvent(int doorIdx)
{
    DoorState& s = DoorStates[doorIdx];
    s.anomaly = ANOMALYID::None;
    s.eventCooldown = 0.0f;
    s.ocdOffsetX = 0.0f;
    s.ocdOffsetY = 0.0f;
}

void Doors::InitializeRandomOnce()
{
    // Global RNG is seeded once in utils.cpp; nothing to do here.
}

// ============================================================
// ANOMALY TRIGGERS
// ============================================================

void Doors::TriggerHandprintSlam(int doorIdx)
{
    DoorState& s = DoorStates[doorIdx];
    ClearDoorEvent(doorIdx);

    s.anomaly = ANOMALYID::Door_HandSlam;
    s.eventCooldown = 0.8f;
}

void Doors::TriggerLightChange(int doorIdx)
{
    DoorState& s = DoorStates[doorIdx];
    ClearDoorEvent(doorIdx);

    s.anomaly = ANOMALYID::Door_LightChange;
    s.eventCooldown = 0.8f;
}

void Doors::TriggerDoorOCD(int doorIdx)
{
    DoorState& s = DoorStates[doorIdx];
    ClearDoorEvent(doorIdx);

    s.anomaly = ANOMALYID::Door_Position;
    s.eventCooldown = 1.2f;

    // Randomly shift the window around the door a little bit
    s.ocdOffsetX = static_cast<float>(RandInt(-30, 30)); // -30 to +30
    s.ocdOffsetY = static_cast<float>(RandInt(-20, 20)); // -20 to +20
}

// ============================================================
// ILLNESS -> DOOR EVENT POOLS
// ============================================================

ANOMALYID Doors::PickFromPool(std::vector<ANOMALYID> const& pool)
{
    if (pool.empty()) return ANOMALYID::None;
    int r = RandInt(0, static_cast<int>(pool.size()) - 1);
    return pool[r];
}

ANOMALYID Doors::GetDoorEventByIllness(ILLNESSES illness)
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

ILLNESSES Doors::GetEffectiveIllnessForDoorEvents()
{
    ILLNESSES current = Player_GetTrueDiagnosis();
    if (current == ILLNESSES::GHOST)
        return Player_GetAnomalyIllness();
    return current;
}

// ============================================================
// LOAD / INIT / UNLOAD
// ============================================================

void Doors::Load()
{
    FontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

    DoorTextures.Door = LoadTextureChecked(Assets::Background::DoorBg);
    DoorTextures.DoorBoarded = LoadTextureChecked(Assets::Background::DoorBoarded);
    DoorTextures.Window0 = LoadTextureChecked(Assets::Door_Anomaly::Window_0);
    DoorTextures.Window1 = LoadTextureChecked(Assets::Door_Anomaly::Window_1);
    DoorTextures.Handprint = LoadTextureChecked(Assets::Door_Anomaly::Handprint);
}

void Doors::Initialize()
{
    InitializeRandomOnce();

    QuadMesh = CreateSquareMesh(0xFFFFFFFF);

    for (int i = 0; i < NUM_DOORS; ++i)
        DoorStates[i] = DoorState{};

    ResetAllLocks();
}

void Doors::Unload()
{
    FreeMeshSafe(QuadMesh);

    UnloadTextureSafe(DoorTextures.Door);
    UnloadTextureSafe(DoorTextures.DoorBoarded);
    UnloadTextureSafe(DoorTextures.Window0);
    UnloadTextureSafe(DoorTextures.Window1);
    UnloadTextureSafe(DoorTextures.Handprint);

    if (FontId >= 0)
        AEGfxDestroyFont(FontId);

    QuadMesh = nullptr;
    DoorTextures.Door = DoorTextures.DoorBoarded = DoorTextures.Window0 = DoorTextures.Window1 = DoorTextures.Handprint = nullptr;
    FontId = -1;
}

// ============================================================
// UPDATE: returns door index near the player (or -1)
// ============================================================

int Doors::Update(float camX)
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

void Doors::Animate(float dt, int doorNearPlayer, float /*unused*/)
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
        DoorState& s = DoorStates[d];

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

    DoorState& s = DoorStates[doorNearPlayer];


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

    if (RandInt(0, 99) >= EVENT_CHANCE_PERCENT)
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
        if (RandInt(0, 99) < GHOST_EXTRA_CHANCE_PERCENT)
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

void Doors::Draw(float camX, s8 floorNum, float textXoffset, float textY, bool dementia)
{
    if (!QuadMesh) return;

    int maxDoors = dementia ? 1000 : NUM_DOORS;

    for (int i = 0; i < maxDoors; ++i)
    {
        float doorX = DoorWorldX(i, camX);

        if (doorX < -SCREEN_WIDTH_HALF - DOOR_W) continue;
        if (doorX > SCREEN_WIDTH_HALF + DOOR_W) continue;

        int doorIdx = i % NUM_DOORS;
        DoorState& s = DoorStates[doorIdx];

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

        bool isBoarded = (floorNum == 0 && BasementDoors[doorIdx].lockedThisDay);

        // ------------------------------------------------
        // DOOR BASE
        // ------------------------------------------------
        DrawTextureMesh(QuadMesh, DoorTextures.Door, doorX, DOOR_Y, DOOR_W, DOOR_H, 1.0f);

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
        AEGfxTexture* windowTex = DoorTextures.Window0;

        if (s.anomaly == ANOMALYID::Door_LightChange)
        {
            windowTex = DoorTextures.Window1;
        }

        // Window background
        DrawSquareMesh(QuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        // Window image
        if (windowTex)
        {
            DrawTextureMesh(QuadMesh, windowTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);
        }

        // ------------------------------------------------
        // WINDOW OVERLAYS
        // ------------------------------------------------
        if (s.anomaly == ANOMALYID::Door_HandSlam && DoorTextures.Handprint)
        {
            DrawTextureMesh(QuadMesh, DoorTextures.Handprint, winX, winY, WINDOW_W, WINDOW_H, 1.0f);
        }

        // ------------------------------------------------
        // BOARDED OVERLAY
        // ------------------------------------------------
        if (isBoarded)
        {
            DrawTextureMesh(QuadMesh, DoorTextures.DoorBoarded, doorX, DOOR_Y, 300.0f, 200.0f, 1.0f);
        }

        // ------------------------------------------------
        // LABEL
        // ------------------------------------------------
        if (FontId >= 0)
        {
            std::ostringstream oss;

            if (floorNum == 0)
                oss << "B1-" << std::setw(2) << std::setfill('0') << (i + 1);
            else
                oss << std::setw(2) << std::setfill('0') << static_cast<int>(floorNum) << "-"
                    << std::setw(2) << std::setfill('0') << (i + 1);

            std::string text = oss.str();

            float x = (doorX / SCREEN_WIDTH_HALF) - textXoffset;
            float y = textY / SCREEN_HEIGHT_HALF;

            AEGfxPrint(FontId, text.c_str(), x, y, 1.0f, 1, 0, 0, 1);
        }
    }
}
