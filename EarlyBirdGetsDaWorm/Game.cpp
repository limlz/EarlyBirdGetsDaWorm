#include "pch.hpp"

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;
static AEGfxTexture* lightingtest = nullptr;
static f32 camX{}, playerY{}, playerX{};

f32 textXoffset{ 0.06f }, textY{ 50.0f };
s8 floorNum{ 1 };
s8 demonFloorNum{ 0 };
s8 demonRoomNum{ 3 };
s8 doorNumAtPlayer{ -1 };

bool liftActive{}, left_right{ 1 };
bool dementia{ false };

static int CurrentDay = 1;
static bool gSessionStarted = false;

static float gSpawnX = 50.0f;
static float gSpawnY = 0.0f;

static bool isFadingToBoss = false;
static float bossFadeAlpha = 0.0f;

static bool isFadingFromBoss = false;
static float returnFadeAlpha = 1.0f;

const float BOSS_FADE_SPEED = 2.0f;

static float ComputeSpawnYFromBorder()
{
    float borderCenterY = -650.0f;
    float borderHeight = 800.0f;
    float borderTopY = borderCenterY + (borderHeight * 0.5f);
    return borderTopY + (Player_GetHeight() * 0.5f);
}

void Game_Load()
{
	std::cout << "Startup: Load\n";

	PauseMenu_Load();
	Debug_Load();
	Timer_Load();
	Doors_Load();
	Lift_Load();
	Player_Load();
	Prompts_Load();
	Notifications_Load();
	Tutorial_Load();
	JumpScare_Load();

	AllAnomalies_Load();
	//Wall_Load();			//moved to central_pool.cpp
	//Frames_Load();

    if (!gSessionStarted) {
        CurrentDay = 1;
        Timer_Reset();
        Timer_StartDayOverlay(CurrentDay);
        camX = 0.0f;
        gSessionStarted = true;
    }
}

void Game_Initialize()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);

    isFadingToBoss = false;
    bossFadeAlpha = 0.0f;
    isFadingFromBoss = true;
    returnFadeAlpha = 1.0f;

    Doors_Initialize();
    Lift_Initialize();
    JumpScare_Initialize();
    PauseMenu_Initialize();

    Player_ResetPatientCounter(CurrentDay);
    Player_GenerateMission();
    Player_SetScaryByDay(CurrentDay);
    Notifications_Initialize();
    Tutorial_Initialize();
    AllAnomalies_Initialize();
}

void Game_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();
    Debug_Update();

    if (isFadingToBoss) {
        bossFadeAlpha += BOSS_FADE_SPEED * dt;
        if (bossFadeAlpha >= 1.0f) {
            bossFadeAlpha = 1.0f;
            PauseMenu_SetPaused(false);
            isFadingToBoss = false;
            next = BOSS_FIGHT_STATE;
        }
        return;
    }
    else if (bossFadeAlpha > 0.0f) {
        bossFadeAlpha -= BOSS_FADE_SPEED * dt;
    }

    if (isFadingFromBoss) {
        returnFadeAlpha -= BOSS_FADE_SPEED * dt;
        if (returnFadeAlpha <= 0.0f) {
            returnFadeAlpha = 0.0f;
            isFadingFromBoss = false;
        }
    }

    bool freeze = JumpScare_Update(dt);
    if (freeze) return;

    Timer_UpdateDayOverlay(dt);
    if (Timer_IsDayOverlayActive()) return;

    PauseMenu_Update(dt);
    if (PauseMenu_IsPaused()) return;

    if (Tutorial_Prompt_Answered() == false && IsTutorialActive()) {
        Tutorial_Update(dt);

		// Skip all gameplay input
        return;
    }

    Timer_Update(dt);

    if (Timer_IsTimeUp()) {
        if (CurrentDay >= 5) {
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
        Player_ResetPatientCounter(CurrentDay);
        Player_GenerateMission();
        Player_SetScaryByDay(CurrentDay);
        return;
    }

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

    float maxDist = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;
    if (camX > 0) camX = 0;
    else if ((camX < -maxDist) && !dementia) camX = -maxDist;

    doorNumAtPlayer = Doors_Update(camX);
    Doors_Animate(dt, doorNumAtPlayer, camX);
    Lift_Update(dt, camX, maxDist);
    Lift_HandleInput(floorNum);
    Lighting_Update(floorNum, camX, dementia);
    Frames_Update(dt);

    if (AEInputCheckTriggered(AEVK_E) && doorNumAtPlayer != -1) {
        if (floorNum == 0 && Doors_TryDisposal(floorNum, doorNumAtPlayer)) {
            // No jumpscare triggered! Now check if patient is a monster
            if (Player_IsScaryPatient()) {
                isFadingToBoss = true;
            }
            else {
                currentEndReason = REASON_WRONG_BASEMENT_DELIVERY;
                next = ENDGAME_STATE;
            }
        }

        bool success = Player_HandleInteraction(floorNum, (s8)doorNumAtPlayer + 1, CurrentDay);
        if (success) {
            if (!Player_HasPatient()) {
                liftActive = false;
                Player_SetScaryByDay(CurrentDay);
            }
        }
        else {
            Prompts_TriggerWrongRoom();
        }
    }

    Notifications_Update(liftActive, dt);
    Prompts_Update(dt, camX, doorNumAtPlayer, Lift_IsActive(), Lift_IsNear());
    Tutorial_Update(dt);
}

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
	if (camX > -(2 * DIST_BETWEEN_DOORS)) {
		DrawSquareMesh(squareMesh, -600.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
		Lift_DrawWorld(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
		Lift_DrawBackground(squareMesh, -700.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, floorNum);
	}

	// End Lift
	if ((camX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) && !dementia) {
		float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
		float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;
		DrawSquareMesh(squareMesh, endOffset + camX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
		Lift_DrawWorld(squareMesh, liftOffset + camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, floorNum, textXoffset, textY);
		Lift_DrawBackground(squareMesh, endOffset + camX + 200.0f, 0.0f, 800.0f, 900.0f, floorNum);
	}

    float playerY = -650.0f + (800.0f * 0.5f) + (Player_GetHeight() * 0.5f);

    AEMtx33 scale;
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(0.7f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEMtx33Scale(&scale, 2000.0f, 2000.0f);
    AEGfxSetTransform(scale.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    Player_Draw(50.0f, playerY);
    Draw_and_Flicker(camX, left_right, floorNum, dementia);

    DrawSquareMesh(squareMesh, 0.0f, 650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -650.0f, 1600.0f, 800.0f, COLOR_BLACK);

    Prompts_Draw();
    s8 targetFloor, targetDoor, destFloor, destDoor;
    Player_GetTargetRoom(targetFloor, targetDoor, destFloor, destDoor);
    Notifications_Draw(targetDoor, targetFloor, destFloor, destDoor);
    Tutorial_Draw();

    Timer_Draw(0.0f, 0.85f);
    Timer_DrawDayOverlay(squareMesh);
    Lift_Draw(squareMesh);

    DebugInfo info;
    info.camX = camX;
    info.day = CurrentDay;
    info.floorNum = floorNum;
    info.doorNumAtPlayer = doorNumAtPlayer;
    info.patientDoorNum = targetDoor;
    info.patientFloorNum = targetFloor;

    JumpScare_Draw();

    if (bossFadeAlpha > 0.0f || returnFadeAlpha > 0.0f) {
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

    PauseMenu_Draw(squareMesh);
    Debug_Draw(info);
}

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

    FreeMeshSafe(squareMesh);
    FreeMeshSafe(circleMesh);
}