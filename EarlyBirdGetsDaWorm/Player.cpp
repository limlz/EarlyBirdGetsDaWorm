#include "pch.hpp"

// --- animation data ---
static AEGfxVertexList* gSpriteMesh = nullptr;
static AEGfxTexture* gWheelTex[2]{};

static int gFacing = 1; // 1 = facing right, -1 = facing left
static int gWheelFrame = 0;
static float gWheelTimer = 0.0f;

static constexpr float FRAME_TIME = 0.50f;  // walking speed (time per frame)
static constexpr float PLAYER_W = 220.0f;
static constexpr float PLAYER_H = 220.0f;

// @brief: Returns the player's width
float Player_GetWidth()
{
    return PLAYER_W;
}

float Player_GetHeight()
{
    return PLAYER_H;
}

// @brief: Sets the player's facing direction
void Player_SetFacing(int dir)
{
    gFacing = (dir >= 0) ? 1 : -1;
}

// @brief: Loads player resources
void Player_Load()
{
    std::cout << "Player_Load called\n";

    // create ONE sprite mesh with UVs (2 triangles making a quad)
    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    gSpriteMesh = AEGfxMeshEnd();

    // load all 4 frames
    gWheelTex[0] = AEGfxTextureLoad("Assets/playeer_1.png");
    gWheelTex[1] = AEGfxTextureLoad("Assets/playeer_2.png");
}

// @brief: Updates player animation based on input
void Player_Update(float dt, bool walkKey)
{
    if (walkKey) {
        gWheelTimer += dt;
        if (gWheelTimer >= FRAME_TIME) {
            gWheelTimer -= FRAME_TIME;
            gWheelFrame = (gWheelFrame + 1) % 2;
        }
    }
    else {
        gWheelFrame = 0;
        gWheelTimer = 0.0f;
    }
}

// @brief: Draws the player at the specified position
void Player_Draw(float x, float y)
{
    AEMtx33 scale, trans, transform;

    float sx = PLAYER_W * (float)gFacing;   // // 1 = normal, -1 = flipped
    float sy = PLAYER_H;

	// build transform matrix
    AEMtx33Scale(&scale, sx, sy);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    // texture drawing settings (same as demo)
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
    AEGfxSetColorToAdd(0.f, 0.f, 0.f, 0.f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxTextureSet(gWheelTex[gWheelFrame], 0.0f, 0.0f);
    AEGfxSetTransform(transform.m);

    // draw the UV mesh
    AEGfxMeshDraw(gSpriteMesh, AE_GFX_MDM_TRIANGLES);

    // (optional) reset 
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}

// @brief: Unloads player resources
void Player_Unload()
{
    for (int i = 0; i < 2; i++) 
    {
        if (gWheelTex[i]) AEGfxTextureUnload(gWheelTex[i]);
        gWheelTex[i] = nullptr;
    }

    if (gSpriteMesh) 
    {
        AEGfxMeshFree(gSpriteMesh);
        gSpriteMesh = nullptr;
    }
}

