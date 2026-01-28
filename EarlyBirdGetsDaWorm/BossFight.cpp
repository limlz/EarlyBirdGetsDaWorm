#include "pch.hpp"

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;

static f32 playerX, playerY;
float playerVelY = 0.0f;   // Current vertical speed
bool isGrounded = false;   // Are we touching the floor?

const float GRAVITY = 1500.0f;
const float JUMP_FORCE = 850.0f;
const float GROUND_Y = -360.0f; 
int jumpCount = 0;          
const int MAX_JUMPS = 2;    

void Boss_Fight_Load()
{
    return;
}

void Boss_Fight_Initialize()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
}

void Boss_Fight_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        next = GAME_STATE;
	}
    if (AEInputCheckCurr(AEVK_D)) {
        playerX += PLAYER_SPEED * dt;
    }
    if (AEInputCheckCurr(AEVK_A)) {
        playerX -= PLAYER_SPEED * dt;
    }
	// --- JUMP LOGIC ---
    if (AEInputCheckTriggered(AEVK_SPACE)) {
        if (isGrounded || jumpCount < MAX_JUMPS) {
            playerVelY = JUMP_FORCE;  
            isGrounded = false;        
            jumpCount++;              
        }
    }

    playerVelY -= GRAVITY * dt;
    playerY += playerVelY * dt;

    // --- FLOOR COLLISION & RESET ---
    if (playerY <= GROUND_Y) {
        playerY = GROUND_Y;
        playerVelY = 0.0f;
        isGrounded = true;

        // Reset jump count when we touch the floor
        jumpCount = 0;
    }
    else {
        isGrounded = false;
    }

    // fall faster
    if (AEInputCheckCurr(AEVK_S)) {
        // fast fall
        playerVelY -= 1000.0f * dt;
    }
}

void Boss_Fight_Draw()
{
    AEGfxSetBackgroundColor(0.7f, 0.0f, 0.0f);

	// Draw Player
    DrawSquareMesh(squareMesh, playerX, playerY, 70.0f, 70.0f, COLOR_WHITE);

    // Top and bottom floor lines
    DrawSquareMesh(squareMesh, 0.0f, 500.0f, 1600.0f, 200.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -500.0f, 1600.0f, 200.0f, COLOR_BLACK);
}

void Boss_Fight_Free()
{
    std::cout << "Startup: Free\n";
}

void Boss_Fight_Unload()
{
    std::cout << "Startup: Unload\n";
}