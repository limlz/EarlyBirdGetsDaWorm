#include "pch.hpp"

static constexpr float FRAME_TIME = 0.4f;

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

static int CurrentDayPool[5]{};
static int PatientsHandled = 0;
static int PlayerPool[6][5] = {
    {0,0,0,0,0},
    {0,1,0,0,0},
    {0,1,0,0,1},
    {0,1,1,0,0},
    {0,1,1,1,0},
    {0,1,1,1,0}
};

/******************************* PLAYER & PATIENT LOGIC **********************************/
static ILLNESSES PatientRandomIllness() {
    const std::vector<ILLNESSES>& activePool = AllAnomalies_CurrentRun();
    if (activePool.empty()) { return ILLNESSES::NONE; }
    int randomIndex = rand() % (int)activePool.size();
    return activePool[randomIndex];
}

static void ApplyNewPatientFromTruth(bool isGhostTruth) {
    CarriedPatientIsGhost = isGhostTruth;
    AnomalyIllness = PatientRandomIllness();

    if (CarriedPatientIsGhost) {
        TrueDiagnosis = ILLNESSES::GHOST;
        GhostExtraAnomalies = 1;
    }
    else {
        TrueDiagnosis = AnomalyIllness;
        GhostExtraAnomalies = 0;
    }

    VisualBluff = (std::rand() % 2 == 0);
    Frame = 0;
    Timer = 0.0f;

    CentralPool_GeneratePatientAnomalies(AnomalyIllness, GhostExtraAnomalies);
}

void Player_GenerateMission() {
    PatientPickedUp = false;
    CarriedPatientIsGhost = false;
    VisualBluff = false;
    GhostExtraAnomalies = 0;
    TrueDiagnosis = AnomalyIllness = ILLNESSES{};

    CentralPool_ClearPatientAnomalies();
    Journal_Clear();

    PickupDoor = (s8)(rand() % 10) + 1;
    PickupFloor = (s8)(rand() % 9) + 1;
    do {
        DestDoor = (s8)(rand() % 10) + 1;
        DestFloor = (s8)(rand() % 9) + 1;
    } while (DestDoor == PickupDoor && DestFloor == PickupFloor);
}

void SetPatientScaryByDay(int day) {
    int Day = (day < 1) ? 1 : (day > 5 ? 5 : day);
    bool IsScary = false;

    if (PatientsHandled < 5) {
        IsScary = (CurrentDayPool[PatientsHandled] == 1);
    }
    else {
        static constexpr float extraRollProb[] = { 0.0f, 0.20f, 0.20f, 0.35f, 0.35f, 0.40f };
        float roll = (float)(std::rand() % 101) / 100.0f;
        IsScary = (roll < extraRollProb[Day]);
    }

    ApplyNewPatientFromTruth(IsScary);
    PatientsHandled++;
}

bool Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day) {
    if (!PatientPickedUp) {
        if (currentFloor == PickupFloor && doorNumAtPlayer == PickupDoor) {
            PatientPickedUp = true;
            SetPatientScaryByDay(day);
            return true;
        }
    }
    else {
        if (currentFloor == DestFloor && doorNumAtPlayer == DestDoor) {
            Player_GenerateMission();
            return true;
        }
    }
    return false;
}

void ResetPatientCounter(int day) {
    PatientsHandled = 0;
    int CurrentDay = (day < 1) ? 1 : (day > 5 ? 5 : day);

    for (int i = 0; i < 5; ++i) {
        CurrentDayPool[i] = PlayerPool[CurrentDay][i];
    }

    unsigned seed = (unsigned)time(NULL);
    std::shuffle(std::begin(CurrentDayPool), std::end(CurrentDayPool), std::default_random_engine(seed));
}

static AEGfxTexture* GetActiveFrameTex() {
    if (!PatientPickedUp) return NoPatientTex[Frame];
    return VisualBluff ? ScaryTex[Frame] : HumanTex[Frame];
}

/***************************************** LOAD ******************************************/
void Player_Load() {
    std::srand((unsigned)std::time(nullptr));

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

    // --- AUDIO LOGIC ---
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

            AudioManager_PlayWalkingSFX(SFX_PLAYER_WALKING, 0.5f, 1.0f, -1);
            AudioManager_PlayWalkingSFX(SFX_WHEELCHAIR_SQUEAK, 0.1f, 1.0f, -1);

            WalkingSoundPlaying = true;
            WheelchairSoundPlaying = true;
        }
    }
    else
    {
        if (!WalkingSoundPlaying || WheelchairSoundPlaying)
        {
            AudioManager_StopWalkingSFX();

            AudioManager_PlayWalkingSFX(SFX_PLAYER_WALKING, 0.5f, 1.0f, -1);

            WalkingSoundPlaying = true;
            WheelchairSoundPlaying = false;
        }
    }
}

/***************************************** DRAW ******************************************/
void Player_Draw(float x, float y) {
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

void Player_SetIllness(ILLNESSES illness) { TrueDiagnosis = illness; }
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