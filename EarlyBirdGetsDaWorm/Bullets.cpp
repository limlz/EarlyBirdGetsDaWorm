#include "pch.hpp"
#include <cmath> // Required for atan2f (rotation math)

struct Bullet {
    float x, y;
    float dirX, dirY; // The normalized direction vector
    float speed;
    bool active;
};

static AEGfxVertexList* squareMesh;

#define MAX_BULLETS 20
std::vector<Bullet> bullets(MAX_BULLETS);

void Bullets_Initialize() {
    squareMesh = CreateSquareMesh(0xFFFFFFFF);
    bullets.assign(MAX_BULLETS, Bullet{});
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
}

// Updated to take dirX and dirY instead of facingRight
void Fire_Bullet(float startX, float startY, float dirX, float dirY) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].x = startX;
            bullets[i].y = startY;
            bullets[i].dirX = dirX; // Save the exact angle vector
            bullets[i].dirY = dirY;
            bullets[i].speed = 1200.0f; // Increased speed for mouse aiming (feels better!)
            break; // Fire only one bullet per click
        }
    }
}

void Bullets_Update(float dt, float camX, Boss& myBoss)
{
    float bulletW = 20.0f;
    float bulletH = 10.0f;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {

            // 1. Move Bullet along the vector
            bullets[i].x += bullets[i].dirX * bullets[i].speed * dt;
            bullets[i].y += bullets[i].dirY * bullets[i].speed * dt;

            // 2. Culling (Off-screen)
            // Adjusted to check both X and Y axis since we can shoot up/down now
            float screenPosX = bullets[i].x + camX;
            float screenPosY = bullets[i].y;

            if (screenPosX < -1000.0f || screenPosX > 1000.0f ||
                screenPosY < -1000.0f || screenPosY > 1000.0f)
            {
                bullets[i].active = false;
                continue;
            }

            // 3. COLLISION CHECK (Damage)
            if (myBoss.active) {
                if (IsColliding(bullets[i].x, bullets[i].y, bulletW, bulletH,
                    myBoss.x, myBoss.y, myBoss.w, myBoss.h))
                {
                    myBoss.health -= 20;
                    bullets[i].active = false;

                    std::cout << "Boss Hit! HP: " << myBoss.health << "\n";

                    if (myBoss.health <= 0) {
                        myBoss.active = false;
                        std::cout << "BOSS DEFEATED!\n";
                    }
                }
            }
        }
    }
}

void Bullets_Draw(float camX) {
    float w = 20.0f;
    float h = 10.0f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {

            // Calculate the rotation angle based on the direction vector
            // atan2f(y, x) returns the angle in radians exactly pointing to the target
            float angle = atan2f(bullets[i].dirY, bullets[i].dirX);

            // Build TRS matrix to draw the rotated bullet
            AEMtx33 scale, rot, trans, rotScale, finalTransform;
            AEMtx33Scale(&scale, w, h);
            AEMtx33Rot(&rot, angle);
            AEMtx33Trans(&trans, bullets[i].x, bullets[i].y);

            AEMtx33Concat(&rotScale, &rot, &scale);
            AEMtx33Concat(&finalTransform, &trans, &rotScale);

            AEGfxSetTransform(finalTransform.m);

            // Draw the rotated bullet (White)
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Reset transform when done drawing bullets
    AEMtx33 finalTransform;
    AEMtx33Identity(&finalTransform);
    AEGfxSetTransform(finalTransform.m);
}

void Bullets_Free() {
    FreeMeshSafe(squareMesh);
    std::vector<Bullet>().swap(bullets);
}