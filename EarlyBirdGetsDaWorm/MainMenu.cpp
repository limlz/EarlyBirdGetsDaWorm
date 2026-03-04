#include "pch.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib> // Needed for rand()

static AEGfxVertexList* squareMesh;

static AEGfxTexture* main_menu_bg = nullptr;
static AEGfxTexture* admit_patient_tag = nullptr;
static AEGfxTexture* admit_patient_text = nullptr;
static AEGfxTexture* options_text = nullptr;
static AEGfxTexture* options_tag = nullptr;
static AEGfxTexture* quit_tag = nullptr;
static AEGfxTexture* quit_text = nullptr;
static AEGfxTexture* title_bg = nullptr;

// --- NEW: Credits Textures ---
static AEGfxTexture* credits_guy = nullptr;
static AEGfxTexture* credits_guy2 = nullptr;

s8 menuFontId = 0;
s32 mouseX, mouseY;
f32 worldMouseX, worldMouseY;

bool first_load = true;

const float FADE_IN_DURATION = 0.3f;
static float fade_in_timer = FADE_IN_DURATION;
static bool isFadingOut = false;
static float fade_out_alpha = 0.0f;
const float FADE_OUT_SPEED = 3.0f;
static int pendingNextState = 0;
float menuLightX = -200.0f;
float menuLightY = 500.0f;

// Hover animation scales
static float admit_scale = 1.0f;
static float options_scale = 1.0f;
static float quit_scale = 1.0f;

// --- CREDITS JITTER VARIABLES ---
static bool isHoveringCredits = false;
static float credits_jitter_x = 0.0f;
static float credits_jitter_y = 0.0f;
static float credits_twitch_duration = 0.0f;
static float credits_idle_timer = 2.0f;
static float jitter_update_timer = 0.0f;

//title ani
const int NUM_TITLE_FRAMES = 13;
static AEGfxTexture* title_frames[NUM_TITLE_FRAMES];
static int currentTitleFrame = 0;
static float titleAnimTimer = 0.0f;
const float TITLE_ANIM_SPEED = 0.2f;

// --- OPTIONS OVERLAY VARIABLES ---
static bool isOptionsOpen = false;
static float optionsOverlayAlpha = 0.0f;
static float masterVolume = 1.0f;
static bool isDraggingVolume = false;

// --- CREDITS OVERLAY VARIABLES ---
struct CreditLine {
    std::string text;
    float scale;
    bool isHeader; // Track if it's an H1/H2 to add spacing
};
static std::vector<CreditLine> parsedCredits;
static bool isCreditsOpen = false;
static float creditsOverlayAlpha = 0.0f;
static float creditsScrollY = -500.0f;

// Helper Function to Load and Parse the Text File
void LoadCreditsFile()
{
    parsedCredits.clear();
    std::ifstream file("Assets/credits.txt");

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            CreditLine cl;
            cl.scale = 0.8f; // Default size (like <p>)
            cl.isHeader = false;

            // Check for H1
            if (line.find("<h1>") != std::string::npos) {
                cl.scale = 2.0f;
                cl.isHeader = true;
                line.erase(line.find("<h1>"), 4);
                if (line.find("</h1>") != std::string::npos) line.erase(line.find("</h1>"), 5);
            }
            // Check for H2
            else if (line.find("<h2>") != std::string::npos) {
                cl.scale = 1.5f;
                cl.isHeader = true;
                line.erase(line.find("<h2>"), 4);
                if (line.find("</h2>") != std::string::npos) line.erase(line.find("</h2>"), 5);
            }
            // Check for P
            else if (line.find("<p>") != std::string::npos) {
                cl.scale = 0.8f;
                line.erase(line.find("<p>"), 3);
                if (line.find("</p>") != std::string::npos) line.erase(line.find("</p>"), 4);
            }

            cl.text = line;
            parsedCredits.push_back(cl);
        }
        file.close();
    }
    else {
        // Fallback if file isn't found
        parsedCredits.push_back({ "ERROR", 2.0f, true });
        parsedCredits.push_back({ "Could not find Assets/credits.txt", 0.8f, false });
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

    // Load the text file immediately
    LoadCreditsFile();

    for (int i = 0; i < NUM_TITLE_FRAMES; i++)
    {
        std::string filePath = std::string(Assets::Main_Menu::TitleFramePrefix) + std::to_string(i + 1) + Assets::Main_Menu::TitleFrameSuffix;
        title_frames[i] = LoadTextureChecked(filePath.c_str());
    }

    std::cout << "MainMenu: Load\n";
    menuFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
}

void MainMenu_Initialize()
{
    Lighting_Initialize(0);
    Particles_Initialize();

    fade_in_timer = FADE_IN_DURATION;
    isFadingOut = false;
    fade_out_alpha = 0.0f;
    pendingNextState = 0;

    isCreditsOpen = false;
    creditsOverlayAlpha = 0.0f;
    creditsScrollY = -500.0f;

    // Reset Jitter variables
    isHoveringCredits = false;
    credits_jitter_x = 0.0f;
    credits_jitter_y = 0.0f;
    credits_twitch_duration = 0.0f;
    credits_idle_timer = 2.0f;

    squareMesh = CreateSquareMesh(0xFFFFFFFF);
    std::cout << "MainMenu: Initialize\n";
}

void MainMenu_Update()
{
    float dt = (f32)AEFrameRateControllerGetFrameTime();

    if (isFadingOut) {
        fade_out_alpha += FADE_OUT_SPEED * dt;
        if (fade_out_alpha >= 1.0f) {
            fade_out_alpha = 1.0f;
            next = pendingNextState;
        }
        return;
    }

    if (fade_in_timer > 0.0f) {
        fade_in_timer -= dt;
        if (fade_in_timer < 0.0f) fade_in_timer = 0.0f;
    }

    if (currentTitleFrame < NUM_TITLE_FRAMES - 1)
    {
        titleAnimTimer += dt;
        if (titleAnimTimer >= TITLE_ANIM_SPEED) {
            titleAnimTimer = 0.0f;
            currentTitleFrame++;
        }
    }

    Particles_Update();
    Update_StandaloneLight(dt, menuLightX, menuLightY);

    // --- CREDITS OVERLAY LOGIC ---
    if (isCreditsOpen)
    {
        creditsOverlayAlpha += FADE_OUT_SPEED * dt;
        if (creditsOverlayAlpha > 0.9f) creditsOverlayAlpha = 0.9f;

        // Scroll text upwards
        creditsScrollY += 60.0f * dt;

        // Close on escape or click
        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON)) {
            isCreditsOpen = false;
        }

        return; // BLOCK MAIN MENU CLICKS
    }
    else {
        creditsOverlayAlpha -= FADE_OUT_SPEED * dt;
        if (creditsOverlayAlpha < 0.0f) creditsOverlayAlpha = 0.0f;
    }

    // --- OPTIONS OVERLAY LOGIC ---
    if (isOptionsOpen)
    {
        optionsOverlayAlpha += FADE_OUT_SPEED * dt;
        if (optionsOverlayAlpha > 0.8f) optionsOverlayAlpha = 0.8f;

        if (AEInputCheckTriggered(AEVK_ESCAPE)) isOptionsOpen = false;

        if (AEInputCheckTriggered(AEVK_LBUTTON) && IsAreaClickedByMouse(-160.0f, 15.0f, 320.0f, 30.0f)) {
            isDraggingVolume = true;
        }
        if (AEInputCheckReleased(AEVK_LBUTTON)) isDraggingVolume = false;

        if (isDraggingVolume) {
            masterVolume = (Input_GetMouseX() + 150.0f) / 300.0f;
            if (masterVolume < 0.0f) masterVolume = 0.0f;
            if (masterVolume > 1.0f) masterVolume = 1.0f;
        }

        return; // BLOCK MAIN MENU CLICKS
    }
    else
    {
        optionsOverlayAlpha -= FADE_OUT_SPEED * dt;
        if (optionsOverlayAlpha < 0.0f) optionsOverlayAlpha = 0.0f;
    }

    // --- MENU BUTTON HOVER LOGIC ---
    bool hoverAdmit = IsAreaClickedByMouse(480.0f, 270.0f, 490.0f, 290.0f);
    bool hoverOptions = IsAreaClickedByMouse(500.0f, 15.0f, 500.0f, 200.0f);
    bool hoverQuit = IsAreaClickedByMouse(490.0f, -210.0f, 490.0f, 190.0f);

    float animationSpeed = 15.0f;
    admit_scale += ((hoverAdmit ? 1.15f : 1.0f) - admit_scale) * animationSpeed * dt;
    options_scale += ((hoverOptions ? 1.15f : 1.0f) - options_scale) * animationSpeed * dt;
    quit_scale += ((hoverQuit ? 1.15f : 1.0f) - quit_scale) * animationSpeed * dt;

    // --- NEW: CREDITS JITTER LOGIC ---
    isHoveringCredits = IsAreaClickedByMouse(-600.0f, -350.0f, 300.0f, 100.0f);
    jitter_update_timer -= dt;

    if (isHoveringCredits) {
        // Intense, constant horizontal jitter while hovered
        if (jitter_update_timer <= 0.0f) {
            credits_jitter_x = ((rand() % 200 - 100) / 100.0f) * 12.0f; // +/- 12 pixels left/right
            credits_jitter_y = 0.0f; // CHANGED: Locked Y to 0
            jitter_update_timer = 0.03f; // Update super fast (30x a second)
        }
    }
    else {
        // Idle random twitch logic
        if (credits_idle_timer > 0.0f) {
            credits_idle_timer -= dt;
            credits_jitter_x = 0.0f;
            credits_jitter_y = 0.0f;
        }
        else {
            // Trigger a small twitch
            credits_twitch_duration = 0.3f; // Twitch lasts 0.3 seconds
            credits_idle_timer = ((rand() % 300) / 100.0f) + 1.5f; // Wait 1.5 to 4.5 seconds for next twitch
        }

        if (credits_twitch_duration > 0.0f) {
            credits_twitch_duration -= dt;
            if (jitter_update_timer <= 0.0f) {
                credits_jitter_x = ((rand() % 200 - 100) / 100.0f) * 5.0f; // Smaller +/- 5 pixel jitter left/right
                credits_jitter_y = 0.0f; // CHANGED: Locked Y to 0
                jitter_update_timer = 0.05f; // Slower twitch rate
            }
        }
        else {
            credits_jitter_x = 0.0f;
            credits_jitter_y = 0.0f;
        }
    }

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (hoverAdmit) {
            pendingNextState = GAME_STATE;
            isFadingOut = true;
        }
        else if (hoverOptions) {
            isOptionsOpen = true;
        }
        else if (hoverQuit) {
            pendingNextState = GS_QUIT;
            isFadingOut = true;
        }
        else if (isHoveringCredits) {
            isCreditsOpen = true;
            creditsScrollY = -600.0f;
        }
    }
}

void MainMenu_Draw()
{
    DrawTextureMesh(squareMesh, main_menu_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);
    DrawTextureMesh(squareMesh, title_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

    if (title_frames[currentTitleFrame]) {
        DrawTextureMesh(squareMesh, title_frames[currentTitleFrame], 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);
    }

    // --- DRAW CREDITS BUTTON (TEXTURE SWAP & JITTER) ---
    AEGfxTexture* currentCreditTex = isHoveringCredits ? credits_guy2 : credits_guy;

    float base_credit_x = -680.0f;
    float base_credit_y = -250.0f;

    if (currentCreditTex) {
        // Draw the texture using the base coordinates plus our dynamic jitter logic
        DrawTextureMesh(squareMesh, currentCreditTex, base_credit_x + credits_jitter_x, base_credit_y + credits_jitter_y, 400.0f, 600.0f, 1.0f);
    }

    Draw_StandaloneConeLight(menuLightX, menuLightY);
    Particles_Draw(squareMesh, 0.0f);

    DrawTextureMesh(squareMesh, admit_patient_tag, 490.0f, 280.0f, 500.0f * admit_scale, 300.0f * admit_scale, 1.0f);
    DrawTextureMesh(squareMesh, admit_patient_text, 470.0f, 280.0f, 400.0f * admit_scale, 200.0f * admit_scale, 1.0f);

    DrawTextureMesh(squareMesh, options_tag, 500.0f, 15.0f, 500.0f * options_scale, 200.0f * options_scale, 1.0f);
    DrawTextureMesh(squareMesh, options_text, 500.0f, 15.0f, 400.0f * options_scale, 150.0f * options_scale, 1.0f);

    DrawTextureMesh(squareMesh, quit_tag, 500.0f, -220.0f, 500.0f * quit_scale, 200.0f * quit_scale, 1.0f);
    DrawTextureMesh(squareMesh, quit_text, 515.0f, -220.0f, 400.0f * quit_scale, 150.0f * quit_scale, 1.0f);


    float currentBlackAlpha = 0.0f;
    if (fade_in_timer > 0.0f) currentBlackAlpha = fade_in_timer / FADE_IN_DURATION;
    else if (isFadingOut) currentBlackAlpha = fade_out_alpha;

    if (currentBlackAlpha > 0.0f)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(currentBlackAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    // --- DRAW OPTIONS OVERLAY ---
    if (optionsOverlayAlpha > 0.0f)
    {
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

        if (optionsOverlayAlpha >= 0.5f)
        {
            AEGfxSetTransparency(1.0f);
            DrawSquareMesh(squareMesh, 0.0f, 0.0f, 300.0f, 20.0f, 0xFF444444);
            float fillWidth = 300.0f * masterVolume;
            float fillStartX = -150.0f + (fillWidth / 2.0f);
            DrawSquareMesh(squareMesh, fillStartX, 0.0f, fillWidth, 20.0f, COLOR_WHITE);

            float handleX = -150.0f + (300.0f * masterVolume);
            DrawSquareMesh(squareMesh, handleX, 0.0f, 15.0f, 40.0f, 0xFF0000FF);
        }
    }

    // --- DRAW CREDITS OVERLAY ---
    if (creditsOverlayAlpha > 0.0f)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(creditsOverlayAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        if (menuFontId >= 0 && creditsOverlayAlpha >= 0.5f)
        {
            AEGfxSetTransparency(1.0f);
            float currentY = creditsScrollY;

            for (const auto& line : parsedCredits)
            {
                // ADD SPACING BEFORE HEADERS
                if (line.isHeader) {
                    currentY -= 40.0f; // Extra space before an H1 or H2
                }

                if (currentY < -600.0f || currentY > 600.0f) {
                    currentY -= (40.0f * line.scale);
                    continue;
                }

                float ndcX = -0.8f;
                float ndcY = currentY / 450.0f;

                if (!line.text.empty()) {

                    // C-Style cast to fix the string format specifically for this engine

                    AEGfxPrint(menuFontId, line.text.c_str(), ndcX, ndcY, line.scale, 1.0f, 1.0f, 1.0f, 1.0f);

                }

                currentY -= (40.0f * line.scale);
            }
        }
    }
}

void MainMenu_Free()
{
    std::cout << "MainMenu: Free\n";
}

void MainMenu_Unload()
{
    FreeMeshSafe(squareMesh);

    UnloadTextureSafe(admit_patient_tag);
    UnloadTextureSafe(admit_patient_text);
    UnloadTextureSafe(options_tag);
    UnloadTextureSafe(options_text);
    UnloadTextureSafe(quit_tag);
    UnloadTextureSafe(quit_text);
    UnloadTextureSafe(title_bg);

    UnloadTextureSafe(credits_guy);
    UnloadTextureSafe(credits_guy2);

    for (int i = 0; i < NUM_TITLE_FRAMES; i++) {
        UnloadTextureSafe(title_frames[i]);
    }

    if (menuFontId >= 0) {
        AEGfxDestroyFont(menuFontId);
        menuFontId = -1;
    }
    Particles_Free();
    std::cout << "MainMenu: Unload\n";
}