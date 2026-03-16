// BossDebug.cpp
#include "pch.hpp"
#include "BossDebug.hpp"

static bool gBossDebugMode = false;
static bool gGodMode       = false;
static s8   gDebugFont     = -1;
static char gDebugBuffer[256];

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
    sprintf_s(gDebugBuffer, "PLAYER: Pos[%.0f, %.0f] | Vel[%.0f, %.0f] | HP:%d/%d | GRND:%s CROUCH:%s",
        info.player_x, info.player_y,
        info.player_vel_x, info.player_vel_y,
        info.player_hp, info.player_hp_max,
        info.is_grounded  ? "YES" : "NO",
        info.is_crouching ? "YES" : "NO");
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY, textScale, 0.0f, 1.0f, 0.0f, 1.0f);

    // Row 1 (yellow) — Boss position, health, state
    sprintf_s(gDebugBuffer, "BOSS: Pos[%.0f, %.0f] Base:%.0f | HP:%d/%d | State:%s Timer:%.1f",
        info.boss_x, info.boss_y, info.boss_base_y,
        info.boss_health, info.boss_max_health,
        state_str, info.boss_state_timer);
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - lineHeight, textScale, 1.0f, 1.0f, 0.0f, 1.0f);

    // Row 2 (orange) — Boss shield, dodge cooldown, bullet count, god mode
    sprintf_s(gDebugBuffer, "BOSS: Shield:%s | DodgeCD:%.2f | Active Bullets:%d | GOD MODE:%s",
        info.boss_shield_active ? "ON" : "OFF",
        info.boss_dodge_cooldown,
        info.active_bullet_count,
        gGodMode ? "ON" : "OFF");
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 2), textScale, 1.0f, 0.5f, 0.0f, 1.0f);

    // Row 3 (cyan) — FPS / DT
    f32 fps = 1.0f / (f32)AEFrameRateControllerGetFrameTime();
    sprintf_s(gDebugBuffer, "FPS: %.2f | DT: %.5f", fps, (f32)AEFrameRateControllerGetFrameTime());
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 3), textScale, 0.0f, 1.0f, 1.0f, 1.0f);

    // Row 4 (grey) — Controls hint
    sprintf_s(gDebugBuffer, "[F3] Toggle  [G] Boss HP->1  [I] God Mode  [N] Toggle Shield  [B] Clear Bullets");
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 4), textScale, 0.5f, 0.5f, 0.5f, 1.0f);
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
