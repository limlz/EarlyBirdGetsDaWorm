#include "pch.hpp"
#include "Boss.hpp" // <--- Include the new header
#include <cmath>

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;

// --- PLAYER PHYSICS ---
static f32 playerX, playerY;
float playerVelY = 0.0f;
bool isGrounded = false;
const float GRAVITY = 1500.0f;
const float JUMP_FORCE = 850.0f;
const float GROUND_Y = -360.0f;
int jumpCount = 0;
const int MAX_JUMPS = 2;
bool facingRight = true;

// --- PLAYER HEALTH ---
int playerHP = 3;
const int MAX_PLAYER_HP = 3;
bool isPlayerHit = false;
float hitTimer = 0.0f;

// --- BOSS INSTANCE ---
static Boss myBoss;

// ---------------------------------------------------------
// GAME STATE FUNCTIONS
// ---------------------------------------------------------

void Boss_Fight_Load()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
}

void Boss_Fight_Initialize()
{
    Bullets_Initialize();

    // Initialize Boss (Logic is now in Boss.cpp)
    Boss_Initialize(myBoss);

    // Setup Player
    playerX = -600.0f;
    playerY = GROUND_Y;
    playerVelY = 0.0f;
    isGrounded = true;
    jumpCount = 0;
    facingRight = true;
    playerHP = MAX_PLAYER_HP;
    isPlayerHit = false;
}

void Boss_Fight_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    // --- EXIT LOGIC ---
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        Player_NewPatientRandom();
        Timer_SetPaused(false);
        Notifications_Trigger();
        next = GAME_STATE;
        return;
    }

    // --- PLAYER CONTROLS ---
    if (AEInputCheckCurr(AEVK_D)) { playerX += PLAYER_SPEED * dt; facingRight = true; }
    if (AEInputCheckCurr(AEVK_A)) { playerX -= PLAYER_SPEED * dt; facingRight = false; }

    if (AEInputCheckTriggered(AEVK_SPACE)) {
        if (isGrounded || jumpCount < MAX_JUMPS) {
            playerVelY = JUMP_FORCE;
            isGrounded = false;
            jumpCount++;
        }
    }

    // --- SHOOT & BOSS DODGE TRIGGER ---
    if (AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(AEVK_J)) {
        Fire_Bullet(playerX, playerY + 15.0f, facingRight);

        // Interaction: If player shoots, 30% chance Boss dodges
        // We modify the boss's properties directly
        if (myBoss.active && (rand() % 100 < 30)) {
            // Swap between high and low positions
            if (myBoss.baseY > 0) myBoss.baseY = -200.0f;
            else myBoss.baseY = 200.0f;
        }
    }

    // --- GRAVITY ---
    playerVelY -= GRAVITY * dt;
    playerY += playerVelY * dt;

    if (playerY <= GROUND_Y) {
        playerY = GROUND_Y; playerVelY = 0.0f; isGrounded = true; jumpCount = 0;
    }
    else {
        isGrounded = false;
    }
    if (AEInputCheckCurr(AEVK_S)) { playerVelY -= 1000.0f * dt; }

    // ----------------------------------------------------
    // --- UPDATE BOSS ---
    // ----------------------------------------------------
    // We pass player position so the boss knows where to aim
    Boss_Update(myBoss, dt, playerX, playerY);

    // --- COLLISION: PLAYER vs BOSS BULLETS ---
    if (myBoss.active) {
        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (myBoss.bullets[i].active) {

                // Simple Box Collision check
                if (abs(myBoss.bullets[i].x - playerX) < 35.0f &&
                    abs(myBoss.bullets[i].y - playerY) < 35.0f)
                {
                    playerHP--;
                    myBoss.bullets[i].active = false; // Destroy bullet
                    isPlayerHit = true;
                    hitTimer = 0.2f;

                    if (playerHP <= 0) {
                        Boss_Fight_Initialize(); // Restart on death
                    }
                }
            }
        }
    }

    // Handle Flash Effect
    if (isPlayerHit) {
        hitTimer -= dt;
        if (hitTimer <= 0.0f) isPlayerHit = false;
    }

    // Update Player Bullets (Make sure Bullets_Update takes Boss&)
    Bullets_Update(dt, 0.0f, myBoss);
}

void Boss_Fight_Draw()
{
    AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);

    // Draw Player (Flash Red if hit)
    u32 playerColor = isPlayerHit ? 0xFFFF00FF : COLOR_WHITE;
    DrawSquareMesh(squareMesh, playerX, playerY, 70.0f, 70.0f, playerColor);

    // Draw Floor
    DrawSquareMesh(squareMesh, 0.0f, 500.0f, 1600.0f, 200.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, -500.0f, 1600.0f, 200.0f, COLOR_BLACK);

    // Draw Player Bullets
    Bullets_Draw(0.0f);

    // Draw Boss AND Boss Bullets (Now handled inside Boss.cpp)
    Boss_Draw(myBoss, squareMesh);

    // Draw HP UI (Top Left Green Squares)
    float uiStartX = -700.0f;
    for (int i = 0; i < playerHP; i++) {
        DrawSquareMesh(squareMesh, uiStartX + (i * 40.0f), 550.0f, 30.0f, 30.0f, 0xFF00FF00);
    }
}

void Boss_Fight_Free() {
    // Freeing handled in Unload
}
void Boss_Fight_Unload() {
    FreeMeshSafe(squareMesh);
    FreeMeshSafe(circleMesh);
    Bullets_Free();
}