// BossDebug.hpp
#pragma once
#include "boss.hpp"

struct BossDebugInfo {
    float player_x, player_y;
    float player_vel_x, player_vel_y;
    int   player_hp;
    int   player_hp_max;
    bool  is_grounded;
    bool  is_crouching;

    int   boss_health;
    int   boss_max_health;
    float boss_x, boss_y, boss_base_y;
    int   boss_state;
    float boss_state_timer;
    bool  boss_shield_active;
    float boss_dodge_cooldown;
    int   active_bullet_count;
};

void BossDebug_Load();
void BossDebug_Update(Boss& boss);
void BossDebug_Draw(const BossDebugInfo& info);
void BossDebug_Unload();
bool BossDebug_IsActive();
bool BossDebug_IsGodMode();
