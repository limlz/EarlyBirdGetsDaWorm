#include "pch.hpp"

struct Bullet {
    float x, y;
    float speed;
    bool active;
    bool direction; // 0 = Left, 1 = Right
};

static AEGfxVertexList* squareMesh;

#define MAX_BULLETS 20
std::vector<Bullet> bullets(MAX_BULLETS);

void Bullets_Initialize() {
    squareMesh = CreateSquareMesh(0xFFFFFFFF);
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
}

void Fire_Bullet(float startX, float startY, bool facingRight) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].x = startX;
            bullets[i].y = startY;
            bullets[i].direction = facingRight;
            bullets[i].speed = 800.0f; // Adjust speed as needed
            break; // Fire only one bullet per key press
        }
    }
}

// Update this function signature to accept the Boss pointer (or use global)
void Bullets_Update(float dt, float camX, Boss myBoss)
{
    float bulletW = 20.0f;
    float bulletH = 10.0f;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {

            // 1. Move Bullet
            if (bullets[i].direction) bullets[i].x += bullets[i].speed * dt;
            else bullets[i].x -= bullets[i].speed * dt;

            // 2. Culling (Off-screen)
            float screenPos = bullets[i].x + camX;
            if (screenPos < -900.0f || screenPos > 900.0f) {
                bullets[i].active = false;
                continue; // Skip collision if killed
            }

            // 3. COLLISION CHECK (Damage)
            // Only check if Boss is alive
            if (myBoss.active) {
                if (IsColliding(bullets[i].x, bullets[i].y, bulletW, bulletH,
                    myBoss.x, myBoss.y, myBoss.w, myBoss.h))
                {
                    // HIT CONFIRMED!
                    myBoss.health -= 20;       // Deal Damage
                    bullets[i].active = false; // Destroy Bullet

                    std::cout << "Boss Hit! HP: " << myBoss.health << "\n";

                    // Check Boss Death
                    if (myBoss.health <= 0) {
                        myBoss.active = false;
                        std::cout << "BOSS DEFEATED!\n";
                        // Play sound or transition state here
                    }
                }
            }
        }
    }
}

void Bullets_Draw(float camX) {
    // Define bullet size
    float w = 20.0f;
    float h = 10.0f;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            // Draw bullet (Yellow)
            // Note: Pass coordinates relative to the world
            DrawSquareMesh(squareMesh, bullets[i].x, bullets[i].y, w, h, 0xFFFFFFFF);
        }
    }
}