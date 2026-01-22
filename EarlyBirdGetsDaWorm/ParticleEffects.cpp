#include "pch.hpp"

// --- 1. SPAWN SPARKS ---
void Particle_Create(float startX, float startY, int count)
{
    for (int i = 0; i < count; i++)
    {
        Particle p;
        p.x = startX;
        p.y = startY;

        // Random Velocity for "Explosion" effect
        // X: Spread left/right (-200 to +200)
        p.velX = (float)(rand() % 400 - 200);

        // Y: Shoot UP (-100 to +400), acting against gravity
        p.velY = (float)(rand() % 400 + 100);

        p.life = 1.0f; // Lives for roughly 1 second
        p.scale = (float)(rand() % 4 + 2); // Random size 2px - 6px

        // Random Yellow/Orange Colors
        p.r = 1.0f;
        p.g = (rand() % 100) / 100.0f + 0.5f; // Random 0.5 to 1.0 (Yellow to Orange)
        p.b = 0.0f;

        g_ParticleSystem.push_back(p);
    }
}

// --- 2. UPDATE PHYSICS ---
void Particle_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    for (int i = 0; i < g_ParticleSystem.size(); i++)
    {
        Particle& p = g_ParticleSystem[i];

        // Apply Gravity
        p.velY -= 1200.0f * dt;

        // Apply Velocity
        p.x += p.velX * dt;
        p.y += p.velY * dt;

        // Reduce Life
        p.life -= 1.5f * dt; // Die fast

        // Remove dead particles
        if (p.life <= 0.0f) {
            g_ParticleSystem.erase(g_ParticleSystem.begin() + i);
            i--; // Adjust index since we removed an element
        }
    }
}

// --- 3. DRAW ---
void Particle_Draw(AEGfxVertexList* mesh, float camX)
{
    // Important: Use ADDITIVE blending for "Fire/Light" look
    AEGfxSetBlendMode(AE_GFX_BM_ADD);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    for (const auto& p : g_ParticleSystem)
    {
        // Convert World Pos to Screen Pos
        float screenX = p.x - camX; // Apply Camera Offset

        // Optimization: Don't draw if off screen
        if (screenX < -900 || screenX > 900) continue;

        // Fade out as life decreases
        AEGfxSetColorToMultiply(p.r, p.g, p.b, p.life);
        AEGfxSetTransparency(p.life);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, p.scale, p.scale);
        AEMtx33Trans(&trans, screenX, p.y);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }

    // Reset State
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);
}