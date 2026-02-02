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
bool facingRight = true;    // Player starts facing right
static Boss myBoss;

void Boss_Fight_Load()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
}

void Boss_Fight_Initialize()
{
    Bullets_Initialize();
	Boss_Initialize(myBoss);
    playerX = 0.0f;
    playerY = GROUND_Y;
    playerVelY = 0.0f;
    isGrounded = true;
    jumpCount = 0;

    // Default to facing right at start
    facingRight = true;
}

void Boss_Fight_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    // --- EXIT ---
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        Player_NewPatientRandom();  // New patient on room exit
        Timer_SetPaused(false);     // Timer unpause
        Notifications_Trigger();
        next = GAME_STATE;
        return;
    }

    // --- MOVEMENT ---
    if (AEInputCheckCurr(AEVK_D)) {
        playerX += PLAYER_SPEED * dt;
        facingRight = true; // <--- UPDATE DIRECTION
    }
    if (AEInputCheckCurr(AEVK_A)) {
        playerX -= PLAYER_SPEED * dt;
        facingRight = false; // <--- UPDATE DIRECTION
    }

    // --- JUMP LOGIC (Spacebar) ---
    if (AEInputCheckTriggered(AEVK_SPACE)) {
        if (isGrounded || jumpCount < MAX_JUMPS) {
            playerVelY = JUMP_FORCE;
            isGrounded = false;
            jumpCount++;
        }
        // REMOVED Fire_Bullet from here!
    }

    // --- SHOOT LOGIC (New Key: 'J') ---
    // You can also use AEVK_LBUTTON (Mouse Click) or AEVK_RETURN
    if (AEInputCheckTriggered(AEVK_J)) {
        // Offset Y by +15.0f so it shoots from chest height, not feet
        Fire_Bullet(playerX, playerY + 15.0f, facingRight);
    }

    // --- GRAVITY & PHYSICS ---
    playerVelY -= GRAVITY * dt;
    playerY += playerVelY * dt;

    // --- FLOOR COLLISION ---
    if (playerY <= GROUND_Y) {
        playerY = GROUND_Y;
        playerVelY = 0.0f;
        isGrounded = true;
        jumpCount = 0;
    }
    else {
        isGrounded = false;
    }

    // --- FAST FALL ---
    if (AEInputCheckCurr(AEVK_S)) {
        playerVelY -= 1000.0f * dt;
    }

    // --- BULLET UPDATES ---
    // Since this is a static boss room (no scrolling camera), 
    // passing 0.0f as camX is correct.
    Bullets_Update(dt, 0.0f,myBoss);
}

void Boss_Fight_Draw()
{
    AEGfxSetBackgroundColor(0.7f, 0.0f, 0.0f);

	// Draw Player
    DrawSquareMesh(squareMesh, playerX, playerY, 70.0f, 70.0f, COLOR_WHITE);

    // Top and bottom floor lines
    DrawSquareMesh(squareMesh, 0.0f, 500.0f, 1600.0f, 200.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -500.0f, 1600.0f, 200.0f, COLOR_BLACK);
	Bullets_Draw(0.0f);
    if (myBoss.active) {
        // Draw boss as a BIG RED square (0xFFFF0000)
        DrawSquareMesh(squareMesh, myBoss.x, myBoss.y, myBoss.w, myBoss.h, 0xFFFFFFFF);
    }
}

void Boss_Fight_Free()
{
    std::cout << "Startup: Free\n";
}

void Boss_Fight_Unload()
{
    if (squareMesh) { AEGfxMeshFree(squareMesh); squareMesh = nullptr; }
    if (circleMesh) { AEGfxMeshFree(circleMesh); circleMesh = nullptr; }
    std::cout << "Startup: Unload\n";
}