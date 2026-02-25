#include "pch.hpp"

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
static int      PlayerPool[6][5] = {
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
void Player_GenerateMission() {
    Patient_PickedUp = false;

    PickupDoor = (s8)(rand() % 10) + 1;
    PickupFloor = (s8)(rand() % 9) + 1;

    do {
        DestDoor = (s8)(rand() % 10) + 1;
        DestFloor = (s8)(rand() % 9) + 1;
    } while (DestDoor == PickupDoor && DestFloor == PickupFloor);
}

bool Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day) {

    // PHASE 1: PICKUP
    if (!Patient_PickedUp) {
        if (currentFloor == PickupFloor && doorNumAtPlayer == PickupDoor) {
            Patient_PickedUp = true;
            // ROLL FOR GHOST: The walk to delivery is now haunted
            Player_SetScaryByDay(day);
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
    // 1. Reset animation/timers so the new patient starts clean
    gFrame = 0;
    gTimer = 0.0f;

    // 2. Assign Illness based on the Scary status already set by the Brain
    if (!gIsScary) {
        // Randomly pick 1 of the 4 standard illnesses (0 to 3)
        // PARANOIA, MANIA, DEPRESSION, DEMENTIA
        gCurrentIllness = (ILLNESSES)(std::rand() % 4);
    }
    else {
        // Ghosts always use the special ALL state for the Mania animation
        gCurrentIllness = ALL;
    }
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
void Player_GetTargetRoom(s8& floor, s8& door) {
    if (!Patient_PickedUp) {
        floor = PickupFloor; door = PickupDoor;
    }
    else {
        floor = DestFloor; door = DestDoor;
    }
}

float Player_GetWidth()  { return PLAYER_WIDTH; }
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
    gHumanTex[0] = AEGfxTextureLoad("Assets/Player/human player_1.png");
    gHumanTex[1] = AEGfxTextureLoad("Assets/Player/human player_2.png");

    gScaryTex[0] = AEGfxTextureLoad("Assets/Player/scary player_1.png");
    gScaryTex[1] = AEGfxTextureLoad("Assets/Player/scary player_2.png");

    gNoPatientTex[0] = AEGfxTextureLoad("Assets/Player/nurse_1.png");
    gNoPatientTex[1] = AEGfxTextureLoad("Assets/Player/nurse_2.png");
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
    float sx = (PLAYER_WIDTH * widthMultiplier)*(float)gFacing;   // // 1 = normal, -1 = flipped
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
        if (gHumanTex[i]) AEGfxTextureUnload(gHumanTex[i]);
        if (gScaryTex[i]) AEGfxTextureUnload(gScaryTex[i]);
        gHumanTex[i] = nullptr;
        gScaryTex[i] = nullptr;
    }

    if (gNoPatientTex)
    {
        AEGfxTextureUnload(gNoPatientTex[gFrame]);
        gNoPatientTex[gFrame] = nullptr;
    }

    if (gSpriteMesh)
    {
        AEGfxMeshFree(gSpriteMesh);
        gSpriteMesh = nullptr;
    }
}

/*****************************************************************************************/
