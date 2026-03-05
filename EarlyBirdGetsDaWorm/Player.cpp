#include "pch.hpp"
#include <algorithm> // std::shuffle
#include <random>    // std::default_random_engine
#include <ctime>     // std::time
#include <vector>    // safe to keep (not used now)

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
static constexpr float FRAME_TIME = 0.4f;   // walking speed (time per frame)

/*************************************** RENDER DATA ********************************************/
static AEGfxVertexList* gSpriteMesh = nullptr;

static AEGfxTexture* gHumanTex[2]{};
static AEGfxTexture* gScaryTex[2]{};
static AEGfxTexture* gNoPatientTex[2]{};

/*************************************** STATE **************************************************/
static bool gIsScary = false; // TRUTH from day pool (we treat scary==ghost truth)
static bool Patient_PickedUp = false;
static bool gVisualIsScary = false; // PNG BLUFF only (random)

static int  gFacing = 1; // 1 right, -1 left
static int  gFrame = 0;

static float gTimer = 0.0f;

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
// gCurrentIllness is what OTHER systems read using Player_GetCurrentIllness().
// If the patient is a ghost, we set gCurrentIllness = ILLNESSES::GHOST,
// but we keep a "mimic" illness for effects/clues base.
static ILLNESSES gCurrentIllness{};  // What the game sees (can be GHOST)
static ILLNESSES gMimicIllness{};    // The real illness the ghost is mimicking

// ============================================================
// GHOST LOGIC = "truth"
// ============================================================
static bool gCarriedPatientIsGhost = false;

// "Ghost = +1 extra anomaly" (like ALL but only +1)
static int gGhostExtraAnomalies = 0;

/************************************* HELPERS ***************************************************/

// Pick a REAL illness for the base/mimic.
// IMPORTANT:
// - Do NOT include ILLNESSES::ALL here.
// - Do NOT include ILLNESSES::GHOST here (ghost is a TAG, not a real illness).
static ILLNESSES Player_RandomRealIllness()
{
    static ILLNESSES illnessPool[] =
    {
        ILLNESSES::PARANOIA,
        ILLNESSES::MANIA,
        ILLNESSES::DEPRESSION,
        ILLNESSES::DEMENTIA,
        ILLNESSES::SCHIZOPHRENIA,
        ILLNESSES::AIW_SYNDROME,
        ILLNESSES::INSOMNIA,
        ILLNESSES::OCD,
        ILLNESSES::SCOTOPHOBIA
    };

    const int n = (int)(sizeof(illnessPool) / sizeof(illnessPool[0]));
    return illnessPool[rand() % n];
}

// Decide and apply everything for a newly picked up patient.
// Keeps logic in one place so you don't desync ghost/illness/visual.
static void Player_ApplyNewPatientFromTruth(bool isGhostTruth)
{
    // TRUTH
    gCarriedPatientIsGhost = isGhostTruth;

    // Base illness that the patient (or ghost) is "about"
    gMimicIllness = Player_RandomRealIllness();

    // If ghost: tag illness as GHOST, but keep mimic illness around.
    // If human: current illness is the real illness.
    if (gCarriedPatientIsGhost)
    {
        gCurrentIllness = ILLNESSES::GHOST;
        gGhostExtraAnomalies = 1;   // <- this is your "+1 extra anomaly" signal
    }
    else
    {
        gCurrentIllness = gMimicIllness;
        gGhostExtraAnomalies = 0;
    }

    // PNG bluff stays random (purely visuals)
    gVisualIsScary = (std::rand() % 2 == 0);

    // reset walk anim
    gFrame = 0;
    gTimer = 0.0f;
}

/************************************* MISSION ***************************************************/

void Player_GenerateMission()
{
    // Reset carry state
    Patient_PickedUp = false;
    gCarriedPatientIsGhost = false;
    gIsScary = false;
    gVisualIsScary = false;
    gGhostExtraAnomalies = 0;

    // Reset illness values
    gCurrentIllness = ILLNESSES{}; // default
    gMimicIllness = ILLNESSES{}; // default

    // Clear evidence for next patient (so old anomalies don't carry over)
    Journal_Clear();

    // Generate pickup and destination rooms
    PickupDoor = (s8)(rand() % 10) + 1;
    PickupFloor = (s8)(rand() % 9) + 1;

    do
    {
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
    if (!Patient_PickedUp)
    {
        if (currentFloor == PickupFloor && doorNumAtPlayer == PickupDoor)
        {
            Patient_PickedUp = true;

            // Roll ghost/human truth using day pool
            Player_SetScaryByDay(day);            // sets gIsScary (truth)
            Player_ApplyNewPatientFromTruth(gIsScary);

            return true;
        }
    }
    // -------------------------
    // PHASE 2: DELIVERY
    // -------------------------
    else
    {
        if (currentFloor == DestFloor && doorNumAtPlayer == DestDoor)
        {
            // delivered successfully (if your Game.cpp decides to kill player for ghost delivery,
            // it should do so BEFORE calling this, or stop calling this on that case)
            Patient_PickedUp = false;
            gCarriedPatientIsGhost = false;
            gIsScary = false;
            gVisualIsScary = false;
            gGhostExtraAnomalies = 0;

            Player_GenerateMission();
            return true;
        }
    }

    return false; // Wrong room / no interaction
}

/************************************* DAY POOL **************************************************/

void Player_ResetPatientCounter(int day)
{
    PatientsHandled = 0;

    int current_day = (day < 1) ? 1 : (day > 5 ? 5 : day);

    // Copy the master pool for the specific day into active pool
    for (int i = 0; i < 5; ++i)
        CurrentDayPool[i] = PlayerPool[current_day][i];

    // Shuffle active pool so ghost positions are different each day run
    unsigned seed = (unsigned)time(NULL);
    std::shuffle(std::begin(CurrentDayPool), std::end(CurrentDayPool), std::default_random_engine(seed));
}

// If you still want to keep this helper, it just resets animation.
// (Illness is assigned in Player_ApplyNewPatientFromTruth now.)
void Player_NewPatientRandom()
{
    gFrame = 0;
    gTimer = 0.0f;
}

// Decide ghost/human based on day pool / probabilities.
// This sets gIsScary (truth).
void Player_SetScaryByDay(int day)
{
    int d = (day < 1) ? 1 : (day > 5 ? 5 : day);
    bool isScary = false;

    // Balanced pool for the first 5 patients
    if (PatientsHandled < 5)
    {
        isScary = (CurrentDayPool[PatientsHandled] == 1);
    }
    // After 5 patients, use percentage roll
    else
    {
        float extraRollProb[] = { 0.0f, 0.20f, 0.20f, 0.35f, 0.35f, 0.40f };
        isScary = ((float)(rand() % 101) / 100.0f) < extraRollProb[d];
    }

    // Set truth
    Player_SetScary(isScary);

    // Keep your reset helper
    Player_NewPatientRandom();

    // Next patient slot
    PatientsHandled++;
}

/*************************************** MODIFIERS ***********************************************/

void Player_SetScary(bool scary)
{
    // IMPORTANT:
    // gIsScary = TRUTH (ghost/human) decided by day pool.
    gIsScary = scary;

    // Keep this mirrored so gameplay checks stay consistent.
    // (If you prefer, you can remove gCarriedPatientIsGhost entirely and just use gIsScary.)
    gCarriedPatientIsGhost = scary;

    gFrame = 0;
    gTimer = 0.0f;
}

void Player_SetIllness(ILLNESSES illness)
{
    gCurrentIllness = illness;
}

void Player_SetFacing(int dir)
{
    gFacing = (dir >= 0) ? 1 : -1;
}

/*************************************** ACCESSORS ***********************************************/

// TRUTH: ghost/human
bool Player_IsGhostPatient()
{
    return gCarriedPatientIsGhost;
}

// NOTE: this name is confusing now, but we keep it for compatibility.
// It returns TRUTH (same as ghost/human roll), not PNG visuals.
bool Player_IsScaryPatient()
{
    return gIsScary;
}

// What the rest of the game reads.
// If ghost: returns ILLNESSES::GHOST
// If human: returns real illness
ILLNESSES Player_GetCurrentIllness()
{
    return gCurrentIllness;
}

// Useful if you want illness effects/clues even when patient is ghost.
ILLNESSES Player_GetMimicIllness()
{
    return gMimicIllness;
}

// "Ghost = +1 extra anomaly" signal
int Player_GetGhostExtraAnomalies()
{
    return gGhostExtraAnomalies; // 1 if ghost, else 0
}

static AEGfxTexture* GetActiveFrameTex()
{
    // Nurse only (no patient yet)
    if (!Patient_PickedUp)
        return gNoPatientTex[gFrame];

    // If carrying a patient: show PNG bluff
    return gVisualIsScary ? gScaryTex[gFrame] : gHumanTex[gFrame];
}

// Used by Notifications.cpp and others
bool Player_HasPatient()
{
    return Patient_PickedUp;
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
    gSpriteMesh = AEGfxMeshEnd();

    // Textures
    gHumanTex[0] = LoadTextureChecked(Assets::Player::Human1);
    gHumanTex[1] = LoadTextureChecked(Assets::Player::Human2);
    gScaryTex[0] = LoadTextureChecked(Assets::Player::Scary1);
    gScaryTex[1] = LoadTextureChecked(Assets::Player::Scary2);
    gNoPatientTex[0] = LoadTextureChecked(Assets::Player::Nurse1);
    gNoPatientTex[1] = LoadTextureChecked(Assets::Player::Nurse2);
}

/***************************************** UPDATE ************************************************/

void Player_Update(float dt, bool walkKey)
{
    if (walkKey)
    {
        gTimer += dt;
        if (gTimer >= FRAME_TIME)
        {
            gTimer -= FRAME_TIME;
            gFrame = (gFrame + 1) % 2;
        }
    }
    else
    {
        gFrame = 0;
        gTimer = 0.0f;
    }
}

/***************************************** DRAW **************************************************/

void Player_Draw(float x, float y)
{
    AEMtx33 scale, trans, transform;

    float widthMultiplier = Patient_PickedUp ? 1.0f : 0.3f;
    float sx = (PLAYER_WIDTH * widthMultiplier) * (float)gFacing;
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

    AEGfxMeshDraw(gSpriteMesh, AE_GFX_MDM_TRIANGLES);

    // reset (optional)
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}

/**************************************** UNLOAD *************************************************/

void Player_Unload()
{
    for (int i = 0; i < 2; ++i)
    {
        UnloadTextureSafe(gHumanTex[i]);
        UnloadTextureSafe(gScaryTex[i]);
        gHumanTex[i] = nullptr;
        gScaryTex[i] = nullptr;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (gNoPatientTex[i])
            UnloadTextureSafe(gNoPatientTex[i]);
        gNoPatientTex[i] = nullptr;
    }

    FreeMeshSafe(gSpriteMesh);
}