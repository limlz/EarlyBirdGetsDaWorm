#include "pch.hpp"

// --- MACROS --- //
// Colors
#define COLOR_WHITE         0xFFFFFFFF
#define COLOR_BLACK         0x000000FF
#define COLOR_NIGHT_BLUE    0x191970FF
#define COLOR_DOOR_BROWN    0x8B4513FF
#define COLOR_LIFT_GREY     0x696969FF
#define COLOR_LIFT_BG       0x2F4F4FFF
#define COLOR_LIFT_CONSOLE  0xC0C0C0FF
#define COLOR_LIFT_BUTTON   0xFFD700FF

// Dimensions & Settings
#define PLAYER_SPEED        1500.0f
#define DOOR_WIDTH          150.0f
#define DOOR_HEIGHT         300.0f
#define DIST_BETWEEN_DOORS  600.0f
#define LIFT_WIDTH          200.0f
#define LIFT_HEIGHT         300.0f
#define NUM_OF_FLOOR        10
#define NUM_DOORS           10
#define SCREEN_WIDTH_HALF   800.0f
#define SCREEN_HEIGHT_HALF  450.0f

// Global Variables
static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh; 
s8 fontId = 0;

f32 playerX{}, playerY{};
float textXoffset{ 0.06f }, textY{ 50.0f };
int floorNum{1}; // Current floor the player is on

bool liftPromptActivated{}, liftActive{};

void Game_Load()
{
    fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
    std::cout << "Startup: Load\n";
}

void Game_Initialize()
{
    // Removed unused circleMesh
    squareMesh = CreateSquareMesh(COLOR_WHITE);
	circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
    std::cout << "Startup: Initialize\n";
}

void Game_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    if (AEInputCheckCurr(AEVK_ESCAPE)) {
        next = GS_QUIT;
    }

    // Movement Logic
    if (AEInputCheckCurr(AEVK_A)) {
        playerX += PLAYER_SPEED * dt;
    }
    if (AEInputCheckCurr(AEVK_D)) {
        playerX -= PLAYER_SPEED * dt;
    }

    // Camera/World Bounds
    // Right bound calculation: (NUM_DOORS + 1) accounts for the extra space for the right lift
    float maxDist = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

    if (playerX > 0) {
        playerX = 0;
    }
    else if (playerX < -maxDist) {
        playerX = -maxDist;
    }

    // Lift Interaction Check
    // Checks if player is at the far left (start) or far right (end)
    if (playerX > -5 || playerX < -maxDist + 5) {
        liftPromptActivated = true;
        if (AEInputCheckTriggered(AEVK_L)) {
            liftActive = !liftActive;
        }
    }
    else {
        liftPromptActivated = false;
        liftActive = false;
    }
}

void Game_Draw()
{
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
    for (int i = 0; i < NUM_DOORS; i++) {
        // Calculate world position of the door based on player offset
        float wallX = DIST_BETWEEN_DOORS + playerX + (DIST_BETWEEN_DOORS * i);

        // Simple Culling: Only draw if within reasonable screen bounds
        // (Adjusted logic to be relative to screen width rather than hardcoded 10)
        if (wallX > -SCREEN_WIDTH_HALF - DOOR_WIDTH && wallX < SCREEN_WIDTH_HALF + DOOR_WIDTH) {

            DrawSquareMesh(squareMesh, wallX, -100.0f, DOOR_WIDTH, DOOR_HEIGHT, COLOR_DOOR_BROWN);

            // Door Text
            char textBuffer[32];

            if (floorNum == 0) {
                sprintf_s(textBuffer, "B1-%02d", i + 1);
            }
            else {
                sprintf_s(textBuffer, "%02d-%02d", floorNum, i + 1);
            }   

            float textNDC_X = (wallX / SCREEN_WIDTH_HALF) - textXoffset;
            float textNDC_Y = textY / SCREEN_HEIGHT_HALF;

            AEGfxPrint(fontId, textBuffer, textNDC_X, textNDC_Y, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    // Draw Left Wall + Lift (Start)
    if (playerX > -(2 * DIST_BETWEEN_DOORS)) {
        DrawSquareMesh(squareMesh, -600.0f + playerX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        DrawSquareMesh(squareMesh, playerX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
        if (floorNum != 0) {
            DrawSquareMesh(squareMesh, -700.0f + playerX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
		}
    }

    // Draw Right Wall + Lift (End)
    // Dynamic check based on NUM_DOORS
    if (playerX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) {
        float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
        float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;

        DrawSquareMesh(squareMesh, endOffset + playerX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
        DrawSquareMesh(squareMesh, liftOffset + playerX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
        if (floorNum != 0) {
            DrawSquareMesh(squareMesh, endOffset + playerX + 200.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
        }
    }

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
}

void Game_Free()
{
    std::cout << "Startup: Free\n";
}

void Game_Unload()
{
    std::cout << "Startup: Unload\n";
}