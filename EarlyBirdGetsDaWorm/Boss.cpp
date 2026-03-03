#include "pch.hpp"
#include "Boss.hpp"
#include <cmath> // Required for sinf(), cosf(), sqrtf(), atan2f()

// --- Boss Textures ---
static AEGfxTexture* bossBulletFrames[2] = { nullptr, nullptr };

// --- Bullet Animation Variables ---
static int currentBulletFrame = 0;
static float bulletAnimTimer = 0.0f;
const float BULLET_ANIM_SPEED = 0.1f; // How fast it swaps frames (0.1s is very rapid!)

// --- Define Max Health ---
const int BOSS_MAX_HEALTH = 500;

void Boss_Load()
{
    // Load the two frames of the bullet animation
    bossBulletFrames[0] = LoadTextureChecked("Assets/Boss/boss_bullet.png");
    bossBulletFrames[1] = LoadTextureChecked("Assets/Boss/boss_bullet2.png");
}

void Boss_Initialize(Boss& boss)
{
    // 1. Setup Basic Stats
    boss.active = true;
    boss.health = BOSS_MAX_HEALTH; // Changed to 10 hits max
    boss.w = 100.0f;
    boss.h = 100.0f;

    // 2. Position
    boss.x = 600.0f;
    boss.y = 100.0f;
    boss.baseY = 100.0f;

    // 3. AI Timers & States
    boss.moveTimer = 0.0f;
    boss.shootTimer = 2.0f;

    // NEW: Initialize AI States
    boss.currentState = 0;
    boss.stateTimer = 4.0f; // Start with 4 seconds of normal attacks
    boss.shieldActive = false;

    // 4. Clear Ammo
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        boss.bullets[i].active = false;
    }
}

void Boss_Update(Boss& boss, float dt, float playerX, float playerY)
{
    if (!boss.active) {
        next = GAME_STATE; // End boss fight
    }

    // --- BULLET ANIMATION UPDATE ---
    bulletAnimTimer += dt;
    if (bulletAnimTimer >= BULLET_ANIM_SPEED) {
        bulletAnimTimer = 0.0f;
        currentBulletFrame = (currentBulletFrame == 0) ? 1 : 0;
    }

    // --- HOVER MOVEMENT (Sine Wave) ---
    boss.moveTimer += dt * 2.0f;
    // Move faster when shielded
    float hoverSpeed = boss.shieldActive ? 4.0f : 2.0f;
    float hoverOffset = sinf(boss.moveTimer * hoverSpeed) * 150.0f;

    float targetY = boss.baseY + hoverOffset;
    boss.y += (targetY - boss.y) * 4.0f * dt;

    // --- AI STATE MACHINE ---
    boss.stateTimer -= dt;

    // Switch to a new random state when the timer runs out
    if (boss.stateTimer <= 0.0f) {
        boss.currentState = rand() % 3; // Randomly pick state 0, 1, or 2

        if (boss.currentState == 0) {
            boss.stateTimer = 4.0f; // 4s of Normal Attack
            boss.shieldActive = false;
        }
        else if (boss.currentState == 1) {
            boss.stateTimer = 3.0f; // 3s of Spread Attack
            boss.shieldActive = false;
        }
        else if (boss.currentState == 2) {
            boss.stateTimer = 3.5f; // 3.5s of Shield Defense
            boss.shieldActive = true;
            boss.shootTimer = 0.5f; // Reuse shoot timer to track healing!
        }
    }

    // --- SHOOTING & HEALING LOGIC (Depends on current state) ---
    boss.shootTimer -= dt;

    // State 0: Normal Targeted Shot
    if (boss.currentState == 0 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 1.0f; // Shoot every 1.0s

        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (!boss.bullets[i].active) {
                boss.bullets[i].active = true;
                boss.bullets[i].hp = 10;
                boss.bullets[i].x = boss.x;
                boss.bullets[i].y = boss.y;

                float diffX = playerX - boss.x;
                float diffY = playerY - boss.y;
                float length = sqrtf(diffX * diffX + diffY * diffY);

                if (length != 0) {
                    boss.bullets[i].dirX = (diffX / length) * 600.0f;
                    boss.bullets[i].dirY = (diffY / length) * 600.0f;
                }
                break;
            }
        }
    }
    // State 1: Spread Attack (Shotgun)
    else if (boss.currentState == 1 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 1.5f; // Shoot every 1.5s

        // Find base angle to player
        float diffX = playerX - boss.x;
        float diffY = playerY - boss.y;
        float baseAngle = atan2f(diffY, diffX);

        int bulletsFired = 0;
        // Fire 5 bullets in an arc (-2, -1, 0, 1, 2)
        for (int i = 0; i < MAX_BOSS_BULLETS && bulletsFired < 5; i++) {
            if (!boss.bullets[i].active) {
                boss.bullets[i].active = true;
                boss.bullets[i].hp = 5;
                boss.bullets[i].x = boss.x;
                boss.bullets[i].y = boss.y;

                // Offset the angle by 0.2 radians for each bullet
                float spreadAngle = baseAngle + ((bulletsFired - 2) * 0.2f);

                boss.bullets[i].dirX = cosf(spreadAngle) * 500.0f;
                boss.bullets[i].dirY = sinf(spreadAngle) * 500.0f;

                bulletsFired++;
            }
        }
    }
    // State 2: Shield Phase (Healing over time)
    else if (boss.currentState == 2 && boss.shootTimer <= 0.0f) {
        boss.shootTimer = 0.5f; // Heal 1 HP every 0.5 seconds
        if (boss.health < BOSS_MAX_HEALTH) {
            boss.health += 1; // Regenerate health!
        }
    }

    // --- UPDATE BOSS BULLETS ---
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            boss.bullets[i].x += boss.bullets[i].dirX * dt;
            boss.bullets[i].y += boss.bullets[i].dirY * dt;

            // Despawn if off screen to save processing
            if (boss.bullets[i].x < -1000 || boss.bullets[i].x > 1000 || boss.bullets[i].y < -1000 || boss.bullets[i].y > 1000) {
                boss.bullets[i].active = false;
            }
        }
    }
}

// --- NEW UI FUNCTION: Boss Health Bar ---
void Boss_DrawHealthBar(Boss& boss, AEGfxVertexList* mesh)
{
    if (!boss.active) return;

    // Calculate health percentage
    float hpPercent = (float)boss.health / (float)BOSS_MAX_HEALTH;
    if (hpPercent < 0.0f) hpPercent = 0.0f;

    // Bar dimensions and location (Top Right)
    float barWidth = 400.0f;
    float barHeight = 25.0f; 
    float startX = 500.0f; // Shifted right
    float startY = 400.0f; // Near top of the screen

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // 1. Draw Background (Dark Red / Black)
    DrawSquareMesh(mesh, startX, startY, barWidth, barHeight, 0xFF4400FF);

    // 2. Draw Foreground (Bright Red) based on current health
    if (boss.health > 0) {
        float currentWidth = barWidth * hpPercent;
        // Adjust X so the bar empties from right to left properly
        float fillX = startX - (barWidth / 2.0f) + (currentWidth / 2.0f);
        DrawSquareMesh(mesh, fillX, startY, currentWidth, barHeight, 0xFFFF00FF);
    }

    // Optional: If shielded, draw a subtle blue glow over the health bar to indicate healing
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
        // Draw a larger, semi-transparent blue box behind the boss
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.5f); // 50% opacity

        // Slightly bigger than the boss (140x140)
        DrawSquareMesh(mesh, boss.x, boss.y, 140.0f, 140.0f, 0xFF0088FF);
        AEGfxSetTransparency(1.0f); // Reset transparency
    }

    // Draw Boss (Magenta Square)
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    DrawSquareMesh(mesh, boss.x, boss.y, boss.w, boss.h, 0xFFFF00FF);

    // Get the current frame texture
    AEGfxTexture* currentTex = bossBulletFrames[currentBulletFrame];

    // Draw Boss Bullets (Textured & Rotated)
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            if (currentTex) {
                float angle = atan2f(boss.bullets[i].dirY, boss.bullets[i].dirX);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f); // Prevents white-box bug
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
                // Fallback square 
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                DrawSquareMesh(mesh, boss.bullets[i].x, boss.bullets[i].y, 50.0f, 50.0f, 0xFFFFFFFF);
            }
        }
    }
}

void Boss_Unload()
{
    // Clean up textures to prevent memory leaks
    UnloadTextureSafe(bossBulletFrames[0]);
    UnloadTextureSafe(bossBulletFrames[1]);
}