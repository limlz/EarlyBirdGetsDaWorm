#include "pch.hpp"

// ============================================================
// CONFIG
// ============================================================

static constexpr float LIFT_TIMER = 2.0f;      // seconds for the door animation to finish

// If you still use the UI lift interior, this is the size of that UI panel.
static constexpr float LIFT_UI_W = 500.0f;
static constexpr float LIFT_UI_H = 800.0f;

// ============================================================
// RENDER DATA (Texture + Mesh)
// ============================================================

static AEGfxVertexList* gQuadMesh = nullptr;    // quad mesh used for drawing textures
static AEGfxTexture* gLiftTex = nullptr;        // lift PNG texture

// ============================================================
// STATE
// ============================================================

static bool  gLiftActive = false;           // whether lift overlay/UI is active
static bool  gNearLift = false;             // player near lift area
static float gLiftAnimTimer = 0.0f;         // counts down from LIFT_TIMER to 0


// ============================================================
// LOAD / INIT / UNLOAD
// ============================================================

void Lift_Load()
{
    // Load lift art (must be called from Game_Load)
    gLiftTex = LoadTextureChecked("Assets/Background/Lift_bg.png");
}

void Lift_Initialize()
{
    // Create the quad mesh once (must be called from Game_Initialize)
    gQuadMesh = CreateSquareMesh(0xFFFFFFFF);

    // Reset runtime state
    gLiftActive = false;
    gNearLift = false;
    gLiftAnimTimer = 0.0f;
}

void Lift_Unload()
{
    // Unload texture + mesh (must be called from Game_Unload)
    UnloadTextureSafe(gLiftTex);
    FreeMeshSafe(gQuadMesh);
}


// ============================================================
// UPDATE (open/close + pause timer)
// ============================================================

void Lift_Update(float /*dt*/, float camX, float maxDist)
{
    // --------------------------------------------------------
    // Detect if player is near either end lift
    // (tweak these thresholds if needed)
    // --------------------------------------------------------
    const bool nearLeftLift = (camX > -5.0f);
    const bool nearRightLift = (camX < -(maxDist - 5.0f));

    gNearLift = (nearLeftLift || nearRightLift);

    // --------------------------------------------------------
    // Toggle lift with 'L' (only if near lift)
    // --------------------------------------------------------
    if (gNearLift && AEInputCheckTriggered(AEVK_L))
    {
        gLiftActive = !gLiftActive;

        // Start animation when opening
        if (gLiftActive)
            gLiftAnimTimer = LIFT_TIMER;
    }

    // Auto-close when walking away
    if (!gNearLift)
        gLiftActive = false;

    // Pause game timer when lift overlay is open
    Timer_SetPaused(gLiftActive);
}


// ============================================================
// HANDLE FLOOR INPUT (0..NUM_OF_FLOOR-1)
// ============================================================

void Lift_HandleInput(s8& floorNum)
{
    if (!gLiftActive) return;

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
// DRAW (WORLD LIFT)  <--- THIS replaces your grey rectangles in Game_Draw
// ============================================================
// Use this for the lift that exists in the hallway scene.
// Example call from Game_Draw:
//   Lift_DrawWorld(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT);
// ============================================================

void Lift_DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h)
{
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Draw lift PNG if available, else fallback to grey rectangle
    if (gQuadMesh && gLiftTex)
        DrawTextureMesh(gQuadMesh, gLiftTex, x, y, w, h, 1.0f);
    else
        DrawSquareMesh(squareMesh, x, y, w, h, COLOR_LIFT_GREY);
}


// ============================================================
// DRAW (OPTIONAL UI OVERLAY)
// ============================================================
// This is your old door-animation overlay. Call it only if you still want it.
// (In your Game_Draw you currently call Lift_Draw(squareMesh); at the end.)
// ============================================================

void Lift_Draw(AEGfxVertexList* squareMesh)
{
    if (!gLiftActive) return;

    // ----------------------------
    // Update animation timer
    // ----------------------------
    float dt = (f32)AEFrameRateControllerGetFrameTime();
    if (gLiftAnimTimer > 0.0f) gLiftAnimTimer -= dt;
    if (gLiftAnimTimer < 0.0f) gLiftAnimTimer = 0.0f;

    // ----------------------------
    // Sliding door overlay (full-screen style)
    // (This is your existing animation)
    // ----------------------------
    float gapOffset = (gLiftAnimTimer / 4.0f) * SCREEN_W;

    DrawSquareMesh(squareMesh, -450.0f - gapOffset, 0.0f, SCREEN_WIDTH_HALF + (0.5f * LIFT_WIDTH), (f32)SCREEN_H, COLOR_LIFT_GREY);
    DrawSquareMesh(squareMesh, 450.0f + gapOffset, 0.0f, SCREEN_WIDTH_HALF + (0.5f * LIFT_WIDTH), (f32)SCREEN_H, COLOR_LIFT_GREY);

    // ----------------------------
    // Once opened: show interior panel (optional)
    // ----------------------------
    if (gLiftAnimTimer <= 0.0f)
    {
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 2.0f, (f32)SCREEN_H, COLOR_BLACK);

        // If you want the UI to be the PNG instead, swap these squares with DrawTextureMesh.
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, LIFT_UI_W, LIFT_UI_H, COLOR_LIFT_BG);
        DrawSquareMesh(squareMesh, 0.0f, 0.0f, 400.0f, 700.0f, COLOR_LIFT_CONSOLE);
    }
}


// ============================================================
// GETTERS
// ============================================================

bool Lift_IsActive() { return gLiftActive; }
bool Lift_IsNear() { return gNearLift; }
