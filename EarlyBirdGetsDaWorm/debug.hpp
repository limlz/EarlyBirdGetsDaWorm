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

// Functions
void Debug_Load();
void Debug_Update();
void Debug_Draw(const DebugInfo& info);
void Debug_Unload();
bool Debug_IsActive();