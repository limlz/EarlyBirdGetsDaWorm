#include "pch.hpp"
#include "Endgame.hpp"

// Define the actual global variable here so the linker can find it
EndGameReason currentEndReason = REASON_SURVIVED_5_DAYS;

static s8 endgameFontId = -1;
static s8 subFontId = -1;

// Fade transitions
static bool isFadingIn = true;
static bool isFadingToMenu = false;
static float fadeAlpha = 1.0f;
const float FADE_SPEED = 1.5f;

static AEGfxVertexList* squareMesh = nullptr;

void Endgame_Load()
{
    // Lowered from 50 to 35 to prevent AlphaEngine from overflowing its memory buffer
    endgameFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 25);

    subFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 25);
}

void Endgame_Initialize()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);

    isFadingIn = true;
    isFadingToMenu = false;
    fadeAlpha = 1.0f; // Start completely black

    // Set background color based on HOW they finished the game
    if (currentEndReason == REASON_SURVIVED_5_DAYS) {
        AEGfxSetBackgroundColor(0.0f, 0.2f, 0.1f); // Dark Greenish/Blue (Victory)
    }
    else {
        AEGfxSetBackgroundColor(0.3f, 0.0f, 0.0f); // Dark Red (Game Over)
    }
}

void Endgame_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    // 1. Handle Fade In
    if (isFadingIn) {
        fadeAlpha -= FADE_SPEED * dt;
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            isFadingIn = false;
        }
        return; // Don't allow clicking while fading in
    }

    // 2. Handle Fade Out (Returning to Menu)
    if (isFadingToMenu) {
        fadeAlpha += FADE_SPEED * dt;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            next = MAIN_MENU; // Change this to whatever your Main Menu state is named!
        }
        return;
    }

    // 3. Wait for Player Input to leave
    if (AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(AEVK_SPACE)) {
        isFadingToMenu = true;
    }
}

void Endgame_Draw()
{

    float dt = (f32)AEFrameRateControllerGetFrameTime();
    // --- DRAW ENDGAME TEXT ---
    if (endgameFontId >= 0 && subFontId >= 0) {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(1.0f);
        if (currentEndReason == REASON_SURVIVED_5_DAYS) {
            AEGfxPrint(endgameFontId, "SHIFT COMPLETE", -0.4f, 0.2f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
            AEGfxPrint(subFontId, "You survived all 5 days!!", -0.3f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        else if (currentEndReason == REASON_DIED_TO_BOSS) {
            AEGfxPrint(endgameFontId, "GAME OVER", -0.3f, 0.2f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f);
            AEGfxPrint(subFontId, "You Died", -0.25f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        else if (currentEndReason == REASON_WRONG_BASEMENT_DELIVERY) {
            AEGfxPrint(endgameFontId, "YOUVE BEEN FIRED", -0.35f, 0.2f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f);
            AEGfxPrint(subFontId, "You've sent a human to the basement.", -0.6f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        // Blinking "Click to Continue" prompt
        float blinkAlpha = (sinf(dt * 5.0f) + 1.0f) * 0.5f;
        AEGfxPrint(subFontId, "Click or Press SPACE to return to menu", -0.55f, -0.4f, 1.0f, 1.0f, 1.0f, 1.0f, blinkAlpha);
    }

    // --- DRAW FADE OVERLAY ---
    if (fadeAlpha > 0.0f) {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(fadeAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // Pure Black

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void Endgame_Free()
{
    // Clean up anything allocated in Initialize
    FreeMeshSafe(squareMesh);
}

void Endgame_Unload()
{
    // Clean up fonts
    if (endgameFontId >= 0) {
        AEGfxDestroyFont(endgameFontId);
        endgameFontId = -1;
    }
}