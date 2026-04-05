#pragma once
#include "pch.hpp"

/**
 * @brief Loads assets required by the settings menu.
 */
void Settings_Load();

/**
 * @brief Unloads all assets used by the settings menu.
 */
void Settings_Unload();

/**
 * @brief Initializes the settings menu state to its defaults.
 */
void Settings_Initialize();

/**
 * @brief Updates the settings menu logic each frame.
 * @param dt Delta time in seconds since the last frame.
 */
void Settings_Update(float dt);

/**
 * @brief Draws the settings menu overlay.
 * @param squareMesh Pointer to the square mesh used for rendering UI elements.
 */
void Settings_Draw(AEGfxVertexList* squareMesh);

/**
 * @brief Checks whether the settings menu is currently open.
 * @return True if the settings menu is open, false otherwise.
 */
bool Settings_IsOpen();

/**
 * @brief Opens the settings menu.
 */
void Settings_Open();

/**
 * @brief Closes the settings menu.
 */
void Settings_Close();

/**
 * @brief Gets the current master volume level.
 * @return Master volume as a float in the range [0.0, 1.0].
 */
float Settings_GetMasterVolume();

/**
 * @brief Gets the current background music volume level.
 * @return BGM volume as a float in the range [0.0, 1.0].
 */
float Settings_GetBGMVolume();

/**
 * @brief Gets the current sound effects volume level.
 * @return SFX volume as a float in the range [0.0, 1.0].
 */
float Settings_GetSFXVolume();

/**
 * @brief Checks whether horror-less mode is enabled.
 * @return True if horror-less mode is enabled, false otherwise.
 */
bool  Settings_IsHorrorLessEnabled();

/**
 * @brief Checks whether presentation mode is enabled.
 * @return True if presentation mode is enabled, false otherwise.
 */
bool  Settings_IsPresentationModeEnabled();
