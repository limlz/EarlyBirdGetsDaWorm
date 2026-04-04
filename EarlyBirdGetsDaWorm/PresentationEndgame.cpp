#include "pch.hpp"
#include "presentation_endgame.hpp"

// -------------------------------------------------------
//  PRESENTATION END SCREEN
//  Shown only in Presentation Mode, after the boss fight.
//  Hands the floor over to Ara and Syadeeya for takeaways.
// -------------------------------------------------------

static s8 titleFontId = -1;
static s8 bodyFontId  = -1;

static AEGfxVertexList* squareMesh = nullptr;

static bool  isFadingIn    = true;
static bool  isFadingOut   = false;
static float fadeAlpha     = 1.0f;
static float totalTime     = 0.0f;
static bool  audioPlayed   = false;

// Staggered line reveal timers (seconds after fade-in completes)
static float lineRevealTime = 0.0f;
static constexpr float FADE_SPEED = 1.2f;

void PresentationEndgame_Load()
{
    titleFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 34);
    bodyFontId  = AEGfxCreateFont(Assets::Fonts::Buggy, 22);
}

void PresentationEndgame_Initialize()
{
    squareMesh = CreateSquareMesh(COLOR_WHITE);

    isFadingIn    = true;
    isFadingOut   = false;
    fadeAlpha     = 1.0f;
    totalTime     = 0.0f;
    lineRevealTime = 0.0f;
    audioPlayed   = false;

    AEGfxSetBackgroundColor(0.04f, 0.04f, 0.06f); // near-black, slightly blue — clinical calm
}

void PresentationEndgame_Update()
{
    float dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());

    if (!audioPlayed) {
        AudioManager_StopBGM();
        AudioManager_PlayBGM(SoundID::BGM_MAIN_MENU, 0.35f);
        audioPlayed = true;
    }

    if (isFadingIn) {
        fadeAlpha -= FADE_SPEED * dt;
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            isFadingIn = false;
        }
        return;
    }

    totalTime     += dt;
    lineRevealTime += dt;

    if (isFadingOut) {
        fadeAlpha += FADE_SPEED * dt;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            AudioManager_StopBGM();
            next = MAIN_MENU;
        }
        return;
    }

    if (AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(AEVK_SPACE)) {
        isFadingOut = true;
    }
}

void PresentationEndgame_Draw()
{
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(1.0f);

    if (titleFontId >= 0 && bodyFontId >= 0) {

        // --- Title: game name ---
        // titleFontId size 34 → ~0.042f NDC per char (same calibration as BossFight's size 35)
        // bodyFontId  size 22 → ~0.026f NDC per char (scaled from Endgame's 0.036f at size 30)
        auto titleX = [](std::string_view text) -> float {
            return -(static_cast<float>(text.size()) * 0.042f * 0.5f);
        };

        if (lineRevealTime > 0.0f) {
            AEGfxPrintWithGlow(titleFontId, "EARLY BIRD GETS DA WORM",
                titleX("EARLY BIRD GETS DA WORM"), 0.55f, 1.0f,
                0.95f, 0.90f, 0.70f, 1.0f,
                0.40f, 0.30f, 0.05f, 0.85f,
                0.004f);
        }

        // --- Subtitle: demo complete ---
        if (lineRevealTime > 0.6f) {
            float alpha2 = AEClamp((lineRevealTime - 0.6f) / 0.5f, 0.0f, 1.0f);
            AEGfxPrintCentered(bodyFontId, "Thank you for experiencing our demo!",
                0.0f, 0.28f, 1.0f, 0.85f, 0.85f, 0.85f, alpha2, 0.026f);
        }

        // --- Divider line ---
        if (lineRevealTime > 1.2f) {
            float alpha3 = AEClamp((lineRevealTime - 1.2f) / 0.4f, 0.0f, 1.0f);
            AEGfxSetTransparency(alpha3 * 0.4f);
            DrawSquareMesh(squareMesh, 0.0f, 144.0f, 600.0f, 2.0f, 0xFFAA9966);
            AEGfxSetTransparency(1.0f);
        }

        // --- Handover header ---
        if (lineRevealTime > 1.5f) {
            float alpha4 = AEClamp((lineRevealTime - 1.5f) / 0.5f, 0.0f, 1.0f);
            AEGfxPrintCentered(bodyFontId, "We now hand it over to our team",
                0.0f, 0.10f, 1.0f, 0.70f, 0.85f, 0.95f, alpha4, 0.026f);
        }

        if (lineRevealTime > 2.0f) {
            float alpha5 = AEClamp((lineRevealTime - 2.0f) / 0.5f, 0.0f, 1.0f);
            AEGfxPrintCentered(bodyFontId, "for the main project takeaways:",
                0.0f, -0.02f, 1.0f, 0.70f, 0.85f, 0.95f, alpha5, 0.026f);
        }

        // --- Names ---
        if (lineRevealTime > 2.7f) {
            float alpha6 = AEClamp((lineRevealTime - 2.7f) / 0.6f, 0.0f, 1.0f);
            AEGfxPrintWithGlow(titleFontId, "Ara  &  Syadeeya",
                titleX("Ara  &  Syadeeya"), -0.22f, 1.0f,
                0.55f, 1.0f, 0.70f, alpha6,
                0.0f, 0.35f, 0.20f, alpha6 * 0.9f,
                0.004f);
        }

        // --- Blinking prompt ---
        if (lineRevealTime > 3.5f && !isFadingOut) {
            float blinkAlpha = (sinf(totalTime * 4.0f) + 1.0f) * 0.5f;
            AEGfxPrintCentered(bodyFontId, "[ Click or Space to return to menu ]",
                0.0f, -0.52f, 1.0f, 0.55f, 0.55f, 0.55f, blinkAlpha * 0.65f, 0.026f);
        }
    }

    // --- Fade overlay ---
    DrawBlackOverlay(squareMesh, fadeAlpha);
}

void PresentationEndgame_Free()
{
    FreeMeshSafe(squareMesh);
}

void PresentationEndgame_Unload()
{
    if (titleFontId >= 0) { AEGfxDestroyFont(titleFontId); titleFontId = -1; }
    if (bodyFontId  >= 0) { AEGfxDestroyFont(bodyFontId);  bodyFontId  = -1; }
}
