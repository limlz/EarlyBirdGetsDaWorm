#include "pch.hpp"
#include <cmath>

// ============================================================
//  TYPES & CONSTANTS
// ============================================================

// Tracks whether a light is currently visible and how long until its next state change
struct FlickerData {
    float timer;
    bool  isVisible;
};

// Defines how fast a light flickers for a given illness.
// ON = how long the light stays lit, OFF = how long it stays dark.
// Values are in 1/100ths of a second, randomized within (min, min+range).
struct FlickerTimes {
    int onMin, onRange;   // light turning ON
    int offMin, offRange;  // light turning OFF
};

// Each illness has its own flicker personality.
// DEPRESSION flickers lazily, MANIA is rapid and jittery,
// OCD is perfectly metronomic, INSOMNIA barely ever turns off.
static constexpr FlickerTimes kFlickerTable[] = {
    /* PARANOIA      */ { 10,  50,   5,  25 },
    /* MANIA         */ {  5,  30,   2,  15 },
    /* DEPRESSION    */ {100, 200,  50, 150 },
    /* DEMENTIA      */ { 20, 300,   5,  25 },
    /* SCHIZOPHRENIA */ {  5, 150,   5, 100 },
    /* AIW_SYNDROME  */ { 10, 140,   5,  25 },  // default 
    /* INSOMNIA      */ {200, 400,   5,   1 },  // barely ever turns off
    /* OCD           */ {100,   1, 100,   1 },  // exactly 1 second on, 1 second off
    /* SCOTOPHOBIA   */ { 10, 140, 100, 200 },
    /* ALL           */ { 10, 140,   5,  25 },  // gets replaced with a real illness at runtime
};

// When the illness is set to ALL, we randomly pick one of these at the start of each day
static const ILLNESSES kAllRollTable[] = {
    MANIA, PARANOIA, DEPRESSION, SCHIZOPHRENIA,
    DEMENTIA, INSOMNIA, OCD, AIW_SYNDROME
};
static constexpr int kAllRollCount = static_cast<int>(sizeof(kAllRollTable) / sizeof(kAllRollTable[0]));

// ============================================================
//  STATE
// ============================================================

// Per-light flicker state for every light on every floor (10 floors x 11 lights)
static std::array<std::array<FlickerData, 11>, 10> flickerMem;
static AEGfxVertexList* squareMesh;           // reusable quad mesh for drawing light rays
static AEGfxTexture* frames_arr[10] = { nullptr };

// 0 = broken/off, 1 = always on, 2 = flickering
std::array<std::array<int, 11>, 10> lightingMatrix;

// State for the single standalone hallway light (used outside of the main grid)
static float singleLightTimer = 0.0f;
static bool  singleLightIsOn = true;

static float g_TotalTime = 0.0f;             // used to drive animated lighting effects (sin waves, etc.)
static ILLNESSES s_RandomAllIllness = PARANOIA; // the illness picked when ALL is active today

// ============================================================
//  HELPERS
// ============================================================

// If the illness is ALL, swap it out for today's randomly chosen one.
// Call this at the top of any function that needs to act on a real illness.
static inline ILLNESSES ResolveIllness(ILLNESSES illness) {
    return (illness == ALL) ? s_RandomAllIllness : illness;
}

// Rolls a random flicker duration for a given illness, in seconds.
// Pass isOn=true to get the "light on" duration, false for "light off".
static inline float FlickerDuration(ILLNESSES illness, bool isOn) {
    const FlickerTimes& t = kFlickerTable[static_cast<int>(illness)];
    if (isOn) return static_cast<float>((rand() % t.onRange) + t.onMin) / 100.0f;
    else      return static_cast<float>((rand() % t.offRange) + t.offMin) / 100.0f;
}

// Draws a single light ray as a thin, rotated quad stretching down from the light source.
// The ray is positioned and rotated in world space using matrix transforms.
static void DrawRay(float screenLightX, float lightY,
    float angle, float dist,
    AEGfxVertexList* mesh)
{
    AEMtx33 scale, rot, trans, pivot, final;
    AEMtx33Scale(&scale, 15.0f, dist);       // make the quad as long as the ray
    AEMtx33Trans(&pivot, 0.0f, -dist / 2.0f); // anchor from the top instead of center
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

// Handles the flickering logic for the lone hallway light.
// Every time it pops on or off, it plays a sound and shoots out a burst of sparks.
void Update_StandaloneLight(float dt, float lightX, float lightY)
{
    singleLightTimer -= dt;
    if (singleLightTimer > 0.0f) return;

    if (singleLightTimer <= 0.0f)
    {
        AudioManager_PlaySFX(SFX_LIGHT_FLICKER, 0.15f, 1.0f, -1);

        singleLightIsOn = !singleLightIsOn;

        // Shoot sparks from the bulb every time it changes state
        Particles_Spawn(lightX, lightY, 8);

        // How long until the next flicker? Varies depending on whether it just turned on or off.
        if (singleLightIsOn) {
            singleLightTimer = (float)((rand() % 140) + 10) / 100.0f;
        }
        else {
            singleLightTimer = (float)((rand() % 25) + 5) / 100.0f;
        }
    }
}

// Draws a wide cone of light rays downward from the standalone light's position.
// Rays are clipped when they hit the floor. Does nothing if the light is currently off.
void Draw_StandaloneConeLight(float x, float y)
{
    if (!singleLightIsOn) return;

    static constexpr int   numRays = 300;
    static constexpr float coneAngle = 1.2f;
    static constexpr float maxDist = 1500.0f;
    static constexpr float floorLevel = -700.0f;
    static constexpr float brightness = 0.15f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);   // additive blend so rays stack and glow nicely
    AEGfxSetColorToMultiply(1.0f, 0.9f, 0.6f, brightness); // warm yellowish tint
    AEGfxSetTransparency(brightness);

    for (int i = 0; i < numRays; i++) {
        // Spread rays evenly across the cone angle
        float progress = static_cast<float>(i) / static_cast<float>(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);
        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        // March along the ray until it hits the floor
        float dist = 0.0f;
        while (dist < maxDist) {
            dist += 2.0f;
            if (y + (dirY * dist) < floorLevel) break;
        }
        DrawRay(x, y, angle, dist, squareMesh);
    }

    // Always reset blend state after drawing so nothing else is affected
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);
}

// ============================================================
//  LOAD / INITIALIZE / UNLOAD
// ============================================================

void Lighting_Load() {}

// Sets up the lighting grid for the current day.
// The "fucked floor" gets a higher chance of broken/flickering lights than normal floors.
void Lighting_Initialize(int fucked_floor)
{
    Particles_Initialize();

    // Lock in which illness ALL maps to for the rest of the day
    s_RandomAllIllness = kAllRollTable[rand() % kAllRollCount];

    for (int f = 0; f < 10; f++) {
        for (int l = 0; l < 11; l++) {
            // Ground floor is always fully lit
            if (f == 0) {
                lightingMatrix[f][l] = 2;
                continue;
            }
            // The "fucked floor" has a higher chance of flickering lights (60% threshold vs 70%)
            int threshold = (f == fucked_floor) ? 60 : 70;
            lightingMatrix[f][l] = ((rand() % 100) > threshold) ? 2 : 1;
        }
    }
    squareMesh = CreateSquareMesh(0xFFFFFFFF);
}

void Lighting_Unload()
{
    FreeMeshSafe(squareMesh);
    Particles_Free();
}

// ============================================================
//  UPDATE
// ============================================================

// Ticks all flickering lights on the current floor and fires sparks whenever one toggles.
// Also syncs animation frames to light state for PARANOIA/SCOTOPHOBIA (their visuals depend on it).
void Lighting_Update(s8 floorNum, float camX, bool dementia, bool isGhost, ILLNESSES illness)
{
    illness = ResolveIllness(illness);
    Particles_Update();

    float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());
    g_TotalTime += dt;

    for (int i = 0; i < 11; i++) {
        if (lightingMatrix[floorNum][i] != 2) continue; // only tick flickering lights

        FlickerData& fd = flickerMem[floorNum][i];
        fd.timer -= dt;
        if (fd.timer > 0.0f) continue; // not time to toggle yet

        fd.isVisible = !fd.isVisible;

        // These illnesses need their sprite frames to match the light state
        if (illness == PARANOIA || illness == SCOTOPHOBIA)
            Frames_SyncToLight(floorNum, i, fd.isVisible);

        // Figure out where the light bulb actually is in world space
        // (dementia warps the layout, so we snap to the nearest repeat)
        float lightWx = i * 600.0f + 300.0f;
        if (dementia) {
            float repeatDist = 10 * 600.0f;
            float k = roundf((-camX - lightWx) / repeatDist);
            lightWx += k * repeatDist;
        }
        Particles_Spawn(lightWx, 250.0f, 8); // spark burst at the bulb position

        fd.timer = FlickerDuration(illness, fd.isVisible);
    }
}

// ============================================================
//  DRAW CONE LIGHT
// ============================================================

// Draws one ceiling light's cone, with per-illness visual tweaks (color, brightness, cone width).
// Each ray is individually ray-marched and stops early if it hits the player, bed, or floor.
void DrawConeLight(float lightWorldX, float lightY, float camX,
    bool right_left, bool isGhost, ILLNESSES illness)
{
    illness = ResolveIllness(illness);

    static constexpr int   numRays = 300;
    static constexpr float maxDist = 1000.0f;
    static constexpr float floorLevel = -400.0f;

    // Cull entirely if the light is offscreen — no point drawing it
    float screenLightX = lightWorldX - camX;
    if (screenLightX < -1800.0f || screenLightX > 1800.0f) return;

    float time = g_TotalTime;

    // Sensible neutral defaults — warm white, medium cone, low brightness
    float coneAngle = 1.2f, brightness = 0.1f;
    float r = 0.95f, g = 0.92f, b = 0.85f;

    // Each illness subtly warps the lighting to reinforce the mood:
    // MANIA pulses fast and bright, DEPRESSION is dim and cold,
    // SCHIZOPHRENIA twitches unpredictably, OCD is unnervingly perfect, etc.
    switch (illness) {
    case MANIA:
        coneAngle = 1.2f + 0.02f * sinf(time * 3.0f);
        brightness = 0.11f + 0.01f * sinf(time * 10.0f);
        r = 1.0f; g = 0.90f - 0.02f * sinf(time * 2.0f); b = 0.75f;
        break;
    case DEPRESSION:
        coneAngle = 1.18f - 0.01f * sinf(time * 0.5f);
        brightness = 0.08f + 0.01f * sinf(time * 0.8f);
        r = 0.85f; g = 0.88f; b = 0.98f + 0.02f * sinf(time * 0.3f);
        break;
    case DEMENTIA:
        coneAngle = 1.2f + 0.02f * sinf(time * 0.2f);
        brightness = 0.09f - 0.01f * sinf(time * 0.4f);
        r = 0.98f; g = 0.90f - 0.02f * sinf(time * 0.3f); b = 0.80f;
        break;
    case SCHIZOPHRENIA:
        // Two interfering sine waves create an irregular, unsettling twitch
        coneAngle = 1.2f + 0.01f * sinf(time * 15.0f) * sinf(time * 1.5f);
        brightness = 0.1f + 0.01f * sinf(time * 12.0f);
        r = 0.90f; g = 0.95f; b = 0.92f;
        break;
    case AIW_SYNDROME:
        coneAngle = 1.3f + 0.05f * sinf(time * 0.6f); // slowly breathes in and out
        r = 0.95f; g = 0.95f; b = 1.0f;
        break;
    case INSOMNIA:
        // Mostly steady, with a rare sharp spike — like a light about to die
        brightness = 0.1f + (sinf(time * 20.0f) > 0.98f ? 0.015f : 0.0f);
        r = 0.98f; g = 0.98f; b = 1.0f;
        break;
    case OCD:
        // Perfectly stable — no variation at all, which feels eerie in context
        coneAngle = 1.15f; brightness = 0.11f;
        r = 0.95f; g = 0.95f; b = 0.95f;
        break;
    case PARANOIA:
        // Narrow cone that twitches slightly, like something always watching
        coneAngle = 0.9f + 0.01f * sinf(time * 15.0f);
        brightness = 0.09f;
        r = 0.95f; g = 0.90f; b = 0.80f;
        break;
    default: break;
    }

    // Shadow-casting bounds for the player's head, body, and bed.
    // Rays that enter these boxes are clipped early to fake a shadow.
    float pHeadLeft{}, pHeadRight{};
    float pBodyLeft{}, pBodyRight{};
    float bedLeft{}, bedRight{}, bedTop{};
    static constexpr float bedBot = -200.0f;
    static constexpr float pHeadTop = -60.0f;
    static constexpr float pHeadBot = -105.0f;

    // Shadow box positions differ depending on which side the patient is on
    if (!Player_HasPatient()) {
        pHeadLeft = 30.0f;  pHeadRight = 70.0f;
        pBodyLeft = 35.0f;  pBodyRight = 65.0f;
    }
    else if (right_left) {
        pHeadLeft = -45.0f; pHeadRight = 5.0f;
        pBodyLeft = -35.0f; pBodyRight = -5.0f;
        bedLeft = 30.0f; bedRight = 135.0f; bedTop = -150.0f;
    }
    else {
        pHeadLeft = 95.0f; pHeadRight = 145.0f;
        pBodyLeft = 105.0f; pBodyRight = 135.0f;
        bedLeft = -30.0f; bedRight = 70.0f; bedTop = -150.0f;
    }

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_ADD);
    AEGfxSetColorToMultiply(r, g, b, brightness);
    AEGfxSetTransparency(brightness);

    for (int i = 0; i < numRays; i++) {
        float progress = static_cast<float>(i) / static_cast<float>(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);
        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        // Step along the ray, stopping early if it hits the floor, the patient's body, or the bed
        float dist = 0.0f;
        while (dist < maxDist) {
            dist += 1.0f;
            float cx = screenLightX - (dirX * dist);
            float cy = lightY + (dirY * dist);
            if (cy < floorLevel)                                            break;
            if (cx > pHeadLeft && cx < pHeadRight && cy < pHeadTop && cy > pHeadBot) break;
            if (cx > pBodyLeft && cx < pBodyRight && cy < pHeadTop && cy > bedBot)   break;
            if (cx > bedLeft && cx < bedRight && cy < bedTop && cy > bedBot)         break;
        }
		// Draw the ray, which will be clipped at dist
        DrawRay(screenLightX, lightY, angle, dist, squareMesh);
    }

    // Reset blend state so the rest of the frame renders normally
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);
}

// ============================================================
//  DRAW & FLICKER
// ============================================================

// Master draw call for all lights on the current floor.
// Skips offscreen lights, respects their on/flicker/off state, and handles
// the dementia "infinite corridor" effect by tiling the light layout.
void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum,
    bool dementia, bool isGhost, ILLNESSES illness)
{
    illness = ResolveIllness(illness);

    if (floorNum < 0 || floorNum >= 10) return;

    // Dementia loops the corridor endlessly, more iterations to fill the screen
    int max_iter = dementia ? 1000 : 11;

    for (int i = 0; i < max_iter; i++) {
        float lightWx = i * 600.0f + 300.0f;
        float screenPos = lightWx + camX;
        if (screenPos < -1200.0f || screenPos > 1200.0f) continue; // offscreen, skip

        // In dementia mode, wrap the index so the 11-light pattern tiles seamlessly
        int  idx = dementia ? (i % 10) : i;
        int  state = lightingMatrix[floorNum][idx];

        // state 1 = always on, state 2 = flickering (check current visibility)
        bool shouldDraw = (state == 1) || (state == 2 && flickerMem[floorNum][idx].isVisible);

        if (state != 0 && shouldDraw)
            DrawConeLight(lightWx, 250.0f, -camX, left_right, isGhost, illness);
    }

    Particles_Draw(squareMesh, -camX);
}