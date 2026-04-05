#include "pch.hpp"
#include "Analytics.hpp"

// ============================================================
//  RENDER DATA / MESHES
// ============================================================

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;

static AEGfxTexture* lightingtest = nullptr;

// ============================================================
//  CAMERA / PLAYER POS
// ============================================================

static f32 camX{}, playerY{}, playerX{};
static f32 camVelX{}; // Velocity used for smooth camera acceleration/deceleration

f32 textXoffset{ 0.06f }, textY{ 50.0f };

// ============================================================
//  FLOOR / DOOR STATE
// ============================================================

s8 floorNum{ 1 };
s8 demonFloorNum{ 0 };
s8 demonRoomNum{ 3 };
s8 doorNumAtPlayer{ -1 };

// ============================================================
//  GAMEPLAY FLAGS
// ============================================================

bool liftActive{}, left_right{ 1 };
bool dementia{ false };

// ============================================================
//  DAY / SESSION STATE
// ============================================================

static int  CurrentDay = 1;
static bool gSessionStarted = false; // Guards Game_Load so day/timer only reset on a fresh session, not a reload
static bool gPlaytestMode = false; // Compressed demo mode: 2 patients/day, 3x clock, no visual bluff
static bool gPresentationMode = false; // Demo-day mode: 1 day, 1 forced Scotophobia ghost, all anomalies

// ============================================================
//  SPAWN
// ============================================================

static float gSpawnX = 50.0f;
static float gSpawnY = 0.0f;

// ============================================================
//  BOSS TRANSITION FADE
// ============================================================

// Two separate fades: one going into the boss fight, one returning from it
static bool  isFadingToBoss = false;
static float bossFadeAlpha = 0.0f;

static bool  isFadingFromBoss = false;
static float returnFadeAlpha = 1.0f;

const float BOSS_FADE_SPEED = 2.0f;

// ============================================================
//  JUMPSCARE -> ENDGAME HANDOFF
// ============================================================

// Endgame can't trigger mid-jumpscare; these flags defer the state transition until the jumpscare finishes
static bool gPendingEndgameAfterJumpscare = false;
EndGameReason currentEndReason = EndGameReason::REASON_SENT_GHOST_TO_ROOM;
static EndGameReason gPendingEndReason = EndGameReason::REASON_SENT_GHOST_TO_ROOM;

void JumpScare_Start();
bool JumpScare_IsActive();

static float totalTimeSurvived = 0.0f;

// Computes the player's Y so they spawn flush with the top of the border collider, not clipped into it
static float ComputeSpawnYFromBorder()
{
    float borderCenterY = -650.0f;
    float borderHeight = 800.0f;
    float borderTopY = borderCenterY + (borderHeight * 0.5f);
    return borderTopY + (Player_GetHeight() * 0.5f);
}

// ============================================================
//  OUTSIDE WORLD PERSONA
// ============================================================
static AEGfxTexture* OutsideGraveTex[2]{}; // Grave Textures
static AEGfxTexture* OutsideNadTex[4]{}; // Persona Textures
static AEGfxTexture* OutsideLZTex[4]{};
static AEGfxTexture* OutsideSyaTex[4]{};
static AEGfxTexture* OutsideAraTex[4]{};

// --- Outside Animation State ---
static s32 currentImage;                        // Current animation image index
static f32 imageDuration;                       // How long the image will be drawn
static f32 imageTimer;                          // Timer used to track when to switch images
static f32 floatingTimer;                       // Timer used to track floating time



// ============================================================
//  LOAD
// ============================================================

void Game_Load()
{
    std::cout << "Startup: Load\n";

    PauseMenu_Load();
    Debug_Load();
    Timer_Load();
    Player_Load();
    Journal_Load();
    Prompts_Load();
    Notifications_Load();
    Tutorial_Load();
    Guide_Load();
    JumpScare_Load();
    AllAnomalies_Load();

    // Grave + Persona Loading Textures
    for (int i = 0; i < 2; i++) {
        OutsideGraveTex[i] = AEGfxTextureLoad(Assets::Background::OutsidePersonaGrave[i]);
    }
    for (int i = 0; i < 4; i++) {
        OutsideNadTex[i] = AEGfxTextureLoad(Assets::Background::OutsidePersonaNad[i]);
    }
    for (int i = 0; i < 4; i++) {
        OutsideLZTex[i] = AEGfxTextureLoad(Assets::Background::OutsidePersonaLZ[i]);
    }
    for (int i = 0; i < 4; i++) {
        OutsideSyaTex[i] = AEGfxTextureLoad(Assets::Background::OutsidePersonaSya[i]);
    }
    for (int i = 0; i < 4; i++) {
        OutsideAraTex[i] = AEGfxTextureLoad(Assets::Background::OutsidePersonaAra[i]);
    }

    // gSessionStarted prevents this block from re-running on floor transitions or reloads mid-session
    if (!gSessionStarted)
    {
        CurrentDay = 1;
        floorNum = 1;
        Timer_Reset();
        Timer_StartDayOverlay(CurrentDay);
        camX = -(NUM_DOORS + 1) * DIST_BETWEEN_DOORS / 2.0f;  // start at corridor center
        camVelX = 0.0f;
        gSessionStarted = true;
    }
}

// ============================================================
//  INITIALIZE
// ============================================================

void Game_Initialize()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);

    // Always enter a new floor with a fade-in; boss fade-out starts clean
    isFadingToBoss = false;
    bossFadeAlpha = 0.0f;
    // Skip the return-fade when the day overlay is already covering the screen;
    // using both would hide "DAY X" behind a second black overlay.
    if (Timer_IsDayOverlayActive()) {
        isFadingFromBoss = false;
        returnFadeAlpha = 0.0f;
    }
    else {
        isFadingFromBoss = true;
        returnFadeAlpha = 1.0f;
    }

    JumpScare_Initialize();
    PauseMenu_Initialize();
    Notifications_Initialize();
    Tutorial_Initialize();
    Journal_Initialize();
    Guide_Initialize();
    AllAnomalies_Initialize();

    // Initialize animation variables
    currentImage = 0;
    imageDuration = 0.4f;
    imageTimer = 0.5f;
    floatingTimer = 0.0f;

    ResetPatientCounter(CurrentDay);
    Player_GenerateMission();

    totalTimeSurvived = 0.0f;

    AudioManager_PlayBGM(SoundID::BGM_GAME_AMBIENCE, 0.5f);
}

// ============================================================
//  UPDATE
// ============================================================

void Game_Update()
{
    float dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());
    Debug_Update();

    Config::PollAndReload();

    // Restore guide icon after returning from boss fight (Game_Initialize is
    // skipped on resume, so this is the earliest point to clear the flag).
    Guide_ShowIcon();

    // ------------------------------------------------------------
    // PHASE 1: JUMPSCARE & DEATH HANDOFF
    // ------------------------------------------------------------
    if (gPendingEndgameAfterJumpscare)
    {
        if (!JumpScare_IsActive())
        {
            currentEndReason = gPendingEndReason;

            Analytics::LogRun(currentEndReason, CurrentDay, Player_GetPatientsHandled(), IsGhostPatient(),
                static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), totalTimeSurvived);
            AudioManager_StopBGM();

            next = ENDGAME_STATE;
            gPendingEndgameAfterJumpscare = false;
            return;
        }
    }

    // ------------------------------------------------------------
    // PHASE 2: BOSS FIGHT TRANSITIONS
    // ------------------------------------------------------------
    if (isFadingToBoss)
    {
        bossFadeAlpha += BOSS_FADE_SPEED * dt;

        // Switch to BOSS_FIGHT_STATE only once the screen is fully black
        if (bossFadeAlpha >= 1.0f)
        {
            bossFadeAlpha = 1.0f;
            PauseMenu_SetPaused(false);
            isFadingToBoss = false;
            AudioManager_StopBGM();
            AudioManager_StopSFX();
            next = BOSS_FIGHT_STATE;
        }
        return;
    }
    else if (bossFadeAlpha > 0.0f)
    {
        bossFadeAlpha -= BOSS_FADE_SPEED * dt;
    }

    if (isFadingFromBoss)
    {
        returnFadeAlpha -= BOSS_FADE_SPEED * dt;
        if (returnFadeAlpha <= 0.0f)
        {
            returnFadeAlpha = 0.0f;
            isFadingFromBoss = false;
        }
    }

    // ------------------------------------------------------------
    // PHASE 3: INTERRUPTIONS & OVERLAYS
    // ------------------------------------------------------------
    bool freeze = JumpScare_Update(dt);
    if (freeze) { Player_StopSounds(); return; }

    Timer_UpdateDayOverlay(dt);
    if (Timer_IsDayOverlayActive()) { Player_StopSounds(); return; }

    PauseMenu_Update(dt);
    if (PauseMenu_IsPaused()) { Player_StopSounds(); return; }

    // For Tutorial DT
    f32 gameDT = Timer_IsPaused() ? 0.0f : dt;
    if (Doing_Tutorial() && !IsTutorialFinished()) {
        Journal_Update(liftActive, IsGuideActive(), IsPagerActive(), dt);
    }
    else {
        Journal_Update(liftActive, IsGuideActive(), IsPagerActive(), gameDT);
        if (Journal_IsOpen()) {
            Player_StopSounds(); // <--- Stops audio when Journal opens
            return;
        }
    }

    if (IsGuideActive()) {
        Guide_Update(liftActive, dt, IsPagerActive());
        Player_StopSounds();     // <--- Stops audio when Guide opens
        return;
    }

    if (Tutorial_Prompt_Answered() == false && IsTutorialActive())
    {
        Tutorial_Update(dt);
        Player_StopSounds();     // <--- Stops audio when Tutorial overlay is open
        return;
    }

    // ------------------------------------------------------------
    // PHASE 4: TIME TRACKING & DAY ADVANCEMENT
    // ------------------------------------------------------------
    Timer_Update(dt);
    totalTimeSurvived += dt;

    if (Timer_IsTimeUp())
    {
        // Check quota first so a failed playtest day still ends with DIED_TO_BOSS
        const int requiredPatients = (Game_IsPlaytestMode() || Game_IsPresentationMode()) ? 1 : REQUIRED_PATIENTS;
        if (Player_GetPatientsHandled() < requiredPatients) {
            currentEndReason = EndGameReason::REASON_QUOTA_NOT_MET;
            Analytics::LogRun(currentEndReason, CurrentDay, Player_GetPatientsHandled(), IsGhostPatient(),
                static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), totalTimeSurvived);

            AudioManager_StopBGM();
            next = ENDGAME_STATE;
            return;
        }

        // End condition: completed all 5 days (normal and playtest both progress through all days)
        if (CurrentDay >= 5)
        {
            currentEndReason = EndGameReason::REASON_SURVIVED_5_DAYS;
            Analytics::LogRun(currentEndReason, CurrentDay, Player_GetPatientsHandled(), IsGhostPatient(),
                static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), totalTimeSurvived);

            AudioManager_StopBGM();
            next = ENDGAME_STATE;
            return;
        }

        // Advance the day: reset camera, floor, lift, and regenerate the patient mission
        CurrentDay++;
        Timer_Reset();
        Timer_StartDayOverlay(CurrentDay);

        camX = -(NUM_DOORS + 1) * DIST_BETWEEN_DOORS / 2.0f;  // start at corridor center
        camVelX = 0.0f;
        floorNum = 1;
        liftActive = false;

        Doors_ResetAllLocks();   // Unboard all basement doors at the start of each new day
        ResetPatientCounter(CurrentDay);
        Player_GenerateMission();
        return;
    }

    // ------------------------------------------------------------
    // PHASE 5: INPUT & MOVEMENT
    // ------------------------------------------------------------
    // Smooth camera: build a target velocity from input, then lerp camVelX toward it so
    // the corridor accelerates up to full speed and decelerates to a stop instead of snapping.

    const float CAM_ACCEL = 12.0f; // Higher = snappier; lower = more floaty
    float targetVelX = 0.0f;

    bool moveRight = false;
    bool moveLeft = false;

    // Only allow keyboard input if the Lift UI is closed
    if (!Lift_IsActive())
    {
        moveLeft = AEInputCheckCurr(AEVK_A);
        moveRight = AEInputCheckCurr(AEVK_D);

        const bool isRunning = !Player_HasPatient() && AEInputCheckCurr(AEVK_LSHIFT);
        const float speed = isRunning ? Config::playerSpeed * 1.8f : Config::playerSpeed;

        if (moveLeft) { targetVelX = speed; left_right = false; }
        if (moveRight) { targetVelX = -speed; left_right = true; }

        if (Debug_IsActive() && AEInputCheckCurr(AEVK_M)) { camX -= 4000; left_right = true; }
    }

    // Apply velocity
    camVelX += (targetVelX - camVelX) * CAM_ACCEL * dt;
    camX += camVelX * dt;

    // Determine walking state for Audio & Animation
    bool isWalking = moveRight || moveLeft;

    if (moveRight)      Player_SetFacing(1);
    else if (moveLeft)  Player_SetFacing(-1);

    Player_Update(dt, isWalking);

    // Debug: force illness (only when debug overlay is on and carrying a patient)
    if (Debug_IsActive() && Player_HasPatient())
    {
        bool illnessSwitched = false;
        if (AEInputCheckTriggered(AEVK_1)) { Player_SetIllness(ILLNESSES::PARANOIA);      illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_2)) { Player_SetIllness(ILLNESSES::MANIA);         illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_3)) { Player_SetIllness(ILLNESSES::DEPRESSION);    illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_4)) { Player_SetIllness(ILLNESSES::DEMENTIA);      illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_5)) { Player_SetIllness(ILLNESSES::SCHIZOPHRENIA); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_6)) { Player_SetIllness(ILLNESSES::AIW_SYNDROME);  illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_7)) { Player_SetIllness(ILLNESSES::INSOMNIA);      illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_8)) { Player_SetIllness(ILLNESSES::OCD);           illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_9)) { Player_SetIllness(ILLNESSES::SCOTOPHOBIA);   illnessSwitched = true; }
        // Reset sound state so audio levels restore and OnPatientPickup re-fires for the new illness
        if (illnessSwitched) SoundAnomaly.Initialize();

        // 0: Re-roll the central pool anomaly assignment for the current patient
        if (AEInputCheckTriggered(AEVK_0)) {
            CentralPool_GeneratePatientAnomalies(Player_GetAnomalyIllness(), Player_GetGhostExtraAnomalies());
        }
    }

    // Debug-only controls that don't require a patient
    if (Debug_IsActive())
    {
        // F6: Set patients delivered to 5 (satisfies quota without ending the shift)
        if (AEInputCheckTriggered(AEVK_F6)) {
            Player_DebugFulfillPatientQuota();
        }

        // K: Force the current shift to end, satisfying the patient quota → triggers next-day transition
        if (AEInputCheckTriggered(AEVK_K)) {
            Player_DebugFulfillPatientQuota();
            Timer_ForceTimeUp();
        }

        // P: Auto collect patient (if idle) or auto deliver (if carrying)
        if (AEInputCheckTriggered(AEVK_P)) {
            s8 tFloor{}, tDoor{}, dFloor{}, dDoor{}; // <-- ZERO INITIALIZED
            Player_GetTargetRoom(tFloor, tDoor, dFloor, dDoor);
            if (!Player_HasPatient()) {
                Player_HandleInteraction(tFloor, tDoor, CurrentDay);
                SoundAnomaly.Initialize();
            }
            else {
                if (Player_HandleInteraction(dFloor, dDoor, CurrentDay))
                    liftActive = false;
            }
        }
    }

    // Endless hallway: only active when the lighting anomaly is running AND the current illness is Dementia
    dementia = CentralPool_IsSystemActive(AnomalyFiles::Lighting_isActive)
        && (Player_GetAnomalyIllness() == ILLNESSES::DEMENTIA);

    // Clamp camera so the player can't scroll past either end of the corridor.
    // Also kill velocity at each boundary so it doesn't fight against the clamp.
    float maxDist = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;
    if (camX > 0) { camX = 0;        if (camVelX > 0) camVelX = 0; }
    else if ((camX < -maxDist) && !dementia) { camX = -maxDist; if (camVelX < 0) camVelX = 0; }


    // ------------------------------------------------------------
    // PHASE 6: WORLD UPDATES
    // ------------------------------------------------------------
    Wall_Update(dt);
    doorNumAtPlayer = Doors_Update(camX);
    Doors_Animate(dt, doorNumAtPlayer, camX);

    Lift_Update(dt, camX, maxDist, dementia);
    Lift_HandleInput(floorNum);

    Lighting_Update(floorNum, camX, dementia);
    Frames_Update(dt, camX);
    SoundAnomaly.Update(dt, camX, floorNum);

    // ------------------------------------------------------------
    // PHASE 7: DOOR INTERACTION LOGIC (E key)
    // ------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_E) && doorNumAtPlayer != -1)
    {
        const s8 doorAtPlayer1Based = static_cast<s8>(doorNumAtPlayer) + 1;

        s8 targetFloor{}, targetDoor{}, destFloor{}, destDoor{}; // <-- ZERO INITIALIZED
        Player_GetTargetRoom(targetFloor, targetDoor, destFloor, destDoor);

        const bool hasPatient = Player_HasPatient();
        const bool isGhostTruth = (Player_GetTrueDiagnosis() == ILLNESSES::GHOST);

        // ========================================================
        // RULE A: BASEMENT DISPOSAL (Floor 0)
        // ========================================================
        if (hasPatient && floorNum == 0)
        {
            bool didJumpscare = false;
            const bool disposalSuccess = Doors_TryDisposal(floorNum, doorNumAtPlayer, didJumpscare);

            if (!disposalSuccess && !didJumpscare) return;

            if (!isGhostTruth)
            {
                if (didJumpscare)
                {
                    // Jumpscare plays first; endgame triggers once it finishes in Phase 1
                    gPendingEndReason = EndGameReason::REASON_SENT_HUMAN_TO_BASEMENT;
                    gPendingEndgameAfterJumpscare = true;
                }
                else
                {
                    currentEndReason = EndGameReason::REASON_SENT_HUMAN_TO_BASEMENT;
                    Analytics::LogRun(currentEndReason, CurrentDay, Player_GetPatientsHandled(), IsGhostPatient(),
                        static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), totalTimeSurvived);
                    next = ENDGAME_STATE;
                }
                return;
            }

            // Only a genuine GHOST patient should reach the basement disposal; trigger boss fight
            if (disposalSuccess)
            {
                Player_StopSounds();
                isFadingToBoss = true;
                return;
            }
            return;
        }

        // ========================================================
        // RULE B: WRONG DELIVERY (Delivering a Ghost to a normal ward)
        // ========================================================
        if (hasPatient && floorNum == destFloor && doorAtPlayer1Based == destDoor)
        {
            if (isGhostTruth)
            {
                JumpScare_Start();
                gPendingEndReason = EndGameReason::REASON_SENT_GHOST_TO_ROOM;
                gPendingEndgameAfterJumpscare = true; // <-- Unified flag
                return;
            }
        }

        // ========================================================
        // RULE C: NORMAL DELIVERY / PICKUP
        // ========================================================
        bool success = Player_HandleInteraction(floorNum, doorAtPlayer1Based, CurrentDay);

        if (success)
        {
            // Disable lift when the player drops off their last patient
            if (!Player_HasPatient())
                liftActive = false;
        }
        else
        {
            Prompts_TriggerWrongRoom();
        }
    }

    // ------------------------------------------------------------
    // PHASE 8: UI UPDATES
    // ------------------------------------------------------------
    Notifications_Update(liftActive, dt);
    Guide_Update(liftActive, dt, IsPagerActive());
    Prompts_Update(dt, camX, doorNumAtPlayer, Lift_IsActive(), Lift_IsNear());
    Tutorial_Update(dt);
}

// ============================================================
//  DRAW
// ============================================================

void Game_Draw()
{
    Wall_Draw(camX, floorNum);
    Frames_Draw(floorNum, camX);

    if (floorNum >= 1) AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);

    // Black bars mask the top and bottom of the play area
    DrawSquareMesh(squareMesh, 0.0f, FLOOR_CENTER_Y, 1600.0f, FLOOR_HEIGHT, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -FLOOR_CENTER_Y, 1600.0f, FLOOR_HEIGHT, COLOR_BLACK);

    Doors_Draw(camX, floorNum, textXoffset, textY, dementia);
    AEGfxSetTransparency(1.0f);

    // Outside World Persona — only shown in horror-less mode
    f32 floatOffset1 = 0.0f, floatOffset2 = 0.0f;
    if (Settings_IsHorrorLessEnabled()) {
        f32 personaDt = static_cast<f32>(AEFrameRateControllerGetFrameTime());
        imageTimer += personaDt;

        if (imageTimer >= imageDuration) {
            imageTimer = 0.0f;
            currentImage++;
            if (currentImage >= 4) {
                currentImage = 0;
            }
        }

        floatingTimer += personaDt;
        floatOffset1 = sinf(floatingTimer * 2.0f) * 20.0f;
        floatOffset2 = sinf(floatingTimer * 2.0f + 1.5f) * 20.0f;
    }

    // Left lift only renders when the camera is close enough to the left end of the corridor
    if (camX > -(2 * DIST_BETWEEN_DOORS))
    {
        DrawSquareMesh(squareMesh, -600.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        Lift_DrawWorld(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
        Lift_DrawBackground(squareMesh, -700.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, floorNum);

        if (Settings_IsHorrorLessEnabled()) {
            f32 personaX = -700.0f + camX - 100.0f, personaY = 0.0f;
            if (floorNum == 1) {
                DrawTextureMesh(squareMesh, OutsideGraveTex[0], personaX + 200.0f, personaY - 210.0f, 200.0f, 103.0f, 1.0f);
            }
            if (floorNum >= 2) {
                DrawTextureMesh(squareMesh, OutsideNadTex[currentImage], personaX + 300.0f, personaY + 150.0f + floatOffset1, 100.0f, 106.0f, 0.35f);
                DrawTextureMesh(squareMesh, OutsideLZTex[currentImage], personaX + 100.0f, personaY - 120.0f + floatOffset2, 132.0f, 106.0f, 0.35f);
            }
        }
    }

    // Right lift only renders near the right end; suppressed entirely in dementia mode where the corridor is infinite
    if ((camX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) && !dementia)
    {
        float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
        float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

        DrawSquareMesh(squareMesh, endOffset + camX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        Lift_DrawWorld(squareMesh, liftOffset + camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
        Lift_DrawBackground(squareMesh, endOffset + camX + 200.0f, 0.0f, 800.0f, 900.0f, floorNum);

        if (Settings_IsHorrorLessEnabled()) {
            f32 personaX = endOffset + camX + 200.0f, personaY = 0.0f;
            if (floorNum == 1) {
                DrawTextureMesh(squareMesh, OutsideGraveTex[1], personaX - 200.0f, personaY - 210.0f, 200.0f, 103.0f, 1.0f);
            }
            if (floorNum >= 2) {
                DrawTextureMesh(squareMesh, OutsideSyaTex[currentImage], personaX - 100.0f, personaY - 150.0f + floatOffset2, 86.0f, 106.0f, 0.35f);
                DrawTextureMesh(squareMesh, OutsideAraTex[currentImage], personaX - 280.0f, personaY + 150.0f + floatOffset1, 160.0f, 133.0f, 0.35f);
            }
        }
    }

    AEMtx33 scale{}; // <-- ZERO INITIALIZED

    // Full-screen dark overlay between world geometry and lights; alpha driven by active illness
    DrawBlackOverlay(squareMesh, Lighting_GetOverlayAlpha());

    // Y is computed from the border collider so the player always sits flush on the floor regardless of sprite height
    float computedPlayerY = -650.0f + (800.0f * 0.5f) + (Player_GetHeight() * 0.5f);
    Player_Draw(50.0f, computedPlayerY);

    Draw_and_Flicker(camX, left_right, floorNum, dementia);

    // Top and bottom black bars drawn last to crop anything that bleeds outside the play area
    DrawSquareMesh(squareMesh, 0.0f, 650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -650.0f, 1600.0f, 800.0f, COLOR_BLACK);

    Prompts_Draw();

    s8 targetFloor{}, targetDoor{}, destFloor{}, destDoor{}; // <-- ZERO INITIALIZED
    Player_GetTargetRoom(targetFloor, targetDoor, destFloor, destDoor);

    Journal_DrawSmallIcon(squareMesh);
    Timer_Draw(0.0f, 0.85f);
    Guide_Draw();
    Lift_Draw(squareMesh, floorNum);
    Notifications_Draw(targetFloor, targetDoor, destFloor, destDoor);
    Timer_DrawDayOverlay(squareMesh);

    DebugInfo info{}; // <-- ZERO INITIALIZED
    info.camX = camX;
    info.day = CurrentDay;
    info.floorNum = floorNum;
    info.doorNumAtPlayer = doorNumAtPlayer;
    info.patientDoorNum = targetDoor;
    info.patientFloorNum = targetFloor;

    JumpScare_Draw();

    // Shared fade overlay used by both boss-entry and boss-return; whichever alpha is non-zero wins
    if (bossFadeAlpha > 0.0f || returnFadeAlpha > 0.0f)
    {
        float currentAlpha = (bossFadeAlpha > 0.0f) ? bossFadeAlpha : returnFadeAlpha;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(currentAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        AEMtx33 trans{}, transform{}; // <-- ZERO INITIALIZED
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    Player_DrawBlinkOverlay(squareMesh);

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    Journal_Draw(squareMesh);
    Tutorial_Draw();
    PauseMenu_Draw(squareMesh);
    Debug_Draw(info);

}

// ============================================================
//  FREE & UNLOAD
// ============================================================

void Game_Free() {}

void Game_Unload()
{
    AudioManager_StopBGM();
    AudioManager_StopSFX();
    AudioManager_StopWalkingSFX();

    Player_Unload();
    Prompts_Unload();
    Debug_Unload();
    Notifications_Free();
    Timer_Unload();
    PauseMenu_Unload();
    JumpScare_Unload();
    Tutorial_Free();
    Guide_Free();
    Journal_Unload();
    AllAnomalies_Unload();

    // Grave + Persona Unloading
    for (int i{}; i < 2; i++) {
        if (OutsideGraveTex[i]) {
            AEGfxTextureUnload(OutsideGraveTex[i]);
            OutsideGraveTex[i] = nullptr;
        }
    }
    for (int i{}; i < 4; i++) {
        if (OutsideNadTex[i]) {
            AEGfxTextureUnload(OutsideNadTex[i]);
            OutsideNadTex[i] = nullptr;
        }
        if (OutsideLZTex[i]) {
            AEGfxTextureUnload(OutsideLZTex[i]);
            OutsideLZTex[i] = nullptr;
        }
        if (OutsideSyaTex[i]) {
            AEGfxTextureUnload(OutsideSyaTex[i]);
            OutsideSyaTex[i] = nullptr;
        }
        if (OutsideAraTex[i]) {
            AEGfxTextureUnload(OutsideAraTex[i]);
            OutsideAraTex[i] = nullptr;
        }
    }

    FreeMeshSafe(squareMesh);
    FreeMeshSafe(circleMesh);
}

// ============================================================
//  SESSION MANAGEMENT
// ============================================================

// Called before a full game restart; clears the guard so Game_Load reinitialises day and timer
void Game_ResetSession()
{
    gSessionStarted = false;
    gPlaytestMode = false;
    totalTimeSurvived = 0.0f;
    Notifications_ResetTaskComplete();
}

int Game_GetCurrentDay()
{
    return CurrentDay;
}

float Game_GetTimeSurvived()
{
    return totalTimeSurvived;
}

bool  Game_IsPlaytestMode() { return gPlaytestMode; }
void  Game_SetPlaytestMode(bool enabled) { gPlaytestMode = enabled; }

bool  Game_IsPresentationMode() { return gPresentationMode; }
void  Game_SetPresentationMode(bool enabled) { gPresentationMode = enabled; }