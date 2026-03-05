#include "pch.hpp"
#include <algorithm> // shuffle
#include <random>    // default_random_engine
#include <ctime>     // time
#include <vector>    // (optional, not used now but safe)

/*************************************** VARIABLES ***************************************/
static constexpr float FRAME_TIME = 0.4f;  // walking speed (time per frame)

static AEGfxVertexList* gSpriteMesh = nullptr;
static AEGfxTexture* gHumanTex[2]{};
static AEGfxTexture* gScaryTex[2]{};
static AEGfxTexture* gNoPatientTex[2]{};

static bool     gIsScary = false;
static bool     Patient_PickedUp = false;
static bool     gVisualIsScary = false;

static int      gFacing = 1;     // 1 right, -1 left
static int      gFrame = 0;
static int      CurrentDayPool[5];
static int      PatientsHandled = 0;
static int      PlayerPool[6][5] =
{
    {0,0,0,0,0}, // Day 0 (Unused)
    {0,1,0,0,0}, // Day 1: 1 Ghost (20%)
    {0,1,0,0,1}, // Day 2: 2 Ghosts (40%)
    {0,1,1,0,0}, // Day 3: 2 Ghosts (40%)
    {0,1,1,1,0}, // Day 4: 3 Ghosts (60%)
    {0,1,1,1,0}  // Day 5: 3 Ghosts (60%)
};

static float gTimer = 0.0f;

static s8 PickupDoor, PickupFloor;
static s8 DestDoor, DestFloor;

ILLNESSES gCurrentIllness{};

/************************************* HELPERS *******************************************/

// Pick a REAL illness (no ALL / no NONE).
// IMPORTANT: We rely on Player_HasPatient() to decide whether systems should apply illness effects.
// If you want "no illness" state, do NOT use ILLNESSES::NONE (not defined) — just gate by Player_HasPatient().
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

void Player_GenerateMission() {
    Patient_PickedUp = false;

    // Clear evidence for next patient (so old anomalies don't carry over)
    Journal_Clear();

    // Reset "truth" state for next patient
    Player_SetScary(false);

    PickupDoor = (s8)(rand() % 10) + 1;
    PickupFloor = (s8)(rand() % 9) + 1;

    do {
        DestDoor = (s8)(rand() % 10) + 1;
        DestFloor = (s8)(rand() % 9) + 1;   // floors 1-9 ONLY (no 00)
    } while (DestDoor == PickupDoor && DestFloor == PickupFloor);
}

bool Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day) {

    // PHASE 1: PICKUP
    if (!Patient_PickedUp) {
        if (currentFloor == PickupFloor && doorNumAtPlayer == PickupDoor) {
            Patient_PickedUp = true;

            // ROLL FOR GHOST (truth state)
            // Ghost logic should NOT depend on PNG. PNG stays random bluff.
            Player_SetScaryByDay(day);

            // Always assign a REAL illness (ghost mimics a real illness).
            // Extra anomaly that makes it "ghost" should be handled by your anomaly system,
            // not by setting illness to ALL.
            gCurrentIllness = Player_RandomRealIllness();

            // KEEP PNG RANDOM BLUFF (do NOT change this)
            gVisualIsScary = (std::rand() % 2 == 0);

            return true;
        }
    }

    // PHASE 2: DELIVERY
    else {
        if (currentFloor == DestFloor && doorNumAtPlayer == DestDoor) {
            Patient_PickedUp = false;
            // Mission Complete: Set up the next one
            Player_GenerateMission();
            return true;
        }
    }
    return false; // Wrong room or no door nearby
}

void Player_ResetPatientCounter(int day) {
    PatientsHandled = 0;
    int current_day = (day < 1) ? 1 : (day > 5 ? 5 : day);

    // 1. Copy the master pool for the specific day into our active pool
    for (int i = 0; i < 5; ++i) {
        CurrentDayPool[i] = PlayerPool[current_day][i];
    }

    // 2. Shuffle the active pool using the current time as a seed
    // This ensures the order is different every single time the day starts
    unsigned seed = (unsigned)time(NULL);
    std::shuffle(std::begin(CurrentDayPool), std::end(CurrentDayPool), std::default_random_engine(seed));
}

void Player_NewPatientRandom()
{
    gFrame = 0;
    gTimer = 0.0f;

    // Assign Illness based on the Scary status already set by the Brain
    // NOTE: We no longer use AllAnomalies_CurrentRun() and we do NOT use ALL.
    // Ghost still gets a REAL illness (mimic), and the "extra anomaly" clue is handled elsewhere.
    gCurrentIllness = Player_RandomRealIllness();
}

void Player_SetScaryByDay(int day) {
    int d = (day < 1) ? 1 : (day > 5 ? 5 : day);
    bool isScary = false;

    // Use the balanced pool for the first 5 patients
    if (PatientsHandled < 5) {
        isScary = (CurrentDayPool[PatientsHandled] == 1);
    }
    // After 5 patients, use your percentage roll
    else {
        float extraRollProb[] = { 0.0f, 0.20f, 0.20f, 0.35f, 0.35f, 0.40f };
        isScary = ((float)(rand() % 101) / 100.0f) < extraRollProb[d];
    }

    // 1. Set the global status
    Player_SetScary(isScary);

    // 2. Set up the specific patient details (Illness, Anim Resets)
    Player_NewPatientRandom();

    // 3. Increment for the next call
    PatientsHandled++;
}

/*************************************** MODIFIERS ***************************************/
void Player_SetScary(bool scary)
{
    gIsScary = scary;
    gFrame = 0;
    gTimer = 0.0f;
}

void Player_SetIllness(ILLNESSES illness) {
    gCurrentIllness = illness;
}

void Player_SetFacing(int dir)
{
    gFacing = (dir >= 0) ? 1 : -1;
}

/*************************************** ACCESSORS ***************************************/

bool Player_IsScaryPatient()
{
    return gIsScary;
}

ILLNESSES Player_GetCurrentIllness()
{
    return gCurrentIllness;
}

static AEGfxTexture* GetActiveFrameTex()
{
    // If the nurse hasn't picked up a patient yet
    if (!Patient_PickedUp)
    {
        return gNoPatientTex[gFrame];
    }

    // If a patient is picked up, choose between Human and Scary sets
    return gVisualIsScary ? gScaryTex[gFrame] : gHumanTex[gFrame];
}

// Getters for Notifications.cpp
bool Player_HasPatient() { return Patient_PickedUp; }
void Player_GetTargetRoom(s8& patientFloor, s8& patientDoor, s8& destFloor, s8& destDoor) {
    patientFloor = PickupFloor;
    patientDoor = PickupDoor;

    destFloor = DestFloor;
    destDoor = DestDoor;

    /*if (!Patient_PickedUp) {
        floor = PickupFloor; door = PickupDoor;
    }
    else {
        floor = DestFloor; door = DestDoor;
    }*/
}

float Player_GetWidth() { return PLAYER_WIDTH; }
float Player_GetHeight() { return PLAYER_HEIGHT; }

/***************************************** LOAD ******************************************/
void Player_Load()
{
    std::srand((unsigned)std::time(nullptr));

    // mesh 
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    gSpriteMesh = AEGfxMeshEnd();

    // load BOTH sets
    gHumanTex[0] = LoadTextureChecked(Assets::Player::Human1);
    gHumanTex[1] = LoadTextureChecked(Assets::Player::Human2);
    gScaryTex[0] = LoadTextureChecked(Assets::Player::Scary1);
    gScaryTex[1] = LoadTextureChecked(Assets::Player::Scary2);
    gNoPatientTex[0] = LoadTextureChecked(Assets::Player::Nurse1);
    gNoPatientTex[1] = LoadTextureChecked(Assets::Player::Nurse2);
}

/***************************************** UPDATE ****************************************/
void Player_Update(float dt, bool walkKey)
{
    if (walkKey)
    {
        gTimer += dt;
        if (gTimer >= FRAME_TIME)
        {
            gTimer -= FRAME_TIME;
            gFrame = (gFrame + 1) % 2; // 2 frames
        }
    }
    else
    {
        gFrame = 0;
        gTimer = 0.0f;
    }
}

/***************************************** DRAW ******************************************/
void Player_Draw(float x, float y)
{
    AEMtx33 scale, trans, transform;
    float widthMultiplier = Patient_PickedUp ? 1.0f : 0.3f;
    float sx = (PLAYER_WIDTH * widthMultiplier) * (float)gFacing;   // // 1 = normal, -1 = flipped
    float sy = PLAYER_HEIGHT;

    // build transform matrix
    AEMtx33Scale(&scale, sx, sy);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    // texture drawing settings (same as demo)
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxTextureSet(GetActiveFrameTex(), 0.0f, 0.0f);
    AEGfxSetTransform(transform.m);

    // draw the UV mesh
    AEGfxMeshDraw(gSpriteMesh, AE_GFX_MDM_TRIANGLES);

    // (optional) reset 
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}

/**************************************** UNLOAD *****************************************/
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
        {
            UnloadTextureSafe(gNoPatientTex[i]);
        }
    }

    FreeMeshSafe(gSpriteMesh);
}

/*****************************************************************************************/