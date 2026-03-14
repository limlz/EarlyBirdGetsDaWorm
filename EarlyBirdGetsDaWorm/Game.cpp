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
EndGameReason currentEndReason = REASON_SURVIVED_5_DAYS;
static EndGameReason gPendingEndReason = REASON_SURVIVED_5_DAYS;

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
//  LOAD
// ============================================================

void Game_Load()
{
    std::cout << "Startup: Load\n";

    PauseMenu_Load();
    Debug_Load();
    Timer_Load();
    Doors_Load();
    Lift_Load();
    Player_Load();
    Journal_Load();
    Prompts_Load();
    Notifications_Load();
    Tutorial_Load();
    Guide_Load();
    JumpScare_Load();
    AllAnomalies_Load();

    // gSessionStarted prevents this block from re-running on floor transitions or reloads mid-session
    if (!gSessionStarted)
    {
        CurrentDay = 1;
        Timer_Reset();
        Timer_StartDayOverlay(CurrentDay);
        camX = 0.0f;
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
    } else {
        isFadingFromBoss = true;
        returnFadeAlpha = 1.0f;
    }

    Doors_Initialize();
    Lift_Initialize();
    JumpScare_Initialize();
    PauseMenu_Initialize();
    Notifications_Initialize();
    Tutorial_Initialize();
    Guide_Initialize();
    AllAnomalies_Initialize();

    ResetPatientCounter(CurrentDay);
    Player_GenerateMission();

    Config::Load();
    totalTimeSurvived = 0.0f;
}

// ============================================================
//  UPDATE
// ============================================================

void Game_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();
    Debug_Update();

    if (AEInputCheckTriggered(AEVK_F5)) {
        Config::Load();
    }

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

            next = ENDGAME_STATE;
            gPendingEndgameAfterJumpscare = false;
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
    if (freeze) return;

    Timer_UpdateDayOverlay(dt);
    if (Timer_IsDayOverlayActive()) return;

    PauseMenu_Update(dt);
    if (PauseMenu_IsPaused()) return;

    if (IsGuideActive())
    {
        Guide_Update(liftActive, dt, IsPagerActive());
        return;
    }

    Journal_Update();
    if (Journal_IsOpen()) return;

    if (Tutorial_Prompt_Answered() == false && IsTutorialActive())
    {
        Tutorial_Update(dt);
        return;
    }
    // ------------------------------------------------------------
    // PHASE 4: TIME TRACKING & DAY ADVANCEMENT
    // ------------------------------------------------------------
    Timer_Update(dt);
    totalTimeSurvived += dt;

    if (Timer_IsTimeUp())
    {
        if (CurrentDay >= 5)
        {
            currentEndReason = REASON_SURVIVED_5_DAYS;
            Analytics::LogRun(currentEndReason, CurrentDay, Player_GetPatientsHandled(), IsGhostPatient(),
                static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), totalTimeSurvived);
            next = ENDGAME_STATE;
            return;
        }

        if (Player_GetPatientsHandled() < 5) {
            currentEndReason = REASON_DIED_TO_BOSS;
            Analytics::LogRun(currentEndReason, CurrentDay, Player_GetPatientsHandled(), IsGhostPatient(),
                static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), totalTimeSurvived);
            next = ENDGAME_STATE;
			return;
        }

        // Advance the day: reset camera, floor, lift, and regenerate the patient mission
        CurrentDay++;
        Timer_Reset();
        Timer_StartDayOverlay(CurrentDay);

        camX = 0.0f;
        camVelX = 0.0f;
        floorNum = 1;
        liftActive = false;

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

        if (moveLeft) { targetVelX = Config::playerSpeed; left_right = false; }
        if (moveRight) { targetVelX = -Config::playerSpeed; left_right = true; }

        if (AEInputCheckTriggered(AEVK_O)) dementia = !dementia;
        if (AEInputCheckCurr(AEVK_M)) { camX -= 4000; left_right = true; }
        if (AEInputCheckTriggered(AEVK_K))   Timer_DebugSetTime(5 * 60 + 58);
    }

    // Apply velocity
    camVelX += (targetVelX - camVelX) * CAM_ACCEL * dt;
    camX += camVelX * dt;

    // Determine walking state for Audio & Animation
    bool isWalking = moveRight || moveLeft;

    if (moveRight)      Player_SetFacing(1);
    else if (moveLeft)  Player_SetFacing(-1);

    Player_Update(dt, isWalking);

    // Debug: force illness + sound system active (only when debug overlay is on and carrying a patient)
    if (Debug_IsActive() && Player_HasPatient())
    {
        bool illnessSwitched = false;
        if (AEInputCheckTriggered(AEVK_1)) { Player_SetIllness(ILLNESSES::PARANOIA);      CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_2)) { Player_SetIllness(ILLNESSES::MANIA);         CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_3)) { Player_SetIllness(ILLNESSES::DEPRESSION);    CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_4)) { Player_SetIllness(ILLNESSES::DEMENTIA);      CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_5)) { Player_SetIllness(ILLNESSES::SCHIZOPHRENIA); CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_6)) { Player_SetIllness(ILLNESSES::AIW_SYNDROME);  CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_7)) { Player_SetIllness(ILLNESSES::INSOMNIA);      CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_8)) { Player_SetIllness(ILLNESSES::OCD);           CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        if (AEInputCheckTriggered(AEVK_9)) { Player_SetIllness(ILLNESSES::SCOTOPHOBIA);   CentralPool_ForceSystemActive(AnomalyFiles::Sound_isActive); illnessSwitched = true; }
        // Reset sound state so audio levels restore and OnPatientPickup re-fires for the new illness
        if (illnessSwitched) SoundAnomaly_Initialize();
    }

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

    Lift_Update(dt, camX, maxDist);
    Lift_HandleInput(floorNum);

    Lighting_Update(floorNum, camX, dementia);
    Frames_Update(dt, camX);
    SoundAnomaly_Update(dt, camX, floorNum);

    // ------------------------------------------------------------
    // PHASE 7: DOOR INTERACTION LOGIC (E key)
    // ------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_E) && doorNumAtPlayer != -1)
    {
        const s8 doorAtPlayer1Based = (s8)doorNumAtPlayer + 1;

        s8 targetFloor, targetDoor, destFloor, destDoor;
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
                    gPendingEndReason = REASON_SENT_HUMAN_TO_BASEMENT;
                    gPendingEndgameAfterJumpscare = true;
                }
                else
                {
                    currentEndReason = REASON_SENT_HUMAN_TO_BASEMENT;
                    Analytics::LogRun(currentEndReason, CurrentDay, Player_GetPatientsHandled(), IsGhostPatient(),
                        static_cast<int>(Player_GetTrueDiagnosis()), static_cast<int>(Player_GetAnomalyIllness()), totalTimeSurvived);
                    next = ENDGAME_STATE;
                }
                return;
            }

            // Only a genuine GHOST patient should reach the basement disposal; trigger boss fight
            if (disposalSuccess)
            {
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
                gPendingEndReason = REASON_SENT_GHOST_TO_ROOM;
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

    // Left lift only renders when the camera is close enough to the left end of the corridor
    if (camX > -(2 * DIST_BETWEEN_DOORS))
    {
        DrawSquareMesh(squareMesh, -600.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        Lift_DrawWorld(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
        Lift_DrawBackground(squareMesh, -700.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, floorNum);
    }

    // Right lift only renders near the right end; suppressed entirely in dementia mode where the corridor is infinite
    if ((camX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) && !dementia)
    {
        float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
        float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

        DrawSquareMesh(squareMesh, endOffset + camX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        Lift_DrawWorld(squareMesh, liftOffset + camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
        Lift_DrawBackground(squareMesh, endOffset + camX + 200.0f, 0.0f, 800.0f, 900.0f, floorNum);
    }

    AEMtx33 scale;

    // Full-screen dark overlay between world geometry and lights; alpha driven by active illness
    DrawBlackOverlay(squareMesh, Lighting_GetOverlayAlpha());

    // Basement (floor 0) gets an extra red tint overlay with a slow sin pulse and rare random brightness spikes
    //if (floorNum == 0)
    //{
    //    static float t = 0.0f;
    //    t += (float)AEFrameRateControllerGetFrameTime();

    //    AEMtx33Scale(&scale, 2000.0f, 2000.0f);

    //    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    //    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    //    AEGfxSetColorToMultiply(0.5f, 0.1f, 0.1f, 1.0f);
    //    AEGfxSetTransform(scale.m);
    //    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    //    float pulse = 0.10f + 0.12f * (0.5f + 0.5f * sinf(t * 2.5f));
    //    if ((rand() % 100) < 2) pulse += 0.05f; // 2% chance per frame of a sudden red flash

    //    AEGfxSetTransparency(pulse);
    //    AEGfxSetColorToMultiply(0.6f, 0.15f, 0.15f, 1.0f);
    //    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    //    AEGfxSetTransparency(1.0f);
    //    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    //}

    // Y is computed from the border collider so the player always sits flush on the floor regardless of sprite height
    float computedPlayerY = -650.0f + (800.0f * 0.5f) + (Player_GetHeight() * 0.5f);
    Player_Draw(50.0f, computedPlayerY);

    Draw_and_Flicker(camX, left_right, floorNum, dementia);

    // Top and bottom black bars drawn last to crop anything that bleeds outside the play area
    DrawSquareMesh(squareMesh, 0.0f, 650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -650.0f, 1600.0f, 800.0f, COLOR_BLACK);

    Prompts_Draw();

    s8 targetFloor, targetDoor, destFloor, destDoor;
    Player_GetTargetRoom(targetFloor, targetDoor, destFloor, destDoor);

    Notifications_Draw(targetDoor, targetFloor, destFloor, destDoor);
    Guide_DrawSmallIcon();
    Timer_Draw(0.0f, 0.85f);
    Timer_DrawDayOverlay(squareMesh);
    Guide_Draw();
    Lift_Draw(squareMesh);

    DebugInfo info;
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

        AEMtx33 trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

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
    Frames_Unload();
    Player_Unload();
    Prompts_Unload();
    Boss_Fight_Unload();
    Lighting_Unload();
    Doors_Unload();
    Debug_Unload();
    Notifications_Free();
    Timer_Unload();
    Wall_Unload();
    Lift_Unload();
    PauseMenu_Unload();
    JumpScare_Unload();
    Tutorial_Free();
    Guide_Free();
    Journal_Unload();

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
}

int Game_GetCurrentDay()
{
    return CurrentDay;
}

float Game_GetTimeSurvived()
{
    return totalTimeSurvived;
}