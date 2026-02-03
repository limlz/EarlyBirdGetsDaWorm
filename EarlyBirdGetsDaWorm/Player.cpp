#include "pch.hpp"

// ------------------------------
// CONFIG
// ------------------------------
static constexpr float FRAME_TIME = 0.50f;  // walking speed (time per frame)
static constexpr float PLAYER_W = 220.0f;
static constexpr float PLAYER_H = 220.0f;

static AEGfxVertexList* gSpriteMesh = nullptr;

// 2 frames per set
static AEGfxTexture* gHumanTex[2]{};
static AEGfxTexture* gScaryTex[2]{};

static bool  gIsScary = false;
static int   gFacing = 1;     // 1 right, -1 left
static int   gFrame = 0;
static float gTimer = 0.0f;
ILLNESSES gCurrentIllness{};


static AEGfxTexture* GetActiveFrameTex()
{
    return gIsScary ? gScaryTex[gFrame] : gHumanTex[gFrame];
}

// DEBUG
void Player_SetScary(bool scary)
{
    gIsScary = scary;
    gFrame = 0;
    gTimer = 0.0f;
}

void Player_SetIllness(ILLNESSES illness) {
	gCurrentIllness = illness;
}


// @brief: Sets the player's facing direction
void Player_SetFacing(int dir)
{
    gFacing = (dir >= 0) ? 1 : -1;
}

// @brief: Randomly determines if the new patient is scary or not
bool Player_IsScaryPatient()
{
    return gIsScary;
}

ILLNESSES Player_GetCurrentIllness()
{
    return gCurrentIllness;
}

void Player_NewPatientRandom()
{
    // reset animation so patient swap doesn’t “jump” frames
    gFrame = 0;
    gTimer = 0.0f;

    // 50/50 random
    gIsScary = (std::rand() % 2) == 1;
    
    if (!gIsScary) {
        // Randomly pick 1 of the 4 illnesses (0 to 3)
        gCurrentIllness = (ILLNESSES)(std::rand() % 4);
    } else {
		gCurrentIllness = ALL;
    }
}

// @brief: Loads player resources
void Player_Load()
{
    std::srand((unsigned)std::time(nullptr));

    // mesh (your existing quad)
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    gSpriteMesh = AEGfxMeshEnd();

    // load BOTH sets (names exactly as you wrote)
    gHumanTex[0] = AEGfxTextureLoad("Assets/Player/human player_1.png");
    gHumanTex[1] = AEGfxTextureLoad("Assets/Player/human player_2.png");

    gScaryTex[0] = AEGfxTextureLoad("Assets/Player/scary player_1.png");
    gScaryTex[1] = AEGfxTextureLoad("Assets/Player/scary player_2.png");

    // pick first patient at game start
    Player_NewPatientRandom();
}

// @brief: Updates player animation based on input
void Player_Update(float dt, bool walkKey)
{
    if (walkKey)
    {
        gTimer += dt;
        if (gTimer >= FRAME_TIME)
        {
            gTimer -= FRAME_TIME;
            gFrame = (gFrame + 1) % 2; // 2 frames
        }
    }
    else
    {
        gFrame = 0;
        gTimer = 0.0f;
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

    AEGfxTextureSet(GetActiveFrameTex(), 0.0f, 0.0f);
    AEGfxSetTransform(transform.m);

    // draw the UV mesh
    AEGfxMeshDraw(gSpriteMesh, AE_GFX_MDM_TRIANGLES);

    // (optional) reset 
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
}

// @brief: Unloads player resources
void Player_Unload()
{
    for (int i = 0; i < 2; ++i)
    {
        if (gHumanTex[i]) AEGfxTextureUnload(gHumanTex[i]);
        if (gScaryTex[i]) AEGfxTextureUnload(gScaryTex[i]);
        gHumanTex[i] = nullptr;
        gScaryTex[i] = nullptr;
    }

    if (gSpriteMesh)
    {
        AEGfxMeshFree(gSpriteMesh);
        gSpriteMesh = nullptr;
    }
}

// @brief: Returns the player's 
float Player_GetWidth() { return PLAYER_W; }
float Player_GetHeight() { return PLAYER_H; }