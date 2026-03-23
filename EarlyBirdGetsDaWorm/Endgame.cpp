#include "pch.hpp"

static s8 endgameFontId = -1;
static s8 subFontId = -1;

// Fade transitions
static bool isFadingIn = true;
static bool isFadingToMenu = false;
static float fadeAlpha = 1.0f;
const float FADE_SPEED = 1.5f;

static AEGfxVertexList* squareMesh = nullptr;

static bool first_audio = true;
static float totalTime = 0.0f; // accumulates dt for blink animation

void Endgame_Load()
{
    // Title font slightly larger than subtitle for visual hierarchy
    endgameFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 30);
    subFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 25);
}

void Endgame_Initialize()
{
    first_audio = true;
    totalTime = 0.0f;

    squareMesh = CreateSquareMesh(COLOR_WHITE);

    isFadingIn = true;
    isFadingToMenu = false;
    fadeAlpha = 1.0f; // Start completely black, fade in

    if (currentEndReason == REASON_SURVIVED_5_DAYS) {
        AEGfxSetBackgroundColor(0.0f, 0.2f, 0.1f); // Dark green (Victory)
    }
    else {
        AEGfxSetBackgroundColor(0.3f, 0.0f, 0.0f); // Dark red (Game Over)
    }
}

void Endgame_Update()
{
    if (first_audio) {
        AudioManager_StopBGM();

        // ---- INDIVIDUAL ENDGAME AUDIO TRIGGERS ----
        if (currentEndReason == REASON_SURVIVED_5_DAYS) {
            AudioManager_PlayBGM(BGM_MAIN_MENU, 0.5f);
            AudioManager_PlaySFX(SFX_AA, 1.0f, 1.0f, 0); // Optional SFX for winning
        }
        else if (currentEndReason == REASON_DIED_TO_BOSS) {
            AudioManager_PlaySFX(SFX_BOSS_FIGHT, 1.0f, 1.0f, 0); // Replace SFX_AA with Boss Death SFX
        }
        else if (currentEndReason == REASON_SENT_HUMAN_TO_BASEMENT) {
            AudioManager_PlaySFX(SFX_FAH, 1.0f, 1.0f, 0); // Replace SFX_AA with Human Fired SFX
        }
        else if (currentEndReason == REASON_SENT_GHOST_TO_ROOM) {
            AudioManager_PlaySFX(SFX_FAH, 1.0f, 1.0f, 0); // Replace SFX_AA with Ghost Fired SFX
        }

        first_audio = false;
    }

    float dt = (f32)AEFrameRateControllerGetFrameTime();

    // 1. Handle Fade In
    if (isFadingIn) {
        fadeAlpha -= FADE_SPEED * dt;
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            isFadingIn = false;
        }
        return; // Don't allow input while fading in
    }

    // 2. Handle Fade Out (Returning to Menu)
    if (isFadingToMenu) {
        fadeAlpha += FADE_SPEED * dt;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            AudioManager_StopBGM();
            next = MAIN_MENU;
        }
        return;
    }

    totalTime += dt;

    // 3. Wait for player input to leave
    if (AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(AEVK_SPACE)) {
        isFadingToMenu = true;
    }
}

void Endgame_Draw()
{
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(1.0f);

    if (endgameFontId >= 0 && subFontId >= 0) {

        // Computes a centered start-X for AEGfxPrintWithGlow (title font size 30 -> ~0.036 NDC per char)
        auto titleX = [](const char* text) -> float {
            return -(strlen(text) * 0.036f / 2.0f);
            };

        // --- Title with coloured glow + subtitle ---
        if (currentEndReason == REASON_SURVIVED_5_DAYS) {
            AEGfxPrintWithGlow(endgameFontId, "SHIFT COMPLETE",
                titleX("SHIFT COMPLETE"), 0.25f, 1.0f,
                0.4f, 1.0f, 0.5f, 1.0f,   // bright green text
                0.0f, 0.4f, 0.15f, 0.9f,  // dark green glow
                0.004f);
            AEGfxPrintCentered(subFontId, "You survived all 5 days!", 0.0f, 0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.030f);
        }
        else if (currentEndReason == REASON_DIED_TO_BOSS) {
            AEGfxPrintWithGlow(endgameFontId, "GAME OVER",
                titleX("GAME OVER"), 0.25f, 1.0f,
                1.0f, 0.2f, 0.2f, 1.0f,   // red text
                0.5f, 0.0f, 0.0f, 0.9f,   // dark red glow
                0.004f);
            AEGfxPrintCentered(subFontId, "You Died", 0.0f, 0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.030f);
        }
        else if (currentEndReason == REASON_SENT_HUMAN_TO_BASEMENT) {
            AEGfxPrintWithGlow(endgameFontId, "YOU'VE BEEN FIRED",
                titleX("YOU'VE BEEN FIRED"), 0.25f, 1.0f,
                1.0f, 0.5f, 0.1f, 1.0f,   // orange text
                0.5f, 0.2f, 0.0f, 0.9f,   // dark orange glow
                0.004f);
            AEGfxPrintCentered(subFontId, "You sent a human to the basement.", 0.0f, 0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.030f);
        }
        else if (currentEndReason == REASON_SENT_GHOST_TO_ROOM) {
            AEGfxPrintWithGlow(endgameFontId, "YOU'VE BEEN FIRED",
                titleX("YOU'VE BEEN FIRED"), 0.25f, 1.0f,
                1.0f, 0.5f, 0.1f, 1.0f,
                0.5f, 0.2f, 0.0f, 0.9f,
                0.004f);
            AEGfxPrintCentered(subFontId, "You assigned a ghost to a guest room.", 0.0f, 0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.030f);
        }

        // --- Run stats (day reached + real-time survived) ---
        int   day = Game_GetCurrentDay();
        float seconds = Game_GetTimeSurvived();
        int   mins = (int)(seconds / 60);
        int   secs = (int)(seconds) % 60;
        char statsBuf[64];
        sprintf_s(statsBuf, "Day %d / 5   |   Time: %02d:%02d", day, mins, secs);
        AEGfxPrintCentered(subFontId, statsBuf, 0.0f, -0.15f, 0.85f, 0.75f, 0.75f, 0.75f, 1.0f, 0.030f);

        // --- Blinking "click to continue" prompt ---
        float blinkAlpha = (sinf(totalTime * 5.0f) + 1.0f) * 0.5f;
        AEGfxPrintCentered(subFontId, "Click or Press SPACE to return to menu", 0.0f, -0.4f, 1.0f, 1.0f, 1.0f, 1.0f, blinkAlpha, 0.030f);
    }

    // --- Draw fade overlay ---
    DrawBlackOverlay(squareMesh, fadeAlpha);
}

void Endgame_Free()
{
    FreeMeshSafe(squareMesh);
}

void Endgame_Unload()
{
    if (endgameFontId >= 0) {
        AEGfxDestroyFont(endgameFontId);
        endgameFontId = -1;
    }
    if (subFontId >= 0) {
        AEGfxDestroyFont(subFontId);
        subFontId = -1;
    }
}