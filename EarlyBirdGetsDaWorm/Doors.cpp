// ============================================================
// DOORS MODULE
// ============================================================

#include "pch.hpp"
#include <vector>
#include <ctime>
#include <cstdlib>

// ============================================================
// CONFIG
// ============================================================

static constexpr float DOOR_W = DOOR_WIDTH;
static constexpr float DOOR_H = DOOR_HEIGHT;

static float ComputeDoorY()
{
    const float floorLineY = (-FLOOR_CENTER_Y) + (FLOOR_HEIGHT * 0.5f);
    return floorLineY + (DOOR_H * 0.5f);
}
static const float DOOR_Y = ComputeDoorY();

// Window placement (UV-ish placement within door)
static constexpr float WINDOW_CENTER_U = 0.50f;
static constexpr float WINDOW_CENTER_V = 0.62f;

static constexpr float WINDOW_SIZE_U = 0.45f;
static constexpr float WINDOW_SIZE_V = 0.18f;

static constexpr float WINDOW_W = DOOR_W * WINDOW_SIZE_U;
static constexpr float WINDOW_H = DOOR_H * WINDOW_SIZE_V;

// Event tuning
static constexpr float EVENT_ROLL_COOLDOWN = 2.0f;
static constexpr int   EVENT_CHANCE_PERCENT = 35;

// Ghost "+1 extra anomaly" tuning (extra roll is rarer + gated)
static constexpr float GHOST_EXTRA_ROLL_COOLDOWN = 3.0f;
static constexpr int   GHOST_EXTRA_CHANCE_PERCENT = 18;

// ============================================================
// RENDER DATA
// ============================================================

static AEGfxVertexList* gQuadMesh = nullptr;
static s8               gDoorFontId = -1;

static AEGfxTexture* gDoorTex = nullptr;
static AEGfxTexture* gWindowBaseTex = nullptr;
static AEGfxTexture* gHandprintTex = nullptr;
static AEGfxTexture* gShadowTex = nullptr;

// ============================================================
// EVENT STATE
// ============================================================

enum class DoorEvent
{
    None,
    HandprintSlam,
    ShadowWalk,
    Knock
};

struct HandprintSlamState
{
    bool  active = false;
    float t = 0.0f;
    float duration = 0.55f;
};

struct ShadowWalkState
{
    bool  active = false;
    float offsetX = 0.0f;
    float speed = 220.0f;
};

struct DoorEventState
{
    DoorEvent event = DoorEvent::None;

    float rollCooldown = 0.0f; // normal roll gate
    float eventCooldown = 0.0f; // event active gate

    // Ghost "+1 extra" roll gate
    float ghostRollCooldown = 0.0f;

    HandprintSlamState slam;
    ShadowWalkState    shadow;
};

static DoorEventState gDoorState[NUM_DOORS]{};
static bool gDoorsEnabled = true;

// ============================================================
// BASEMENT SYSTEM
// ============================================================

struct BasementCheck
{
    bool lockedThisDay = false;
};

static BasementCheck basementDoors[NUM_DOORS]{};

void Doors_ResetAllLocks()
{
    for (int i = 0; i < NUM_DOORS; ++i)
        basementDoors[i].lockedThisDay = false;
}

// Disposal door logic:
// - Each door can only be used once per day (lockedThisDay)
// - 30% chance of jumpscare fake-out (returns false)
// - Otherwise disposal succeeds (returns true)
bool Doors_TryDisposal(int floorNum, int doorIdx, bool& outDidJumpscare)
{
    outDidJumpscare = false;

    if (doorIdx < 0 || doorIdx >= NUM_DOORS) return false;

    BasementCheck& s = basementDoors[doorIdx];
    if (s.lockedThisDay) return false;

    s.lockedThisDay = true;

    if ((rand() % 100) < 30)
    {
        JumpScare_Start();
        outDidJumpscare = true;
        return false; // blocked disposal, but we DID jumpscare
    }

    return true; // disposal success
}

// ============================================================
// ENABLE TOGGLE
// ============================================================

void Doors_SetEnabled(bool enabled)
{
    gDoorsEnabled = enabled;

    if (!enabled)
    {
        for (int i = 0; i < NUM_DOORS; ++i)
            gDoorState[i] = DoorEventState{};
    }
}

// ============================================================
// HELPERS
// ============================================================

static float DoorWorldX(int doorIndex, float camX)
{
    return DIST_BETWEEN_DOORS + camX + (DIST_BETWEEN_DOORS * doorIndex);
}

static void WindowWorldCenter(float doorX, float& x, float& y)
{
    const float offX = (WINDOW_CENTER_U - 0.5f) * DOOR_W;
    const float offY = (WINDOW_CENTER_V - 0.5f) * DOOR_H;

    x = doorX + offX;
    y = DOOR_Y + offY;
}

static void ClearDoorEvent(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];

    s.event = DoorEvent::None;
    s.slam = HandprintSlamState{};
    s.shadow = ShadowWalkState{};
}

static void TriggerKnock(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::Knock;

    // sound
    AudioManager_PlaySFX(SFX_DOOR_KNOCK, 0.35f, 1.0f, -1);

    // small cooldown so it doesn't chain instantly
    s.eventCooldown = 1.0f;
}

static void TriggerHandprintSlam(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::HandprintSlam;
    s.slam.active = true;
    s.slam.t = 0.0f;
}

static void TriggerShadowWalk(int doorIdx)
{
    DoorEventState& s = gDoorState[doorIdx];
    ClearDoorEvent(doorIdx);

    s.event = DoorEvent::ShadowWalk;
    s.shadow.active = true;

    // start offscreen left within window, then slide through
    s.shadow.offsetX = -(WINDOW_W * 0.5f) - (WINDOW_W * 0.6f);
    s.shadow.speed = 200.0f + (rand() % 120);
}

static void SeedRNGOnce()
{
    static bool seeded = false;

    if (!seeded)
    {
        srand((unsigned)time(nullptr));
        seeded = true;
    }
}

// ============================================================
// ILLNESS EVENT POOLS
// ============================================================

static DoorEvent GetDoorEventByIllness(ILLNESSES illness)
{
    switch (illness)
    {
    case ILLNESSES::PARANOIA:      return (rand() % 2) ? DoorEvent::ShadowWalk : DoorEvent::Knock;
    case ILLNESSES::MANIA:         return DoorEvent::HandprintSlam;
    case ILLNESSES::DEPRESSION:    return DoorEvent::Knock;
    case ILLNESSES::DEMENTIA:      return DoorEvent::ShadowWalk;
    case ILLNESSES::SCHIZOPHRENIA: return (rand() % 2) ? DoorEvent::ShadowWalk : DoorEvent::HandprintSlam;
    case ILLNESSES::AIW_SYNDROME:  return DoorEvent::Knock;
    case ILLNESSES::INSOMNIA:      return DoorEvent::Knock;
    case ILLNESSES::OCD:           return DoorEvent::HandprintSlam;
    case ILLNESSES::SCOTOPHOBIA:   return DoorEvent::ShadowWalk;
    default:                       return DoorEvent::None;
    }
}

// If current illness is GHOST, use mimic illness for event selection.
static ILLNESSES GetEffectiveIllnessForDoorEvents()
{
    ILLNESSES current = Player_GetCurrentIllness();
    if (current == ILLNESSES::GHOST)
        return Player_GetMimicIllness();
    return current;
}

// ============================================================
// LOAD / INIT / UNLOAD
// ============================================================

void Doors_Load()
{
    gDoorFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

    gDoorTex = LoadTextureChecked(Assets::Background::DoorBg);
    gWindowBaseTex = LoadTextureChecked(Assets::Door_Anomaly::WindowBase);
    gHandprintTex = LoadTextureChecked(Assets::Door_Anomaly::Handprint);
    gShadowTex = LoadTextureChecked(Assets::Door_Anomaly::Shadow);
}

void Doors_Initialize()
{
    SeedRNGOnce();

    gQuadMesh = CreateSquareMesh(0xFFFFFFFF);

    for (int i = 0; i < NUM_DOORS; ++i)
        gDoorState[i] = DoorEventState{};
}

void Doors_Unload()
{
    FreeMeshSafe(gQuadMesh);

    UnloadTextureSafe(gDoorTex);
    UnloadTextureSafe(gWindowBaseTex);
    UnloadTextureSafe(gHandprintTex);
    UnloadTextureSafe(gShadowTex);

    if (gDoorFontId >= 0)
        AEGfxDestroyFont(gDoorFontId);

    gQuadMesh = nullptr;
    gDoorTex = gWindowBaseTex = gHandprintTex = gShadowTex = nullptr;
    gDoorFontId = -1;
}

// ============================================================
// UPDATE: returns door index near the player (or -1)
// ============================================================

int Doors_Update(float camX)
{
    const float detectionRange = DOOR_W * 0.5f;

    for (int i = 0; i < NUM_DOORS; ++i)
    {
        float doorX = DoorWorldX(i, camX);

        // Your existing condition (kept)
        if (doorX > -detectionRange + 50 && doorX < detectionRange + 50)
            return i;
    }

    return -1;
}

// ============================ss================================
// EVENTS
// ============================================================

void Doors_Animate(float dt, int doorNearPlayer, float /*unused*/)
{
    if (!gDoorsEnabled) return;
    if (!Player_HasPatient()) return;

    // Tick down cooldowns for all doors
    for (int d = 0; d < NUM_DOORS; ++d)
    {
        DoorEventState& s = gDoorState[d];

        if (s.rollCooldown > 0)      s.rollCooldown -= dt;
        if (s.eventCooldown > 0)     s.eventCooldown -= dt;
        if (s.ghostRollCooldown > 0) s.ghostRollCooldown -= dt;

        // Update running event internals (minimal: only shadow movement timer-ish)
        if (s.event == DoorEvent::ShadowWalk && s.shadow.active)
        {
            s.shadow.offsetX += s.shadow.speed * dt;

            // when fully passed window, stop it and cooldown a bit
            if (s.shadow.offsetX > (WINDOW_W * 1.2f))
            {
                s.shadow.active = false;
                s.event = DoorEvent::None;
                s.eventCooldown = 0.8f;
            }
        }

        if (s.event == DoorEvent::HandprintSlam && s.slam.active)
        {
            s.slam.t += dt;
            if (s.slam.t >= s.slam.duration)
            {
                s.slam.active = false;
                s.event = DoorEvent::None;
                s.eventCooldown = 0.8f;
            }
        }
    }

    if (doorNearPlayer < 0 || doorNearPlayer >= NUM_DOORS)
        return;

    DoorEventState& s = gDoorState[doorNearPlayer];

    // If door is already "busy", don't trigger new events
    if (s.eventCooldown > 0 || s.rollCooldown > 0)
        return;

    // Normal roll gate
    s.rollCooldown = EVENT_ROLL_COOLDOWN;

    // Chance roll
    if ((rand() % 100) >= EVENT_CHANCE_PERCENT)
        return;

    // Choose event based on EFFECTIVE illness (ghost uses mimic illness)
    const ILLNESSES eff = GetEffectiveIllnessForDoorEvents();
    DoorEvent chosen = GetDoorEventByIllness(eff);

    switch (chosen)
    {
    case DoorEvent::HandprintSlam: TriggerHandprintSlam(doorNearPlayer); break;
    case DoorEvent::Knock:         TriggerKnock(doorNearPlayer); break;
    case DoorEvent::ShadowWalk:    TriggerShadowWalk(doorNearPlayer); break;
    default: break;
    }

    // --------------------------------------------------------
    // GHOST "+1 extra anomaly":
    // If carrying a ghost, we give ONE extra chance to trigger
    // a SECOND event (rarer, and separately cooldowned).
    // --------------------------------------------------------
    const bool isGhost = (Player_GetCurrentIllness() == ILLNESSES::GHOST);
    const bool wantExtra = isGhost && (Player_GetGhostExtraAnomalies() >= 1);

    if (wantExtra && s.ghostRollCooldown <= 0.0f)
    {
        s.ghostRollCooldown = GHOST_EXTRA_ROLL_COOLDOWN;

        // extra event roll is smaller chance so it doesn't spam
        if ((rand() % 100) < GHOST_EXTRA_CHANCE_PERCENT)
        {
            DoorEvent extraEvent = GetDoorEventByIllness(eff);

            // Avoid repeating the exact same event twice in a row if possible
            if (extraEvent == s.event)
            {
                for (int tries = 0; tries < 4; ++tries)
                {
                    DoorEvent re = GetDoorEventByIllness(eff);
                    if (re != s.event && re != DoorEvent::None)
                    {
                        extraEvent = re;
                        break;
                    }
                }
            }

            // Only trigger if the current one isn't still actively blocking
            // (we keep it simple: if eventCooldown already set, skip extra)
            if (s.eventCooldown <= 0.0f)
            {
                switch (extraEvent)
                {
                case DoorEvent::HandprintSlam: TriggerHandprintSlam(doorNearPlayer); break;
                case DoorEvent::Knock:         TriggerKnock(doorNearPlayer); break;
                case DoorEvent::ShadowWalk:    TriggerShadowWalk(doorNearPlayer); break;
                default: break;
                }
            }
        }
    }
}

// ============================================================
// DRAW
// ============================================================

void Doors_Draw(float camX, s8 floorNum, float textXoffset, float textY, bool dementia)
{
    if (!gQuadMesh) return;

    // dementia effect: draw many repeated doors
    int maxDoors = dementia ? 1000 : NUM_DOORS;

    for (int i = 0; i < maxDoors; ++i)
    {
        float doorX = DoorWorldX(i, camX);

        if (doorX < -SCREEN_WIDTH_HALF - DOOR_W) continue;
        if (doorX > SCREEN_WIDTH_HALF + DOOR_W) continue;

        int doorIdx = i % NUM_DOORS;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Door body
        if (gDoorTex)
            DrawTextureMesh(gQuadMesh, gDoorTex, doorX, DOOR_Y, DOOR_W, DOOR_H, 1.0f);

        // Window
        float winX, winY;
        WindowWorldCenter(doorX, winX, winY);

        DrawSquareMesh(gQuadMesh, winX, winY, WINDOW_W, WINDOW_H, 0xFF000000);

        if (gWindowBaseTex)
            DrawTextureMesh(gQuadMesh, gWindowBaseTex, winX, winY, WINDOW_W, WINDOW_H, 1.0f);

        // Door event visuals
        DoorEventState& s = gDoorState[doorIdx];

        // Shadow walk
        if (s.event == DoorEvent::ShadowWalk && s.shadow.active && gShadowTex)
        {
            float sx = winX + s.shadow.offsetX;

            DrawTextureMesh(
                gQuadMesh,
                gShadowTex,
                sx,
                winY,
                WINDOW_W * 0.95f,
                WINDOW_H * 0.95f,
                0.85f);
        }

        // Handprint slam (optional texture draw if you want it visible)
        if (s.event == DoorEvent::HandprintSlam && s.slam.active && gHandprintTex)
        {
            // simple pop timing: appear near middle of slam
            float alpha = 1.0f;
            if (s.slam.t < 0.08f) alpha = (s.slam.t / 0.08f);
            if (s.slam.t > s.slam.duration - 0.10f) alpha = (s.slam.duration - s.slam.t) / 0.10f;
            if (alpha < 0.0f) alpha = 0.0f;

            DrawTextureMesh(gQuadMesh, gHandprintTex, winX, winY, WINDOW_W, WINDOW_H, alpha);
        }

        // Door label text
        if (gDoorFontId >= 0)
        {
            char text[32]{};

            if (floorNum == 0)
                sprintf_s(text, "B1-%02d", i + 1);
            else
                sprintf_s(text, "%02d-%02d", floorNum, i + 1);

            float x = (doorX / SCREEN_WIDTH_HALF) - textXoffset;
            float y = textY / SCREEN_HEIGHT_HALF;

            AEGfxPrint(gDoorFontId, text, x, y, 1.0f, 1, 0, 0, 1);
        }
    }
}