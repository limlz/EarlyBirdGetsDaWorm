#include "pch.hpp"
#include <cmath>

// ============================================================
//  TYPES & CONSTANTS
// ============================================================

struct FlickerData {
    float timer;
    bool  isVisible;
};

// Per-illness ON/OFF flicker durations (min, range) in 1/100 s
struct FlickerTimes {
    int onMin, onRange;   // light turning ON
    int offMin, offRange;  // light turning OFF
};

static constexpr FlickerTimes kFlickerTable[] = {
    /* PARANOIA      */ { 10,  50,   5,  25 },
    /* MANIA         */ {  5,  30,   2,  15 },
    /* DEPRESSION    */ {100, 200,  50, 150 },
    /* DEMENTIA      */ { 20, 300,   5,  25 },
    /* SCHIZOPHRENIA */ {  5, 150,   5, 100 },
    /* AIW_SYNDROME  */ { 10, 140,   5,  25 },  // default behaviour
    /* INSOMNIA      */ {200, 400,   5,   1 },  // off is near-instant
    /* OCD           */ {100,   1, 100,   1 },  // fixed 1 s both ways
    /* SCOTOPHOBIA   */ { 10, 140, 100, 200 },
    /* ALL           */ { 10, 140,   5,  25 },  // resolved before use
};

static const ILLNESSES kAllRollTable[] = {
    MANIA, PARANOIA, DEPRESSION, SCHIZOPHRENIA,
    DEMENTIA, INSOMNIA, OCD, AIW_SYNDROME
};
static constexpr int kAllRollCount = static_cast<int>(sizeof(kAllRollTable) / sizeof(kAllRollTable[0]));

// ============================================================
//  STATE
// ============================================================

static std::array<std::array<FlickerData, 11>, 10> flickerMem;
static AEGfxVertexList* squareMesh;
static AEGfxTexture* frames_arr[10] = { nullptr };
std::array<std::array<int, 11>, 10> lightingMatrix;

static float singleLightTimer = 0.0f;
static bool  singleLightIsOn = true;
static float g_TotalTime = 0.0f;
static ILLNESSES s_RandomAllIllness = PARANOIA;

// ============================================================
//  HELPERS
// ============================================================

// Resolve ALL to the day's chosen illness everywhere in one place
static inline ILLNESSES ResolveIllness(ILLNESSES illness) {
    return (illness == ALL) ? s_RandomAllIllness : illness;
}

// Generate a flicker duration from the table (in seconds)
static inline float FlickerDuration(ILLNESSES illness, bool isOn) {
    const FlickerTimes& t = kFlickerTable[static_cast<int>(illness)];
    if (isOn) return static_cast<float>((rand() % t.onRange) + t.onMin) / 100.0f;
    else      return static_cast<float>((rand() % t.offRange) + t.offMin) / 100.0f;
}

// Build a cone-light transform matrix and draw one ray
static void DrawRay(float screenLightX, float lightY,
    float angle, float dist,
    AEGfxVertexList* mesh)
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

    if (singleLightTimer <= 0.0f)
    {
        AudioManager_PlaySFX(SFX_LIGHT_FLICKER, 0.15f, 1.0f, -1);
        // Toggle visibility
        singleLightIsOn = !singleLightIsOn;

        // Spawn 8 sparks right at the light's source!
        // We trigger this every time it "pops" and changes state.
        Particles_Spawn(lightX, lightY, 8);

        // Randomize the next timer
        if (singleLightIsOn) {
            singleLightTimer = (float)((rand() % 140) + 10) / 100.0f;
        }
        else {
            singleLightTimer = (float)((rand() % 25) + 5) / 100.0f;
        }
    }
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

    for (int i = 0; i < numRays; i++) {
        float progress = static_cast<float>(i) / static_cast<float>(numRays - 1);
        float angle = -coneAngle / 2.0f + (progress * coneAngle);
        float dirX = sinf(angle);
        float dirY = -cosf(angle);

        float dist = 0.0f;
        while (dist < maxDist) {
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

    // Roll the random illness for ALL patients once per day
    s_RandomAllIllness = kAllRollTable[rand() % kAllRollCount];

    for (int f = 0; f < 10; f++) {
        for (int l = 0; l < 11; l++) {
            if (f == 0) {
                lightingMatrix[f][l] = 2;
                continue;
            }
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

void Lighting_Update(s8 floorNum, float camX, bool dementia, bool isGhost, ILLNESSES illness)
{
    illness = ResolveIllness(illness);
    Particles_Update();

    float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());
    g_TotalTime += dt;

    for (int i = 0; i < 11; i++) {
        if (lightingMatrix[floorNum][i] != 2) continue;

        FlickerData& fd = flickerMem[floorNum][i];
        fd.timer -= dt;
        if (fd.timer > 0.0f) continue;

        fd.isVisible = !fd.isVisible;

        if (illness == PARANOIA || illness == SCOTOPHOBIA)
            Frames_SyncToLight(floorNum, i, fd.isVisible);

        float lightWx = i * 600.0f + 300.0f;
        if (dementia) {
            float repeatDist = 10 * 600.0f;
            float k = roundf((-camX - lightWx) / repeatDist);
            lightWx += k * repeatDist;
        }
        Particles_Spawn(lightWx, 250.0f, 8);

        fd.timer = FlickerDuration(illness, fd.isVisible);
    }
}

// ============================================================
//  DRAW CONE LIGHT
// ============================================================

void DrawConeLight(float lightWorldX, float lightY, float camX,
    bool right_left, bool isGhost, ILLNESSES illness)
{
    illness = ResolveIllness(illness);

    static constexpr int   numRays = 300;
    static constexpr float maxDist = 1000.0f;
    static constexpr float floorLevel = -400.0f;

    float screenLightX = lightWorldX - camX;
    if (screenLightX < -1800.0f || screenLightX > 1800.0f) return;

    float time = g_TotalTime;

    // Defaults
    float coneAngle = 1.2f, brightness = 0.1f;
    float r = 0.95f, g = 0.92f, b = 0.85f;

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
        coneAngle = 1.2f + 0.01f * sinf(time * 15.0f) * sinf(time * 1.5f);
        brightness = 0.1f + 0.01f * sinf(time * 12.0f);
        r = 0.90f; g = 0.95f; b = 0.92f;
        break;
    case AIW_SYNDROME:
        coneAngle = 1.3f + 0.05f * sinf(time * 0.6f);
        r = 0.95f; g = 0.95f; b = 1.0f;
        break;
    case INSOMNIA:
        brightness = 0.1f + (sinf(time * 20.0f) > 0.98f ? 0.015f : 0.0f);
        r = 0.98f; g = 0.98f; b = 1.0f;
        break;
    case OCD:
        coneAngle = 1.15f; brightness = 0.11f;
        r = 0.95f; g = 0.95f; b = 0.95f;
        break;
    case PARANOIA:
        coneAngle = 0.9f + 0.01f * sinf(time * 15.0f);
        brightness = 0.09f;
        r = 0.95f; g = 0.90f; b = 0.80f;
        break;
    default: break;
    }

    // Shadow culling bounds
    float pHeadLeft{}, pHeadRight{};
    float pBodyLeft{}, pBodyRight{};
    float bedLeft{}, bedRight{}, bedTop{};
    static constexpr float bedBot = -200.0f;
    static constexpr float pHeadTop = -60.0f;
    static constexpr float pHeadBot = -105.0f;

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

        float dist = 0.0f;
        while (dist < maxDist) {
            dist += 1.0f;
            float cx = screenLightX - (dirX * dist);
            float cy = lightY + (dirY * dist);
            if (cy < floorLevel)                                            break;
            if (cx > pHeadLeft && cx < pHeadRight && cy < pHeadTop && cy > pHeadBot) break;
            if (cx > pBodyLeft && cx < pBodyRight && cy < pHeadTop && cy > bedBot)   break;
            if (cx > bedLeft && cx < bedRight && cy < bedTop && cy > bedBot)   break;
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

void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum,
    bool dementia, bool isGhost, ILLNESSES illness)
{
    illness = ResolveIllness(illness);

    if (floorNum < 0 || floorNum >= 10) return;

    int max_iter = dementia ? 1000 : 11;

    for (int i = 0; i < max_iter; i++) {
        float lightWx = i * 600.0f + 300.0f;
        float screenPos = lightWx + camX;
        if (screenPos < -1200.0f || screenPos > 1200.0f) continue;

        int  idx = dementia ? (i % 10) : i;
        int  state = lightingMatrix[floorNum][idx];
        bool shouldDraw = (state == 1) || (state == 2 && flickerMem[floorNum][idx].isVisible);

        if (state != 0 && shouldDraw)
            DrawConeLight(lightWx, 250.0f, -camX, left_right, isGhost, illness);
    }

    Particles_Draw(squareMesh, -camX);
}