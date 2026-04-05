#pragma once

/******************************************************************************************
    @brief  Loads pause menu textures and button assets.
******************************************************************************************/
void PauseMenu_Load();

/******************************************************************************************
    @brief  Resets pause menu state and button scales to defaults.
******************************************************************************************/
void PauseMenu_Initialize();

/******************************************************************************************
    @brief  Handles pause menu input (Resume, Options, Quit) and hover animations.

    @param  dt
            Frame delta time in seconds.
******************************************************************************************/
void PauseMenu_Update(float dt);

/******************************************************************************************
    @brief  Renders the pause menu overlay with darkened background and buttons.

    @param  squareMesh
            The square mesh used for drawing the overlay and buttons.
******************************************************************************************/
void PauseMenu_Draw(AEGfxVertexList* squareMesh);

/******************************************************************************************
    @brief  Unloads all pause menu textures and resources from memory.
******************************************************************************************/
void PauseMenu_Unload();

/******************************************************************************************
    @brief  Queries whether the game is currently paused.

    @return true if the pause menu is active.
******************************************************************************************/
bool PauseMenu_IsPaused();

/******************************************************************************************
    @brief  Programmatically sets the pause state (e.g., to unpause before a boss fight).

    @param  state
            true to pause, false to unpause.
******************************************************************************************/
void PauseMenu_SetPaused(bool state);
