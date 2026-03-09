#include "pch.hpp"
#include <algorithm> // std::shuffle
#include <random>    // std::default_random_engine
#include <ctime>     // std::time

/**************************************************************************************************
    PLAYER.CPP
    - Handles:
        * Mission generation (pickup door/floor, destination door/floor)
        * Picking up / delivering a patient
        * Truth: Human vs Ghost (gameplay)
        * Visual bluff: scary PNG vs human PNG (purely visuals)
        * Illness assignment (real illness) + Ghost tag illness (ILLNESSES::GHOST)
        * "Ghost = +1 extra anomaly" flag (minimal, doesn't require big file changes)
**************************************************************************************************/

/*************************************** CONFIG *************************************************/

static constexpr float      FRAME_TIME = 0.4f;
static AEGfxVertexList      *SpriteMesh = nullptr;
static AEGfxTexture         *HumanTex[2]{}, *ScaryTex[2]{}, *NoPatientTex[2]{};

/*************************************** STATE **************************************************/
static bool CarriedPatientIsGhost = false; // TRUTH from day pool (we treat scary==ghost truth)
static bool PatientPickedUp = false;
static bool VisualBluff = false; // PNG BLUFF only (random)
static bool WalkingSoundPlaying = false;    
static bool WheelchairSoundPlaying = false;


static int  FacingDir = 1; // 1 right, -1 left
static int  Frame = 0;
static float Timer = 0.0f;

// Mission targets
static s8 PickupDoor = 0, PickupFloor = 0;
static s8 DestDoor = 0, DestFloor = 0;

// Day pool / pacing
static int CurrentDayPool[5]{};
static int PatientsHandled = 0;

// 0 = human, 1 = ghost
static int PlayerPool[6][5] =
{
    {0,0,0,0,0}, // Day 0 (Unused)
    {0,1,0,0,0}, // Day 1: 1 Ghost (20%)
    {0,1,0,0,1}, // Day 2: 2 Ghosts (40%)
    {0,1,1,0,0}, // Day 3: 2 Ghosts (40%)
    {0,1,1,1,0}, // Day 4: 3 Ghosts (60%)
    {0,1,1,1,0}  // Day 5: 3 Ghosts (60%)
};

// ============================================================
// ILLNESS / GHOST TAG
// ============================================================
// CurrentIllness is what OTHER systems read using Player_GetCurrentIllness().
// If the patient is a ghost, we set CurrentIllness = ILLNESSES::GHOST,
// but we keep a "mimic" illness for effects/clues base.
static ILLNESSES CurrentIllness{};  // What the game sees (can be GHOST)
static ILLNESSES MimicIllness{};    // The real illness the ghost is mimicking

// ============================================================
// GHOST LOGIC = "truth"
// ============================================================
// "Ghost = +1 extra anomaly" (like ALL but only +1)
static int GhostExtraAnomalies = 0;

/************************************* HELPERS ***************************************************/

static ILLNESSES PatientRandomIllness()
{
    const std::vector<ILLNESSES>& activePool = AllAnomalies_CurrentRun();

    if (activePool.empty()) { return ILLNESSES::NONE; }

    int randomIndex = rand() % (int)activePool.size();

    return activePool[randomIndex];
}

// Decide and apply everything for a newly picked up patient.
// Keeps logic in one place so you don't desync ghost/illness/visual.
static void ApplyNewPatientFromTruth(bool isGhostTruth)
{
    CarriedPatientIsGhost = isGhostTruth;
    MimicIllness = PatientRandomIllness();

    if (CarriedPatientIsGhost) {
        CurrentIllness = ILLNESSES::GHOST;
        GhostExtraAnomalies = 1;
    } else {
        CurrentIllness = MimicIllness;
        GhostExtraAnomalies = 0;
    }

    VisualBluff = (std::rand() % 2 == 0);
    Frame = 0;
    Timer = 0.0f;

    CentralPool_GeneratePatientAnomalies(MimicIllness, GhostExtraAnomalies);
}

/************************************* MISSION ***************************************************/

void Player_GenerateMission()
{
    // Reset carry state
    PatientPickedUp = false;
    CarriedPatientIsGhost = false;
    VisualBluff = false;
    GhostExtraAnomalies = 0;
    CurrentIllness = MimicIllness = ILLNESSES{}; // default

    // Clear evidence for next patient (so old anomalies don't carry over)
    CentralPool_ClearPatientAnomalies();
    Journal_Clear();

    // Generate pickup and destination rooms
    PickupDoor = (s8)(rand() % 10) + 1;
    PickupFloor = (s8)(rand() % 9) + 1;
    do {
        DestDoor = (s8)(rand() % 10) + 1;
        DestFloor = (s8)(rand() % 9) + 1; // floors 1-9 ONLY (no 00)
    } while (DestDoor == PickupDoor && DestFloor == PickupFloor);
}

// Called by your door interaction logic.
// Returns true if an interaction happened (pickup or delivery).
bool Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day)
{
    // -------------------------
    // PHASE 1: PICKUP
    // -------------------------
    if (!PatientPickedUp) {
        if (currentFloor == PickupFloor && doorNumAtPlayer == PickupDoor) {
            PatientPickedUp = true;
            SetPatientScaryByDay(day);            // This calls ApplyNewPatientFromTruth internally
            return true;
        }
    }
    // -------------------------
    // PHASE 2: DELIVERY
    // -------------------------
    else {
        if (currentFloor == DestFloor && doorNumAtPlayer == DestDoor) {
            // delivered successfully (if your Game.cpp decides to kill player for ghost delivery,
            // it should do so BEFORE calling this, or stop calling this on that case)
            Player_GenerateMission();
            return true;
        }
    }
    return false; // Wrong room / no interaction
}

/************************************* DAY POOL **************************************************/

void ResetPatientCounter(int day)
{
    PatientsHandled = 0;

    int CurrentDay = (day < 1) ? 1 : (day > 5 ? 5 : day);

    // Copy the master pool for the specific day into active pool
    for (int i = 0; i < 5; ++i)
        CurrentDayPool[i] = PlayerPool[CurrentDay][i];

    // Shuffle active pool so ghost positions are different each day run
    unsigned seed = (unsigned)time(NULL);
    std::shuffle(std::begin(CurrentDayPool), std::end(CurrentDayPool), std::default_random_engine(seed));
}


void SetPatientScaryByDay(int day) {
    int Day = (day < 1) ? 1 : (day > 5 ? 5 : day);
    bool IsScary = false;

    if (PatientsHandled < 5) {
        IsScary = (CurrentDayPool[PatientsHandled] == 1);
    } else {
        // Define the odds for Day 0 through 5 clearly
        static constexpr float extraRollProb[] = { 0.0f, 0.20f, 0.20f, 0.35f, 0.35f, 0.40f };

        // Roll a random float between 0.00 and 1.00
        float roll = (float)(std::rand() % 101) / 100.0f;

        // Check if our roll falls within the scary probability for today
        IsScary = (roll < extraRollProb[Day]);
    }

    ApplyNewPatientFromTruth(IsScary);
    PatientsHandled++;
}

/*************************************** MODIFIERS ***********************************************/
void Player_ForceHumanPatient()
{
    // Run the full generation logic, forcing 'false' (Human)
    ApplyNewPatientFromTruth(false);

    // Override the visual bluff so the tutorial PNG is definitely normal
    VisualBluff = false;
}

void Player_SetIllness(ILLNESSES illness) {
    CurrentIllness = illness;
}

void Player_SetFacing(int dir) {
    FacingDir = (dir >= 0) ? 1 : -1;
}

/*************************************** ACCESSORS ***********************************************/

// TRUTH: ghost/human
bool IsGhostPatient()
{
    return CarriedPatientIsGhost;
}

// What the rest of the game reads.
// If ghost: returns ILLNESSES::GHOST
// If human: returns real illness
ILLNESSES Player_GetCurrentIllness()
{
    return CurrentIllness;
}

// Useful if you want illness effects/clues even when patient is ghost.
ILLNESSES Player_GetMimicIllness()
{
    return MimicIllness;
}

// "Ghost = +1 extra anomaly" signal
int Player_GetGhostExtraAnomalies()
{
    return GhostExtraAnomalies; // 1 if ghost, else 0
}

static AEGfxTexture* GetActiveFrameTex()
{
    // Nurse only (no patient yet)
    if (!PatientPickedUp)
        return NoPatientTex[Frame];

    // If carrying a patient: show PNG bluff
    return VisualBluff ? ScaryTex[Frame] : HumanTex[Frame];
}

// Used by Notifications.cpp and others
bool Player_HasPatient()
{
    return PatientPickedUp;
}

void Player_GetTargetRoom(s8& patientFloor, s8& patientDoor, s8& destFloor, s8& destDoor)
{
    patientFloor = PickupFloor;
    patientDoor = PickupDoor;

    destFloor = DestFloor;
    destDoor = DestDoor;
}

float Player_GetWidth() { return PLAYER_WIDTH; }
float Player_GetHeight() { return PLAYER_HEIGHT; }

/***************************************** LOAD **************************************************/

void Player_Load()
{
    std::srand((unsigned)std::time(nullptr));

    // Mesh
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    SpriteMesh = AEGfxMeshEnd();

    // Textures
    HumanTex[0] = LoadTextureChecked(Assets::Player::Human1);
    HumanTex[1] = LoadTextureChecked(Assets::Player::Human2);
    ScaryTex[0] = LoadTextureChecked(Assets::Player::Scary1);
    ScaryTex[1] = LoadTextureChecked(Assets::Player::Scary2);
    NoPatientTex[0] = LoadTextureChecked(Assets::Player::Nurse1);
    NoPatientTex[1] = LoadTextureChecked(Assets::Player::Nurse2);
}

/***************************************** UPDATE ************************************************/
void Player_Update(float dt, bool walkKey)
{
    // ----------------------------------------
    // Walking animation
    // ----------------------------------------
    if (walkKey)
    {
        Timer += dt;

        if (Timer >= FRAME_TIME)
        {
            Timer -= FRAME_TIME;
            Frame = (Frame + 1) % 2;
        }
    }
    else
    {
        Frame = 0;
        Timer = 0.0f;
    }

    // ----------------------------------------
    // Audio logic
    // ----------------------------------------
    if (!walkKey)
    {
        if (WalkingSoundPlaying || WheelchairSoundPlaying)
        {
            AudioManager_StopSFX();
            WalkingSoundPlaying = false;
            WheelchairSoundPlaying = false;
        }
        return;
    }

    if (Player_HasPatient())
    {
        // if wheelchair mode should play
        if (!WheelchairSoundPlaying)
        {
            AudioManager_StopSFX();
            AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.5f);
            AudioManager_PlaySFX(SFX_WHEELCHAIR_SQUEAK, 0.1f);

            WalkingSoundPlaying = true;
            WheelchairSoundPlaying = true;
        }
    }
    else
    {
        // normal walking only
        if (!WalkingSoundPlaying || WheelchairSoundPlaying)
        {
            AudioManager_StopSFX();
            AudioManager_PlaySFX(SFX_PLAYER_WALKING, 0.5f);

            WalkingSoundPlaying = true;
            WheelchairSoundPlaying = false;
        }
    }
}

/***************************************** DRAW **************************************************/

void Player_Draw(float x, float y)
{
    AEMtx33 scale, trans, transform;

    float widthMultiplier = PatientPickedUp ? 1.0f : 0.3f;
    float sx = (PLAYER_WIDTH * widthMultiplier) * (float)FacingDir;
    float sy = PLAYER_HEIGHT;

    AEMtx33Scale(&scale, sx, sy);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxTextureSet(GetActiveFrameTex(), 0.0f, 0.0f);
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(SpriteMesh, AE_GFX_MDM_TRIANGLES);

    // reset (optional)
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}

/**************************************** UNLOAD *************************************************/

void Player_Unload()
{
    for (int i = 0; i < 2; ++i)
    {
        UnloadTextureSafe(HumanTex[i]);
        UnloadTextureSafe(ScaryTex[i]);
        HumanTex[i] = nullptr;
        ScaryTex[i] = nullptr;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (NoPatientTex[i])
            UnloadTextureSafe(NoPatientTex[i]);
        NoPatientTex[i] = nullptr;
    }

    FreeMeshSafe(SpriteMesh);
}