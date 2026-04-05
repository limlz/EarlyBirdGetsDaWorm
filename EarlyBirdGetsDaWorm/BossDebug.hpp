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

/******************************************************************************************
    @brief  Loads the debug overlay font and initializes render resources.
******************************************************************************************/
void BossDebug_Load();

/******************************************************************************************
    @brief  Polls input to toggle the debug overlay and god mode, and snapshots
            the current boss state for display.

    @param  boss    The boss entity whose state is being inspected.
******************************************************************************************/
void BossDebug_Update(Boss& boss);

/******************************************************************************************
    @brief  Renders the debug text overlay showing player and boss telemetry.

    @param  info    A snapshot of all debug-relevant player and boss values.
******************************************************************************************/
void BossDebug_Draw(const BossDebugInfo& info);

/******************************************************************************************
    @brief  Destroys the debug font and releases render resources.
******************************************************************************************/
void BossDebug_Unload();

/******************************************************************************************
    @brief  Queries whether the debug overlay is currently visible.

    @return true if the debug overlay is toggled on.
******************************************************************************************/
bool BossDebug_IsActive();

/******************************************************************************************
    @brief  Queries whether god mode (invincibility) is enabled.

    @return true if the player cannot take damage.
******************************************************************************************/
bool BossDebug_IsGodMode();
