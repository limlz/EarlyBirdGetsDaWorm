#include "pch.hpp"
#include "Config.hpp" 
#include <cmath>
#include <array>

// ============================================================
//  LOCAL STATE & CONSTANTS
// ============================================================

struct FlickerData
{
    float timer = 0.0f;
    bool  isVisible = true;
};

// Grid memory for every light
static std::array<std::array<FlickerData, 11>, 10> flickerMem;
std::array<std::array<int, 11>, 10> lightingMatrix; // 0 = off, 1 = on, 2 = flickering

static AEGfxVertexList* squareMesh = nullptr;

static float singleLightTimer = 0.0f;
static bool  singleLightIsOn = true;
static float g_TotalTime = 0.0f;

// ============================================================
//  GHOST HELPERS
// ============================================================

// Use mimic illness whenever current illness is GHOST.
static ILLNESSES GetEffectiveIllness()
{
    ILLNESSES current = Player_GetCurrentIllness();
    if (current == ILLNESSES::GHOST)
        return Player_GetMimicIllness();

    return current;
}

// +1 anomaly count (1 for ghost, 0 otherwise)
static int GetGhostExtraCount()
{
    if (Player_GetCurrentIllness() == ILLNESSES::GHOST)
        return Player_GetGhostExtraAnomalies();
    return 0;
}

// ============================================================
//  HELPERS
// ============================================================

static inline float FlickerDuration(ILLNESSES effIllness, bool isOn, int ghostExtra)
{
    int idx = static_cast<int>(effIllness);
    if (idx < 0 || idx > 10) idx = 1; // Safety bound check (defaults to PARANOIA)

    // --- HOT-RELOADED DATA FETCH ---
    const FlickerTimes& t = Config::flickerTable[idx];

    int range = isOn ? t.onRange : t.offRange;
    int minv = isOn ? t.onMin : t.offMin;

    if (range <= 0) range = 1;

    float seconds = (float)((rand() % range) + minv) / 100.0f;

    // GHOST +1 anomaly: Make flicker slightly more unstable
    if (ghostExtra >= 1)
    {
        seconds *= isOn ? 0.90f : 0.85f;
        if (seconds < 0.03f) seconds = 0.03f;
    }

    return seconds;
}

static void DrawRay(float screenLightX, float lightY, float angle, float dist, AEGfxVertexList* mesh)
{
    AEMtx33 scale, rot, trans, pivot, final;
    AEMtx33Scale(&scale, 15.0f, dist);          // quad as long as the ray
    AEMtx33Trans(&pivot, 0.0f, -dist / 2.0f);    // anchor from top instead of center
    AEMtx33Rot(&rot, -angle);
    AEMtx33Trans(&trans, screenLightX, lightY);

    AEMtx33Concat(&final, &pivot, &scale);
    AEMtx33Concat(&final, &rot, &final);
    AEMtx33Concat(&final, &trans, &final);

    AEGfxSetTransform(final.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// ============================================================
//  STANDALONE LIGHT
// ============================================================

void Update_StandaloneLight(float dt, float lightX, float lightY)
{
    singleLightTimer -= dt;
    if (singleLightTimer > 0.0f) return;

    AudioManager_PlaySFX(SFX_LIGHT_FLICKER, 0.15f, 1.0f, -1);

    singleLightIsOn = !singleLightIsOn;

    Particles_Spawn(lightX, lightY, 8);

    if (singleLightIsOn)
        singleLightTimer = (float)((rand() % 140) + 10) / 100.0f;
    else
        singleLightTimer = (float)((rand() % 25) + 5) / 100.0f;
}

void Draw_StandaloneConeLight(float x, float y)
{
    if (!singleLightIsOn) return;

    static constexpr int   numRays = 300;
    static constexpr float coneAngle = 1.2f;
    static constexpr float maxDist = 1500.0f;
    static constexpr float floorLevel = -700.0f;

    // --- HOT-RELOADED DATA FETCH ---
    float brightness = Config::standaloneBrightness;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);
    AEGfxSetColorToMultiply(1.0f, 0.9f, 0.6f, brightness);
    AEGfxSetTransparency(brightness);

    for (int i = 0; i < numRays; i++)
    {
        float progress = (float)i / (float)(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);

        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        float dist = 0.0f;
        while (dist < maxDist)
        {
            dist += 2.0f;
            if (y + (dirY * dist) < floorLevel) break;
        }

        DrawRay(x, y, angle, dist, squareMesh);
    }

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);
}

// ============================================================
//  LIFECYCLE
// ============================================================

void Lighting_Load() {}

void Lighting_Initialize(int fucked_floor)
{
    Particles_Initialize();

    for (int f = 0; f < 10; f++)
    {
        for (int l = 0; l < 11; l++)
        {
            if (f == 0)
            {
                lightingMatrix[f][l] = 2; // Ground floor always flickers
                continue;
            }

            // --- HOT-RELOADED DATA FETCH ---
            int threshold = (f == fucked_floor) ? Config::floorFuckedChance : Config::floorNormalChance;
            lightingMatrix[f][l] = ((rand() % 100) > threshold) ? 2 : 1;
        }
    }

    squareMesh = CreateSquareMesh(0xFFFFFFFF);

    for (int f = 0; f < 10; ++f)
    {
        for (int l = 0; l < 11; ++l)
        {
            flickerMem[f][l].timer = 0.0f;
            flickerMem[f][l].isVisible = true;
        }
    }
}

void Lighting_Unload()
{
    FreeMeshSafe(squareMesh);
    squareMesh = nullptr;
    Particles_Free();
}

// ============================================================
//  UPDATE
// ============================================================

void Lighting_Update(s8 floorNum, float camX, bool dementia)
{
    if (floorNum < 0 || floorNum >= 10) return;

    const float dt = (float)AEFrameRateControllerGetFrameTime();

    static bool  prevHasPatient = false;
    static float ramp = 0.0f;
    bool hasPatient = Player_HasPatient();

    if (hasPatient && !prevHasPatient) ramp = 0.0f;
    if (!hasPatient) ramp = 0.0f;

    ramp += 2.0f * dt;
    if (ramp > 1.0f) ramp = 1.0f;

    prevHasPatient = hasPatient;

    Particles_Update();
    g_TotalTime += dt;

    const ILLNESSES effIllness = GetEffectiveIllness();
    const int ghostExtra = GetGhostExtraCount();

    for (int i = 0; i < 11; i++)
    {
        if (lightingMatrix[floorNum][i] != 2) continue;

        FlickerData& fd = flickerMem[floorNum][i];
        fd.timer -= dt;

        if (fd.timer > 0.0f) continue;

        fd.isVisible = !fd.isVisible;

        if (effIllness == ILLNESSES::PARANOIA || effIllness == ILLNESSES::SCOTOPHOBIA)
            Frames_SyncToLight(floorNum, i, fd.isVisible);

        float lightWx = i * 600.0f + 300.0f;
        if (dementia)
        {
            float repeatDist = 10 * 600.0f;
            float k = roundf((-camX - lightWx) / repeatDist);
            lightWx += k * repeatDist;
        }

        Particles_Spawn(lightWx, 250.0f, 8);
        Particles_Spawn(lightWx, 250.0f, (ghostExtra >= 1) ? 10 : 8);

        fd.timer = FlickerDuration(effIllness, fd.isVisible, ghostExtra);

        if (ghostExtra >= 1 && (rand() % 100) < 8)
        {
            fd.isVisible = !fd.isVisible;
            fd.timer *= 0.75f;
            if (fd.timer < 0.03f) fd.timer = 0.03f;
        }
    }
}

// ============================================================
//  DRAW CONE LIGHT
// ============================================================

static void DrawConeLight(float lightWorldX, float lightY, float camX, bool right_left, ILLNESSES effIllness, int ghostExtra)
{
    static constexpr int   numRays = 300;
    static constexpr float maxDist = 1000.0f;
    static constexpr float floorLevel = -400.0f;

    float screenLightX = lightWorldX - camX;
    if (screenLightX < -1800.0f || screenLightX > 1800.0f) return;

    float time = g_TotalTime;

    // --- HOT-RELOADED DATA FETCH ---
    int idx = static_cast<int>(effIllness);
    if (idx < 0 || idx > 10) idx = 1; // Safety bound check (defaults to PARANOIA)

    float coneAngle = Config::visualsTable[idx].baseConeAngle;
    float brightness = Config::visualsTable[idx].baseBrightness;
    float r = Config::visualsTable[idx].r;
    float g = Config::visualsTable[idx].g;
    float b = Config::visualsTable[idx].b;

    switch (effIllness)
    {
    case ILLNESSES::MANIA:
        coneAngle += 0.02f * sinf(time * 3.0f);
        brightness += 0.01f * sinf(time * 10.0f);
        g -= 0.02f * sinf(time * 2.0f);
        break;
    case ILLNESSES::DEPRESSION:
        coneAngle -= 0.01f * sinf(time * 0.5f);
        brightness += 0.01f * sinf(time * 0.8f);
        b += 0.02f * sinf(time * 0.3f);
        break;
    case ILLNESSES::DEMENTIA:
        coneAngle += 0.02f * sinf(time * 0.2f);
        brightness -= 0.01f * sinf(time * 0.4f);
        g -= 0.02f * sinf(time * 0.3f);
        break;
    case ILLNESSES::SCHIZOPHRENIA:
        coneAngle += 0.01f * sinf(time * 15.0f) * sinf(time * 1.5f);
        brightness += 0.01f * sinf(time * 12.0f);
        break;
    case ILLNESSES::AIW_SYNDROME:
        coneAngle += 0.05f * sinf(time * 0.6f);
        break;
    case ILLNESSES::INSOMNIA:
        brightness += (sinf(time * 20.0f) > 0.98f ? 0.015f : 0.0f);
        break;
    case ILLNESSES::PARANOIA:
    case ILLNESSES::SCOTOPHOBIA:
        coneAngle += 0.01f * sinf(time * 15.0f);
        break;
    case ILLNESSES::OCD:
    case ILLNESSES::NONE:
    case ILLNESSES::GHOST:
    default:
        break;
    }

    if (ghostExtra >= 1)
    {
        brightness += 0.005f * (0.5f + 0.5f * sinf(time * 9.0f));
        coneAngle += 0.02f * sinf(time * 2.0f);
    }

    float pHeadLeft{}, pHeadRight{};
    float pBodyLeft{}, pBodyRight{};
    float bedLeft{}, bedRight{}, bedTop{};
    static constexpr float bedBot = -200.0f;
    static constexpr float pHeadTop = -60.0f;
    static constexpr float pHeadBot = -105.0f;

    if (!Player_HasPatient())
    {
        pHeadLeft = 30.0f;  pHeadRight = 70.0f;
        pBodyLeft = 35.0f;  pBodyRight = 65.0f;
    }
    else if (right_left)
    {
        pHeadLeft = -45.0f; pHeadRight = 5.0f;
        pBodyLeft = -35.0f; pBodyRight = -5.0f;
        bedLeft = 30.0f; bedRight = 135.0f; bedTop = -150.0f;
    }
    else
    {
        pHeadLeft = 95.0f;  pHeadRight = 145.0f;
        pBodyLeft = 105.0f; pBodyRight = 135.0f;
        bedLeft = -30.0f; bedRight = 70.0f; bedTop = -150.0f;
    }

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);
    AEGfxSetColorToMultiply(r, g, b, brightness);
    AEGfxSetTransparency(brightness);

    for (int i = 0; i < numRays; i++)
    {
        float progress = (float)i / (float)(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);

        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        float dist = 0.0f;
        while (dist < maxDist)
        {
            dist += 1.0f;

            float cx = screenLightX - (dirX * dist);
            float cy = lightY + (dirY * dist);
            if (cy < floorLevel)                                         break;
            if (cx > pHeadLeft && cx < pHeadRight && cy < pHeadTop && cy > pHeadBot) break;
            if (cx > pBodyLeft && cx < pBodyRight && cy < pHeadTop && cy > bedBot)   break;
            if (cx > bedLeft && cx < bedRight && cy < bedTop && cy > bedBot)         break;
        }

        DrawRay(screenLightX, lightY, angle, dist, squareMesh);
    }

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);
}

// ============================================================
//  DRAW & FLICKER
// ============================================================

void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum, bool dementia)
{
    if (floorNum < 0 || floorNum >= 10) return;

    const ILLNESSES effIllness = GetEffectiveIllness();
    const int ghostExtra = GetGhostExtraCount();

    int max_iter = dementia ? 1000 : 11;

    for (int i = 0; i < max_iter; i++) {
        float lightWx = i * 600.0f + 300.0f;
        float screenPos = lightWx + camX;

        if (screenPos < -1200.0f || screenPos > 1200.0f)
            continue;

        int idx = dementia ? (i % 10) : i;

        int state = lightingMatrix[floorNum][idx];

        bool shouldDraw = (state == 1) || (state == 2 && flickerMem[floorNum][idx].isVisible);

        if (state != 0 && shouldDraw)
            DrawConeLight(lightWx, 250.0f, -camX, left_right, effIllness, ghostExtra);
    }

    Particles_Draw(squareMesh, -camX);
}