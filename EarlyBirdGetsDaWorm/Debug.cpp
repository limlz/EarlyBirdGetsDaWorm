// Debug.cpp
#include "pch.hpp" 

// Internal Debug Variables
static bool gDebugMode = false;
static s8 gDebugFont = -1;
static char gDebugBuffer[256];

void Debug_Load()
{
    // Load your font here
    gDebugFont = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
}

void Debug_Update()
{
    // Toggle Debug Mode
    if (AEInputCheckTriggered(AEVK_F3)) {
        gDebugMode = !gDebugMode;
    }
}

void Debug_Draw(const DebugInfo& info)
{
    if (!gDebugMode) return;

    // 1. Get Mouse Info (Global function, doesn't need struct)
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    // 1. Declare a pointer to a constant character (string literal)
    const char* illness = "";

    switch (Player_GetCurrentIllness()) {
    case 0:
        illness = "PARANOIA";
        break;
    case 1:
        illness = "MANIA";
        break;
    case 2:
        illness = "DEPRESSION";
        break;
    case 3:
        illness = "DEMENTIA";
        break;
    case 4:
        illness = "ALL/GHOST";
        break;
    default:
        illness = "UNKNOWN"; // Good practice to handle unexpected values
        break;
    }
    // Calculate World Mouse using info struct
    float worldMouseX = -info.camX + ((float)mouseX - 800.0f);
    float worldMouseY = -((float)mouseY - 450.0f);

    // 2. Setup Text Drawing (Bottom Left)
    f32 textScale = 1.0f;
    f32 startX = -0.98f;
    f32 startY = -0.60f;
    f32 lineHeight = 0.06f;

    // 3. Print Info Block 

    // --- MOUSE & CAMERA ---
    sprintf_s(gDebugBuffer, "MOUSE Scrn: [%d, %d] | World: [%.0f, %.0f]", mouseX, mouseY, worldMouseX, worldMouseY);
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY, textScale, 0.0f, 1.0f, 0.0f, 1.0f);

    sprintf_s(gDebugBuffer, "CAM X: %.2f | Player Facing: %s", info.camX, info.left_right ? "RIGHT" : "LEFT");
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 1), textScale, 0.0f, 1.0f, 0.0f, 1.0f);

    // --- GAME LOGIC ---
    sprintf_s(gDebugBuffer, "STATE: Day %d | Floor: %d | Dementia: %s | Current Illness: %s", info.day, info.floorNum, info.dementia ? "ON" : "OFF", illness);
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 2), textScale, 1.0f, 1.0f, 0.0f, 1.0f);

    sprintf_s(gDebugBuffer, "DOOR: At: %d | Patient Target: Rm %d (Flr %d)", info.doorNumAtPlayer, info.patientDoorNum, info.patientFloorNum);
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 3), textScale, 1.0f, 1.0f, 0.0f, 1.0f);

    // --- BOSS INFO ---
    sprintf_s(gDebugBuffer, "DEMON: Floor: %d | Room: %d", info.demonFloorNum, info.demonRoomNum);
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 4), textScale, 1.0f, 0.5f, 0.5f, 1.0f);

    // --- SYSTEM ---
    f32 fps = 1.0f / (f32)AEFrameRateControllerGetFrameTime();
    sprintf_s(gDebugBuffer, "FPS: %.2f | DT: %.5f", fps, (f32)AEFrameRateControllerGetFrameTime());
    AEGfxPrint(gDebugFont, gDebugBuffer, startX, startY - (lineHeight * 5), textScale, 0.0f, 1.0f, 1.0f, 1.0f);
}

void Debug_Unload()
{
    if (gDebugFont != -1) {
        AEGfxDestroyFont(gDebugFont);
        gDebugFont = -1;
    }
}