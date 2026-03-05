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

// --- Slide Animation Variable ---
static float panelOffsetY = 800.0f; // Starts off-screen (high up)

static constexpr float SETTINGS_FADE_SPEED = 4.0f;
static constexpr float SETTINGS_SLIDE_SPEED = 12.0f;

void Settings_Load()
{
    // Load the assets specifically needed for the Settings Panel
    settingsCloseTex = LoadTextureChecked(Assets::Main_Menu::CloseButton);
    settingsFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
}

void Settings_Initialize()
{
    isOptionsOpen = false;
    optionsOverlayAlpha = 0.0f;

    isDraggingMaster = false;
    isDraggingBGM = false;
    isDraggingSFX = false;

    panelOffsetY = 800.0f;
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

    // Close Button Hitbox
    bool hoverClose = IsAreaClicked(250.0f, 150.0f + panelOffsetY, 50.0f, 50.0f, mouseX, mouseY);

    // Slider Hitboxes (Stacked vertically)
    // Master is at Y = 20, BGM is at Y = -40, SFX is at Y = -100
    bool hoverMaster = IsAreaClicked(0.0f, 20.0f + panelOffsetY, 320.0f, 40.0f, mouseX, mouseY);
    bool hoverBGM = IsAreaClicked(0.0f, -40.0f + panelOffsetY, 320.0f, 40.0f, mouseX, mouseY);
    bool hoverSFX = IsAreaClicked(0.0f, -100.0f + panelOffsetY, 320.0f, 40.0f, mouseX, mouseY);

    // --- INPUT LOGIC ---
    if (isOptionsOpen) {

        // Handle Closing
        if (AEInputCheckTriggered(AEVK_ESCAPE) || (AEInputCheckTriggered(AEVK_LBUTTON) && hoverClose)) {
            Settings_Close();
        }

        // Handle Drag Initiation
        if (AEInputCheckTriggered(AEVK_LBUTTON)) {
            if (hoverMaster) isDraggingMaster = true;
            else if (hoverBGM) isDraggingBGM = true;
            else if (hoverSFX) isDraggingSFX = true;
        }

        // Handle Drag Release
        if (AEInputCheckReleased(AEVK_LBUTTON)) {
            isDraggingMaster = false;
            isDraggingBGM = false;
            isDraggingSFX = false;
        }

        // Handle Drag Updates
        if (isDraggingMaster || isDraggingBGM || isDraggingSFX) {
            // Convert screen X to a 0.0 to 1.0 ratio
            float normalizedVolume = AEClamp((mouseX + 150.0f) / 300.0f, 0.0f, 1.0f);

            if (isDraggingMaster) {
                masterVolume = normalizedVolume;
				AudioManager_SetBGMVolume(bgmVolume * masterVolume);
				AudioManager_SetSFXVolume(sfxVolume * masterVolume);
            }
            else if (isDraggingBGM) {
                bgmVolume = normalizedVolume;
                AudioManager_SetBGMVolume(bgmVolume);
            }
            else if (isDraggingSFX) {
                sfxVolume = normalizedVolume;
                AudioManager_SetSFXVolume(sfxVolume);
            }
        }
    }
}

// Helper function to draw a single slider
static void DrawSlider(AEGfxVertexList* squareMesh, float currentVolume, float yOffset, const char* labelStr, s8 fontId, float panelOffsetY)
{
    float sliderY = yOffset + panelOffsetY;

    // Draw the text label just above the slider
    if (fontId >= 0) {
        float labelNdcX = (-150.0f) / 800.0f;
        float labelNdcY = (yOffset + 25.0f + panelOffsetY) / 450.0f;
        AEGfxPrint(fontId, labelStr, labelNdcX, labelNdcY, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Calculate dynamic slider math
    float fillWidth = 300.0f * currentVolume;
    float fillStartX = -150.0f + (fillWidth / 2.0f);
    float handleX = -150.0f + (300.0f * currentVolume);

    // Track Background (Dark)
    DrawSquareMesh(squareMesh, 0.0f, sliderY, 300.0f, 15.0f, 0xFF111111);

    // Fill Bar (White)
    if (fillWidth > 0.0f) {
        DrawSquareMesh(squareMesh, fillStartX, sliderY, fillWidth, 15.0f, COLOR_WHITE);
    }

    // Draggable Knob (Blue)
    DrawSquareMesh(squareMesh, handleX, sliderY, 15.0f, 30.0f, 0xFF0000FF);
}

void Settings_Draw(AEGfxVertexList* squareMesh)
{
    if (optionsOverlayAlpha <= 0.0f) return;

    // 1. Draw Black Backdrop
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(optionsOverlayAlpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // 2. Draw UI Elements
    AEGfxSetTransparency(1.0f);

    // Solid Backing Panel (Made it slightly taller to fit 3 sliders)
    DrawSquareMesh(squareMesh, 0.0f, panelOffsetY, 600.0f, 450.0f, 0xFF2A2A2A);
    DrawSquareMesh(squareMesh, 0.0f, 175.0f + panelOffsetY, 600.0f, 100.0f, 0xFF3A3A3A); // Header bar

    // Close Button
    if (settingsCloseTex) {
        DrawTextureMesh(squareMesh, settingsCloseTex, 250.0f, 175.0f + panelOffsetY, 40.0f, 40.0f, 1.0f);
    }
    else {
        DrawSquareMesh(squareMesh, 250.0f, 175.0f + panelOffsetY, 40.0f, 40.0f, 0xFFD32F2F);
    }

    // Title Text
    if (settingsFontId >= 0) {
        float titleNdcY = (160.0f + panelOffsetY) / 450.0f;
        AEGfxPrint(settingsFontId, "SETTINGS", (-100.0f / 800.0f), titleNdcY, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Draw the 3 Sliders using the helper function
    DrawSlider(squareMesh, masterVolume, 20.0f, "Master Volume", settingsFontId, panelOffsetY);
    DrawSlider(squareMesh, bgmVolume, -40.0f, "Background Music Volume", settingsFontId, panelOffsetY);
    DrawSlider(squareMesh, sfxVolume, -100.0f, "Sound Effect Volume", settingsFontId, panelOffsetY);
}

void Settings_Unload()
{
    UnloadTextureSafe(settingsCloseTex);
    if (settingsFontId >= 0) {
        AEGfxDestroyFont(settingsFontId);
        settingsFontId = -1;
    }
}

bool Settings_IsOpen() { return isOptionsOpen; }
void Settings_Open() { isOptionsOpen = true; }
void Settings_Close() {
    isOptionsOpen = false;
    isDraggingMaster = false;
    isDraggingBGM = false;
    isDraggingSFX = false;
}

float Settings_GetMasterVolume() { return masterVolume; }
float Settings_GetBGMVolume() { return bgmVolume; }
float Settings_GetSFXVolume() { return sfxVolume; }