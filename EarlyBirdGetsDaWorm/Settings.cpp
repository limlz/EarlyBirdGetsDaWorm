#include "pch.hpp"
#include "Settings.hpp"

// --- Internally Managed Assets ---
static AEGfxTexture* settingsCloseTex = nullptr;
static s8            settingsFontId = -1;

// --- State Variables ---
static bool  isOptionsOpen = false;
static float optionsOverlayAlpha = 0.0f;

// --- Audio Volumes ---
static float masterVolume = 1.0f;
static float bgmVolume = 1.0f;
static float sfxVolume = 1.0f;

// --- Dragging States ---
static bool  isDraggingMaster = false;
static bool  isDraggingBGM = false;
static bool  isDraggingSFX = false;

// --- Toggle States ---
static bool  horrorLessMode = false;
static bool  isFullscreen   = false;

// --- Slide Animation Variable ---
static float panelOffsetY = 800.0f; // Starts off-screen (high up)

static constexpr float SETTINGS_FADE_SPEED  = 4.0f;
static constexpr float SETTINGS_SLIDE_SPEED = 12.0f;

static AEGfxTexture* settingsBgOn  = nullptr;
static AEGfxTexture* settingsBgOff = nullptr;

void Settings_Load()
{
    settingsCloseTex = LoadTextureChecked(Assets::Main_Menu::CloseButton);
    settingsFontId   = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    settingsBgOn     = LoadTextureChecked(Assets::Main_Menu::SettingsBgOn);
    settingsBgOff    = LoadTextureChecked(Assets::Main_Menu::SettingsBgOff);
    isFullscreen     = AESysIsFullScreen() != 0;
}

void Settings_Initialize()
{
    isOptionsOpen       = false;
    optionsOverlayAlpha = 0.0f;

    isDraggingMaster = false;
    isDraggingBGM    = false;
    isDraggingSFX    = false;

    panelOffsetY = 800.0f;
    // horrorLessMode intentionally NOT reset so it persists across game sessions
}

void Settings_Update(float dt)
{
    // --- ANIMATIONS ---
    optionsOverlayAlpha += (isOptionsOpen ? 1.0f : -1.0f) * SETTINGS_FADE_SPEED * dt;
    optionsOverlayAlpha = AEClamp(optionsOverlayAlpha, 0.0f, 0.8f);

    float targetY = isOptionsOpen ? 0.0f : 800.0f;
    panelOffsetY += (targetY - panelOffsetY) * SETTINGS_SLIDE_SPEED * dt;

    if (!isOptionsOpen && optionsOverlayAlpha <= 0.0f) {
        return; // Fully closed
    }

    // --- HITBOXES (Account for the sliding Y position!) ---
    float mouseX = Input_GetMouseX();
    float mouseY = Input_GetMouseY();

    // Slider Hitboxes
    bool hoverMaster = IsAreaClicked(0.0f,  90.0f + panelOffsetY, 320.0f, 40.0f, mouseX, mouseY);
    bool hoverBGM    = IsAreaClicked(0.0f,  30.0f + panelOffsetY, 320.0f, 40.0f, mouseX, mouseY);
    bool hoverSFX    = IsAreaClicked(0.0f, -30.0f + panelOffsetY, 320.0f, 40.0f, mouseX, mouseY);

    // Horror-Less inline pill switch
    bool hoverHorrorToggle = IsAreaClicked(0.0f, -90.0f + panelOffsetY, 80.0f, 30.0f, mouseX, mouseY);

    // Fullscreen inline pill switch
    bool hoverFullscreenToggle = IsAreaClicked(0.0f, -150.0f + panelOffsetY, 80.0f, 30.0f, mouseX, mouseY);

    // CRT power button — closes settings and switches texture
    bool hoverPowerBtn = IsAreaClicked(310.0f, -290.0f + panelOffsetY, 50.0f, 50.0f, mouseX, mouseY);

    // --- INPUT LOGIC ---
    if (isOptionsOpen) {

        // Close via Escape or CRT power button
        if (AEInputCheckTriggered(AEVK_ESCAPE)
            || (AEInputCheckTriggered(AEVK_LBUTTON) && hoverPowerBtn)) {
            Settings_Close();
        }

        // Horror-Less pill switch
        if (AEInputCheckTriggered(AEVK_LBUTTON) && hoverHorrorToggle) {
            horrorLessMode = !horrorLessMode;
        }

        // Fullscreen pill switch
        if (AEInputCheckTriggered(AEVK_LBUTTON) && hoverFullscreenToggle) {
            isFullscreen = !isFullscreen;
            AESysSetFullScreen(isFullscreen ? 1 : 0);
        }

        // Drag initiation
        if (AEInputCheckTriggered(AEVK_LBUTTON)) {
            if      (hoverMaster) isDraggingMaster = true;
            else if (hoverBGM)    isDraggingBGM    = true;
            else if (hoverSFX)    isDraggingSFX    = true;
        }

        // Drag release
        if (AEInputCheckReleased(AEVK_LBUTTON)) {
            isDraggingMaster = false;
            isDraggingBGM    = false;
            isDraggingSFX    = false;
        }

        // Drag updates
        if (isDraggingMaster || isDraggingBGM || isDraggingSFX) {
            float normalizedVolume = AEClamp((mouseX + 150.0f) / 300.0f, 0.0f, 1.0f);

            if (isDraggingMaster) {
                masterVolume = normalizedVolume;
                AudioManager_SetBGMVolume(bgmVolume * masterVolume);
                AudioManager_SetSFXVolume(sfxVolume * masterVolume);
                AudioManager_SetWalkingVolume(sfxVolume * masterVolume);
            }
            else if (isDraggingBGM) {
                bgmVolume = normalizedVolume;
                AudioManager_SetBGMVolume(bgmVolume);
            }
            else if (isDraggingSFX) {
                sfxVolume = normalizedVolume;
                AudioManager_SetSFXVolume(sfxVolume);
                AudioManager_SetWalkingVolume(sfxVolume);
            }
        }
    }
}

// Helper: draw a single labeled slider
static void DrawSlider(AEGfxVertexList* squareMesh, float currentVolume, float yOffset,
                       const char* labelStr, s8 fontId, float panelOffsetY)
{
    float sliderY = yOffset + panelOffsetY;

    // Centered label above slider
    if (fontId >= 0) {
        float labelNdcY = (yOffset + 28.0f + panelOffsetY) / 450.0f;
        AEGfxPrintCentered(fontId, labelStr, 0.0f, labelNdcY, 0.8f, 0.75f, 0.95f, 0.75f, 1.0f);
    }

    float fillWidth  = 300.0f * currentVolume;
    float fillStartX = -150.0f + (fillWidth * 0.5f);
    float handleX    = -150.0f + (300.0f * currentVolume);

    // Track — slightly taller and brighter for visibility
    DrawSquareMesh(squareMesh, 0.0f, sliderY, 300.0f, 20.0f, 0xFF1A3A1A);

    // Fill bar
    if (fillWidth > 0.0f)
        DrawSquareMesh(squareMesh, fillStartX, sliderY, fillWidth, 20.0f, 0xFF5FAF5F);

    // Knob (red, matches CRT accent)
    DrawSquareMesh(squareMesh, handleX, sliderY, 16.0f, 32.0f, 0xFFCC2222);
}

void Settings_Draw(AEGfxVertexList* squareMesh)
{
    if (optionsOverlayAlpha <= 0.0f) return;

    // Black backdrop
    DrawBlackOverlay(squareMesh, optionsOverlayAlpha);

    // Background texture: On while open, Off while closing
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    AEGfxTexture* bgTex = isOptionsOpen ? settingsBgOn : settingsBgOff;
    if (bgTex)
        DrawTextureMesh(squareMesh, bgTex, 0.0f, panelOffsetY, 800.0f, 680.0f, 1.0f);

    // Title (centered)
    if (settingsFontId >= 0) {
        float titleNdcY = (160.0f + panelOffsetY) / 450.0f;
        AEGfxPrintCentered(settingsFontId, "SETTINGS", 0.0f, titleNdcY, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Three sliders
    DrawSlider(squareMesh, masterVolume,  90.0f, "Master Volume",           settingsFontId, panelOffsetY);
    DrawSlider(squareMesh, bgmVolume,     30.0f, "Background Music Volume", settingsFontId, panelOffsetY);
    DrawSlider(squareMesh, sfxVolume,    -30.0f, "Sound Effect Volume",     settingsFontId, panelOffsetY);

    // --- Horror-Less Mode label + inline pill switch ---
    if (settingsFontId >= 0) {
        float labelNdcY = (-65.0f + panelOffsetY) / 450.0f;
        AEGfxPrintCentered(settingsFontId, "Horror-Less Mode", 0.0f, labelNdcY, 0.8f, 0.75f, 0.95f, 0.75f, 1.0f);
    }

    // Pill track
    const float swX = 0.0f, swY = -90.0f + panelOffsetY;
    u32 trackColor = horrorLessMode ? 0xFF228B22 : 0xFF2A2A2A;
    DrawSquareMesh(squareMesh, swX, swY, 80.0f, 26.0f, trackColor);

    // Pill nub slides right (ON) or left (OFF)
    float nubX = swX + (horrorLessMode ? 22.0f : -22.0f);
    DrawSquareMesh(squareMesh, nubX, swY, 28.0f, 28.0f, 0xFFEEEEEE);

    // ON / OFF label on the opposite side of the nub
    if (settingsFontId >= 0) {
        float swNdcY   = (swY - 7.0f) / 450.0f;
        float textNdcX = (swX + (horrorLessMode ? -18.0f : 18.0f)) / 800.0f;
        AEGfxPrintCentered(settingsFontId, horrorLessMode ? "ON" : "OFF",
            textNdcX, swNdcY, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // --- Fullscreen Mode label + inline pill switch ---
    if (settingsFontId >= 0) {
        float labelNdcY = (-125.0f + panelOffsetY) / 450.0f;
        AEGfxPrintCentered(settingsFontId, "Fullscreen", 0.0f, labelNdcY, 0.8f, 0.75f, 0.95f, 0.75f, 1.0f);
    }

    const float fsX = 0.0f, fsY = -150.0f + panelOffsetY;
    u32 fsTrackColor = isFullscreen ? 0xFF228B22 : 0xFF2A2A2A;
    DrawSquareMesh(squareMesh, fsX, fsY, 80.0f, 26.0f, fsTrackColor);

    float fsNubX = fsX + (isFullscreen ? 22.0f : -22.0f);
    DrawSquareMesh(squareMesh, fsNubX, fsY, 28.0f, 28.0f, 0xFFEEEEEE);

    if (settingsFontId >= 0) {
        float fsNdcY = (fsY - 7.0f) / 450.0f;
        float fsNdcX = (fsX + (isFullscreen ? -18.0f : 18.0f)) / 800.0f;
        AEGfxPrintCentered(settingsFontId, isFullscreen ? "ON" : "OFF",
            fsNdcX, fsNdcY, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // --- CRT power button indicator (closes settings) ---
    // Glow ring
    DrawSquareMesh(squareMesh, 310.0f, -290.0f + panelOffsetY, 54.0f, 54.0f, 0xFF551111);
    // Button face (always red since it's the OFF/close button)
    DrawSquareMesh(squareMesh, 310.0f, -290.0f + panelOffsetY, 44.0f, 44.0f, 0xFF881111);
}

void Settings_Unload()
{
    UnloadTextureSafe(settingsCloseTex);
    UnloadTextureSafe(settingsBgOn);
    UnloadTextureSafe(settingsBgOff);
    if (settingsFontId >= 0) {
        AEGfxDestroyFont(settingsFontId);
        settingsFontId = -1;
    }
}

bool Settings_IsOpen()  { return isOptionsOpen; }
void Settings_Open()    { isOptionsOpen = true; }
void Settings_Close()
{
    isOptionsOpen    = false;
    isDraggingMaster = false;
    isDraggingBGM    = false;
    isDraggingSFX    = false;
}

float Settings_GetMasterVolume()     { return masterVolume; }
float Settings_GetBGMVolume()        { return bgmVolume; }
float Settings_GetSFXVolume()        { return sfxVolume; }
bool  Settings_IsHorrorLessEnabled() { return horrorLessMode; }
bool  Settings_IsPresentationModeEnabled()  { return Game_IsPresentationMode(); }
