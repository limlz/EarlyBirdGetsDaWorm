#include "pch.hpp"

static constexpr float FRAME_TIME = 0.3f;

static AEGfxVertexList* SpriteMesh = nullptr;
static AEGfxTexture* HumanTex[2]{};
static AEGfxTexture* ScaryTex[2]{};
static AEGfxTexture* NoPatientTex[2]{};

static bool CarriedPatientIsGhost = false;
static bool PatientPickedUp = false;
static bool VisualBluff = false;
static bool WalkingSoundPlaying = false;
static bool WheelchairSoundPlaying = false;
static int FacingDir = 1;
static int Frame = 0;
static float Timer = 0.0f;
static int GhostExtraAnomalies = 0;
static ILLNESSES TrueDiagnosis{};
static ILLNESSES AnomalyIllness{};

static s8 PickupDoor = 0, PickupFloor = 0;
static s8 DestDoor = 0, DestFloor = 0;

// Tracks the daily quota and random pool of normal vs ghost patients
static int CurrentDayPool[5]{};
static int PatientsHandled = 0;
// Playtest pool: exactly 1 ghost and 1 human per day, shuffled each day
static int PlaytestDayPool[2] = { 1, 0 };

// 2D Array acting as a truth table for predetermined patient scares based on the Day (1-5)
static int PlayerPool[6][5] = {
    {0,0,0,0,0}, // Day 0 (Padding)
    {0,1,0,0,0}, // Day 1
    {0,1,0,0,1}, // Day 2
    {0,1,1,0,0}, // Day 3
    {0,1,1,1,0}, // Day 4
    {0,1,1,1,0}  // Day 5
};

// --- Blink Transition State ---
static bool IsBlinking = false;
static float BlinkTimer = 0.0f;
static constexpr float BLINK_DURATION = 1.0f;
static float TopLidY = 1000.0f;
static float BottomLidY = -1000.0f;

// Trackers to delay the game logic until eyes are closed
static bool PendingPickup = false;
static bool PendingDelivery = false;
static int PendingDay = 0;
static bool StateChangedDuringBlink = false;


/******************************* PLAYER & PATIENT LOGIC **********************************/
/*****************************************************************************************/
// Selects a random illness from the actively generated Central Pool run.
/*****************************************************************************************/
static ILLNESSES PatientRandomIllness() {
    const std::vector<ILLNESSES>& activePool = AllAnomalies_CurrentRun();
    if (activePool.empty()) { return ILLNESSES::NONE; }
    int randomIndex = RandInt(0, static_cast<int>(activePool.size()) - 1);
    return activePool[randomIndex];
}

/*****************************************************************************************/
// Determines the actual identity of the patient (Ghost vs Human) and assigns the
// appropriate illness anomalies. Ghosts secretly mimic human illnesses but add an extra modifier.
/*****************************************************************************************/
static void ApplyNewPatientFromTruth(bool isGhostTruth) {
    // Presentation mode: always a Scotophobia ghost with every anomaly active
    if (Game_IsPresentationMode()) {
        CarriedPatientIsGhost = true;
        TrueDiagnosis  = ILLNESSES::GHOST;
        AnomalyIllness = ILLNESSES::SCOTOPHOBIA;
        GhostExtraAnomalies = 99; // force all anomalies
        VisualBluff = false;
        Frame = 0;
        Timer = 0.0f;
        CentralPool_GeneratePatientAnomalies(AnomalyIllness, GhostExtraAnomalies);
        return;
    }

    CarriedPatientIsGhost = isGhostTruth;
    AnomalyIllness = PatientRandomIllness();

    if (CarriedPatientIsGhost) {
        TrueDiagnosis = ILLNESSES::GHOST;   // according to the ILLNESSES enum in central_pool.hpp
        GhostExtraAnomalies = 1;            // Ghost Patients +1 extra anomaly, instead of the regular three
    }
    else {
        TrueDiagnosis = AnomalyIllness;     // keep current ILLNESS as chosen by central_pool.hpp
        GhostExtraAnomalies = 0;
    }

    // 50% chance the patient visually looks like a monster, purely to bluff the player
    // (disabled in playtest mode so ghost vs. human is always visually clear)
    VisualBluff = Game_IsPlaytestMode() ? false : (RandInt(0, 1) == 0);
    Frame = 0;
    Timer = 0.0f;

    // Send the data to the central pool so other files know which anomalies to activate
    CentralPool_GeneratePatientAnomalies(AnomalyIllness, GhostExtraAnomalies);
}

void Player_GenerateMission() {
    PatientPickedUp = false;
    CarriedPatientIsGhost = false;
    VisualBluff = false;
    GhostExtraAnomalies = 0;
    TrueDiagnosis = AnomalyIllness = ILLNESSES::NONE;

    IsBlinking              = false;
    BlinkTimer              = 0.0f;
    PendingPickup           = false;
    PendingDelivery         = false;
    StateChangedDuringBlink = false;

    CentralPool_ClearPatientAnomalies();
    Journal_Clear();

    PickupDoor = static_cast<s8>(RandInt(1, 10));
    PickupFloor = static_cast<s8>(RandInt(1, 9));

    // Door can be rolled once since the floor will guarantee they are different locations
    DestDoor = static_cast<s8>(RandInt(1, 10));

    // --- CONSTRAINED RANDOMIZATION ---
    // Keep rolling the destination floor until it is strictly different from the pickup floor.
    // This forces the player to actually navigate the hospital rather than walking next door.
    do {
        DestFloor = static_cast<s8>(RandInt(1, 9));
    } while (DestFloor == PickupFloor);
}

void SetPatientScaryByDay(int day) {
    int Day = (day < 1) ? 1 : (day > 5 ? 5 : day);
    bool IsScary = false;

    // Use the predetermined shuffled pool for the first REQUIRED_PATIENTS patients of the day
    if (PatientsHandled < REQUIRED_PATIENTS) {
        IsScary = (CurrentDayPool[PatientsHandled] == 1);
    }
    else {
        // --- DIFFICULTY SCALING ---
        // If the player exceeds the quota, use a percentage roll that gets harder each day.
        static constexpr float extraRollProb[] = { 0.0f, 0.20f, 0.20f, 0.35f, 0.35f, 0.40f };
        float roll = static_cast<float>(RandInt(0, 100)) / 100.0f;
        IsScary = (roll < extraRollProb[Day]);
    }

    ApplyNewPatientFromTruth(IsScary);
}

/*****************************************************************************************/
// Validates door interaction. Triggers the cinematic Eye-Blink transition and sets flags
// to securely defer the actual level swapping until the screen is fully black.
/*****************************************************************************************/
bool Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day) {
    // Prevent interacting again if we are already in the middle of a transition!
    if (IsBlinking) return false;

    if (!PatientPickedUp) {
        if (currentFloor == PickupFloor && doorNumAtPlayer == PickupDoor) {
            // --- DEFERRED STATE LOGIC ---
            // Start the blink, but DELAY the actual pickup logic. 
            IsBlinking = true;
            BlinkTimer = 0.0f;
            PendingPickup = true;
            PendingDay = day;
            StateChangedDuringBlink = false;

            return true;
        }
    }
    else {
        if (currentFloor == DestFloor && doorNumAtPlayer == DestDoor) {
            // --- DEFERRED STATE LOGIC ---
            // Start the blink, but DELAY the actual delivery logic.
            IsBlinking = true;
            BlinkTimer = 0.0f;
            PendingDelivery = true;
            StateChangedDuringBlink = false;
            PatientsHandled++;

            return true;
        }
    }
    return false;
}

void ResetPatientCounter(int day) {
    PatientsHandled = 0;
    int CurrentDay = (day < 1) ? 1 : (day > 5 ? 5 : day);

    if (Game_IsPresentationMode()) {
        // Presentation: exactly 1 ghost patient, no shuffle needed
        CurrentDayPool[0] = 1;
    }
    else if (Game_IsPlaytestMode()) {
        // Playtest: guaranteed 1 ghost and 1 human per day, order randomised
        CurrentDayPool[0] = PlaytestDayPool[0];
        CurrentDayPool[1] = PlaytestDayPool[1];
        std::shuffle(CurrentDayPool, CurrentDayPool + 2, GetGlobalRng());
    }
    else {
        // Normal: use the full predetermined pool for the day
        for (int i = 0; i < 5; ++i)
            CurrentDayPool[i] = PlayerPool[CurrentDay][i];
        std::shuffle(std::begin(CurrentDayPool), std::end(CurrentDayPool), GetGlobalRng());
    }
}

static AEGfxTexture* GetActiveFrameTex() {
    if (!PatientPickedUp) return NoPatientTex[Frame];
    return VisualBluff ? ScaryTex[Frame] : HumanTex[Frame];
}


/***************************************** LOAD ******************************************/
void Player_Load() {
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    SpriteMesh = AEGfxMeshEnd();

    HumanTex[0] = LoadTextureChecked(Assets::Player::Human1);
    HumanTex[1] = LoadTextureChecked(Assets::Player::Human2);
    ScaryTex[0] = LoadTextureChecked(Assets::Player::Scary1);
    ScaryTex[1] = LoadTextureChecked(Assets::Player::Scary2);
    NoPatientTex[0] = LoadTextureChecked(Assets::Player::Nurse1);
    NoPatientTex[1] = LoadTextureChecked(Assets::Player::Nurse2);
}


/***************************************** UPDATE ****************************************/
void Player_Update(float dt, bool walkKey) {

    // --- HEAVY EYE-BLINK LOGIC ---
    if (IsBlinking) {
        BlinkTimer += dt;

        // The exact midpoint of the blink (when screen is pitch black)
        float midPoint = BLINK_DURATION * 0.5f;

        // --- APPLY DEFERRED LOGIC WHEN EYES ARE SHUT ---
        // This guarantees the level and patient swap happens invisibly behind the black screen
        if (BlinkTimer >= midPoint && !StateChangedDuringBlink) {
            if (PendingPickup) {
                PatientPickedUp = true;
                SetPatientScaryByDay(PendingDay);
                PendingPickup = false;
            }
            else if (PendingDelivery) {
                Player_GenerateMission();
                PendingDelivery = false;
            }
            StateChangedDuringBlink = true;
        }

        float progress = 0.0f;
        // Eyelids take 35% of the total time to close, hold black for 30%, and take 35% to open
        float slideTime = BLINK_DURATION * 0.35f;

        // Phase 1: Eyelids sliding closed
        if (BlinkTimer <= slideTime) {
            progress = BlinkTimer / slideTime;
        }
        // Phase 2: Eyes tightly shut (pitch black)
        else if (BlinkTimer <= BLINK_DURATION - slideTime) {
            progress = 1.0f;
        }
        // Phase 3: Eyelids heavily sliding open
        else if (BlinkTimer <= BLINK_DURATION) {
            float timeOpening = BlinkTimer - (BLINK_DURATION - slideTime);
            progress = 1.0f - (timeOpening / slideTime);
        }
        // Phase 4: Blink finished
        else {
            IsBlinking = false;
            progress = 0.0f;
        }

        // --- SINE WAVE EASING ---
        // Using sine wave (sinf) to create a smooth, cinematic ease-out.
        // The eyelids will slide fast initially, then smoothly slow down as they meet in the middle.
        float easedProgress = sinf(progress * 1.570796f);

        TopLidY = 1000.0f - (easedProgress * 500.0f);
        BottomLidY = -1000.0f + (easedProgress * 500.0f);
    }

    if (walkKey) {
        Timer += dt;
        if (Timer >= FRAME_TIME) {
            Timer -= FRAME_TIME;
            Frame = (Frame + 1) % 2;
        }
    }
    else {
        Frame = 0;
        Timer = 0.0f;
    }

    // --- AUDIO STATE MACHINE ---
    // Safely toggles between footstep and wheelchair sounds depending on the player's inventory,
    // and forcefully stops looping audio if the player stops moving or is interrupted.
    if (!walkKey) {
        if (WalkingSoundPlaying || WheelchairSoundPlaying) {
            AudioManager_StopWalkingSFX();
            WalkingSoundPlaying = false;
            WheelchairSoundPlaying = false;
        }
        return;
    }

    if (Player_HasPatient())
    {
        if (!WheelchairSoundPlaying)
        {
            AudioManager_StopWalkingSFX();

            AudioManager_PlayWalkingSFX(SoundID::SFX_PLAYER_WALKING, 0.6f, 1.0f, -1);
            AudioManager_PlayWalkingSFX(SoundID::SFX_WHEELCHAIR_SQUEAK, 0.2f, 1.0f, -1);

            WalkingSoundPlaying = true;
            WheelchairSoundPlaying = true;
        }
    }
    else
    {
        if (!WalkingSoundPlaying || WheelchairSoundPlaying)
        {
            AudioManager_StopWalkingSFX();

            AudioManager_PlayWalkingSFX(SoundID::SFX_PLAYER_WALKING, 0.6f, 1.0f, -1);

            WalkingSoundPlaying = true;
            WheelchairSoundPlaying = false;
        }
    }

}


/***************************************** DRAW UI ***************************************/
void Player_DrawBlinkOverlay(AEGfxVertexList* squareMesh) {
    if (!IsBlinking || squareMesh == nullptr) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Set color to solid pitch black
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetTransparency(1.0f);

    // --- MATRIX TRANSFORMATIONS (EYELIDS) ---
    // Draws two massive 2000x1000 black rectangles that slide together based on TopLidY / BottomLidY

    // Draw the Top Eyelid (sliding down)
    AEMtx33 scaleTop, transTop, transformTop;
    AEMtx33Scale(&scaleTop, 2000.0f, 1000.0f);
    AEMtx33Trans(&transTop, 0.0f, TopLidY);
    AEMtx33Concat(&transformTop, &transTop, &scaleTop);
    AEGfxSetTransform(transformTop.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // Draw the Bottom Eyelid (sliding up)
    AEMtx33 scaleBot, transBot, transformBot;
    AEMtx33Scale(&scaleBot, 2000.0f, 1000.0f);
    AEMtx33Trans(&transBot, 0.0f, BottomLidY);
    AEMtx33Concat(&transformBot, &transBot, &scaleBot);
    AEGfxSetTransform(transformBot.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // 4. Clean up the engine state
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
}


/***************************************** DRAW ******************************************/
void Player_Draw(float x, float y) {
    AEMtx33 scale, trans, transform;

    // --- DYNAMIC MATRIX TRANSFORMATIONS ---
    // Dynamically scales the player's width (sx) depending on whether they are pushing a wheelchair,
    // and flips the sprite horizontally based on FacingDir (1 or -1).
    float widthMultiplier = PatientPickedUp ? 1.0f : 0.3f;
    float sx = (PLAYER_WIDTH * widthMultiplier) * static_cast<float>(FacingDir);
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
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}


/**************************************** UNLOAD *****************************************/
void Player_Unload() {
    for (int i = 0; i < 2; ++i) {
        UnloadTextureSafe(HumanTex[i]);
        UnloadTextureSafe(ScaryTex[i]);
        HumanTex[i] = nullptr;
        ScaryTex[i] = nullptr;
    }
    for (int i = 0; i < 2; ++i) {
        if (NoPatientTex[i]) UnloadTextureSafe(NoPatientTex[i]);
        NoPatientTex[i] = nullptr;
    }
    FreeMeshSafe(SpriteMesh);
}


/*************************************** MODIFIERS ***************************************/
void Player_ForceHumanPatient() {
    ApplyNewPatientFromTruth(false);
    VisualBluff = false;
}

void Player_ForceGhostPatient() {
    ApplyNewPatientFromTruth(true);
    VisualBluff = true;
}

void Player_StopSounds() {
    AudioManager_StopWalkingSFX();
    WalkingSoundPlaying = false;
    WheelchairSoundPlaying = false;
}

void Player_DebugFulfillPatientQuota() {
    // Ensures PatientsHandled >= REQUIRED_PATIENTS so the day-end check passes when debug-skipping a day
    if (PatientsHandled < REQUIRED_PATIENTS) PatientsHandled = REQUIRED_PATIENTS;
}

void Player_SetIllness(ILLNESSES illness) { TrueDiagnosis = AnomalyIllness = illness; }
void Player_SetFacing(int dir) { FacingDir = (dir >= 0) ? 1 : -1; }


/*************************************** ACCESSORS ***************************************/
bool        IsGhostPatient() { return CarriedPatientIsGhost; }
ILLNESSES   Player_GetTrueDiagnosis() { return TrueDiagnosis; }
ILLNESSES   Player_GetAnomalyIllness() { return AnomalyIllness; }
int         Player_GetGhostExtraAnomalies() { return GhostExtraAnomalies; }
int         Player_GetPatientsHandled() { return PatientsHandled; }
bool        Player_HasPatient() { return PatientPickedUp; }
void        Player_GetTargetRoom(s8& patientFloor, s8& patientDoor, s8& destFloor, s8& destDoor) {
    patientFloor = PickupFloor;
    patientDoor = PickupDoor;
    destFloor = DestFloor;
    destDoor = DestDoor;
}
float       Player_GetWidth() { return PLAYER_WIDTH; }
float       Player_GetHeight() { return PLAYER_HEIGHT; }
bool        Player_IsWalking() { return WalkingSoundPlaying; }