#include "pch.hpp"
#include <cmath> 

struct Bullet {
    float x, y;
    float dirX, dirY;
    float speed;
    float size;
    bool active;
};

static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;
static s8 ammoFontId = -1;
// Allow more water droplets on the screen at once
static constexpr int MAX_BULLETS = 300;
std::vector<Bullet> bullets(MAX_BULLETS);

// Gives the player a larger "Water Tank" before needing to reload
const int MAX_AMMO = 150; 
static int currentAmmo = MAX_AMMO;
static bool isReloading = false;
static float reloadTimer = 0.0f;
const float RELOAD_TIME = 1.5f; // Takes 1.5 seconds to reload

void Bullets_Initialize() {
    squareMesh = CreateSquareMesh(0xFFFFFFFF);
    circleMesh = CreateCircleMesh(0.5f, 16, 0xFFFFFFFF);
    if (ammoFontId >= 0) { AEGfxDestroyFont(ammoFontId); }
    ammoFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 35);
    bullets.assign(MAX_BULLETS, Bullet{});
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    // Reset Ammo on initialization
    currentAmmo = MAX_AMMO;
    isReloading = false;
    reloadTimer = 0.0f;
}

void Fire_Bullet(float startX, float startY, float dirX, float dirY) {
    // --- NEW: AMMO CHECK ---
    if (isReloading || currentAmmo <= 0) {
        return; // Gun goes *click*, cannot shoot!
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].x = startX;
            bullets[i].y = startY;
            bullets[i].dirX = dirX;
            bullets[i].dirY = dirY;
            bullets[i].speed = 1200.0f;
            bullets[i].size = 8.0f + RandInt(0, 799) / 100.0f;

			// Consume ammo for each shot fired
            currentAmmo--;
            if (currentAmmo <= 0) {
                isReloading = true;
                reloadTimer = RELOAD_TIME;
            }
            break;
        }
    }
}

void Bullets_Update(float dt, float camX, Boss& myBoss)
{
    float bulletW = 20.0f;
    float bulletH = 10.0f;

    // --- NEW: RELOAD TIMER LOGIC ---
    if (isReloading) {
        reloadTimer -= dt;
        if (reloadTimer <= 0.0f) {
            isReloading = false;
            currentAmmo = MAX_AMMO; // Reload complete!
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {

            // 1. Move Bullet
            bullets[i].x += bullets[i].dirX * bullets[i].speed * dt;
            bullets[i].y += bullets[i].dirY * bullets[i].speed * dt;

            // GRAVITAY
            bullets[i].dirY -= 1.0f * dt;

            // 2. Culling
            float screenPosX = bullets[i].x + camX;
            float screenPosY = bullets[i].y;

            if (screenPosX < -1000.0f || screenPosX > 1000.0f ||
                screenPosY < -1000.0f || screenPosY > 1000.0f)
            {
                bullets[i].active = false;
                continue;
            }

            // --- 3. NEW: NEUTRALIZE BOSS BULLETS ---
            bool hitBossBullet = false;
            if (myBoss.active) {
                for (int j = 0; j < MAX_BOSS_BULLETS; j++) {
                    if (myBoss.bullets[j].active) {

                        // Check collision against the 50x50 Boss Bullet
                        if (IsColliding(bullets[i].x, bullets[i].y, bulletW, bulletH,
                            myBoss.bullets[j].x, myBoss.bullets[j].y, 50.0f, 50.0f))
                        {
                            hitBossBullet = true;
                            bullets[i].active = false; // Destroy Player's bullet

                            myBoss.bullets[j].hp -= 1; // Damage Boss's bullet

                            if (myBoss.bullets[j].hp <= 0) {
                                myBoss.bullets[j].active = false; // Neutralized!
                            }
                            break; // Stop checking other boss bullets for this specific player bullet
                        }
                    }
                }
            }

            // If we hit a boss bullet, stop the code here so we don't ALSO hit the boss
            if (hitBossBullet) {
                continue;
            }

            // 4. COLLISION CHECK (Player Bullet vs Boss)
            if (myBoss.active) {
                if (Boss_CheckCollision(myBoss, bullets[i].x, bullets[i].y, bulletW, bulletH)) {
                    if (myBoss.shieldActive) {
                        // Ricochet Logic
                        float bounceX = bullets[i].x - myBoss.x;
                        float bounceY = bullets[i].y - myBoss.y;

                        float length = sqrtf((bounceX * bounceX) + (bounceY * bounceY));
                        if (length > 0.0001f) {
                            bullets[i].dirX = bounceX / length;
                            bullets[i].dirY = bounceY / length;
                        }
                        else {
                            bullets[i].dirX = -bullets[i].dirX;
                            bullets[i].dirY = -bullets[i].dirY;
                        }

                        bullets[i].x += bullets[i].dirX * 25.0f;
                        bullets[i].y += bullets[i].dirY * 25.0f;
                        bullets[i].speed = 1800.0f;
                    }
                    else {
                        myBoss.health -= 5;
                        bullets[i].active = false;

                        // Roll dodge chance on actual hit, not on bullet fire
                        Boss_Fight_TryDodge();

                        if (myBoss.health <= 0) {
                            myBoss.active = false;
                        }
                    }
                }
            }
        }
    }
}

void Bullets_Draw(float camX) {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            // 1. Calculate the exact angle the water is traveling
            float angle = atan2f(bullets[i].dirY, bullets[i].dirX);

            AEMtx33 scale, rot, trans, rotScale, finalTransform;

            // 2. VELOCITY STRETCHING
            float stretchLength = bullets[i].size * 4.5f;
            // Pinch the thickness slightly so it looks like a high-pressure jet
            float streamThickness = bullets[i].size * 0.7f;

            AEMtx33Scale(&scale, stretchLength, streamThickness);
            AEMtx33Rot(&rot, angle); // Rotate the stretched oval to match its travel arc
            AEMtx33Trans(&trans, bullets[i].x, bullets[i].y);

            AEMtx33Concat(&rotScale, &rot, &scale);
            AEMtx33Concat(&finalTransform, &trans, &rotScale);

            AEGfxSetTransform(finalTransform.m);

            // 3. Holy Water Color (Lower alpha to 0.4f because the overlapped stretched shapes will compound)
            AEGfxSetColorToMultiply(0.1f, 0.9f, 1.0f, 0.4f);

            // Draw using the circle mesh (a stretched circle naturally becomes an oval/capsule!)
            AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Reset Transform & Color
    AEMtx33 finalTransform;
    AEMtx33Identity(&finalTransform);
    AEGfxSetTransform(finalTransform.m);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
}

// Ammo UI — horizontal glass-cylinder bar, bottom-right.
// Fill drains left-to-right as shots are fired (smooth spray = smooth drain).
// Cyan = holy water active; orange = reloading.
void Bullets_DrawAmmoUI()
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // BOTTOM RIGHT — mirrors the HP bar position on the right side
    const float barCenterX =  535.0f;  // right edge aligns with boss health bar right edge (675)
    const float barCenterY = -390.0f;
    const float barW       = 280.0f;
    const float barH       =  18.0f;
    const float pad        =   3.0f;
    const float barLeft    = barCenterX - barW * 0.5f;
    const float barRight   = barCenterX + barW * 0.5f + pad;

    // Outer dark border
    DrawSquareMesh(squareMesh, barCenterX, barCenterY, barW + pad * 2.0f, barH + pad * 2.0f, 0xFF111111);
    // Dark glass-tinted background
    DrawSquareMesh(squareMesh, barCenterX, barCenterY, barW, barH, 0xFF021010);

    // Pressure-gauge tick marks at 25%, 50%, 75% (vertical notches, extend 4 px above/below)
    AEGfxSetTransparency(0.45f);
    for (int i = 1; i <= 3; i++) {
        float tickX = barLeft + barW * (i * 0.25f);
        DrawSquareMesh(squareMesh, tickX, barCenterY, 2.0f, barH + 8.0f, 0xFF00CCCC);
    }
    AEGfxSetTransparency(1.0f);

    if (isReloading) {
        // Dark navy reload-progress bar grows left-to-right
        float reloadProgress = (RELOAD_TIME - reloadTimer) / RELOAD_TIME;
        float fillW  = barW * reloadProgress;
        float fillCX = barLeft + fillW * 0.5f;
        DrawSquareMesh(squareMesh, fillCX, barCenterY, fillW, barH, 0x0A1A44FF);

        // Gloss stripe on the top edge
        float hiH = barH * 0.22f;
        float hiY = barCenterY + (barH - hiH) * 0.5f;
        AEGfxSetTransparency(0.3f);
        DrawSquareMesh(squareMesh, fillCX, hiY, fillW, hiH, 0x112255FF);
        AEGfxSetTransparency(1.0f);
    }
    else {
        // Glowing cyan holy-water fill — drains smoothly from the right as shots are fired
        float ammoPercent = static_cast<float>(currentAmmo) / static_cast<float>(MAX_AMMO);
        float fillW  = barW * ammoPercent;
        float fillCX = barLeft + fillW * 0.5f;

        if (currentAmmo > 0) {
            DrawSquareMesh(squareMesh, fillCX, barCenterY, fillW, barH, 0x00FFFFFF);

            // Glass-tube gloss stripe on the top edge
            float hiH = barH * 0.22f;
            float hiY = barCenterY + (barH - hiH) * 0.5f;
            AEGfxSetTransparency(0.35f);
            DrawSquareMesh(squareMesh, fillCX, hiY, fillW, hiH, 0xAAFFFFFF);
            AEGfxSetTransparency(1.0f);
        }
    }

    // "H2O" label to the right of the bar — mirrors the HP label style
    if (ammoFontId >= 0) {
        float ndcX = (barRight + 10.0f) / 800.0f;
        float ndcY = (barCenterY - 7.0f) / 450.0f;
        AEGfxPrint(ammoFontId, "H2O", ndcX, ndcY, 0.50f,
                   0.0f, 0.75f, 0.75f, 1.0f);  // cyan, matching the ammo bar fill
    }
}

bool Bullets_HasAmmo() {
    return currentAmmo > 0 && !isReloading;
}

void Bullets_Free() {
    FreeMeshSafe(squareMesh);
    // Clear and reset to default state instead of deallocating — prevents OOB
    // if anything touches bullets[] before the next Bullets_Initialize()
    bullets.assign(MAX_BULLETS, Bullet{});
    if (ammoFontId >= 0) { AEGfxDestroyFont(ammoFontId); ammoFontId = -1; }
}