#include "pch.hpp"

// --- INTERNAL STRUCTURE ---
struct Particle {
    float x, y;          // Position
    float velX, velY;    // Velocity
    float life;          // Life (1.0 -> 0.0)
    float scale;         // Size
    float r, g, b;       // Color
};

// --- GLOBAL LIST (Only visible in this file) ---
static std::vector<Particle> particleList;

// --- IMPLEMENTATION ---

void Particles_Initialize()
{
    particleList.clear();
}

void Particles_Spawn(float startX, float startY, int count)
{
    for (int i = 0; i < count; i++)
    {
        Particle p;
        p.x = startX;
        p.y = startY;

        // Physics: Explosion spread
        // X: Random spread left/right (-200 to +200)
        p.velX = (float)(rand() % 400 - 200);

        // Y: Shoot UP with variation (150 to 450)
        p.velY = (float)(rand() % 100 );

        p.life = 2.0f;
        p.scale = (float)(rand() % 5 + 2); // Size 2px to 7px

        // Color: Random variations of Orange/Yellow/Gold
        p.r = 1.0f;
        p.g = (rand() % 60 + 40) / 100.0f; // 0.4 to 1.0
        p.b = 0.1f;

        particleList.push_back(p);
    }
}

void Particles_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    for (int i = 0; i < particleList.size(); i++)
    {
        Particle& p = particleList[i];

        // 1. Gravity (Pull down)
        p.velY -= 1000.0f * dt;

        // 2. Air Resistance (Slow down X movement)
        p.velX *= 0.99f;

        // 3. Move
        p.x += p.velX * dt;
        p.y += p.velY * dt;

        // 4. Age (Die in roughly 0.6 seconds)
        p.life -= 1.5f * dt;

        // 5. Floor Bounce (Assuming floor is roughly -200)
        // You can tweak this number or remove it
        if (p.y < -250.0f) {
            p.y = -250.0f;
            p.velY = -p.velY * 0.5f; // Bounce with energy loss
        }

        // 6. Remove Dead
        if (p.life <= 0.0f) {
            particleList.erase(particleList.begin() + i);
            i--;
        }
    }
}

void Particles_Draw(AEGfxVertexList* mesh, float camX)
{
    // Safety check: Don't draw if mesh is null
    if (!mesh) return;

    // --- RENDER STATE ---
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_ADD); // ADDITIVE makes them glow

    for (const auto& p : particleList)
    {
        // 1. Calculate Screen Position (World - Camera)
        float screenX = p.x - camX;

        // Optimization: Don't draw if off-screen
        if (screenX < -900 || screenX > 900) continue;

        // 2. Set Color & Transparency
        AEGfxSetColorToMultiply(p.r, p.g, p.b, p.life);
        AEGfxSetTransparency(p.life);

        // 3. Transform
        AEMtx33 scale, trans, xform;
        AEMtx33Scale(&scale, p.scale, p.scale);
        AEMtx33Trans(&trans, screenX, p.y);
        AEMtx33Concat(&xform, &trans, &scale);

        AEGfxSetTransform(xform.m);
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- RESET STATE ---
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);
}

void Particles_Free()
{
    std::vector<Particle>().swap(particleList);
}