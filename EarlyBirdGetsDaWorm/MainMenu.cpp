#include "pch.hpp"
#include "Settings.hpp" // --- NEW: Include modular settings ---
#include <fstream>   
#include <string>    
#include <vector>    
#include <cstdlib>   

struct CreditLine {
    std::string text;
    float       scale;
    bool        isHeader;
};

static constexpr float FADE_IN_DURATION = 0.3f;
static constexpr float FADE_OUT_SPEED = 3.0f;
static constexpr int   NUM_TITLE_FRAMES = 13;
static constexpr float TITLE_ANIM_SPEED = 0.2f;

static AEGfxVertexList* squareMesh;

static AEGfxTexture* main_menu_bg = nullptr;
static AEGfxTexture* admit_patient_tag = nullptr;
static AEGfxTexture* admit_patient_text = nullptr;
static AEGfxTexture* options_text = nullptr;
static AEGfxTexture* options_tag = nullptr;
static AEGfxTexture* quit_tag = nullptr;
static AEGfxTexture* quit_text = nullptr;
static AEGfxTexture* title_bg = nullptr;
static AEGfxTexture* credits_guy = nullptr;
static AEGfxTexture* credits_guy2 = nullptr;
static AEGfxTexture* close_btn_tex = nullptr;

static AEGfxTexture* title_frames[NUM_TITLE_FRAMES];

s8   menuFontId = 0;
bool first_load = true;

static float fade_in_timer = FADE_IN_DURATION;
static bool  isFadingOut = false;
static float fade_out_alpha = 0.0f;
static int   pendingNextState = 0;

static float menuLightX = -200.0f;
static float menuLightY = 500.0f;

static float admit_scale = 1.0f;
static float options_scale = 1.0f;
static float quit_scale = 1.0f;

static bool  isHoveringCredits = false;
static float credits_jitter_x = 0.0f;
static float credits_jitter_y = 0.0f;
static float credits_twitch_duration = 0.0f;
static float credits_idle_timer = 2.0f;
static float jitter_update_timer = 0.0f;

static int   currentTitleFrame = 0;
static float titleAnimTimer = 0.0f;

static std::vector<CreditLine> parsedCredits;
static bool  isCreditsOpen = false;
static float creditsOverlayAlpha = 0.0f;
static float creditsScrollY = -500.0f;

static float totalCreditsHeight = 0.0f;

static inline void FadeAlpha(float& alpha, bool open, float dt, float maxVal = 1.0f)
{
    alpha += (open ? 1.0f : -1.0f) * FADE_OUT_SPEED * dt;
    alpha = AEClamp(alpha, 0.0f, maxVal);
}

static void DrawBlackOverlay(float alpha)
{
    if (alpha <= 0.0f) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(alpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
}

static void StripTag(std::string& s, const char* open, const char* close)
{
    auto pos = s.find(open);
    if (pos != std::string::npos) s.erase(pos, strlen(open));
    pos = s.find(close);
    if (pos != std::string::npos) s.erase(pos, strlen(close));
}

void LoadCreditsFile()
{
    parsedCredits.clear();
    totalCreditsHeight = 0.0f;
    std::ifstream file("Assets/credits.txt");

    if (!file.is_open()) {
        parsedCredits.push_back({ "ERROR",                             2.0f, true });
        parsedCredits.push_back({ "Could not find Assets/credits.txt", 0.8f, false });
        totalCreditsHeight = 150.0f;
        return;
    }

    struct TagDef { const char* open; const char* close; float scale; bool isHeader; };
    static constexpr TagDef kTags[] = {
        { "<h1>", "</h1>", 2.0f, true  },
        { "<h2>", "</h2>", 1.5f, true  },
        { "<p>",  "</p>",  0.8f, false },
    };

    std::string line;
    while (std::getline(file, line)) {
        CreditLine cl{ line, 0.8f, false };

        for (const auto& tag : kTags) {
            if (line.find(tag.open) != std::string::npos) {
                cl.scale = tag.scale;
                cl.isHeader = tag.isHeader;
                StripTag(line, tag.open, tag.close);
                cl.text = line;
                break;
            }
        }
        parsedCredits.push_back(cl);

        if (cl.isHeader) totalCreditsHeight += 40.0f;
        totalCreditsHeight += 40.0f * cl.scale;
    }
}

void MainMenu_Load()
{
    main_menu_bg = LoadTextureChecked(Assets::Main_Menu::Background);
    admit_patient_tag = LoadTextureChecked(Assets::Main_Menu::AdmitPatientTag);
    admit_patient_text = LoadTextureChecked(Assets::Main_Menu::AdmitPatientText);
    options_text = LoadTextureChecked(Assets::Main_Menu::OptionsText);
    options_tag = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
    quit_tag = LoadTextureChecked(Assets::Main_Menu::QuitTag);
    quit_text = LoadTextureChecked(Assets::Main_Menu::QuitText);
    title_bg = LoadTextureChecked(Assets::Main_Menu::TitleBg);
    credits_guy = LoadTextureChecked(Assets::Main_Menu::CreditsDown);
    credits_guy2 = LoadTextureChecked(Assets::Main_Menu::CreditsUp);

    close_btn_tex = LoadTextureChecked(Assets::Main_Menu::CloseButton);

    LoadCreditsFile();

    for (int i = 0; i < NUM_TITLE_FRAMES; i++) {
        std::string path = std::string(Assets::Main_Menu::TitleFramePrefix)
            + std::to_string(i + 1)
            + Assets::Main_Menu::TitleFrameSuffix;
        title_frames[i] = LoadTextureChecked(path.c_str());
    }

    menuFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    std::cout << "MainMenu: Load\n";
    Settings_Load(); 
}

void MainMenu_Initialize()
{
    AudioManager_PlayBGM(BGM_MAIN_MENU, 0.05f);
    AudioManager_PlaySFX(SFX_MAIN_MENU_WRITING_SCRATCH, 0.05f);
    Lighting_Initialize(0);
    Particles_Initialize();
    squareMesh = CreateSquareMesh(0xFFFFFFFF);

    fade_in_timer = FADE_IN_DURATION;
    isFadingOut = false;
    fade_out_alpha = 0.0f;
    pendingNextState = 0;

    isCreditsOpen = false;
    creditsOverlayAlpha = 0.0f;
    creditsScrollY = -500.0f;

    isHoveringCredits = false;
    credits_jitter_x = 0.0f;
    credits_jitter_y = 0.0f;
    credits_twitch_duration = 0.0f;
    credits_idle_timer = 2.0f;

    std::cout << "MainMenu: Initialize\n";
}

void MainMenu_Update()
{
    float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());

    // --- NEW: Handle Settings Menu ---
    bool wasSettingsOpen = Settings_IsOpen();
    Settings_Update(dt);
    if (wasSettingsOpen) {
        return; // Block other inputs if settings was open
    }

    if (isFadingOut) {
        fade_out_alpha += FADE_OUT_SPEED * dt;
        if (fade_out_alpha >= 1.0f) { fade_out_alpha = 1.0f; next = pendingNextState; }
        return;
    }

    if (fade_in_timer > 0.0f)
        fade_in_timer = AEMax(fade_in_timer - dt, 0.0f);

    if (currentTitleFrame < NUM_TITLE_FRAMES - 1) {
        titleAnimTimer += dt;
        if (titleAnimTimer >= TITLE_ANIM_SPEED) {
            titleAnimTimer = 0.0f;
            currentTitleFrame++;
        }
    }

    Particles_Update();
    Update_StandaloneLight(dt, menuLightX, menuLightY);

    // --- CREDITS OVERLAY LOGIC ---
    if (isCreditsOpen) {
        FadeAlpha(creditsOverlayAlpha, true, dt, 0.9f);
        creditsScrollY += 60.0f * dt;

        bool hoverCreditsClose = IsAreaClickedByMouse(730.0f, 390.0f, 60.0f, 60.0f);

        if (creditsScrollY > (totalCreditsHeight + 500.0f)) {
            isCreditsOpen = false;
        }

        if (AEInputCheckTriggered(AEVK_ESCAPE) || (AEInputCheckTriggered(AEVK_LBUTTON) && hoverCreditsClose)) {
            isCreditsOpen = false;
        }
        return;
    }
    FadeAlpha(creditsOverlayAlpha, false, dt, 0.9f);

    // Main Menu Buttons
    bool hoverAdmit = IsAreaClickedByMouse(480.0f, 270.0f, 490.0f, 290.0f);
    bool hoverOptions = IsAreaClickedByMouse(500.0f, 15.0f, 500.0f, 200.0f);
    bool hoverQuit = IsAreaClickedByMouse(490.0f, -210.0f, 490.0f, 190.0f);
    isHoveringCredits = IsAreaClickedByMouse(-750.0f, -400.0f, 300.0f, 800.0f);

    float animSpeed = 15.0f;
    admit_scale += ((hoverAdmit ? 1.15f : 1.0f) - admit_scale) * animSpeed * dt;
    options_scale += ((hoverOptions ? 1.15f : 1.0f) - options_scale) * animSpeed * dt;
    quit_scale += ((hoverQuit ? 1.15f : 1.0f) - quit_scale) * animSpeed * dt;

    jitter_update_timer -= dt;

    if (isHoveringCredits) {
        if (jitter_update_timer <= 0.0f) {
            credits_jitter_x = ((rand() % 200 - 100) / 100.0f) * 6.0f;
            credits_jitter_y = 0.0f;
            jitter_update_timer = 0.06f;
        }
    }
    else {
        if (credits_idle_timer > 0.0f) {
            credits_idle_timer -= dt;
            credits_jitter_x = credits_jitter_y = 0.0f;
        }
        else {
            credits_twitch_duration = 0.3f;
            credits_idle_timer = ((rand() % 300) / 100.0f) + 1.5f;
        }

        if (credits_twitch_duration > 0.0f) {
            credits_twitch_duration -= dt;
            if (jitter_update_timer <= 0.0f) {
                credits_jitter_x = ((rand() % 200 - 100) / 100.0f) * 5.0f;
                credits_jitter_y = 0.0f;
                jitter_update_timer = 0.05f;
            }
        }
        else {
            credits_jitter_x = credits_jitter_y = 0.0f;
        }
    }

    if (AEInputCheckTriggered(AEVK_LBUTTON)) {
        if (hoverAdmit) { pendingNextState = GAME_STATE; isFadingOut = true; }
        else if (hoverOptions) { Settings_Open(); } // --- NEW: Call API ---
        else if (hoverQuit) { pendingNextState = GS_QUIT; isFadingOut = true; }
        else if (isHoveringCredits) { isCreditsOpen = true; creditsScrollY = -600.0f; }
    }
}

void MainMenu_Draw()
{
    DrawTextureMesh(squareMesh, main_menu_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);
    DrawTextureMesh(squareMesh, title_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

    if (title_frames[currentTitleFrame])
        DrawTextureMesh(squareMesh, title_frames[currentTitleFrame], 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

    if (AEGfxTexture* creditTex = isHoveringCredits ? credits_guy2 : credits_guy)
        DrawTextureMesh(squareMesh, creditTex,
            -680.0f + credits_jitter_x,
            -250.0f + credits_jitter_y,
            400.0f, 600.0f, 1.0f);

    Draw_StandaloneConeLight(menuLightX, menuLightY);
    Particles_Draw(squareMesh, 0.0f);

    DrawTextureMesh(squareMesh, admit_patient_tag, 490.0f, 280.0f, 500.0f * admit_scale, 300.0f * admit_scale, 1.0f);
    DrawTextureMesh(squareMesh, admit_patient_text, 470.0f, 280.0f, 400.0f * admit_scale, 200.0f * admit_scale, 1.0f);
    DrawTextureMesh(squareMesh, options_tag, 500.0f, 15.0f, 500.0f * options_scale, 200.0f * options_scale, 1.0f);
    DrawTextureMesh(squareMesh, options_text, 500.0f, 15.0f, 400.0f * options_scale, 150.0f * options_scale, 1.0f);
    DrawTextureMesh(squareMesh, quit_tag, 500.0f, -220.0f, 500.0f * quit_scale, 200.0f * quit_scale, 1.0f);
    DrawTextureMesh(squareMesh, quit_text, 515.0f, -220.0f, 400.0f * quit_scale, 150.0f * quit_scale, 1.0f);

    float fadeAlpha = (fade_in_timer > 0.0f) ? (fade_in_timer / FADE_IN_DURATION)
        : (isFadingOut ? fade_out_alpha : 0.0f);
    DrawBlackOverlay(fadeAlpha);

    // --- CREDITS OVERLAY DRAW ---
    DrawBlackOverlay(creditsOverlayAlpha);
    if (menuFontId >= 0 && creditsOverlayAlpha >= 0.5f) {
        AEGfxSetTransparency(1.0f);

        if (close_btn_tex) {
            DrawTextureMesh(squareMesh, close_btn_tex, 730.0f, 390.0f, 60.0f, 60.0f, 1.0f);
        }

        float currentY = creditsScrollY;

        for (const auto& line : parsedCredits) {
            if (line.isHeader) currentY -= 40.0f;

            if (currentY >= -600.0f && currentY <= 600.0f && !line.text.empty()) {
                AEGfxPrint(menuFontId, line.text.c_str(), -0.8f, currentY / 450.0f, line.scale, 1.0f, 1.0f, 1.0f, 1.0f);
            }

            currentY -= 40.0f * line.scale;
        }
    }

    // --- NEW: Draw Modular Settings ---
    Settings_Draw(squareMesh);
}

void MainMenu_Free() {
    AudioManager_StopBGM();
    AudioManager_StopSFX();
}

void MainMenu_Unload()
{
    FreeMeshSafe(squareMesh);

    UnloadTextureSafe(main_menu_bg);
    UnloadTextureSafe(admit_patient_tag);
    UnloadTextureSafe(admit_patient_text);
    UnloadTextureSafe(options_tag);
    UnloadTextureSafe(options_text);
    UnloadTextureSafe(quit_tag);
    UnloadTextureSafe(quit_text);
    UnloadTextureSafe(title_bg);
    UnloadTextureSafe(credits_guy);
    UnloadTextureSafe(credits_guy2);

    UnloadTextureSafe(close_btn_tex);

    for (int i = 0; i < NUM_TITLE_FRAMES; i++) UnloadTextureSafe(title_frames[i]);

    if (menuFontId >= 0) { AEGfxDestroyFont(menuFontId); menuFontId = -1; }

    Particles_Free();
	Settings_Unload();
    std::cout << "MainMenu: Unload\n";
}