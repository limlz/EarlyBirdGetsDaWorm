// Debug.hpp
#pragma once

struct DebugInfo {
    f32 camX;
    bool left_right;
    int day;
    s8 floorNum;
    bool dementia;
    s8 doorNumAtPlayer;
    s8 patientDoorNum;
    s8 patientFloorNum;
    s8 demonFloorNum;
    s8 demonRoomNum;
};

// Functions
void Debug_Load();
void Debug_Update();
void Debug_Draw(const DebugInfo& info);
void Debug_Unload();