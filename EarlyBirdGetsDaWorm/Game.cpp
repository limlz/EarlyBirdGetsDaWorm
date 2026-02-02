#include "pch.hpp"

// Global Variables
static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;
static AEGfxTexture* lightingtest = nullptr;

// REMOVED: s8 fontId = 0; // Now handled inside Prompts.cpp

static f32 camX{}, playerY{}, playerX{};
f32 textXoffset{ 0.06f }, textY{ 50.0f };
s8 floorNum{ 1 };
s8 demonFloorNum{ 1 };
s8 demonRoomNum{ 3 };
s8 doorNumAtPlayer{ -1 };

// REMOVED: bool liftPromptActivated{}, enterPrompt{}; // Handled by Prompts.cpp
bool liftActive{}, left_right{ 1 };
bool dementia{ false };

// Day counter
static int gDay = 1;
static bool gSessionStarted = false;

static float gSpawnX = 50.0f;
static float gSpawnY = 0.0f;
static float ComputeSpawnYFromBorder()
{
    float borderCenterY = -650.0f;
    float borderHeight = 800.0f;
    float borderTopY = borderCenterY + (borderHeight * 0.5f);
    return borderTopY + (Player_GetHeight() * 0.5f);
}

void Game_Load()
{
    Doors_Load();
    Frames_Load();
    Player_Load();
    Player_NewPatientRandom();
    Timer_Load();
    Prompts_Load();

    if (!gSessionStarted)
    {
        gDay = 1;
        Timer_Reset();
        Timer_StartDayOverlay(gDay);
        camX = 0.0f;

        Player_SetScary(false);
        gSessionStarted = true;
    }

    std::cout << "Startup: Load\n";

    Notifications_Load();
    Wall_Load();
}

void Game_Initialize()
{
    Doors_Initialize();
    Frames_Initialize();
    Lighting_Initialize(7);
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
    std::cout << "Startup: Initialize\n";

    Notifications_Initialize();
    Wall_Initialize();
}

void Game_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    Timer_UpdateDayOverlay(dt);

    if (Timer_IsDayOverlayActive()) return;

    Timer_Update(dt);

    if (Timer_IsTimeUp())
    {
        if (gDay >= 5) { next = MAIN_MENU; return; }

        gDay++;
        Timer_Reset();
        Timer_StartDayOverlay(gDay);
        camX = 0.0f;

        if (gDay == 2) Player_SetScary(true);

        return;
    }

    if (AEInputCheckTriggered(AEVK_K)) Timer_DebugSetTime(5 * 60 + 58);
    if (AEInputCheckTriggered(AEVK_ESCAPE)) { next = OTHERS_MENU; return; }

    // Movement
    if (AEInputCheckCurr(AEVK_A)) { camX += PLAYER_SPEED * dt; left_right = false; }
    if (AEInputCheckCurr(AEVK_D)) { camX -= PLAYER_SPEED * dt; left_right = true; }
    if (AEInputCheckTriggered(AEVK_O)) dementia = !dementia;

    bool moveRight = AEInputCheckCurr(AEVK_D);
    bool moveLeft = AEInputCheckCurr(AEVK_A);
    bool isWalking = moveRight || moveLeft;

    if (moveRight) Player_SetFacing(1);
    else if (moveLeft) Player_SetFacing(-1);

    Player_Update(dt, isWalking);
    doorNumAtPlayer = Doors_Update(camX);

    float maxDist = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

    if (camX > 0) camX = 0;
    else if ((camX < -maxDist) && !dementia) camX = -maxDist;

    doorNumAtPlayer = Doors_Update(camX);

    // --- LIFT LOGIC ---
    // Calculate if we are in range
    bool nearLift = (camX > -5 || camX < -maxDist + 5);

    // Only handle the INPUT here. The prompt visualization is now in Prompts_Update
    if (nearLift && AEInputCheckTriggered(AEVK_L)) {
        liftActive = !liftActive;
    }
    // If we walk away, close the lift
    if (!nearLift) {
        liftActive = false;
    }

    Lighting_Update(floorNum, camX, dementia);

    // Interaction Logic
    if (AEInputCheckTriggered(AEVK_E) && doorNumAtPlayer == demonRoomNum - 1 && floorNum == demonFloorNum)
    {
        Timer_SetPaused(true);
        next = BOSS_FIGHT_STATE;
        return;
    }
    else if (AEInputCheckTriggered(AEVK_E) && doorNumAtPlayer == 0 && floorNum == 0) {
        floorNum = 1;
        camX = 0.0f;
    }

    Notifications_Update(liftActive);

    // --- PROMPTS UPDATE ---
    // One clean call to handle all UI logic
    Prompts_Update(camX, doorNumAtPlayer, liftActive, nearLift);
}

void Game_Draw()
{
    Wall_Draw(camX, floorNum);
    Frames_Draw(floorNum, camX);

    if (floorNum >= 1) AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);
    else AEGfxSetBackgroundColor(0.8f, 0.6f, 0.6f);

    DrawSquareMesh(squareMesh, 0.0f, 650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -650.0f, 1600.0f, 800.0f, COLOR_BLACK);

    Doors_Draw(camX, floorNum, textXoffset, textY, dementia);

    if (camX > -(2 * DIST_BETWEEN_DOORS)) {
        DrawSquareMesh(squareMesh, -600.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        DrawSquareMesh(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
        if (floorNum != 0) DrawSquareMesh(squareMesh, -700.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
    }

    if ((camX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) && !dementia) {
        float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
        float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

        DrawSquareMesh(squareMesh, endOffset + camX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        DrawSquareMesh(squareMesh, liftOffset + camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
        if (floorNum != 0) DrawSquareMesh(squareMesh, endOffset + camX + 200.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
    }

    float borderCenterY = -650.0f;
    float borderHeight = 800.0f;
    float borderTopY = borderCenterY + (borderHeight * 0.5f);
    float playerY = borderTopY + (Player_GetHeight() * 0.5f);

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

    if (liftActive) {
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 500.0f, 800.0f, COLOR_LIFT_BG);
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 400.0f, 700.0f, COLOR_LIFT_CONSOLE);
        for (int i{}; i < NUM_OF_FLOOR; i++) {
            if (AEInputCheckTriggered(AEVK_0 + i)) {
                floorNum = i;
                liftActive = false;
            }
        }
    }

    // --- PROMPTS DRAW ---
    Prompts_Draw();
    Notifications_Draw();


    Timer_Draw(0.0f, 0.85f);
    Timer_DrawDayOverlay(squareMesh);
}

void Game_Free()
{
    std::cout << "Startup: Free\n";
}

void Game_Unload()
{
    Frames_Unload();
    Player_Unload();
    Prompts_Unload(); // <--- Cleanup Font
    std::cout << "Startup: Unload\n";
}