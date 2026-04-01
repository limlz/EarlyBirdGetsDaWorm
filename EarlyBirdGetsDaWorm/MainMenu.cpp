#include "pch.hpp"
#include "Credits.hpp"
#include "Animation.hpp"

static constexpr float FADE_IN_DURATION = 0.3f;
static constexpr float FADE_OUT_SPEED = 6.0f;
static constexpr int   NUM_TITLE_FRAMES = 13;
static constexpr float TITLE_ANIM_SPEED = 0.2f;

static AEGfxVertexList* squareMesh;

static AEGfxTexture* main_menu_bg        = nullptr;
static AEGfxTexture* admit_patient_tag   = nullptr;
static AEGfxTexture* admit_patient_text  = nullptr;
static AEGfxTexture* options_text        = nullptr;
static AEGfxTexture* options_tag         = nullptr;
static AEGfxTexture* quit_tag            = nullptr;
static AEGfxTexture* quit_text           = nullptr;
static AEGfxTexture* title_bg            = nullptr;

static AEGfxTexture* title_frames[NUM_TITLE_FRAMES];

bool first_load = true;

static float fade_in_timer    = FADE_IN_DURATION;
static bool  isFadingOut      = false;
static float fade_out_alpha   = 0.0f;
static int   pendingNextState = 0;

static float menuLightX = -200.0f;
static float menuLightY = 500.0f;

static float admit_scale   = 1.0f;
static float options_scale = 1.0f;
static float quit_scale    = 1.0f;

static s8 menuFontId = -1;

static Animation titleAnim = { 0, 0.0f, TITLE_ANIM_SPEED, NUM_TITLE_FRAMES, false }; // play-once

void MainMenu_Load()
{
    main_menu_bg        = LoadTextureChecked(Assets::Main_Menu::Background);
    admit_patient_tag   = LoadTextureChecked(Assets::Main_Menu::AdmitPatientTag);
    admit_patient_text  = LoadTextureChecked(Assets::Main_Menu::AdmitPatientText);
    options_text        = LoadTextureChecked(Assets::Main_Menu::OptionsText);
    options_tag         = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
    quit_tag            = LoadTextureChecked(Assets::Main_Menu::QuitTag);
    quit_text           = LoadTextureChecked(Assets::Main_Menu::QuitText);
    title_bg            = LoadTextureChecked(Assets::Main_Menu::TitleBg);

    Credits_Load();

    for (int i = 0; i < NUM_TITLE_FRAMES; i++) {
        std::string path = std::string(Assets::Main_Menu::TitleFramePrefix)
            + std::to_string(i + 1)
            + Assets::Main_Menu::TitleFrameSuffix;
        title_frames[i] = LoadTextureChecked(path.c_str());
    }

    menuFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 24);

    std::cout << "MainMenu: Load\n";
    Settings_Load();
}

void MainMenu_Initialize()
{
    AudioManager_PlayBGM(SoundID::BGM_MAIN_MENU, 0.50f);
    AudioManager_PlaySFX(SoundID::SFX_MAIN_MENU_WRITING_SCRATCH, 0.35f);
    Lighting_Initialize(0);
    Particles_Initialize();
    squareMesh = CreateSquareMesh(0xFFFFFFFF);

    fade_in_timer    = FADE_IN_DURATION;
    isFadingOut      = false;
    fade_out_alpha   = 0.0f;
    pendingNextState = 0;

    titleAnim.Reset();
    Credits_Initialize();

    std::cout << "MainMenu: Initialize\n";
}

void MainMenu_Update()
{
    float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());

    bool wasSettingsOpen = Settings_IsOpen();
    Settings_Update(dt);
    if (wasSettingsOpen) {
        return;
    }

    if (isFadingOut) {
        fade_out_alpha += FADE_OUT_SPEED * dt;
        if (fade_out_alpha >= 1.0f) { fade_out_alpha = 1.0f; next = pendingNextState; }
        return;
    }

    if (fade_in_timer > 0.0f)
        fade_in_timer = AEMax(fade_in_timer - dt, 0.0f);

    titleAnim.Update(dt);

    Particles_Update();
    Update_StandaloneLight(dt, menuLightX, menuLightY);

    bool creditsWasOpen = Credits_IsOpen();
    Credits_Update(dt);
    if (creditsWasOpen) return;

    // Main Menu Buttons
    bool hoverAdmit    = IsAreaClickedByMouse(480.0f,  270.0f, 490.0f, 290.0f);
    bool hoverPlaytest = IsAreaClickedByMouse(650.0f, -410.0f,  80.0f,  30.0f);
    bool hoverOptions  = IsAreaClickedByMouse(500.0f,   15.0f, 500.0f, 200.0f);
    bool hoverQuit     = IsAreaClickedByMouse(490.0f, -210.0f, 490.0f, 190.0f);

    float animSpeed = 15.0f;
    UpdateButtonScale(admit_scale,   hoverAdmit,   1.15f, animSpeed, dt);
    UpdateButtonScale(options_scale, hoverOptions, 1.15f, animSpeed, dt);
    UpdateButtonScale(quit_scale,    hoverQuit,    1.15f, animSpeed, dt);

    if (AEInputCheckTriggered(AEVK_LBUTTON)) {
        if      (hoverAdmit)            { pendingNextState = GAME_STATE; isFadingOut = true; }
        else if (hoverPlaytest)         { Game_SetPlaytestMode(!Game_IsPlaytestMode()); }
        else if (hoverOptions)          { Settings_Open(); }
        else if (hoverQuit)             { pendingNextState = GS_QUIT; isFadingOut = true; }
        else if (Credits_IsHovering())  { Credits_Open(); }
    }


}

void MainMenu_Draw()
{
    DrawTextureMesh(squareMesh, main_menu_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);
    DrawTextureMesh(squareMesh, title_bg,     0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

    if (title_frames[titleAnim.currentFrame])
        DrawTextureMesh(squareMesh, title_frames[titleAnim.currentFrame], 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

    Credits_DrawCharacter(squareMesh);

    Draw_StandaloneConeLight(menuLightX, menuLightY);
    Particles_Draw(squareMesh, 0.0f);

    DrawTextureMesh(squareMesh, admit_patient_tag,  490.0f,  280.0f, 500.0f * admit_scale,   300.0f * admit_scale,   1.0f);
    DrawTextureMesh(squareMesh, admit_patient_text, 470.0f,  280.0f, 400.0f * admit_scale,   200.0f * admit_scale,   1.0f);
    DrawTextureMesh(squareMesh, options_tag,        500.0f,   15.0f, 500.0f * options_scale, 200.0f * options_scale, 1.0f);
    DrawTextureMesh(squareMesh, options_text,       500.0f,   15.0f, 400.0f * options_scale, 150.0f * options_scale, 1.0f);
    DrawTextureMesh(squareMesh, quit_tag,           500.0f, -220.0f, 500.0f * quit_scale,    200.0f * quit_scale,    1.0f);
    DrawTextureMesh(squareMesh, quit_text,          515.0f, -220.0f, 400.0f * quit_scale,    150.0f * quit_scale,    1.0f);

    // --- Playtest Mode pill switch (bottom right, matches Settings style) ---
    {
        const float ptSwX = 650.0f, ptSwY = -410.0f;
        bool ptOn = Game_IsPlaytestMode();

        // Label above the switch
        if (menuFontId >= 0) {
            AEGfxPrintCentered(menuFontId, "PLAYTEST MODE",
                ptSwX / 800.0f, (ptSwY + 25.0f) / 450.0f,
                0.6f, 0.75f, 0.95f, 0.75f, 1.0f);
        }

        // Pill track (green when ON, dark when OFF)
        DrawSquareMesh(squareMesh, ptSwX, ptSwY, 80.0f, 26.0f,
            ptOn ? 0xFF228B22 : 0xFF2A2A2A);

        // Sliding nub
        DrawSquareMesh(squareMesh, ptSwX + (ptOn ? 22.0f : -22.0f), ptSwY, 28.0f, 28.0f, 0xFFEEEEEE);

        // ON / OFF label on the opposite side of the nub
        if (menuFontId >= 0) {
            AEGfxPrintCentered(menuFontId, ptOn ? "ON" : "OFF",
                (ptSwX + (ptOn ? -18.0f : 18.0f)) / 800.0f, (ptSwY - 7.0f) / 450.0f,
                0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    float fadeAlpha = (fade_in_timer > 0.0f) ? (fade_in_timer / FADE_IN_DURATION)
                                              : (isFadingOut ? fade_out_alpha : 0.0f);
    DrawBlackOverlay(squareMesh, fadeAlpha);

    Credits_DrawOverlay(squareMesh);
    Settings_Draw(squareMesh);
}

void MainMenu_Free()
{
    AudioManager_StopBGM();
    AudioManager_StopSFX();
}

void MainMenu_Unload()
{
    Lighting_Unload();
    FreeMeshSafe(squareMesh);

    UnloadTextureSafe(main_menu_bg);
    UnloadTextureSafe(admit_patient_tag);
    UnloadTextureSafe(admit_patient_text);
    UnloadTextureSafe(options_tag);
    UnloadTextureSafe(options_text);
    UnloadTextureSafe(quit_tag);
    UnloadTextureSafe(quit_text);
    UnloadTextureSafe(title_bg);

    for (int i = 0; i < NUM_TITLE_FRAMES; i++) UnloadTextureSafe(title_frames[i]);

    Credits_Unload();

    if (menuFontId >= 0) { AEGfxDestroyFont(menuFontId); menuFontId = -1; }

    Particles_Free();
    Settings_Unload();
    std::cout << "MainMenu: Unload\n";
}
