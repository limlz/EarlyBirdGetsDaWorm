#include "pch.hpp"

// 1. Define a struct to hold the memory for each light
struct FlickerData {
    float timer;      // How much longer to stay in the current state
    bool isVisible;   // Is it currently ON or OFF?
};

// 2. Create a grid of these structs (Parallel to your lightingMatrix)
static std::array<std::array<FlickerData, 11>, 10> flickerMem;

static AEGfxVertexList* squareMesh;
static AEGfxTexture* frames_arr[10] = { nullptr };
std::array<std::array<int, 11>, 10> lightingMatrix;

void Lighting_Load() {
    // Load textures if needed
}

void Lighting_Initialize(int fucked_floor) {
    Particles_Initialize();

    // Loop through every floor
    for (int f = 0; f < 10; f++) {
        // Loop through every light
        for (int l = 0; l < 11; l++) {
            if (f == 0) {
                lightingMatrix[f][l] = 2; // Floor 0 starts with flickering lights
                continue;
            }
            if (f == fucked_floor) {
                if ((rand() % 100) > 60) {
                    lightingMatrix[f][l] = 2;
                }
                else {
                    lightingMatrix[f][l] = 1;
                }
                continue;
            }
            if ((rand() % 100) > 70) {
                lightingMatrix[f][l] = 2;
            }
            else {
                lightingMatrix[f][l] = 1;
            }
            continue;
        }
    }
    squareMesh = CreateSquareMesh(0xFFFFFFFF);
}

// Change logic to accept Camera Position and Dementia Flag
void Lighting_Update(s8 floorNum, float camX, bool dementia)
{
    Particles_Update();

    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Loop through the BASE 11 lights (the memory templates)
    for (int i = 0; i < 11; i++) {

        // Only calculate logic if this light is set to mode '2' (Flickering)
        if (lightingMatrix[floorNum][i] == 2)
        {
            // Count down the timer
            flickerMem[floorNum][i].timer -= dt;

            // If timer runs out, Switch States!
            if (flickerMem[floorNum][i].timer <= 0.0f)
            {
                // Toggle visibility
                flickerMem[floorNum][i].isVisible = !flickerMem[floorNum][i].isVisible;

                // --- SPARK SPAWN LOGIC FIX ---
                // Whenever the light switches state, spawn sparks.

                float lightWx = i * 600.0f + 300.0f; // Base position (0-10)
                float lightWy = 250.0f;

                if (dementia) {
                    // FIX: If we are in the infinite hallway, the player might be at x=50,000.
                    // We need to spawn the spark at the "Copy" of this light closest to the camera.

                    // The pattern repeats every 10 lights (Indices 0-9 repeat). 
                    // Distance = 10 lights * 600 spacing = 6000.0f.
                    float repeatDist = 10 * 600.0f;

                    // Calculate the offset 'k' to bring the light close to -camX (Camera Center)
                    // roundf finds the nearest integer multiple of 6000.
                    float k = roundf((-camX - lightWx) / repeatDist);

                    // Apply the offset so the spark spawns on screen
                    lightWx += k * repeatDist;
                }

                // Spawn sparks at the calculated position
                Particles_Spawn(lightWx, lightWy, 8);
                // -----------------------------

                // Set a NEW random duration based on state
                if (flickerMem[floorNum][i].isVisible) {
                    flickerMem[floorNum][i].timer = (float)((rand() % 140) + 10) / 100.0f;
                }
                else {
                    flickerMem[floorNum][i].timer = (float)((rand() % 25) + 5) / 100.0f;
                }
            }
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

    if (screenLightX < -1800 || screenLightX > 1800) return;

    float pHeadLeft{};
    float pHeadRight{};

    float pBodyLeft{};
    float pBodyRight{};


    float bedLeft{};
    float bedRight{};
    float bedTop{};
    float bedBot = -200.0f;

    // Keep vertical bounds accurate to the sprite size
    float pHeadTop = -60.0f;
    float pHeadBot = -105.0f;

    if (!Player_HasPatient()) {
        pHeadLeft = 30.0f;
        pHeadRight = 70.0f;
        pBodyLeft = 35.0f;
        pBodyRight = 65.0f;
    }
    else if (right_left) {
        pHeadLeft = -45.0f;
        pHeadRight = 5.0f;
        pBodyLeft = -35.0f;
        pBodyRight = -5.0f;
        bedLeft = 30.0f;
        bedRight = 135.0f;
        bedTop = -150.0f;
    }
    else {
        pHeadLeft = 95.0f;
        pHeadRight = 145.0f;
        pBodyLeft = 105.0f;
        pBodyRight = 135.0f;
        bedLeft = -30.0f;
        bedRight = 70.0f;
        bedTop = -150.0f;
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
            if (checkX > pHeadLeft && checkX < pHeadRight && checkY < pHeadTop && checkY > pHeadBot) break;
            if (checkX > pBodyLeft && checkX < pBodyRight && checkY < pHeadTop && checkY > bedBot) break;
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

void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum, bool dementia)
{
    if (floorNum < 0 || floorNum >= 10) return;

    int max_floor = dementia ? 1000 : 11;

    for (int i = 0; i < max_floor; i++)
    {
        float lightWx = i * 600.0f + 300.0f;

        // --- CULLING FIX ---
        // Convert World Position to Screen Position.
        // Since camX is a negative offset (e.g., -1000), we ADD it.
        float screenPos = lightWx + camX;

        // Check if the light is within the screen bounds (plus some padding)
        if (screenPos < -1200.0f || screenPos > 1200.0f) {
            continue;
        }

        // --- STATE & VISIBILITY ---
        int state = 0;
        bool shouldDraw = false;

        if (dementia) {
            state = lightingMatrix[floorNum][i % 10];

            if (state == 1) shouldDraw = true;
            else if (state == 2) shouldDraw = flickerMem[floorNum][i % 10].isVisible;

        }
        else {
            state = lightingMatrix[floorNum][i];

            if (state == 1) shouldDraw = true;
            else if (state == 2) shouldDraw = flickerMem[floorNum][i].isVisible;
        }

        // --- DRAW ---
        if (state != 0 && shouldDraw) {
            // Note: We still pass -camX here because DrawConeLight likely expects 
            // the positive Camera Position to perform the subtraction internally.
            DrawConeLight(lightWx, 250.0f, -camX, left_right);
        }
    }

    Particles_Draw(squareMesh, -camX);
}

void Lighting_Unload() {
    FreeMeshSafe(squareMesh);
    Particles_Free();
}