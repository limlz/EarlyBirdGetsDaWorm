#include "pch.hpp"
#include <cmath>
#include <array>

// ============================================================
//  TYPES & CONSTANTS
// ============================================================

// Tracks whether a light is currently visible and how long until its next state change
struct FlickerData
{
    float timer = 0.0f;
    bool  isVisible = true;
};

// Defines how fast a light flickers for a given illness.
// ON = how long the light stays lit, OFF = how long it stays dark.
// Values are in 1/100ths of a second, randomized within (min, min+range).
struct FlickerTimes
{
    int onMin, onRange;    // light turning ON
    int offMin, offRange;   // light turning OFF
};

// Each illness has its own flicker personality.
// NOTE: This table is indexed by "effective illness" (ghost uses mimic illness).
static constexpr FlickerTimes kFlickerTable[] =
{
    /* PARANOIA      */ { 10,  50,   5,  25 },
    /* MANIA         */ {  5,  30,   2,  15 },
    /* DEPRESSION    */ {100, 200,  50, 150 },
    /* DEMENTIA      */ { 20, 300,   5,  25 },
    /* SCHIZOPHRENIA */ {  5, 150,   5, 100 },
    /* AIW_SYNDROME  */ { 10, 140,   5,  25 },
    /* INSOMNIA      */ {200, 400,   5,   1 },  // barely ever turns off
    /* OCD           */ {100,   1, 100,   1 },  // exactly 1 second on, 1 second off
    /* SCOTOPHOBIA   */ { 10, 140, 100, 200 },
};

// ============================================================
//  STATE
// ============================================================

// Per-light flicker state for every light on every floor (10 floors x 11 lights)
static std::array<std::array<FlickerData, 11>, 10> flickerMem;

// reusable quad mesh for drawing light rays
static AEGfxVertexList* squareMesh = nullptr;

// 0 = broken/off, 1 = always on, 2 = flickering
std::array<std::array<int, 11>, 10> lightingMatrix;

// State for the single standalone hallway light (used outside of the main grid)
static float singleLightTimer = 0.0f;
static bool  singleLightIsOn = true;

static float g_TotalTime = 0.0f; // used to drive animated lighting effects (sin waves, etc.)

// ============================================================
//  GHOST HELPERS
// ============================================================

// Use mimic illness whenever current illness is GHOST.
// This prevents out-of-range table indexing and makes ghost "look like" a real illness.
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

// Rolls a random flicker duration for a given illness, in seconds.
// Pass isOn=true to get the "light on" duration, false for "light off".
static inline float FlickerDuration(ILLNESSES effIllness, bool isOn, int ghostExtra)
{
    // Convert illness -> safe index
    int idx = (int)effIllness;

    const int maxIdx = (int)(sizeof(kFlickerTable) / sizeof(kFlickerTable[0])) - 1;
    if (idx < 0 || idx > maxIdx)
        idx = 0; // fallback to PARANOIA settings

    const FlickerTimes& t = kFlickerTable[idx];

    // Prevent rand()%0 crashes
    int range = isOn ? t.onRange : t.offRange;
    int minv = isOn ? t.onMin : t.offMin;

    if (range <= 0) range = 1;

    float seconds = (float)((rand() % range) + minv) / 100.0f;

    // GHOST +1 anomaly:
    // Make flicker slightly more unstable by shortening durations a bit.
    // (Subtle so it doesn't look like everything is broken.)
    if (ghostExtra >= 1)
    {
        seconds *= isOn ? 0.90f : 0.85f;
        if (seconds < 0.03f) seconds = 0.03f;
    }

    return seconds;
}

// Draws a single light ray as a thin, rotated quad stretching down from the light source.
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

    // sparks whenever it toggles
    Particles_Spawn(lightX, lightY, 8);

    // next duration
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
    static constexpr float brightness = 0.15f;

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
//  LOAD / INITIALIZE / UNLOAD
// ============================================================

void Lighting_Load() {}

void Lighting_Initialize(int fucked_floor)
{
    Particles_Initialize();

    for (int f = 0; f < 10; f++)
    {
        for (int l = 0; l < 11; l++)
        {
            // Ground floor always uses flicker mode in your original code
            if (f == 0)
            {
                lightingMatrix[f][l] = 2;
                continue;
            }

            int threshold = (f == fucked_floor) ? 60 : 70;
            lightingMatrix[f][l] = ((rand() % 100) > threshold) ? 2 : 1;
        }
    }

    squareMesh = CreateSquareMesh(0xFFFFFFFF);

    // Initialize flicker memory defaults
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

    // Fade-in ramp when patient is picked up (your original idea kept)
    static bool  prevHasPatient = false;
    static float ramp = 0.0f;

    bool hasPatient = Player_HasPatient();

    if (hasPatient && !prevHasPatient)
        ramp = 0.0f;

    if (!hasPatient)
        ramp = 0.0f;

    ramp += 2.0f * dt;
    if (ramp > 1.0f) ramp = 1.0f;

    prevHasPatient = hasPatient;

    Particles_Update();
    g_TotalTime += dt;

    // Effective illness for behavior (ghost uses mimic)
    const ILLNESSES effIllness = GetEffectiveIllness();
    const int ghostExtra = GetGhostExtraCount();

    for (int i = 0; i < 11; i++)
    {
        if (lightingMatrix[floorNum][i] != 2) continue; // only flickering lights

        FlickerData& fd = flickerMem[floorNum][i];
        fd.timer -= dt;

        if (fd.timer > 0.0f) continue;

        // Toggle visibility
        fd.isVisible = !fd.isVisible;

        // Paranoia/scotophobia sync frames to light state
        if (effIllness == ILLNESSES::PARANOIA || effIllness == ILLNESSES::SCOTOPHOBIA)
            Frames_SyncToLight(floorNum, i, fd.isVisible);

        // Find light bulb world X (dementia repeats corridor)
        float lightWx = i * 600.0f + 300.0f;
        if (dementia)
        {
            float repeatDist = 10 * 600.0f;
            float k = roundf((-camX - lightWx) / repeatDist);
            lightWx += k * repeatDist;
        }

        // Sparks when toggling (scale a bit with ghost +1)
        Particles_Spawn(lightWx, 250.0f, (ghostExtra >= 1) ? 10 : 8);

        // Next flicker duration
        fd.timer = FlickerDuration(effIllness, fd.isVisible, ghostExtra);

        // Optional: GHOST +1 anomaly gives a small extra chance to "double toggle"
        // (very subtle, makes it feel more haunted without becoming spammy)
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

    // Cull if offscreen
    float screenLightX = lightWorldX - camX;
    if (screenLightX < -1800.0f || screenLightX > 1800.0f) return;

    float time = g_TotalTime;

    // defaults
    float coneAngle = 1.2f;
    float brightness = 0.1f;
    float r = 0.95f, g = 0.92f, b = 0.85f;

    switch (effIllness)
    {
    case ILLNESSES::MANIA:
        coneAngle = 1.2f + 0.02f * sinf(time * 3.0f);
        brightness = 0.11f + 0.01f * sinf(time * 10.0f);
        r = 1.0f; g = 0.90f - 0.02f * sinf(time * 2.0f); b = 0.75f;
        break;

    case ILLNESSES::DEPRESSION:
        coneAngle = 1.18f - 0.01f * sinf(time * 0.5f);
        brightness = 0.08f + 0.01f * sinf(time * 0.8f);
        r = 0.85f; g = 0.88f; b = 0.98f + 0.02f * sinf(time * 0.3f);
        break;

    case ILLNESSES::DEMENTIA:
        coneAngle = 1.2f + 0.02f * sinf(time * 0.2f);
        brightness = 0.09f - 0.01f * sinf(time * 0.4f);
        r = 0.98f; g = 0.90f - 0.02f * sinf(time * 0.3f); b = 0.80f;
        break;

    case ILLNESSES::SCHIZOPHRENIA:
        coneAngle = 1.2f + 0.01f * sinf(time * 15.0f) * sinf(time * 1.5f);
        brightness = 0.1f + 0.01f * sinf(time * 12.0f);
        r = 0.90f; g = 0.95f; b = 0.92f;
        break;

    case ILLNESSES::AIW_SYNDROME:
        coneAngle = 1.3f + 0.05f * sinf(time * 0.6f);
        r = 0.95f; g = 0.95f; b = 1.0f;
        break;

    case ILLNESSES::INSOMNIA:
        brightness = 0.1f + (sinf(time * 20.0f) > 0.98f ? 0.015f : 0.0f);
        r = 0.98f; g = 0.98f; b = 1.0f;
        break;

    case ILLNESSES::OCD:
        coneAngle = 1.15f; brightness = 0.11f;
        r = 0.95f; g = 0.95f; b = 0.95f;
        break;

    case ILLNESSES::PARANOIA:
        coneAngle = 0.9f + 0.01f * sinf(time * 15.0f);
        brightness = 0.09f;
        r = 0.95f; g = 0.90f; b = 0.80f;
        break;

    default:
        break;
    }

    // GHOST +1 anomaly: tiny brightness pulse + slightly wider cone (subtle)
    if (ghostExtra >= 1)
    {
        brightness += 0.005f * (0.5f + 0.5f * sinf(time * 9.0f));
        coneAngle += 0.02f * sinf(time * 2.0f);
    }

    // Shadow-casting bounds
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

            if (cy < floorLevel) break;

            if (cx > pHeadLeft && cx < pHeadRight && cy < pHeadTop && cy > pHeadBot) break;
            if (cx > pBodyLeft && cx < pBodyRight && cy < pHeadTop && cy > bedBot)   break;
            if (cx > bedLeft && cx < bedRight && cy < bedTop && cy > bedBot)        break;
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

    // Dementia loops corridor endlessly
    int max_iter = dementia ? 1000 : 11;

    for (int i = 0; i < max_iter; i++)
    {
        float lightWx = i * 600.0f + 300.0f;
        float screenPos = lightWx + camX;

        if (screenPos < -1200.0f || screenPos > 1200.0f)
            continue;

        int idx = dementia ? (i % 10) : i;

        int state = lightingMatrix[floorNum][idx];

        // shouldDraw rules:
        //  - always on (1) draws
        //  - flicker (2) draws only if visible
        bool shouldDraw = (state == 1) || (state == 2 && flickerMem[floorNum][idx].isVisible);

        if (state != 0 && shouldDraw)
            DrawConeLight(lightWx, 250.0f, -camX, left_right, effIllness, ghostExtra);
    }

    Particles_Draw(squareMesh, -camX);
}