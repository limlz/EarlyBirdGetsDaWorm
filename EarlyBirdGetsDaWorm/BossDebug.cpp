// BossDebug.cpp
#include "pch.hpp"
#include "BossDebug.hpp"

static bool gBossDebugMode = false;
static bool gGodMode       = false;
static s8   gDebugFont     = -1;
static std::string gDebugBuffer;

void BossDebug_Load()
{
    gDebugFont = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
}

void BossDebug_Update(Boss& boss)
{
    if (AEInputCheckTriggered(AEVK_F3))
        gBossDebugMode = !gBossDebugMode;

    if (!gBossDebugMode) return;

    // G: set boss HP to 1 (near-death)
    if (AEInputCheckTriggered(AEVK_G))
        boss.health = 1;

    // I: toggle god mode (player takes no damage)
    if (AEInputCheckTriggered(AEVK_I))
        gGodMode = !gGodMode;

    // N: toggle boss shield on/off
    if (AEInputCheckTriggered(AEVK_N))
        boss.shieldActive = !boss.shieldActive;

    // B: clear all active boss bullets
    if (AEInputCheckTriggered(AEVK_B))
        for (int i = 0; i < MAX_BOSS_BULLETS; i++)
            boss.bullets[i].active = false;
}

void BossDebug_Draw(const BossDebugInfo& info)
{
    if (!gBossDebugMode) return;

    const f32 textScale  = 1.0f;
    const f32 startX     = -0.98f;
    const f32 startY     = -0.55f;
    const f32 lineHeight = 0.06f;

    const char* state_str = info.boss_state == 0 ? "NORMAL"
                          : info.boss_state == 1 ? "SPREAD"
                          : info.boss_state == 2 ? "SHIELD"
                          : "UNKNOWN";

    // Row 0 (green) — Player position, velocity, health, flags
    {
        std::ostringstream oss;
        oss << "PLAYER: Pos[" << std::fixed << std::setprecision(0) << info.player_x << ", " << info.player_y
            << "] | Vel[" << info.player_vel_x << ", " << info.player_vel_y
            << "] | HP:" << info.player_hp << "/" << info.player_hp_max
            << " | GRND:" << (info.is_grounded ? "YES" : "NO")
            << " CROUCH:" << (info.is_crouching ? "YES" : "NO");
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY, textScale, 0.0f, 1.0f, 0.0f, 1.0f);

    // Row 1 (yellow) — Boss position, health, state
    {
        std::ostringstream oss;
        oss << "BOSS: Pos[" << std::fixed << std::setprecision(0) << info.boss_x << ", " << info.boss_y
            << "] Base:" << info.boss_base_y
            << " | HP:" << info.boss_health << "/" << info.boss_max_health
            << " | State:" << state_str
            << " Timer:" << std::setprecision(1) << info.boss_state_timer;
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - lineHeight, textScale, 1.0f, 1.0f, 0.0f, 1.0f);

    // Row 2 (orange) — Boss shield, dodge cooldown, bullet count, god mode
    {
        std::ostringstream oss;
        oss << "BOSS: Shield:" << (info.boss_shield_active ? "ON" : "OFF")
            << " | DodgeCD:" << std::fixed << std::setprecision(2) << info.boss_dodge_cooldown
            << " | Active Bullets:" << info.active_bullet_count
            << " | GOD MODE:" << (gGodMode ? "ON" : "OFF");
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 2), textScale, 1.0f, 0.5f, 0.0f, 1.0f);

    // Row 3 (cyan) — FPS / DT
    f32 fps = 1.0f / static_cast<f32>(AEFrameRateControllerGetFrameTime());
    {
        std::ostringstream oss;
        oss << "FPS: " << std::fixed << std::setprecision(2) << fps
            << " | DT: " << std::setprecision(5) << static_cast<f32>(AEFrameRateControllerGetFrameTime());
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 3), textScale, 0.0f, 1.0f, 1.0f, 1.0f);

    // Row 4 (grey) — Controls hint
    gDebugBuffer = "[F3] Toggle  [G] Boss HP->1  [I] God Mode  [N] Toggle Shield  [B] Clear Bullets";
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 4), textScale, 0.5f, 0.5f, 0.5f, 1.0f);
}

bool BossDebug_IsActive() { return gBossDebugMode; }
bool BossDebug_IsGodMode() { return gGodMode; }

void BossDebug_Unload()
{
    if (gDebugFont != -1) {
        AEGfxDestroyFont(gDebugFont);
        gDebugFont = -1;
    }
}
