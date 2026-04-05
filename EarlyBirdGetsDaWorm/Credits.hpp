#pragma once
#include "pch.hpp"

/******************************************************************************************
    @brief  Loads all credits-related textures, fonts, and parses the credits text file.
******************************************************************************************/
void Credits_Load();

/******************************************************************************************
    @brief  Unloads all credits textures and font resources from memory.
******************************************************************************************/
void Credits_Unload();

/******************************************************************************************
    @brief  Resets credits scroll position, animation timers, and scene character states.
******************************************************************************************/
void Credits_Initialize();

/******************************************************************************************
    @brief  Updates credits scroll, character animations, and close-button hover logic.

    @param  dt
            Frame delta time in seconds.
******************************************************************************************/
void Credits_Update(float dt);

/******************************************************************************************
    @brief  Draws the animated side character on the main menu screen.

    @param  mesh
            The square mesh used for rendering the character sprite.
******************************************************************************************/
void Credits_DrawCharacter(AEGfxVertexList* mesh);

/******************************************************************************************
    @brief  Draws the full-screen scrolling credits overlay with all parsed credit lines.

    @param  mesh
            The square mesh used for rendering the overlay background.
******************************************************************************************/
void Credits_DrawOverlay(AEGfxVertexList* mesh);

/******************************************************************************************
    @brief  Queries whether the credits overlay is currently visible.

    @return true if the credits screen is open.
******************************************************************************************/
bool Credits_IsOpen();

/******************************************************************************************
    @brief  Queries whether the mouse is hovering over the credits button.

    @return true if hovering.
******************************************************************************************/
bool Credits_IsHovering();

/******************************************************************************************
    @brief  Opens the credits overlay and resets the scroll position to the start.
******************************************************************************************/
void Credits_Open();

/******************************************************************************************
    @brief  Releases the static parsedCredits vector buffer before the CRT leak check runs.
            Call once at program shutdown.
******************************************************************************************/
void Credits_Shutdown();
