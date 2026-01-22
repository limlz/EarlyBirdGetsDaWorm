#include "pch.hpp"

static AEGfxVertexList* squareMesh;
static AEGfxTexture* frames_arr[10] = { nullptr };
float bedLeft{};
float bedRight{};
float bedTop{};
float bedBot{};



void Lighting_Load() {

}

void Lighting_Initialize() {
	squareMesh = CreateSquareMesh(0xFFFFFFFF);
}

void Lighting_Update() {
	return;
}

void Lighting_Draw(f32 camX, s8 floorNum)
{
    // --- SETTINGS ---
    float lightRadius = 600.0f;   // Total range of the glow
    float lightCore = 40.0f;    // NEW: How wide the "fully bright" center is
    int maxDarkness = 0xDD;     // The darkness of the room
    int stepSize = 2;        // Optimization: Step 2 or 4 is usually plenty smooth

    // 1. DRAW DARKNESS OVERLAY
    for (int row = 0; row < SCREEN_W; row += stepSize)
    {
        float sliceScreenX = -SCREEN_WIDTH_HALF + row;
        float shortestDist = 10000.0f;

        // Find distance to NEAREST light
        for (int i = 0; i < 11; i++) {
            float lightScreenX = camX + (DIST_BETWEEN_DOORS * i) + (DIST_BETWEEN_DOORS / 2);
            float dist = fabsf(sliceScreenX - lightScreenX);
            if (dist < shortestDist) shortestDist = dist;
        }

        int currentAlpha = maxDarkness;

        // --- NEW LOGIC STARTS HERE ---

        // 1. Check if we are inside the "Core" (Fully Bright Area)
        if (shortestDist < lightCore)
        {
            currentAlpha = 0; // 0 Alpha = 100% Transparent (Bright)
        }
        // 2. Otherwise, check if we are in the "Fade" zone
        else if (shortestDist < lightRadius)
        {
            // We need to map the distance from [Core ... Radius] to [0.0 ... 1.0]
            float validRange = lightRadius - lightCore;
            float distFromCore = shortestDist - lightCore;

            // Calculate base linear ratio (0.0 near core, 1.0 near edge)
            float ratio = distFromCore / validRange;

            // 3. Make it NATURAL (Easing)
            // Squaring the ratio makes the gradient softer.
            // It pushes the darkness further away, making the light feel "volumetric".
            ratio = ratio * ratio;

            currentAlpha = (int)(maxDarkness * ratio);
        }
        // --- NEW LOGIC ENDS HERE ---

        // Safety Clamps
        if (currentAlpha > 255) currentAlpha = 255;
        if (currentAlpha < 0) currentAlpha = 0;

        u32 shadowColor = 0x00000000 | (currentAlpha & 0xFF);
        DrawSquareMesh(squareMesh, sliceScreenX, 0.0f, (float)stepSize, SCREEN_H, shadowColor);
    }

    // 2. DRAW PHYSICAL BULBS
    for (int i = 0; i < 11; i++) {
        float lightX = camX + (DIST_BETWEEN_DOORS * i) + (DIST_BETWEEN_DOORS / 2);
        if (lightX > -SCREEN_WIDTH_HALF - DOOR_WIDTH && lightX < SCREEN_WIDTH_HALF + DOOR_WIDTH) {
            DrawSquareMesh(squareMesh, lightX, 200.0f, 50.0f, 50.0f, 0xFFFFFFFF);
        }
    }
}

void DrawConeLight(float lightWorldX, float lightY, float camX, bool right_left)
{
    // ... Settings ...
    int numRays = 300;
    float coneAngle = 1.2f;
    float maxDist = 1000.0f;
    float floorLevel = -400.0f;
    float screenLightX = lightWorldX - camX;

    if (screenLightX < -900 || screenLightX > 900) return;

    float pLeft = -35.0f;
    float pRight = 20.0f; 

    // Keep vertical bounds accurate to the sprite size
    float pTop = -40.0f;
    float pBot = floorLevel;

    if (right_left) {
        bedLeft = 30.0f;
        bedRight = 145.0f;
        bedTop = -160.0f;
        bedBot = -200.0f;
    }
    else {
        bedLeft = -130.0f;
        bedRight = -30.0f;
        bedTop = -160.0f;
        bedBot = -200.0f;

    }


    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    float brightness = 0.1f;

    AEGfxSetBlendMode(AE_GFX_BM_ADD);
    AEGfxSetColorToMultiply(1.0f, 0.9f, 0.6f, brightness);
    AEGfxSetTransparency(brightness);

    for (int i = 0; i < numRays; i++)
    {
        float progress = (float)i / (float)(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);
        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        float currentDist = 0.0f;
        float step = 1.0f;

        while (currentDist < maxDist) {
            currentDist += step;
            float checkX = screenLightX - (dirX * currentDist);
            float checkY = lightY + (dirY * currentDist);
            if (checkY < floorLevel) break;
            if (checkX > pLeft && checkX < pRight && checkY < pTop && checkY > pBot) break;
            if (checkX > bedLeft && checkX < bedRight && checkY < bedTop && checkY > bedBot) break;
        }

        AEMtx33 scale, rot, trans, pivot, finalMtx;
        AEMtx33Scale(&scale, 15.0f, currentDist);
        AEMtx33Trans(&pivot, 0.0f, -currentDist / 2.0f);
        AEMtx33Rot(&rot, -angle);
        AEMtx33Trans(&trans, screenLightX, lightY);

        AEMtx33Concat(&finalMtx, &pivot, &scale);
        AEMtx33Concat(&finalMtx, &rot, &finalMtx);
        AEMtx33Concat(&finalMtx, &trans, &finalMtx);

        AEGfxSetTransform(finalMtx.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- RESET STATE ---
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);
}