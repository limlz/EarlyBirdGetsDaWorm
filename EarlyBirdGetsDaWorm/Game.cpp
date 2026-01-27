#include "pch.hpp"

// Global Variables
static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;
static AEGfxTexture* lightingtest = nullptr;

s8 fontId = 0;

static f32 camX{}, playerY{};
f32 textXoffset{ 0.06f }, textY{ 50.0f };
s8 floorNum{1}; // Current floor the player is on
s8 demonFloorNum{ 1 }; // Floor where the demon is located
s8 demonRoomNum{ 3 }; // Room where the demon is located
s8 doorNumAtPlayer{ -1 }; // Door number the player is currently in front of
bool liftPromptActivated{}, liftActive{}, left_right{}, enterPrompt{};

void Game_Load()
{
    Doors_Load();
	Frames_Load();
    lightingtest = AEGfxTextureLoad("Assets/lightingtest.png");
    fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
    std::cout << "Startup: Load\n";

    // Notifications
    Notifications_Load();
}

void Game_Initialize()
{
    // Removed unused circleMesh
	Doors_Initialize();
	Frames_Initialize();
	Lighting_Initialize(7);
    squareMesh = CreateSquareMesh(COLOR_WHITE);
	circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
    std::cout << "Startup: Initialize\n";

    // Notifications
    Notifications_Initialize();
}

void Game_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();
    if (AEInputCheckCurr(AEVK_ESCAPE)) {
        next = GS_QUIT;
    }

    // Movement Logic
    if (AEInputCheckCurr(AEVK_A)) {
        camX += PLAYER_SPEED * dt;
        left_right = false;
    }
    if (AEInputCheckCurr(AEVK_D)) {
        camX -= PLAYER_SPEED * dt;
        left_right = true;
    }

    // Door update
    doorNumAtPlayer = Doors_Update(camX);

    // Camera/World Bounds
    // Right bound calculation: (NUM_DOORS + 1) accounts for the extra space for the right lift
    float maxDist = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

    if (camX > 0) {
        camX = 0;
    }
    else if (camX < -maxDist) {
        camX = -maxDist;
    }
    doorNumAtPlayer = Doors_Update(camX);
    // Lift Interaction Check
    // Checks if player is at the far left (start) or far right (end)
    if (camX > -5 || camX < -maxDist + 5) {
        liftPromptActivated = true;
        if (AEInputCheckTriggered(AEVK_L)) {
            liftActive = !liftActive;
        }
    }
    else {
        liftPromptActivated = false;
        liftActive = false;
    }
	Lighting_Update(floorNum);

	// Door Interaction Check (Door 3 is special, leads to boss fight ) (index 2 = door 3) (floor 1 only)
    if (AEInputCheckCurr(AEVK_E) && doorNumAtPlayer == demonRoomNum-1 && floorNum == demonFloorNum ) {
        next = BOSS_FIGHT_STATE;
    }
    else if (doorNumAtPlayer == demonRoomNum - 1 && floorNum == demonFloorNum) {
		enterPrompt = true;
    }
    else {
		enterPrompt = false;
    }
}

void Game_Draw()
{   
    Frames_Draw(floorNum, camX);

    // Background Color (1-9)
    if (floorNum >= 1) {
        AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);
    }
    else {
		AEGfxSetBackgroundColor(0.8f, 0.6f, 0.6f);
    }

    // Top and bottom floor lines
    DrawSquareMesh(squareMesh, 0.0f, 650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    // Draw Doors
    Doors_Draw(camX, floorNum, textXoffset, textY);

    // Draw Left Wall + Lift (Start)
    if (camX > -(2 * DIST_BETWEEN_DOORS)) {
        DrawSquareMesh(squareMesh, -600.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        DrawSquareMesh(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
        if (floorNum != 0) {
            DrawSquareMesh(squareMesh, -700.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
		}
    }

    // Draw Right Wall + Lift (End)
    // Dynamic check based on NUM_DOORS
    if (camX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) {
        float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
        float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

        DrawSquareMesh(squareMesh, endOffset + camX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        DrawSquareMesh(squareMesh, liftOffset + camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
        if (floorNum != 0) {
            DrawSquareMesh(squareMesh, endOffset + camX + 200.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
        }
    }


    //basic player (lighting test)

    DrawTextureMesh(squareMesh, lightingtest, 50.0f, -150.0f, 250.0f, 250.0f, 1.0f);

    // 2. Draw "Room Darkness" (Simple dark tint)
    // Just draw one giant black square over the screen with alpha 0.7
    AEMtx33 scale;
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(0.7f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEMtx33Scale(&scale, 2000.0f, 2000.0f);
    AEGfxSetTransform(scale.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // 3. Draw The Flashlights (They will glow on top of the dark)
    Draw_and_Flicker(camX, left_right, floorNum);

    // Top and bottom floor lines
    DrawSquareMesh(squareMesh, 0.0f, 650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -650.0f, 1600.0f, 800.0f, COLOR_BLACK);
    // Lift UI Overlay
    if (liftPromptActivated && !liftActive) {
        AEGfxPrint(fontId, "Click L to access lift!", -0.5f, 0.8f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (liftActive) {
        // Background rectangle
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 500.0f, 800.0f, COLOR_LIFT_BG);
        // Lift console
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 400.0f, 700.0f, COLOR_LIFT_CONSOLE);
        // Buttons would go here
        for (int i{}; i < NUM_OF_FLOOR; i++) {
            if (AEInputCheckCurr(AEVK_0 + i)) {
				floorNum = i;
                liftActive = false;
            }
        }
        
    }

	// Draw Enter Prompt
    if (enterPrompt) {
        AEGfxPrint(fontId, "Press E to enter the room", -0.5f, 0.7f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	}

    // Notification
    Notifications_Update(floorNum, liftActive);
}

void Game_Free()
{
    std::cout << "Startup: Free\n";
}

void Game_Unload()
{
    Frames_Unload();
    std::cout << "Startup: Unload\n";
}