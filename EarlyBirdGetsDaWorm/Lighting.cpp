#include "pch.hpp"

struct FlickerData
{
    float timer = 0.0f;
    bool  isVisible = true;
};

// Per-light flicker state; indexed [floor][light] to match lightingMatrix layout
static std::array<std::array<FlickerData, 11>, 10> flickerMem;
std::array<std::array<int, 11>, 10> lightingMatrix; // 0 = off, 1 = on, 2 = flickering

static AEGfxVertexList* squareMesh = nullptr;

static float singleLightTimer = 0.0f;
static bool  singleLightIsOn = true;
static float g_TotalTime = 0.0f;

// ============================================================
//  GHOST HELPERS
// ============================================================

// GHOST borrows another illness's behaviour at runtime; resolve that here before any logic reads the illness.
// Returns NONE when the lighting anomaly system is not active so all downstream effects fall back to baseline.
static ILLNESSES GetEffectiveIllness()
{
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lighting_isActive))
        return ILLNESSES::NONE;

    ILLNESSES current = Player_GetTrueDiagnosis();
    if (current == ILLNESSES::GHOST)
        return Player_GetAnomalyIllness();

    return current;
}

// GHOST gets an extra anomaly layer on top of its mimic illness; this drives the bonus instability checks below.
// Returns 0 when the lighting anomaly system is not active.
static int GetGhostExtraCount()
{
    if (!CentralPool_IsSystemActive(AnomalyFiles::Lighting_isActive))
        return 0;

    if (Player_GetTrueDiagnosis() == ILLNESSES::GHOST)
        return Player_GetGhostExtraAnomalies();
    return 0;
}

// ============================================================
//  HELPERS
// ============================================================

// Randomises the next on/off duration from Config's hot-reloaded table; GHOST tightens the intervals to increase perceived instability
static inline float FlickerDuration(ILLNESSES effIllness, bool isOn, int ghostExtra)
{
    int idx = static_cast<int>(effIllness);
    if (idx < 0 || idx > 10) idx = 1; // Out-of-range illness index falls back to PARANOIA timings

    const FlickerTimes& t = Config::flickerTable[idx];

    int range = isOn ? t.onRange : t.offRange;
    int minv = isOn ? t.onMin : t.offMin;

    if (range <= 0) range = 1;

    float seconds = static_cast<float>(RandInt(minv, minv + range - 1)) / 100.0f;

    // Shorten both phases slightly so the light feels more erratic under GHOST's extra anomaly
    if (ghostExtra >= 1)
    {
        seconds *= isOn ? 0.90f : 0.85f;
        if (seconds < 0.03f) seconds = 0.03f;
    }

    return seconds;
}

// Draws a single ray quad; pivoted from the top so the mesh stretches downward from the light source, not from its center
static void DrawRay(float screenLightX, float lightY, float angle, float dist, AEGfxVertexList* mesh)
{
    AEMtx33 scale, rot, trans, pivot, final;
    AEMtx33Scale(&scale, 15.0f, dist);
    AEMtx33Trans(&pivot, 0.0f, -dist / 2.0f);
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


    singleLightIsOn = !singleLightIsOn;

    Particles_Spawn(lightX, lightY, 8);

    // On-phase is much longer than off-phase so the light reads as "mostly on with brief cuts"
    if (singleLightIsOn) {
        AudioManager_PlaySFX(SoundID::SFX_LIGHT_FLICKER, 0.15f, 1.0f, 0);
        singleLightTimer = static_cast<float>(RandInt(10, 149)) / 100.0f;
    } else {
        singleLightTimer = static_cast<float>(RandInt(5, 29)) / 100.0f;
    }
}

void Draw_StandaloneConeLight(float x, float y)
{
    if (!singleLightIsOn) return;

    static constexpr int   numRays = 300;
    static constexpr float coneAngle = 1.2f;
    static constexpr float maxDist = 1500.0f;
    static constexpr float floorLevel = -700.0f;

    float brightness = Config::standaloneBrightness;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);
    AEGfxSetColorToMultiply(1.0f, 0.9f, 0.6f, brightness);
    AEGfxSetTransparency(brightness);

    for (int i = 0; i < numRays; i++)
    {
        float progress = static_cast<float>(i) / static_cast<float>(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);

        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        // March the ray until it either hits the floor or exhausts maxDist
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

void Lighting_Initialize(int fucked_floor)
{
    Particles_Initialize();

    for (int f = 0; f < 10; f++)
    {
        for (int l = 0; l < 11; l++)
        {
            if (f == 0)
            {
                lightingMatrix[f][l] = 2; // Ground floor is hardcoded to always flicker
                continue;
            }

            // fucked_floor uses a higher flicker threshold to make that floor visibly more unstable than the rest
            int threshold = (f == fucked_floor) ? Config::floorFuckedChance : Config::floorNormalChance;
            lightingMatrix[f][l] = (RandInt(0, 99) > threshold) ? 2 : 1;
        }
    }

    FreeMeshSafe(squareMesh);                   // free any previous mesh (e.g. from MainMenu) before creating a new one
    squareMesh = CreateSquareMesh(0xFFFFFFFF);

    // Reset all flicker state so lights don't inherit leftover timers from a previous run
    for (int f = 0; f < 10; ++f)
        for (int l = 0; l < 11; ++l)
        {
            flickerMem[f][l].timer = 0.0f;
            flickerMem[f][l].isVisible = true;
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

    const float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());

    // Ramp resets to 0 the moment a patient is picked up, then climbs to 1 over ~0.5s
    static bool  prevHasPatient = false;
    static float ramp = 0.0f;
    bool hasPatient = Player_HasPatient();

    // --- ANOMALY TRIGGER ---
    if (hasPatient && !prevHasPatient) {
        ramp = 0.0f;

        // Reset all light timers to 0 immediately when a patient is picked up. 
        for (int i = 0; i < 11; i++) {
            flickerMem[floorNum][i].timer = 0.0f;
        }
    }

    if (!hasPatient) ramp = 0.0f;

    ramp += 2.0f * dt;
    if (ramp > 1.0f) ramp = 1.0f;

    prevHasPatient = hasPatient;

    Particles_Update();
    g_TotalTime += dt;

    const ILLNESSES effIllness = GetEffectiveIllness();
    const int ghostExtra = GetGhostExtraCount();

    // Only flicker when a patient is present AND the lighting anomaly system is active for them.
    // Otherwise force every flickering light back to visible so the corridor looks normal.
    const bool lightingAnomalyActive = hasPatient
        && CentralPool_IsSystemActive(AnomalyFiles::Lighting_isActive);

    for (int i = 0; i < 11; i++)
    {
        if (lightingMatrix[floorNum][i] != 2) continue;

        FlickerData& fd = flickerMem[floorNum][i];

        if (!lightingAnomalyActive)
        {
            // No patient or patient has no lighting anomaly — hold lights steady
            fd.isVisible = true;
            fd.timer     = 0.0f;
            continue;
        }

        fd.timer -= dt;

        if (fd.timer > 0.0f) continue;

        fd.isVisible = !fd.isVisible;

        float lightWx = i * 600.0f + 300.0f;

        // Dementia wraps light world positions so the corridor appears to repeat infinitely
        if (dementia)
        {
            float repeatDist = 10 * 600.0f;
            float k = roundf((-camX - lightWx) / repeatDist);
            lightWx += k * repeatDist;
        }

        Particles_Spawn(lightWx, 250.0f, 8);
        // GHOST spawns extra particles on each flicker to visually telegraph the added anomaly layer
        Particles_Spawn(lightWx, 250.0f, (ghostExtra >= 1) ? 10 : 8);

        fd.timer = FlickerDuration(effIllness, fd.isVisible, ghostExtra);

        // 8% chance to double-flicker immediately, making the light stutter rather than toggle cleanly
        if (ghostExtra >= 1 && RandInt(0, 99) < 8)
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

// Casts 300 rays in a fan; each illness modulates cone angle, brightness, and RGB over time via sin waves baked into the switch below
static void DrawConeLight(float lightWorldX, float lightY, float camX, bool right_left, ILLNESSES effIllness, int ghostExtra)
{
    static constexpr int   numRays = 300;
    static constexpr float maxDist = 1000.0f;
    static constexpr float floorLevel = -400.0f;

    float screenLightX = lightWorldX - camX;
    if (screenLightX < -1800.0f || screenLightX > 1800.0f) return; // Cull lights that are completely off-screen

    float time = g_TotalTime;

    int idx = static_cast<int>(effIllness);
    if (idx < 0 || idx > 10) idx = 1;

    float coneAngle = Config::visualsTable[idx].baseConeAngle;
    float brightness = Config::visualsTable[idx].baseBrightness;
    float r = Config::visualsTable[idx].r;
    float g = Config::visualsTable[idx].g;
    float b = Config::visualsTable[idx].b;

    // Each case nudges the base values with a unique sin signature to give every illness a distinct visual rhythm
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
        // Two overlapping frequencies create a beating/chaotic shimmer
        coneAngle += 0.01f * sinf(time * 15.0f) * sinf(time * 1.5f);
        brightness += 0.01f * sinf(time * 12.0f);
        break;
    case ILLNESSES::AIW_SYNDROME:
        coneAngle += 0.05f * sinf(time * 0.6f);
        break;
    case ILLNESSES::INSOMNIA:
        // Brief spike only when sin crosses 0.98, producing rare sharp flashes rather than a smooth wave
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

    // GHOST adds a slow brightness pulse and cone drift on top of whatever the mimic illness is doing
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

    // Occlusion boxes shift depending on whether a patient is present and which side of the bed they're on
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
        float progress = static_cast<float>(i) / static_cast<float>(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);

        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        // Step the ray forward until it hits the floor or any of the three occlusion boxes (head, body, bed)
        float dist = 0.0f;
        while (dist < maxDist)
        {
            dist += 1.0f;

            float cx = screenLightX - (dirX * dist);
            float cy = lightY + (dirY * dist);
            if (cy < floorLevel)                                                          break;
            if (cx > pHeadLeft && cx < pHeadRight && cy < pHeadTop && cy > pHeadBot)     break;
            if (cx > pBodyLeft && cx < pBodyRight && cy < pHeadTop && cy > bedBot)       break;
            if (cx > bedLeft && cx < bedRight && cy < bedTop && cy > bedBot)       break;
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

    // Report anomaly to journal
    switch (effIllness)
    {
    case ILLNESSES::PARANOIA:       Journal_ReportAnomaly(ANOMALYID::Light_Paranoia);       break;
    case ILLNESSES::MANIA:          Journal_ReportAnomaly(ANOMALYID::Light_Mania);          break;
    case ILLNESSES::DEPRESSION:     Journal_ReportAnomaly(ANOMALYID::Light_Depression);     break;
    case ILLNESSES::DEMENTIA:       Journal_ReportAnomaly(ANOMALYID::Light_Dementia);       break;
    case ILLNESSES::SCHIZOPHRENIA:  Journal_ReportAnomaly(ANOMALYID::Light_Schizophrenia);  break;
    case ILLNESSES::AIW_SYNDROME:   Journal_ReportAnomaly(ANOMALYID::Light_AIW);            break;
    case ILLNESSES::INSOMNIA:       Journal_ReportAnomaly(ANOMALYID::Light_Insomnia);       break;
    case ILLNESSES::OCD:            Journal_ReportAnomaly(ANOMALYID::Light_OCD);            break;
    case ILLNESSES::SCOTOPHOBIA:    Journal_ReportAnomaly(ANOMALYID::Light_Scotophobia);    break;
    default:break;
    }

    // Dementia iterates 1000 lights to fill the infinitely repeating corridor; normal mode only needs 11
    int max_iter = dementia ? 1000 : 11;

    for (int i = 0; i < max_iter; i++) {
        float lightWx = i * 600.0f + 300.0f;
        float screenPos = lightWx + camX;

        if (screenPos < -1200.0f || screenPos > 1200.0f)
            continue;

        int idx = dementia ? (i % 10) : i; // Wrap index so dementia tiles reuse the same 10-light flicker states
        int state = lightingMatrix[floorNum][idx];

        bool shouldDraw = (state == 1) || (state == 2 && flickerMem[floorNum][idx].isVisible);

        if (state != 0 && shouldDraw)
            DrawConeLight(lightWx, 250.0f, -camX, left_right, effIllness, ghostExtra);
    }

    Particles_Draw(squareMesh, -camX);
}

// ============================================================
//  OVERLAY ALPHA
//  Drives the darkness of the full-screen black overlay drawn
//  in Game_Draw() between the world geometry and the lights.
//  Values are tuned per illness theme; default (NONE) = 0.5.
// ============================================================

float Lighting_GetOverlayAlpha()
{
    int index = static_cast<int>(GetEffectiveIllness());
    if (index < 0 || index > 10) return 0.50f;
    return Config::overlayAlphaTable[index];
}

bool Lighting_IsPlayerInDarkness(float camX, s8 floorNum)
{
    if (floorNum < 0 || floorNum >= 10) return false;

    // Player is always drawn at screen X = 50; screen position = worldX + camX
    const float PLAYER_SCREEN_X = 50.0f;

    float bestDist   = 1e9f;
    bool  nearestLit = false;

    for (int i = 0; i < 11; i++)
    {
        float screenX = i * 600.0f + 300.0f + camX;
        float dist    = fabsf(screenX - PLAYER_SCREEN_X);

        if (dist < bestDist)
        {
            bestDist = dist;
            int state   = lightingMatrix[floorNum][i];
            nearestLit  = (state == 1) || (state == 2 && flickerMem[floorNum][i].isVisible);
        }
    }

    return !nearestLit;
}