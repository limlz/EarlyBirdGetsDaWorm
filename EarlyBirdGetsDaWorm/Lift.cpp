// ============================================================
// LIFT MODULE
// - Draws lift(s) in the hallway (world)
// - Handles lift overlay (sliding doors + panel buttons)
// - Handles lift interaction + floor selection input
// ============================================================

#include "pch.hpp"

// ============================================================
// CONFIG
// ============================================================

static constexpr float LIFT_TIMER = 2.0f;   // seconds for the lift overlay door animation


// ============================================================
// RENDER DATA (Texture + Mesh)
// ============================================================

static s8 liftFontId = -1;                 // font handle used by AEGfxPrint
static AEGfxVertexList* gQuadMesh = nullptr; // quad mesh used by DrawTextureMesh

    gLiftDoorTex = LoadTextureChecked(Assets::Background::LiftDoor);
    gLiftPanelTex = LoadTextureChecked(Assets::Background::LiftPanel);

    UnloadTextureSafe(gLiftDoorTex);
    UnloadTextureSafe(gLiftPanelTex);
static AEGfxTexture* gLiftPanelTex = nullptr; // overlay level button panel texture


// ============================================================
// STATE
// ============================================================

static bool  gLiftActive = false;    // true when lift overlay is open
static bool  gNearLift = false;    // true when player is near a lift entrance
static float gLiftAnimTimer = 0.0f;     // counts down from LIFT_TIMER to 0


// ============================================================
// LOAD / INIT / UNLOAD
// ============================================================

void Lift_Load()
{
    liftFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
    gLiftTex = LoadTextureChecked(Assets::Background::LiftBg);
    gLiftDoorTex = AEGfxTextureLoad(Assets::Background::LiftDoor);
    gLiftPanelTex = AEGfxTextureLoad(Assets::Background::LiftPanel);
}

void Lift_Initialize()
{
    // Quad mesh used by DrawTextureMesh (initialized once)
    gQuadMesh = CreateSquareMesh(0xFFFFFFFF);

    // Reset runtime state
    gLiftActive = false;
    gNearLift = false;
    gLiftAnimTimer = 0.0f;
}

void Lift_Unload()
{
    // Unload world texture + quad mesh using your safe helpers
    UnloadTextureSafe(gLiftTex);
    FreeMeshSafe(gQuadMesh);

    // Unload overlay textures
    if (gLiftDoorTex) { AEGfxTextureUnload(gLiftDoorTex);  gLiftDoorTex = nullptr; }
    if (gLiftPanelTex) { AEGfxTextureUnload(gLiftPanelTex); gLiftPanelTex = nullptr; }

    // Optional: free font (only if your engine provides it)
    // AEGfxDestroyFont(liftFontId);
    // liftFontId = -1;
}


// ============================================================
// UPDATE (interaction + pause logic)
// ============================================================

void Lift_Update(float /*dt*/, float camX, float maxDist)
{
    // --------------------------------------------
    // 1) Detect if player is near a lift
    //    - Left lift is near the start of hallway
    //    - Right lift is near the end of hallway
    // --------------------------------------------
    const bool nearLeftLift = (camX > -5.0f);
    const bool nearRightLift = (camX < -(maxDist - 5.0f));
    gNearLift = (nearLeftLift || nearRightLift);

    // --------------------------------------------
    // 2) Toggle lift overlay with 'L' (only if near)
    // --------------------------------------------
    if (gNearLift && AEInputCheckTriggered(AEVK_L))
    {
        gLiftActive = !gLiftActive;

        // When opening, start the door animation timer
        if (gLiftActive)
            gLiftAnimTimer = LIFT_TIMER;
    }

    // --------------------------------------------
    // 3) Auto-close lift overlay if player walks away
    // --------------------------------------------
    if (!gNearLift)
        gLiftActive = false;

    // --------------------------------------------
    // 4) Pause the game timer while lift overlay is open
    // --------------------------------------------
    Timer_SetPaused(gLiftActive);
}


// ============================================================
// HANDLE FLOOR INPUT (0..NUM_OF_FLOOR-1)
// ============================================================

void Lift_HandleInput(s8& floorNum)
{
    if (!gLiftActive) return;

    // Number keys 0..9 (or however many floors you have)
    for (int i = 0; i < NUM_OF_FLOOR; ++i)
    {
        if (AEInputCheckTriggered(AEVK_0 + i))
        {
            floorNum = (s8)i;
            gLiftActive = false; // close after selecting
            break;
        }
    }
}


// ============================================================
// DRAW (WORLD LIFT + FLOOR LABEL)
// ------------------------------------------------------------
// This draws the lift box in the hallway itself (the one you
// replaced from grey rectangles in Game_Draw).
//
// Pass in x/y/w/h as the world lift placement (already includes camX).
// ============================================================

void Lift_DrawWorld(AEGfxVertexList* squareMesh,
    float x, float y, float w, float h,
    s8 floorNum, float /*textXoffset*/, float textY)
{
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // --------------------------------------------
    // 1) LIFT BASE (world object)
    // --------------------------------------------
    if (gQuadMesh && gLiftTex)
        DrawTextureMesh(gQuadMesh, gLiftTex, x, y, w, h, 1.0f);
    else
        DrawSquareMesh(squareMesh, x, y, w, h, COLOR_LIGHT_GREY);

    // --------------------------------------------
    // 2) FLOOR NUMBER PANEL (background box for text)
    // --------------------------------------------
    const float panelWidth = w * 0.40f;
    const float panelHeight = h * 0.10f;

    const float panelX = x;                // centered to lift
    const float panelY = textY + 20.0f;    // tweak to move it up/down

    // Border (slightly bigger)
    const float borderPad = 4.0f;
    DrawSquareMesh(squareMesh,
        panelX, panelY,
        panelWidth + borderPad, panelHeight + borderPad,
        COLOR_WHITE);

    // Inner fill
    DrawSquareMesh(squareMesh,
        panelX, panelY,
        panelWidth, panelHeight,
        COLOR_DARK_GREY);

    // --------------------------------------------
    // 3) FLOOR NUMBER TEXT (centered + GLOW)
    // --------------------------------------------
    if (liftFontId >= 0)
    {
        char textBuffer[32]{};

        if (floorNum == 0) sprintf_s(textBuffer, "B1");
        else               sprintf_s(textBuffer, "%02d", (int)floorNum);

        const float CHAR_NDC = 0.018f;
        const int   len = (int)strlen(textBuffer);
        const float halfTextWidthNDC = (len * CHAR_NDC) * 0.5f;

        const float panelNDC_X = panelX / SCREEN_WIDTH_HALF;
        const float panelNDC_Y = panelY / SCREEN_HEIGHT_HALF;

        const float TEXT_CENTER_Y_TWEAK = -0.025f;

        const float textNDC_X = panelNDC_X - halfTextWidthNDC;
        const float textNDC_Y = panelNDC_Y + TEXT_CENTER_Y_TWEAK;

        const float SCALE = 1.0f;

        // -----------------------------
        // Glow (Darker Red)
        // -----------------------------
        //AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        const float GLOW_O = 0.006f;

        // Dark red glow (outer)
        const float glowR = 0.45f;
        const float glowG = 0.0f;
        const float glowB = 0.0f;
        const float glowA = 0.75f;

        AEGfxPrintWithGlow(liftFontId, textBuffer, textNDC_X, textNDC_Y,
            SCALE,
            1.0f, 0.1f, 0.1f, 1.0f,
            glowR, glowG, glowB, glowA,
            GLOW_O);
    }
}


// ============================================================
// DRAW (LIFT OVERLAY UI)
// ------------------------------------------------------------
// This draws the full-screen lift overlay when gLiftActive == true:
// - sliding doors texture
// - when opened, show the level button panel texture
// ============================================================

void Lift_Draw(AEGfxVertexList* squareMesh)
{
    if (!gLiftActive) return;

    // --------------------------------------------
    // 1) Update animation timer (countdown)
    // --------------------------------------------
    float dt = (f32)AEFrameRateControllerGetFrameTime();
    if (gLiftAnimTimer > 0.0f) gLiftAnimTimer -= dt;
    if (gLiftAnimTimer < 0.0f) gLiftAnimTimer = 0.0f;

    // gapOffset controls how far doors slide apart
    float gapOffset = (gLiftAnimTimer / 4.0f) * SCREEN_W;

    // --------------------------------------------
    // 2) Draw sliding doors (PNG replaces the old grey rectangles)
    // --------------------------------------------
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    const float doorW = SCREEN_WIDTH_HALF + (0.5f * LIFT_WIDTH);
    const float doorH = (f32)SCREEN_H;

    // LEFT door
    if (gQuadMesh && gLiftDoorTex)
    {
        DrawTextureMesh(gQuadMesh, gLiftDoorTex,
            -450.0f - gapOffset, 0.0f,
            doorW, doorH,
            1.0f);
    }
    else
    {
        // fallback if texture fails
        DrawSquareMesh(squareMesh, -450.0f - gapOffset, 0.0f, doorW, doorH, COLOR_LIFT_BG);
    }

    // RIGHT door
    if (gQuadMesh && gLiftDoorTex)
    {
        DrawTextureMesh(gQuadMesh, gLiftDoorTex,
            450.0f + gapOffset, 0.0f,
            doorW, doorH,
            1.0f);
    }
    else
    {
        // fallback if texture fails
        DrawSquareMesh(squareMesh, 450.0f + gapOffset, 0.0f, doorW, doorH, COLOR_LIFT_BG);
    }

    // --------------------------------------------
    // 3) When doors fully opened: show panel buttons
    // --------------------------------------------
    if (gLiftAnimTimer <= 0.0f)
    {
        // Background behind the panel (optional)
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, LIFT_DOOR_WIDTH, LIFT_DOOR_HEIGHT, COLOR_BLACK);

        // Panel PNG
        if (gQuadMesh && gLiftPanelTex)
        {
            DrawTextureMesh(gQuadMesh, gLiftPanelTex,
                0.0f, 0.0f,
                450.0f, 750.0f,
                1.0f);
        }
        else
        {
            // fallback if texture fails
            DrawSquareMesh(squareMesh, 0.0f, 0.0f, 450.0f, 750.0f, COLOR_LIFT_CONSOLE);
        }
    }
}


// ============================================================
// GETTERS
// ============================================================

bool Lift_IsActive() { return gLiftActive; }
bool Lift_IsNear() { return gNearLift; }