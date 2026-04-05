#pragma once

/**
 * @brief Loads assets needed by the prompt system.
 */
void Prompts_Load();

/**
 * @brief Updates prompt visibility and animation each frame.
 * @param dt             Delta time in seconds since the last frame.
 * @param camX           Current camera X position in world space.
 * @param doorNumAtPlayer Door number the player is currently standing at.
 * @param liftMenuOpen   Whether the lift menu is currently open.
 * @param isLiftRange    Whether the player is within range of the lift.
 */
void Prompts_Update(float dt, float camX, int doorNumAtPlayer, bool liftMenuOpen, bool isLiftRange);

/**
 * @brief Triggers the wrong-room prompt to notify the player.
 */
void Prompts_TriggerWrongRoom();

/**
 * @brief Draws all active prompts to the screen.
 */
void Prompts_Draw();

/**
 * @brief Unloads all assets used by the prompt system.
 */
void Prompts_Unload();
