#include "pch.hpp"
#include "Boss.hpp"
#include <cmath> // Required for sinf(), cosf(), sqrtf(), atan2f(), fabsf()

// --- Boss Textures ---
static AEGfxTexture* bossTex = nullptr;
static AEGfxTexture* bossBulletFrames[2] = { nullptr, nullptr };

// --- Bullet Animation Variables ---
static int currentBulletFrame = 0;
static float bulletAnimTimer = 0.0f;
const float BULLET_ANIM_SPEED = 0.1f;

// --- Define Max Health ---
const int BOSS_MAX_HEALTH = 500;

void Boss_Load()
{
    bossTex = LoadTextureChecked("Assets/Boss/boss.png");
    bossBulletFrames[0] = LoadTextureChecked("Assets/Boss/boss_bullet.png");
    bossBulletFrames[1] = LoadTextureChecked("Assets/Boss/boss_bullet2.png");
}

void Boss_Initialize(Boss& boss)
{
    boss.active = true;
    boss.health = BOSS_MAX_HEALTH;

    // Overall bounds (used mainly for drawing the shield and fallback square)
    boss.w = 240.0f;
    boss.h = 480.0f;

    boss.x = 600.0f;
    boss.y = 0.0f;
    boss.baseY = 0.0f;

    boss.moveTimer = 0.0f;
    boss.shootTimer = 2.0f;

    boss.currentState = 0;
    boss.stateTimer = 4.0f;
    boss.shieldActive = false;

    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        boss.bullets[i].active = false;
    }
}

void Boss_Update(Boss& boss, float dt, float playerX, float playerY)
{
    if (!boss.active) {
        PauseMenu_SetPaused(false);
        next = GAME_STATE;
    }

    bulletAnimTimer += dt;
    if (bulletAnimTimer >= BULLET_ANIM_SPEED) {
        bulletAnimTimer = 0.0f;
        currentBulletFrame = (currentBulletFrame == 0) ? 1 : 0;
    }

    boss.moveTimer += dt * 2.0f;
    float hoverSpeed = boss.shieldActive ? 4.0f : 2.0f;
    float hoverOffset = sinf(boss.moveTimer * hoverSpeed) * 100.0f;

    float targetY = boss.baseY + hoverOffset;
    boss.y += (targetY - boss.y) * 4.0f * dt;

    // Prevent clipping: Ceiling is 350, Floor is -350. Boss half-height is 240.
    if (boss.y > 110.0f) boss.y = 110.0f;
    if (boss.y < -110.0f) boss.y = -110.0f;

    boss.stateTimer -= dt;

    if (boss.stateTimer <= 0.0f) {
        boss.currentState = rand() % 3;

        if (boss.currentState == 0) {
            boss.stateTimer = 4.0f;
            boss.shieldActive = false;
        }
        else if (boss.currentState == 1) {
            boss.stateTimer = 3.0f;
            boss.shieldActive = false;
        }
        else if (boss.currentState == 2) {
            boss.stateTimer = 3.5f;
            boss.shieldActive = true;
            boss.shootTimer = 0.5f;
        }
    }

    // --- NEW: CALCULATE HAND POSITION ---
    // This targets the left arm stretching out towards the player
    float handX = boss.x - 120.0f;
    float handY = boss.y + 20.0f;

    boss.shootTimer -= dt;

    // State 0: Normal Targeted Shot (From the Hand!)
    if (boss.currentState == 0 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 1.0f;

        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (!boss.bullets[i].active) {
                boss.bullets[i].active = true;
                boss.bullets[i].hp = 10;
                boss.bullets[i].x = handX; // Spawn at Hand
                boss.bullets[i].y = handY;

                float diffX = playerX - handX;
                float diffY = playerY - handY;
                float length = sqrtf(diffX * diffX + diffY * diffY);

                if (length != 0) {
                    boss.bullets[i].dirX = (diffX / length) * 600.0f;
                    boss.bullets[i].dirY = (diffY / length) * 600.0f;
                }
                break;
            }
        }
    }
    // State 1: Spread Attack (From the Hand!)
    else if (boss.currentState == 1 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 1.5f;

        float diffX = playerX - handX;
        float diffY = playerY - handY;
        float baseAngle = atan2f(diffY, diffX);

        int bulletsFired = 0;
        for (int i = 0; i < MAX_BOSS_BULLETS && bulletsFired < 5; i++) {
            if (!boss.bullets[i].active) {
                boss.bullets[i].active = true;
                boss.bullets[i].hp = 5;
                boss.bullets[i].x = handX; // Spawn at Hand
                boss.bullets[i].y = handY;

                float spreadAngle = baseAngle + ((bulletsFired - 2) * 0.2f);

                boss.bullets[i].dirX = cosf(spreadAngle) * 500.0f;
                boss.bullets[i].dirY = sinf(spreadAngle) * 500.0f;

                bulletsFired++;
            }
        }
    }
    // State 2: Shield Phase
    else if (boss.currentState == 2 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 0.5f;
        if (boss.health < BOSS_MAX_HEALTH) {
            boss.health += 1;
        }
    }

    // --- UPDATE BOSS BULLETS ---
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            boss.bullets[i].x += boss.bullets[i].dirX * dt;
            boss.bullets[i].y += boss.bullets[i].dirY * dt;

            if (boss.bullets[i].x < -1000 || boss.bullets[i].x > 1000 || boss.bullets[i].y < -1000 || boss.bullets[i].y > 1000) {
                boss.bullets[i].active = false;
            }
        }
    }
}

void Boss_DrawHealthBar(Boss& boss, AEGfxVertexList* mesh)
{
    if (!boss.active) return;

    float hpPercent = (float)boss.health / (float)BOSS_MAX_HEALTH;
    if (hpPercent < 0.0f) hpPercent = 0.0f;

    float barWidth = 400.0f;
    float barHeight = 25.0f;
    float startX = 500.0f;
    float startY = 400.0f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    DrawSquareMesh(mesh, startX, startY, barWidth, barHeight, 0xFF4400FF);

    if (boss.health > 0) {
        float currentWidth = barWidth * hpPercent;
        float fillX = startX - (barWidth / 2.0f) + (currentWidth / 2.0f);
        DrawSquareMesh(mesh, fillX, startY, currentWidth, barHeight, 0xFFFF00FF);
    }

    if (boss.shieldActive) {
        AEGfxSetTransparency(0.3f);
        DrawSquareMesh(mesh, startX, startY, barWidth, barHeight, 0xFF0088FF);
        AEGfxSetTransparency(1.0f);
    }
}

void Boss_Draw(Boss& boss, AEGfxVertexList* mesh)
{
    if (!boss.active) return;

    // --- DRAW DEFENSE SHIELD ---
    if (boss.shieldActive) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.5f);

        DrawSquareMesh(mesh, boss.x, boss.y, boss.w + 60.0f, boss.h + 60.0f, 0xFF0088FF);
        AEGfxSetTransparency(1.0f);
    }

    // --- DRAW BOSS ---
    if (bossTex) {
        DrawTextureMesh(mesh, bossTex, boss.x, boss.y, boss.w, boss.h, 1.0f);
    }
    else {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        DrawSquareMesh(mesh, boss.x, boss.y, boss.w, boss.h, 0xFFFF00FF);
    }

    // =================================================================
    // DEBUG: UNCOMMENT THIS BLOCK TO SEE THE COMPOUND HITBOXES!
    // Feel free to tweak the numbers in Boss_CheckCollision to fit perfectly
    // =================================================================
    
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.4f);
    DrawSquareMesh(mesh, boss.x + 20.0f, boss.y + 110.0f, 100.0f, 180.0f, 0xFF00FFFF); // Head/Torso
    DrawSquareMesh(mesh, boss.x - 70.0f, boss.y - 30.0f, 140.0f,  50.0f, 0xFF00FFFF); // Reaching Arm
    DrawSquareMesh(mesh, boss.x + 70.0f, boss.y - 60.0f, 60.0f, 150.0f, 0xFF00FFFF); // Skirt/Legs
    DrawSquareMesh(mesh, boss.x + 90.0f, boss.y - 120.0f, 50.0f, 240.0f, 0xFF00FFFF); // Dangling Arm
    AEGfxSetTransparency(1.0f);
    

    // Get the current frame texture
    AEGfxTexture* currentTex = bossBulletFrames[currentBulletFrame];

    // Draw Boss Bullets
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            if (currentTex) {
                float angle = atan2f(boss.bullets[i].dirY, boss.bullets[i].dirX);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
                AEGfxSetTransparency(1.0f);
                AEGfxTextureSet(currentTex, 0.0f, 0.0f);

                AEMtx33 scale, rot, trans, temp, finalTransform;
                AEMtx33Scale(&scale, 50.0f, 50.0f);
                AEMtx33Rot(&rot, angle);
                AEMtx33Trans(&trans, boss.bullets[i].x, boss.bullets[i].y);

                AEMtx33Concat(&temp, &rot, &scale);
                AEMtx33Concat(&finalTransform, &trans, &temp);

                AEGfxSetTransform(finalTransform.m);
                AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
            }
            else {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                DrawSquareMesh(mesh, boss.bullets[i].x, boss.bullets[i].y, 50.0f, 50.0f, 0xFFFFFFFF);
            }
        }
    }
}

// --- NEW: COMPOUND COLLISION MESH CHECK ---
bool Boss_CheckCollision(Boss& boss, float objX, float objY, float objW, float objH)
{
    if (!boss.active) return false;

    // Define the 4 custom hitboxes that wrap tightly around the monster's drawing
    // 1. Head & Torso
    float h1X = boss.x + 20.0f, h1Y = boss.y + 80.0f, h1W = 100.0f, h1H = 240.0f;
    // 2. Reaching Arm (Left)
    float h2X = boss.x - 70.0f, h2Y = boss.y + 20.0f, h2W = 140.0f, h2H = 50.0f;
    // 3. Lower Body / Skirt
    float h3X = boss.x + 60.0f, h3Y = boss.y - 80.0f, h3W = 120.0f, h3H = 160.0f;
    // 4. Dangling Arm (Right)
    float h4X = boss.x + 90.0f, h4Y = boss.y - 120.0f, h4W = 50.0f, h4H = 240.0f;

    // AABB collision logic function
    auto checkAABB = [&](float x1, float y1, float w1, float h1) {
        return (fabsf(x1 - objX) < (w1 + objW) / 2.0f) &&
            (fabsf(y1 - objY) < (h1 + objH) / 2.0f);
        };

    // If the bullet touches ANY of these 4 boxes, it's a hit!
    return checkAABB(h1X, h1Y, h1W, h1H) ||
        checkAABB(h2X, h2Y, h2W, h2H) ||
        checkAABB(h3X, h3Y, h3W, h3H) ||
        checkAABB(h4X, h4Y, h4W, h4H);
}

void Boss_Unload()
{
    UnloadTextureSafe(bossTex);
    UnloadTextureSafe(bossBulletFrames[0]);
    UnloadTextureSafe(bossBulletFrames[1]);
}