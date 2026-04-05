// Debug.hpp
#pragma once

struct DebugInfo {
    f32 camX;
    int day;
    s8 floorNum;
    s8 doorNumAtPlayer;
    s8 patientDoorNum;
    s8 patientFloorNum;
};

/******************************************************************************************
    @brief  Loads the debug overlay font and initializes render resources.
******************************************************************************************/
void Debug_Load();

/******************************************************************************************
    @brief  Polls input to toggle the debug overlay on or off.
******************************************************************************************/
void Debug_Update();

/******************************************************************************************
    @brief  Renders the debug text overlay showing camera, floor, door, and patient data.

    @param  info
            A snapshot of all debug-relevant game state values.
******************************************************************************************/
void Debug_Draw(const DebugInfo& info);

/******************************************************************************************
    @brief  Destroys the debug font and releases render resources.
******************************************************************************************/
void Debug_Unload();

/******************************************************************************************
    @brief  Queries whether the debug overlay is currently toggled on.

    @return true if the debug overlay is visible.
******************************************************************************************/
bool Debug_IsActive();
