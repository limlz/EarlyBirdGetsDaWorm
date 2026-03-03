#include "pch.hpp"
#include "Boss.hpp"
#include <cmath>
#include <cstdlib> // For rand()

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;


// Textures
static AEGfxTexture* wallBgTexture = nullptr;

// --- PLAYER PHYSICS ---
static f32 playerX, playerY;
float playerVelY = 0.0f;
bool isGrounded = false;
const float GRAVITY = 1500.0f;
const float JUMP_FORCE = 850.0f;
const float GROUND_Y = -315.0f;
int jumpCount = 0;
const int MAX_JUMPS = 2;
bool facingRight = true;

// --- PLAYER HEALTH ---
int playerHP = 10;
const int MAX_PLAYER_HP = 10;
bool isPlayerHit = false;
float hitTimer = 0.0f;

// --- WEAPON: HOLY WATER GUN ---
static float fireCooldown = 0.0f;
const float FIRE_RATE = 0.05f; // Fires a particle every 0.05 seconds (20 per second!)
static float bossDodgeCooldown = 0.0f;

// --- BOSS INSTANCE ---
static Boss myBoss;

// ---------------------------------------------------------
// NEW UI FUNCTION: Player Health Bar
// ---------------------------------------------------------
void Player_DrawHealthBar(AEGfxVertexList* mesh)
{
    float hpPercent = (float)playerHP / (float)MAX_PLAYER_HP;
    if (hpPercent < 0.0f) hpPercent = 0.0f;

    float barWidth = 400.0f;
    float barHeight = 25.0f;
    float startX = -500.0f;
    float startY = 400.0f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    DrawSquareMesh(mesh, startX, startY, barWidth, barHeight, 0xFF0044FF);

    if (playerHP > 0) {
        float currentWidth = barWidth * hpPercent;
        float fillX = startX - (barWidth / 2.0f) + (currentWidth / 2.0f);
        DrawSquareMesh(mesh, fillX, startY, currentWidth, barHeight, 0xFF00FFFF);
    }
}

// ---------------------------------------------------------
// GAME STATE FUNCTIONS
// ---------------------------------------------------------

void Boss_Fight_Load()
{
    wallBgTexture = LoadTextureChecked("Assets/Background/WALL_BG.png");
    Boss_Load();
    squareMesh = CreateSquareMesh(COLOR_WHITE);
    circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);
}

void Boss_Fight_Initialize()
{
    Bullets_Initialize();

    Boss_Initialize(myBoss);

    playerX = -600.0f;
    playerY = GROUND_Y;
    playerVelY = 0.0f;
    isGrounded = true;
    jumpCount = 0;
    facingRight = true;
    playerHP = MAX_PLAYER_HP;
    isPlayerHit = false;

    fireCooldown = 0.0f;
    bossDodgeCooldown = 0.0f;
}

void Boss_Fight_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    // --- EXIT LOGIC ---
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        Player_NewPatientRandom();
        Timer_SetPaused(false);
        PauseMenu_SetPaused(false);
        Notifications_Trigger();
        next = GAME_STATE;
        return;
    }

    // --- PLAYER CONTROLS & WALL LIMITS ---
    if (AEInputCheckCurr(AEVK_D)) { playerX += PLAYER_SPEED * dt; facingRight = true; }
    if (AEInputCheckCurr(AEVK_A)) { playerX -= PLAYER_SPEED * dt; facingRight = false; }

    if (playerX < -765.0f) playerX = -765.0f;
    if (playerX > 765.0f)  playerX = 765.0f;

    if (AEInputCheckTriggered(AEVK_SPACE)) {
        if (isGrounded || jumpCount < MAX_JUMPS) {
            playerVelY = JUMP_FORCE;
            isGrounded = false;
            jumpCount++;
        }
    }

    // --- HOLY WATER GUN LOGIC ---
    // Decrease timers
    fireCooldown -= dt;
    bossDodgeCooldown -= dt;

    // CHANGED: Now checks if the button is HELD DOWN (AEInputCheckCurr)
    if (AEInputCheckCurr(AEVK_LBUTTON)) {

        // Only fire if the cooldown timer has finished
        if (fireCooldown <= 0.0f) {
            fireCooldown = FIRE_RATE; // Reset timer

            s32 cursorX, cursorY;
            AEInputGetCursorPosition(&cursorX, &cursorY);

            float worldMouseX = (float)cursorX - (AEGfxGetWindowWidth() / 2.0f);
            float worldMouseY = (AEGfxGetWindowHeight() / 2.0f) - (float)cursorY;

            float startX = playerX;
            float startY = playerY + 15.0f;

            float dirX = worldMouseX - startX;
            float dirY = worldMouseY - startY;

            float length = sqrtf((dirX * dirX) + (dirY * dirY));
            if (length > 0.0001f) {
                dirX /= length;
                dirY /= length;
            }
            else {
                dirX = facingRight ? 1.0f : -1.0f;
                dirY = 0.0f;
            }

            // ADD SPREAD: Randomizes the direction slightly to simulate spraying liquid
            float spreadX = ((rand() % 200 - 100) / 1000.0f); // Random between -0.1 and 0.1
            float spreadY = ((rand() % 200 - 100) / 1000.0f);
            dirX += spreadX;
            dirY += spreadY;

            // Re-normalize after adding spread
            length = sqrtf((dirX * dirX) + (dirY * dirY));
            if (length > 0.0001f) {
                dirX /= length;
                dirY /= length;
            }

            Fire_Bullet(startX, startY, dirX, dirY);

            // Interaction: If sprayed, boss occasionally dodges
            if (myBoss.active && !myBoss.shieldActive && bossDodgeCooldown <= 0.0f) {
                if (rand() % 100 < 15) { // 15% chance to dodge per drop hitting near him
                    if (myBoss.baseY > 0) myBoss.baseY = -200.0f;
                    else myBoss.baseY = 200.0f;

                    // Prevent him from dodging again for 1.5 seconds so he doesn't teleport rapidly
                    bossDodgeCooldown = 1.5f;
                }
            }
        }
    }

    // --- GRAVITY & CEILING LIMITS ---
    playerVelY -= GRAVITY * dt;
    playerY += playerVelY * dt;

    if (playerY > 415.0f) {
        playerY = 415.0f;
        if (playerVelY > 0.0f) playerVelY = 0.0f;
    }

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
    Boss_Update(myBoss, dt, playerX, playerY);

    if (myBoss.active) {
        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (myBoss.bullets[i].active) {
                if (abs(myBoss.bullets[i].x - playerX) < 35.0f &&
                    abs(myBoss.bullets[i].y - playerY) < 35.0f)
                {
                    playerHP--;
                    myBoss.bullets[i].active = false;
                    isPlayerHit = true;
                    hitTimer = 0.2f;

                    if (playerHP <= 0) {
                        Boss_Fight_Initialize();
                    }
                }
            }
        }
    }

    if (isPlayerHit) {
        hitTimer -= dt;
        if (hitTimer <= 0.0f) isPlayerHit = false;
    }

    Bullets_Update(dt, 0.0f, myBoss);
}

void Boss_Fight_Draw()
{
    DrawTextureMesh(squareMesh, wallBgTexture, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

    u32 playerColor = isPlayerHit ? 0xFFFF00FF : COLOR_WHITE;
    DrawSquareMesh(squareMesh, playerX, playerY, 70.0f, 70.0f, playerColor);

    DrawSquareMesh(squareMesh, 0.0f, -400.0f, 1600.0f, 100.0f, COLOR_BLACK);
    DrawSquareMesh(squareMesh, 0.0f, 400.0f, 1600.0f, 100.0f, COLOR_BLACK);

    Bullets_Draw(0.0f);
    Boss_Draw(myBoss, squareMesh);
    Player_DrawHealthBar(squareMesh);
    Bullets_DrawAmmoUI();
    Boss_DrawHealthBar(myBoss, squareMesh);
}

void Boss_Fight_Free() {
}
void Boss_Fight_Unload() {
    Boss_Unload();
    FreeMeshSafe(squareMesh);
    FreeMeshSafe(circleMesh);
    Bullets_Free();
}