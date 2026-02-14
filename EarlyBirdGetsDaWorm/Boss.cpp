#include "pch.hpp"
#include "Boss.hpp"
#include <cmath> // Required for sinf(), sqrtf()

void Boss_Initialize(Boss& boss)
{
    // 1. Setup Basic Stats
    boss.active = true;
    boss.health = 100;
    boss.w = 100.0f;
    boss.h = 100.0f;

    // 2. Position (Start in the air for hovering)
    boss.x = 600.0f;
    boss.y = 100.0f;
    boss.baseY = 100.0f;

    // 3. AI Timers
    boss.moveTimer = 0.0f;
    boss.shootTimer = 2.0f; // Wait 2 seconds before first shot

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

    // --- HOVER MOVEMENT (Sine Wave) ---
    // This creates a smooth up/down motion
    boss.moveTimer += dt * 2.0f;
    float hoverOffset = sinf(boss.moveTimer) * 100.0f; // 100px range

    // Smoothly move towards target Y (Logic for dodging)
    float targetY = boss.baseY + hoverOffset;
    boss.y += (targetY - boss.y) * 4.0f * dt;

    // --- SHOOTING LOGIC ---
    boss.shootTimer -= dt;
    if (boss.shootTimer <= 0.0f) {
        boss.shootTimer = 1.2f; // Reset cooldown (Shoot every 1.2s)

        // Find an inactive bullet to fire
        for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
            if (!boss.bullets[i].active) {
                boss.bullets[i].active = true;
                boss.bullets[i].x = boss.x;
                boss.bullets[i].y = boss.y;

                // Math: Aim at Player
                float diffX = playerX - boss.x;
                float diffY = playerY - boss.y;
                float length = sqrtf(diffX * diffX + diffY * diffY);

                // Normalize vector and multiply by speed
                if (length != 0) {
                    boss.bullets[i].dirX = (diffX / length) * 600.0f;
                    boss.bullets[i].dirY = (diffY / length) * 600.0f;
                }
                break; // Fire only one bullet per timer tick
            }
        }
    }

    // --- UPDATE BOSS BULLETS ---
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            boss.bullets[i].x += boss.bullets[i].dirX * dt;
            boss.bullets[i].y += boss.bullets[i].dirY * dt;

            // Despawn if off screen to save processing
            if (boss.bullets[i].x < -1000 || boss.bullets[i].x > 1000) {
                boss.bullets[i].active = false;
            }
        }
    }
}

void Boss_Draw(Boss& boss, AEGfxVertexList* mesh)
{
    if (!boss.active) return;

    // Draw Boss (Red)
    DrawSquareMesh(mesh, boss.x, boss.y, boss.w, boss.h, 0xFFFF00FF);

    // Draw Boss Bullets (Yellow)
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (boss.bullets[i].active) {
            DrawSquareMesh(mesh, boss.bullets[i].x, boss.bullets[i].y, 25.0f, 25.0f, 0xFFFFFFFF);
        }
    }
}