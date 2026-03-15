#include "pch.hpp"
#include "Credits.hpp"
#include "assets_catalog.hpp"
#include "utils.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

struct CreditLine {
    std::string text;
    float       scale;
    bool        isHeader;
};

static constexpr float CREDITS_FADE_SPEED = 6.0f;

static AEGfxTexture* credits_guy     = nullptr;
static AEGfxTexture* credits_guy2    = nullptr;
static AEGfxTexture* close_btn_tex   = nullptr;

static s8 creditsFontId = -1;

static bool  isHoveringCredits       = false;
static float credits_jitter_x        = 0.0f;
static float credits_jitter_y        = 0.0f;
static float credits_twitch_duration = 0.0f;
static float credits_idle_timer      = 2.0f;
static float jitter_update_timer     = 0.0f;

static std::vector<CreditLine> parsedCredits;
static bool  isCreditsOpen           = false;
static float creditsOverlayAlpha     = 0.0f;
static float creditsScrollY          = -500.0f;
static float totalCreditsHeight      = 0.0f;

// ============================================================
//  INTERNAL HELPERS
// ============================================================

static inline void FadeAlpha(float& alpha, bool open, float dt, float maxVal = 1.0f)
{
    alpha += (open ? 1.0f : -1.0f) * CREDITS_FADE_SPEED * dt;
    alpha = AEClamp(alpha, 0.0f, maxVal);
}

static void StripTag(std::string& s, const char* open, const char* close)
{
    auto pos = s.find(open);
    if (pos != std::string::npos) s.erase(pos, strlen(open));
    pos = s.find(close);
    if (pos != std::string::npos) s.erase(pos, strlen(close));
}

static void LoadCreditsFile()
{
    parsedCredits.clear();
    totalCreditsHeight = 0.0f;
    std::ifstream file("Assets/credits.txt");

    if (!file.is_open()) {
        parsedCredits.push_back({ "ERROR",                             2.0f, true  });
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
                cl.scale    = tag.scale;
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

// ============================================================
//  RESOURCE MANAGEMENT
// ============================================================

void Credits_Load()
{
    credits_guy   = LoadTextureChecked(Assets::Main_Menu::CreditsDown);
    credits_guy2  = LoadTextureChecked(Assets::Main_Menu::CreditsUp);
    close_btn_tex = LoadTextureChecked(Assets::Main_Menu::CloseButton);
    creditsFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    LoadCreditsFile();
}

void Credits_Unload()
{
    UnloadTextureSafe(credits_guy);
    UnloadTextureSafe(credits_guy2);
    UnloadTextureSafe(close_btn_tex);
    if (creditsFontId >= 0) { AEGfxDestroyFont(creditsFontId); creditsFontId = -1; }
}

// ============================================================
//  STATE MANAGEMENT
// ============================================================

void Credits_Initialize()
{
    isCreditsOpen           = false;
    creditsOverlayAlpha     = 0.0f;
    creditsScrollY          = -500.0f;
    isHoveringCredits       = false;
    credits_jitter_x        = 0.0f;
    credits_jitter_y        = 0.0f;
    credits_twitch_duration = 0.0f;
    credits_idle_timer      = 2.0f;
}

void Credits_Update(float dt)
{
    isHoveringCredits = IsAreaClickedByMouse(-750.0f, -400.0f, 300.0f, 800.0f);

    if (isCreditsOpen) {
        FadeAlpha(creditsOverlayAlpha, true, dt, 0.9f);
        creditsScrollY += 60.0f * dt;

        bool hoverCreditsClose = IsAreaClickedByMouse(730.0f, 390.0f, 60.0f, 60.0f);

        if (creditsScrollY > (totalCreditsHeight + 500.0f))
            isCreditsOpen = false;

        if (AEInputCheckTriggered(AEVK_ESCAPE) || (AEInputCheckTriggered(AEVK_LBUTTON) && hoverCreditsClose))
            isCreditsOpen = false;

        return;
    }

    FadeAlpha(creditsOverlayAlpha, false, dt, 0.9f);

    // Character jitter logic
    jitter_update_timer -= dt;

    if (isHoveringCredits) {
        if (jitter_update_timer <= 0.0f) {
            credits_jitter_x    = ((rand() % 200 - 100) / 100.0f) * 6.0f;
            credits_jitter_y    = 0.0f;
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
                credits_jitter_x    = ((rand() % 200 - 100) / 100.0f) * 5.0f;
                credits_jitter_y    = 0.0f;
                jitter_update_timer = 0.05f;
            }
        }
        else {
            credits_jitter_x = credits_jitter_y = 0.0f;
        }
    }
}

// ============================================================
//  DRAW
// ============================================================

void Credits_DrawCharacter(AEGfxVertexList* mesh)
{
    if (AEGfxTexture* creditTex = isHoveringCredits ? credits_guy2 : credits_guy)
        DrawTextureMesh(mesh, creditTex,
            -680.0f + credits_jitter_x,
            -250.0f + credits_jitter_y,
            400.0f, 600.0f, 1.0f);
}

void Credits_DrawOverlay(AEGfxVertexList* mesh)
{
    DrawBlackOverlay(mesh, creditsOverlayAlpha);
    if (creditsFontId >= 0 && creditsOverlayAlpha >= 0.5f) {
        AEGfxSetTransparency(1.0f);

        if (close_btn_tex)
            DrawTextureMesh(mesh, close_btn_tex, 730.0f, 390.0f, 60.0f, 60.0f, 1.0f);

        float currentY = creditsScrollY;

        for (const auto& line : parsedCredits) {
            if (line.isHeader) currentY -= 40.0f;

            if (currentY >= -600.0f && currentY <= 600.0f && !line.text.empty())
                AEGfxPrint(creditsFontId, line.text.c_str(), -0.8f, currentY / 450.0f, line.scale, 1.0f, 1.0f, 1.0f, 1.0f);

            currentY -= 40.0f * line.scale;
        }
    }
}

// ============================================================
//  API
// ============================================================

bool Credits_IsOpen()    { return isCreditsOpen; }
bool Credits_IsHovering() { return isHoveringCredits; }

void Credits_Open()
{
    isCreditsOpen  = true;
    creditsScrollY = -600.0f;
}
