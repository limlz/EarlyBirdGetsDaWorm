#include "pch.hpp"
#include "Credits.hpp"
#include "assets_catalog.hpp"
#include "utils.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>

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
//  SCENE CHARACTERS
// ============================================================

static constexpr int   WALKER_COUNT      = 3;
static constexpr float SCENE_LEFT        = -850.0f;
static constexpr float SCENE_RIGHT       =  850.0f;
static constexpr float WALKER_Y          = -360.0f;  // closer to ground
static constexpr float ARA_BASE_Y        = -190.0f;  // flies higher
static constexpr float ARA_FLY_AMPLITUDE = 35.0f;    // reduced to prevent top-clip
static constexpr float ARA_FLY_FREQ      = 1.8f;
static constexpr float SCENE_CHAR_ALPHA  = 0.85f;    // slight transparency to blend with overlay

struct SceneChar {
    AEGfxTexture* frames[4] = {};
    int   frameCount   = 0;
    int   currentFrame = 0;
    float animTimer    = 0.0f;
    float animSpeed    = 0.18f; // seconds per frame
    float x            = 0.0f;
    float speedX       = 80.0f;
    float w            = 80.0f;
    float h            = 120.0f;
};

static SceneChar walkers[WALKER_COUNT]; // Nad, LZ, Sya
static SceneChar araChar;
static AEGfxTexture* graveTex[2] = {};
static float araFlyTimer = 0.0f;

static void UpdateSceneChar(SceneChar& c, float dt)
{
    c.animTimer += dt;
    if (c.animTimer >= c.animSpeed) {
        c.animTimer -= c.animSpeed;
        c.currentFrame = (c.currentFrame + 1) % c.frameCount;
    }
    c.x += c.speedX * dt;
    // Wrap in whichever direction the character travels
    if (c.speedX > 0.0f && c.x > SCENE_RIGHT + c.w)
        c.x = SCENE_LEFT - c.w;
    else if (c.speedX < 0.0f && c.x < SCENE_LEFT - c.w)
        c.x = SCENE_RIGHT + c.w;
}

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

    // Walkers: Nad, LZ, Sya
    const char* const* walkerSources[WALKER_COUNT] = {
        Assets::Background::OutsidePersonaNad,
        Assets::Background::OutsidePersonaLZ,
        Assets::Background::OutsidePersonaSya,
    };
    // Nad and LZ walk right; Sya walks left (counter-direction for variety)
    const float walkerSpeeds[WALKER_COUNT]  = { 90.0f, 75.0f, -105.0f };
    // Evenly spread so all 3 are visible immediately when credits open
    const float walkerStartX[WALKER_COUNT]  = { -400.0f, 100.0f, 600.0f };
    const float walkerAnimSp[WALKER_COUNT]  = { 0.18f, 0.22f, 0.16f };
    // Slight Y offset per walker gives a sense of depth (back → front)
    const float walkerOffsetY[WALKER_COUNT] = { 20.0f, 0.0f, -15.0f };
    // Per-character explicit sizes: Nad, LZ, Sya
    const float walkerW[WALKER_COUNT]       = { 110.0f, 160.0f, 110.0f };
    const float walkerH[WALKER_COUNT]       = { 160.0f, 170.0f, 160.0f };

    for (int i = 0; i < WALKER_COUNT; i++) {
        walkers[i].frameCount   = 4;
        walkers[i].currentFrame = i; // stagger starting frames
        walkers[i].animTimer    = 0.0f;
        walkers[i].animSpeed    = walkerAnimSp[i];
        walkers[i].x            = walkerStartX[i];
        walkers[i].speedX       = walkerSpeeds[i];
        walkers[i].w            = walkerW[i];
        walkers[i].h            = walkerH[i];
        for (int j = 0; j < 4; j++)
            walkers[i].frames[j] = LoadTextureChecked(walkerSources[i][j]);
    }

    // Ara (flyer) — moves RIGHT TO LEFT so the left-facing sprite is correct
    araChar.frameCount   = 4;
    araChar.currentFrame = 0;
    araChar.animTimer    = 0.0f;
    araChar.animSpeed    = 0.17f;  // slowed to better match travel speed
    araChar.x            = 850.0f;   // start from right edge
    araChar.speedX       = -160.0f;  // negative = flies left
    araChar.w            = 140.0f;
    araChar.h            = 110.0f;
    for (int j = 0; j < 4; j++)
        araChar.frames[j] = LoadTextureChecked(Assets::Background::OutsidePersonaAra[j]);

    // Graves (static, bottom right) — larger so they read clearly
    graveTex[0] = LoadTextureChecked(Assets::Background::OutsidePersonaGrave[0]);
    graveTex[1] = LoadTextureChecked(Assets::Background::OutsidePersonaGrave[1]);
}

void Credits_Unload()
{
    parsedCredits.clear();
    parsedCredits.shrink_to_fit();

    UnloadTextureSafe(credits_guy);
    UnloadTextureSafe(credits_guy2);
    UnloadTextureSafe(close_btn_tex);
    if (creditsFontId >= 0) { AEGfxDestroyFont(creditsFontId); creditsFontId = -1; }

    for (int i = 0; i < WALKER_COUNT; i++)
        for (int j = 0; j < 4; j++)
            UnloadTextureSafe(walkers[i].frames[j]);

    for (int j = 0; j < 4; j++)
        UnloadTextureSafe(araChar.frames[j]);

    UnloadTextureSafe(graveTex[0]);
    UnloadTextureSafe(graveTex[1]);
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

        // Update scene characters
        for (int i = 0; i < WALKER_COUNT; i++)
            UpdateSceneChar(walkers[i], dt);
        UpdateSceneChar(araChar, dt);
        araFlyTimer += dt;

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

        // Draw order: graves → walkers → Ara → text
        // Graves behind, walkers walk in front of them

        // Graves — drawn first so walkers pass in front
        if (graveTex[0]) DrawTextureMesh(mesh, graveTex[0], 400.0f, WALKER_Y, 190.0f, 150.0f, SCENE_CHAR_ALPHA);
        if (graveTex[1]) DrawTextureMesh(mesh, graveTex[1], 600.0f, WALKER_Y, 190.0f, 150.0f, SCENE_CHAR_ALPHA);

        // Walkers — in front of graves
        static const float kWalkerOffsetY[WALKER_COUNT] = { 20.0f, 0.0f, -15.0f };
        for (int i = 0; i < WALKER_COUNT; i++) {
            const SceneChar& c = walkers[i];
            if (c.frames[c.currentFrame])
                DrawTextureMesh(mesh, c.frames[c.currentFrame], c.x, WALKER_Y + kWalkerOffsetY[i], c.w, c.h, SCENE_CHAR_ALPHA);
        }

        // Ara — flying with sine wave altitude, above walkers and graves
        float araY = ARA_BASE_Y + sinf(araFlyTimer * ARA_FLY_FREQ) * ARA_FLY_AMPLITUDE;
        if (araChar.frames[araChar.currentFrame])
            DrawTextureMesh(mesh, araChar.frames[araChar.currentFrame], araChar.x, araY, araChar.w, araChar.h, SCENE_CHAR_ALPHA);

        // Scrolling credits text drawn ON TOP of scene characters
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

bool Credits_IsOpen()     { return isCreditsOpen; }
bool Credits_IsHovering() { return isHoveringCredits; }

void Credits_Open()
{
    isCreditsOpen  = true;
    creditsScrollY = -600.0f;
    araFlyTimer    = 0.0f;
}
