#pragma once

/******************************************************************************************
    @brief  Loads all boss fight textures, meshes, fonts, and combat assets.
******************************************************************************************/
void Boss_Fight_Load();

/******************************************************************************************
    @brief  Resets the boss fight state: player position, health, boss entity,
            bullet pools, and horror phase timers.
******************************************************************************************/
void Boss_Fight_Initialize();

/******************************************************************************************
    @brief  Runs the boss fight game loop: player input, boss AI, bullet physics,
            collision detection, and victory/defeat conditions.
******************************************************************************************/
void Boss_Fight_Update();

/******************************************************************************************
    @brief  Renders the boss fight scene: arena, boss, bullets, player, UI overlays,
            and horror visual effects.
******************************************************************************************/
void Boss_Fight_Draw();

/******************************************************************************************
    @brief  Releases session-specific boss fight data and stops all audio channels.
******************************************************************************************/
void Boss_Fight_Free();

/******************************************************************************************
    @brief  Unloads all boss fight textures, meshes, and font resources from memory.
******************************************************************************************/
void Boss_Fight_Unload();

/******************************************************************************************
    @brief  Rolls the dodge chance and repositions the boss if the cooldown allows.
            Called by Bullets.cpp when a player bullet actually hits the boss.
******************************************************************************************/
void Boss_Fight_TryDodge();
