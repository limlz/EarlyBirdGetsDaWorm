#include "pch.hpp"

// ============================================================
// RENDER DATA / MESHES
// ============================================================

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;

// (unused?) keep since it was already here
static AEGfxTexture* lightingtest = nullptr;

// ============================================================
// CAMERA / PLAYER POS
// ============================================================

static f32 camX{}, playerY{}, playerX{};

// UI text offset
f32 textXoffset{ 0.06f }, textY{ 50.0f };

// ============================================================
// FLOOR / DOOR STATE
// ============================================================

s8 floorNum{ 1 };
s8 demonFloorNum{ 0 };
s8 demonRoomNum{ 3 };
s8 doorNumAtPlayer{ -1 };

// ============================================================
// GAMEPLAY FLAGS
// ============================================================

bool liftActive{}, left_right{ 1 };
bool dementia{ false };

// ============================================================
// DAY / SESSION STATE
// ============================================================

static int  CurrentDay = 1;
static bool gSessionStarted = false;

// ============================================================
// SPAWN (unused in this file right now, keeping)
// ============================================================

static float gSpawnX = 50.0f;
static float gSpawnY = 0.0f;

// ============================================================
// BOSS TRANSITION FADE
// ============================================================

static bool  isFadingToBoss = false;
static float bossFadeAlpha = 0.0f;

static bool  isFadingFromBoss = false;
static float returnFadeAlpha = 1.0f;

const float BOSS_FADE_SPEED = 2.0f;

// ============================================================
// JUMPSCARE -> ENDGAME HANDOFF
// ============================================================

// forward decls if not in pch.hpp
void JumpScare_Start();
bool JumpScare_IsActive();

// If we trigger a jumpscare due to GHOST delivered to NON-basement,
// we end the game AFTER the jumpscare finishes.
static bool gPendingGhostDeliveryDeath = false;

static float ComputeSpawnYFromBorder()
{
    float borderCenterY = -650.0f;
    float borderHeight = 800.0f;
    float borderTopY = borderCenterY + (borderHeight * 0.5f);
    return borderTopY + (Player_GetHeight() * 0.5f);
}

// ============================================================
// LOAD
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
    //Wall_Load();			//moved to central_pool.cpp
    //Frames_Load();

    // One-time session setup
    if (!gSessionStarted)
    {
        CurrentDay = 1;
        Timer_Reset();
        Timer_StartDayOverlay(CurrentDay);
        camX = 0.0f;
        gSessionStarted = true;
    }
}

// ============================================================
// INITIALIZE (per-state enter)
// ============================================================

void Game_Initialize()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);

    // Fade states
    isFadingToBoss = false;
    bossFadeAlpha = 0.0f;
    isFadingFromBoss = true;
    returnFadeAlpha = 1.0f;

    // Init sub-systems
    Doors_Initialize();
    Lift_Initialize();
    JumpScare_Initialize();
    PauseMenu_Initialize();
    Notifications_Initialize();
    Tutorial_Initialize();
    Guide_Initialize();
    AllAnomalies_Initialize();

    // Start first mission of the day
    // NOTE: With your new logic, ghost/human is decided from JOURNAL evidence,
    // so we do NOT roll ghost here.
    Player_ResetPatientCounter(CurrentDay);
    Player_GenerateMission();

    // Reset handoff flag
    gPendingGhostDeliveryDeath = false;
}

// ============================================================
// UPDATE
// ============================================================

void Game_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();
    Debug_Update();

    // ------------------------------------------------------------
    // 1) Fade transition TO boss fight
    // ------------------------------------------------------------
    if (isFadingToBoss)
    {
        bossFadeAlpha += BOSS_FADE_SPEED * dt;

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
        // fade back down if we are not actively transitioning
        bossFadeAlpha -= BOSS_FADE_SPEED * dt;
    }

    // ------------------------------------------------------------
    // 2) Fade transition FROM boss fight back to hallway
    // ------------------------------------------------------------
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
    // 3) Jumpscare update (freezes gameplay)
    // ------------------------------------------------------------
    bool freeze = JumpScare_Update(dt);
    if (freeze) return;

    // After jumpscare ends, go endgame (if we triggered it)
    if (gPendingGhostDeliveryDeath && !JumpScare_IsActive())
    {
        gPendingGhostDeliveryDeath = false;
        next = ENDGAME_STATE;
        return;
    }

    // ------------------------------------------------------------
    // 4) Day overlay (freezes gameplay)
    // ------------------------------------------------------------
    Timer_UpdateDayOverlay(dt);
    if (Timer_IsDayOverlayActive()) return;

    // ------------------------------------------------------------
    // 5) Pause menu (freezes gameplay)
    // ------------------------------------------------------------
    PauseMenu_Update(dt);
    if (PauseMenu_IsPaused()) return;

    // ------------------------------------------------------------
    // 6) Guide overlay (freezes gameplay)
    // ------------------------------------------------------------
    if (IsGuideActive())
    {
        Guide_Update(liftActive, dt, IsPagerActive());
        return;
    }

    // ------------------------------------------------------------
    // 7) Tutorial overlay (freezes gameplay)
    // ------------------------------------------------------------
    if (Tutorial_Prompt_Answered() == false && IsTutorialActive())
    {
        Tutorial_Update(dt);

        // Skip all gameplay input
        return;
    }

    // ------------------------------------------------------------
    // 8) Journal update (and freeze if open)
    // ------------------------------------------------------------
    Journal_Update();

    if (Journal_IsOpen())
    {
        return; // freeze gameplay while journal open
    }

    // ------------------------------------------------------------
    // 9) Timer main update + day advance
    // ------------------------------------------------------------
    Timer_Update(dt);

    if (Timer_IsTimeUp())
    {
        if (CurrentDay >= 5)
        {
            currentEndReason = REASON_SURVIVED_5_DAYS;
            next = ENDGAME_STATE;
            return;
        }

        CurrentDay++;
        Timer_Reset();
        Timer_StartDayOverlay(CurrentDay);

        camX = 0.0f;
        floorNum = 1;
        liftActive = false;

        // Start next day mission
        Player_ResetPatientCounter(CurrentDay);
        Player_GenerateMission();

        return;
    }

    // ------------------------------------------------------------
    // 10) Movement input / debugging toggles
    // ------------------------------------------------------------
    if (AEInputCheckCurr(AEVK_A)) { camX += PLAYER_SPEED * dt; left_right = false; }
    if (AEInputCheckCurr(AEVK_D)) { camX -= PLAYER_SPEED * dt; left_right = true; }

    if (AEInputCheckTriggered(AEVK_O)) dementia = !dementia;
    if (AEInputCheckCurr(AEVK_M)) { camX -= 4000; left_right = true; }
    if (AEInputCheckTriggered(AEVK_K)) Timer_DebugSetTime(5 * 60 + 58);

    bool moveRight = AEInputCheckCurr(AEVK_D);
    bool moveLeft = AEInputCheckCurr(AEVK_A);
    bool isWalking = moveRight || moveLeft;

    if (moveRight) Player_SetFacing(1);
    else if (moveLeft) Player_SetFacing(-1);

    Player_Update(dt, isWalking);

    // Clamp cam
    float maxDist = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;
    if (camX > 0) camX = 0;
    else if ((camX < -maxDist) && !dementia) camX = -maxDist;

    // ------------------------------------------------------------
    // 11) Update world systems
    // ------------------------------------------------------------
    doorNumAtPlayer = Doors_Update(camX);
    Doors_Animate(dt, doorNumAtPlayer, camX);

    Lift_Update(dt, camX, maxDist);
    Lift_HandleInput(floorNum);

    Lighting_Update(floorNum, camX, dementia);
    Frames_Update(dt);

    // ------------------------------------------------------------
    // 12) Interactions (E key)
    // ------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_E) && doorNumAtPlayer != -1)
    {
        const s8 doorAtPlayer1Based = (s8)doorNumAtPlayer + 1;

        // Pull current mission targets
        s8 targetFloor, targetDoor, destFloor, destDoor;
        Player_GetTargetRoom(targetFloor, targetDoor, destFloor, destDoor);

        // --------------------------------------------------------
        // Rule:
        // - If carrying a patient and disposing in basement:
        //     - HUMAN -> GAME OVER
        //     - GHOST -> BOSS FIGHT
        // --------------------------------------------------------
        if (Player_HasPatient() &&
            floorNum == 0 &&
            Doors_TryDisposal(floorNum, doorNumAtPlayer))
        {
            // No jumpscare triggered! Now check if patient is a monster
            if (Player_IsScaryPatient())
            {
                isFadingToBoss = true;
            }
            else
            {
                currentEndReason = REASON_WRONG_BASEMENT_DELIVERY;
                next = ENDGAME_STATE;
            }
            return;
        }

        // --------------------------------------------------------
        // Rule:
        // - If carrying a GHOST and delivering to the mission destination
        //   (which is a NON-basement room because pager is a throw-off):
        //     -> JUMPSCARE, then ENDGAME
        // --------------------------------------------------------

        // If player is at the destination door for the pager mission
        if (Player_HasPatient() &&
            floorNum == destFloor &&
            doorAtPlayer1Based == destDoor)
        {
            // If it's a ghost, delivering it like a human = failure
            if (Player_IsScaryPatient())
            {
                currentEndReason = REASON_WRONG_BASEMENT_DELIVERY; // rename later if you want
                JumpScare_Start();
                gPendingGhostDeliveryDeath = true;
                return;
            }
        }

        // --------------------------------------------------------
        // Normal pickup / delivery flow
        // --------------------------------------------------------
        bool success = Player_HandleInteraction(floorNum, doorAtPlayer1Based, CurrentDay);

        if (success)
        {
            // If we just completed delivery (no longer holding patient),
            // we can close lift UI etc.
            if (!Player_HasPatient())
            {
                liftActive = false;

                // NOTE:
                // With your new logic, we DO NOT roll ghost here.
                // Next patient's truth is decided at pickup from JOURNAL evidence.
            }
        }
        else
        {
            Prompts_TriggerWrongRoom();
        }
    }

    // ------------------------------------------------------------
    // 13) UI / prompts updates
    // ------------------------------------------------------------
    Notifications_Update(liftActive, dt);
    Guide_Update(liftActive, dt, IsPagerActive());
    Prompts_Update(dt, camX, doorNumAtPlayer, Lift_IsActive(), Lift_IsNear());
    Tutorial_Update(dt);
}

// ============================================================
// DRAW
// ============================================================

void Game_Draw()
{
    Wall_Draw(camX, floorNum);
    Frames_Draw(floorNum, camX);

    if (floorNum >= 1) AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);

    DrawSquareMesh(squareMesh, 0.0f, FLOOR_CENTER_Y, 1600.0f, FLOOR_HEIGHT, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -FLOOR_CENTER_Y, 1600.0f, FLOOR_HEIGHT, COLOR_BLACK);

    Doors_Draw(camX, floorNum, textXoffset, textY, dementia);
    AEGfxSetTransparency(1.0f);

    // Start Lift
    if (camX > -(2 * DIST_BETWEEN_DOORS))
    {
        DrawSquareMesh(squareMesh, -600.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        Lift_DrawWorld(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
        Lift_DrawBackground(squareMesh, -700.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, floorNum);
    }

    // End Lift
    if ((camX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) && !dementia)
    {
        float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
        float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;
        DrawSquareMesh(squareMesh, endOffset + camX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        Lift_DrawWorld(squareMesh, liftOffset + camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
        Lift_DrawBackground(squareMesh, endOffset + camX + 200.0f, 0.0f, 800.0f, 900.0f, floorNum);
    }

    float playerY = -650.0f + (800.0f * 0.5f) + (Player_GetHeight() * 0.5f);

    AEMtx33 scale;

    // Global Darkness Overlay
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(0.7f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEMtx33Scale(&scale, 2000.0f, 2000.0f);
    AEGfxSetTransform(scale.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // Morgue ambience overlay (basement only)
    if (floorNum == 0)
    {
        static float t = 0.0f;
        t += (float)AEFrameRateControllerGetFrameTime();

        AEMtx33 scale;
        AEMtx33Scale(&scale, 2000.0f, 2000.0f);

        // --------------------------------------------------
        // 1) BASE DARKNESS (constant black hue)
        // --------------------------------------------------
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        AEGfxSetColorToMultiply(0.5f, 0.1f, 0.1f, 1.0f);
        AEGfxSetTransform(scale.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --------------------------------------------------
        // 2) RED EMERGENCY PULSE
        // --------------------------------------------------

        float pulse = 0.10f + 0.12f * (0.5f + 0.5f * sinf(t * 2.5f));

        // small random electrical spike
        if ((rand() % 100) < 2)
            pulse += 0.05f;

        AEGfxSetTransparency(pulse);
        AEGfxSetColorToMultiply(0.6f, 0.15f, 0.15f, 1.0f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --------------------------------------------------
        // RESET RENDER STATE
        // --------------------------------------------------
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    Player_Draw(50.0f, playerY);

    // (illness var currently unused here, keep if you plan to use it for lighting etc.)
    ILLNESSES illness = Player_HasPatient() ? Player_GetCurrentIllness() : ILLNESSES::NONE;
    Draw_and_Flicker(camX, left_right, floorNum, dementia);

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
    Tutorial_Draw();

    DebugInfo info;
    info.camX = camX;
    info.day = CurrentDay;
    info.floorNum = floorNum;
    info.doorNumAtPlayer = doorNumAtPlayer;
    info.patientDoorNum = targetDoor;
    info.patientFloorNum = targetFloor;

    JumpScare_Draw();

    // Boss fade overlay (to/from boss fight)
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

    // Reset render state so overlays don't cover the journal
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    Journal_Draw(squareMesh);

    PauseMenu_Draw(squareMesh);
    Debug_Draw(info);
}

void Game_Free() {}

// ============================================================
// UNLOAD
// ============================================================

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