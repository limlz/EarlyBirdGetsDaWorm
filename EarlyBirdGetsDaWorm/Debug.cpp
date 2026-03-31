// Debug.cpp
#include "pch.hpp" 

// Internal Debug Variables
static bool gDebugMode = false;
static s8 gDebugFont = -1;
static std::string gDebugBuffer;

void Debug_Load()
{
    // Load your font here
    gDebugFont = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
}

void Debug_Update()
{
    // Toggle Debug Mode
    if (AEInputCheckTriggered(AEVK_F3)) {
        gDebugMode = !gDebugMode;
    }

    if (!gDebugMode) return;

    // F4: Pause / unpause the in-game timer
    if (AEInputCheckTriggered(AEVK_F4)) {
        Timer_SetPaused(!Timer_IsPaused());
    }

    // H: Toggle ghost / human patient (only while carrying a patient)
    if (AEInputCheckTriggered(AEVK_H) && Player_HasPatient()) {
        if (IsGhostPatient()) Player_ForceHumanPatient();
        else                  Player_ForceGhostPatient();
        SoundAnomaly.Initialize(); // reset audio state for new patient type
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

    switch (Player_GetTrueDiagnosis()) {
    case ILLNESSES::PARANOIA:
        illness = "PARANOIA";
        break;
    case ILLNESSES::MANIA:
        illness = "MANIA";
        break;
    case ILLNESSES::DEPRESSION:
        illness = "DEPRESSION";
        break;
    case ILLNESSES::DEMENTIA:
        illness = "DEMENTIA";
        break;
    case ILLNESSES::SCHIZOPHRENIA:
        illness = "SCHIZOPHRENIA";
        break;
    case ILLNESSES::AIW_SYNDROME:
        illness = "AIW SYNDROME";
        break;
    case ILLNESSES::INSOMNIA:
        illness = "INSOMNIA";
        break;
    case ILLNESSES::OCD:
        illness = "OCD";
        break;
    case ILLNESSES::SCOTOPHOBIA:
        illness = "SCOTOPHOBIA";
        break;
    case ILLNESSES::NONE:
        illness = "NO PATIENT YET";
        break;
    case ILLNESSES::GHOST:
        illness = "GHOST";
        break;
    default:
        illness = "UNKNOWN";
        break;
    }
    // Calculate World Mouse using info struct
    float worldMouseX = -info.camX + (static_cast<float>(mouseX) - 800.0f);
    float worldMouseY = -(static_cast<float>(mouseY) - 450.0f);

    // 2. Setup Text Drawing (Bottom Left)
    // startY accounts for 7 rows at lineHeight 0.06: bottom row lands at ~-0.93 NDC (safely on screen)
    f32 textScale = 1.0f;
    f32 startX = -0.98f;
    f32 startY = -0.62f;  // shifted down 1.5 rows from the previous -0.48
    f32 lineHeight = 0.06f;

    // 3. Print Info Block

    // --- MOUSE & CAMERA (merged onto one row) ---
    {
        std::ostringstream oss;
        oss << "MOUSE Scrn: [" << mouseX << ", " << mouseY
            << "] | World: [" << std::fixed << std::setprecision(0) << worldMouseX << ", " << worldMouseY
            << "] | CAM X: " << std::setprecision(2) << info.camX;
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY, textScale, 0.0f, 1.0f, 0.0f, 1.0f);

    // --- GAME LOGIC ---
    {
        std::ostringstream oss;
        oss << "STATE: Day " << info.day << " | Floor: " << static_cast<int>(info.floorNum) << " | Current Illness: " << illness;
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 1), textScale, 1.0f, 1.0f, 0.0f, 1.0f);

    {
        std::ostringstream oss;
        oss << "DOOR: At: " << static_cast<int>(info.doorNumAtPlayer)
            << " | Patient Target: Rm " << static_cast<int>(info.patientDoorNum)
            << " (Flr " << static_cast<int>(info.patientFloorNum)
            << ") | Scary: " << (IsGhostPatient() ? "GHOST" : "HUMAN");
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 2), textScale, 1.0f, 1.0f, 0.0f, 1.0f);

    // --- ANOMALY SYSTEMS ---
    {
        std::ostringstream oss;
        oss << "ANOMALIES: FRAMES:" << (CentralPool_IsSystemActive(AnomalyFiles::Frames_isActive)   ? "ON" : "--")
            << "  DOORS:" << (CentralPool_IsSystemActive(AnomalyFiles::Doors_isActive)    ? "ON" : "--")
            << "  LIGHT:" << (CentralPool_IsSystemActive(AnomalyFiles::Lighting_isActive) ? "ON" : "--")
            << "  WALL:" << (CentralPool_IsSystemActive(AnomalyFiles::Wall_isActive)     ? "ON" : "--")
            << "  SOUND:" << (CentralPool_IsSystemActive(AnomalyFiles::Sound_isActive)    ? "ON" : "--")
            << "  LIFT:" << (CentralPool_IsSystemActive(AnomalyFiles::Lift_isActive)     ? "ON" : "--")
            << "  | DARK:" << (Lighting_IsPlayerInDarkness(info.camX, info.floorNum)       ? "YES" : "NO");
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 3), textScale, 1.0f, 0.5f, 0.0f, 1.0f);

    // --- SYSTEM ---
    f32 fps = 1.0f / static_cast<f32>(AEFrameRateControllerGetFrameTime());
    {
        std::ostringstream oss;
        oss << "FPS: " << std::fixed << std::setprecision(2) << fps
            << " | DT: " << std::setprecision(5) << static_cast<f32>(AEFrameRateControllerGetFrameTime())
            << " | TIMER: " << (Timer_IsPaused() ? "PAUSED" : "RUNNING");
        gDebugBuffer = oss.str();
    }
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 4), textScale, 0.0f, 1.0f, 1.0f, 1.0f);

    // --- DEBUG CONTROLS HINT (two rows so text stays on screen) ---
    gDebugBuffer = "[1-9] Illness  [0] Re-roll  [H] Ghost/Human  [P] Auto Patient  [K] Next Day";
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 5), textScale, 0.5f, 0.5f, 0.5f, 1.0f);

    gDebugBuffer = "[F3] Toggle Debug  [F4] Pause Timer  [F6] Set Pts=5  [J] Jumpscare  [M] Warp Cam";
    AEGfxPrint(gDebugFont, gDebugBuffer.c_str(), startX, startY - (lineHeight * 6), textScale, 0.5f, 0.5f, 0.5f, 1.0f);
}

bool Debug_IsActive()
{
    return gDebugMode;
}

void Debug_Unload()
{
    if (gDebugFont != -1) {
        AEGfxDestroyFont(gDebugFont);
        gDebugFont = -1;
    }
}