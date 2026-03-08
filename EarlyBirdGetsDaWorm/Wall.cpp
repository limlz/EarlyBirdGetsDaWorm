#include "pch.hpp"
#include <ctime>
#include <cmath>
#include <vector>

// ============================================================
// MESHES
// ============================================================

static AEGfxVertexList* wallBgMesh = nullptr;

static AEGfxVertexList* crack1Mesh = nullptr;
static AEGfxVertexList* crack2Mesh = nullptr;
static AEGfxVertexList* crack3Mesh = nullptr;
static AEGfxVertexList* crack4Mesh = nullptr;

static AEGfxVertexList* drawing1Mesh = nullptr;
static AEGfxVertexList* drawing2Mesh = nullptr;
static AEGfxVertexList* drawing3Mesh = nullptr;

static AEGfxVertexList* leftHandMesh = nullptr;
static AEGfxVertexList* rightHandMesh = nullptr;

// ============================================================
// TEXTURES
// ============================================================

static AEGfxTexture* wallBgTexture = nullptr;

static AEGfxTexture* crack1Texture = nullptr;
static AEGfxTexture* crack2Texture = nullptr;
static AEGfxTexture* crack3Texture = nullptr;
static AEGfxTexture* crack4Texture = nullptr;

static AEGfxTexture* drawing1Texture = nullptr;
static AEGfxTexture* drawing2Texture = nullptr;
static AEGfxTexture* drawing3Texture = nullptr;

static AEGfxTexture* leftHandTexture = nullptr;
static AEGfxTexture* rightHandTexture = nullptr;

// ============================================================
// STATE / VARIABLES
// ============================================================

#define MAX_FLOORS 10

// Keep your placement randomness the same (DON'T TOUCH)
static s32 randomX[MAX_FLOORS]{};
static s32 randomY[MAX_FLOORS]{};

// Chosen wall anomaly per floor (picked once, consistent)
static ANOMALYID gWallPick[MAX_FLOORS]{};
static bool        gWallPicked[MAX_FLOORS]{};
static ILLNESSES   gLastIllness[MAX_FLOORS]{};
static bool        gWallEnabled = true;

void Wall_SetEnabled(bool enabled)              
{
    gWallEnabled = enabled;

    // optional: force repick when re-enabled
    for (int i = 0; i < MAX_FLOORS; ++i)
        gWallPicked[i] = false;
}

// draw position
static f32 wallX = 0.0f;
static f32 wallY = 0.0f;

// ============================================================
// HELPERS: illness -> pool
// ============================================================

static ANOMALYID PickFromPool(std::vector<ANOMALYID> const& pool)
{
    if (pool.empty()) return ANOMALYID::None;
    int r = rand() % (int)pool.size();
    return pool[r];
}

static ANOMALYID GetWallPickByIllness(ILLNESSES illness)
{
    // Pools to match dictionary exactly
    static const std::vector<ANOMALYID> paranoiaPool{ ANOMALYID::Wall_LeftHand };
    static const std::vector<ANOMALYID> maniaPool{ ANOMALYID::Wall_Crack2 };
    static const std::vector<ANOMALYID> depressionPool{ ANOMALYID::Wall_Drawing1 };
    static const std::vector<ANOMALYID> dementiaPool{ ANOMALYID::Wall_Drawing3 };

    static const std::vector<ANOMALYID> schizophreniaPool{ ANOMALYID::Wall_RightHand };
    static const std::vector<ANOMALYID> aiwPool{ ANOMALYID::Wall_Drawing2 };
    static const std::vector<ANOMALYID> insomniaPool{ ANOMALYID::Wall_Crack1 };
    static const std::vector<ANOMALYID> ocdPool{ ANOMALYID::Wall_Crack4 };
    static const std::vector<ANOMALYID> scotophobiaPool{ ANOMALYID::Wall_LeftHand };

    switch (illness)
    {
    case ILLNESSES::PARANOIA:       return PickFromPool(paranoiaPool);
    case ILLNESSES::MANIA:          return PickFromPool(maniaPool);
    case ILLNESSES::DEPRESSION:     return PickFromPool(depressionPool);
    case ILLNESSES::DEMENTIA:       return PickFromPool(dementiaPool);

    case ILLNESSES::SCHIZOPHRENIA:  return PickFromPool(schizophreniaPool);
    case ILLNESSES::AIW_SYNDROME:   return PickFromPool(aiwPool);
    case ILLNESSES::INSOMNIA:       return PickFromPool(insomniaPool);
    case ILLNESSES::OCD:            return PickFromPool(ocdPool);
    case ILLNESSES::SCOTOPHOBIA:    return PickFromPool(scotophobiaPool);

    default:             return ANOMALYID::None;
    }
}

// ============================================================
// PUBLIC API
// ============================================================

void Wall_Load()
{
    wallBgTexture = LoadTextureChecked(Assets::Background::WallBg);

    crack1Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack1);
    crack2Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack2);
    crack3Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack3);
    crack4Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack4);

    drawing1Texture = LoadTextureChecked(Assets::Wall_Anomaly::Drawing1);
    drawing2Texture = LoadTextureChecked(Assets::Wall_Anomaly::Drawing2);
    drawing3Texture = LoadTextureChecked(Assets::Wall_Anomaly::Drawing3);

    leftHandTexture = LoadTextureChecked(Assets::Wall_Anomaly::LeftHand);
    rightHandTexture = LoadTextureChecked(Assets::Wall_Anomaly::RightHand);
}

void Wall_Initialize()
{
    wallBgMesh = CreateSquareMesh(0xFFFFFFFF);

    crack1Mesh = CreateSquareMesh(0xFFFFFFFF);
    crack2Mesh = CreateSquareMesh(0xFFFFFFFF);
    crack3Mesh = CreateSquareMesh(0xFFFFFFFF);
    crack4Mesh = CreateSquareMesh(0xFFFFFFFF);

    drawing1Mesh = CreateSquareMesh(0xFFFFFFFF);
    drawing2Mesh = CreateSquareMesh(0xFFFFFFFF);
    drawing3Mesh = CreateSquareMesh(0xFFFFFFFF);

    leftHandMesh = CreateSquareMesh(0xFFFFFFFF);
    rightHandMesh = CreateSquareMesh(0xFFFFFFFF);

    srand((unsigned)std::time(nullptr));

    // placement randomness (KEEP SAME)
    for (int i = 0; i < MAX_FLOORS; ++i)
    {
        randomX[i] = rand() % 6000;
        randomY[i] = rand() % 200;

        gWallPick[i] = ANOMALYID::None;
        gWallPicked[i] = false;
    }
}

void Wall_Draw(f32 camX, s8 floorNum)
{
    int idx = (int)floorNum;
    if (idx < 0) idx = 0;
    if (idx >= MAX_FLOORS) idx = MAX_FLOORS - 1;

    // ---- WALL BACKGROUND (world-anchored tiling) ----
    if (wallBgTexture && wallBgMesh)
    {
        const float TILE_W = 1600.0f;
        const float TILE_H = 900.0f;

        const float VIEW_W = 1600.0f;
        const float HALF_W = VIEW_W * 0.5f;

        const float worldLeft = (-HALF_W) - camX;
        const float worldRight = (HALF_W)-camX;

        float firstTileWorldX = std::floor(worldLeft / TILE_W) * TILE_W;

        for (float wx = firstTileWorldX; wx <= worldRight + TILE_W; wx += TILE_W)
        {
            float sx = wx + camX;
            DrawTextureMesh(wallBgMesh, wallBgTexture, sx, 0.0f, TILE_W, TILE_H, 1.0f);
        }
    }

    if (!gWallEnabled) return;
    if (!Player_HasPatient()) return;

    ILLNESSES currentIllness = Player_GetCurrentIllness();

    // Position of anomalies (KEEP SAME)
    wallX = camX + (f32)randomX[idx];
    wallY = (f32)randomY[idx];

    // Pick once per floor based on current illness
    if (!gWallPicked[idx] || gLastIllness[idx] != currentIllness)
    {
        gWallPick[idx] = GetWallPickByIllness(currentIllness); 
        gWallPicked[idx] = true;
        gLastIllness[idx] = currentIllness;

        // Sound
        if (gWallPick[idx] == ANOMALYID::Wall_Drawing1 ||
            gWallPick[idx] == ANOMALYID::Wall_Drawing2 ||
            gWallPick[idx] == ANOMALYID::Wall_Drawing3)
        {
            AudioManager_PlaySFX(SFX_MAIN_MENU_WRITING_SCRATCH, 0.5f);
        }
    }

    switch (gWallPick[idx])
    {
    case ANOMALYID::Wall_Crack1:
        DrawTextureMesh(crack1Mesh, crack1Texture, wallX, wallY, 200.0f, 179.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Crack2:
        DrawTextureMesh(crack2Mesh, crack2Texture, wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Crack3:
        DrawTextureMesh(crack3Mesh, crack3Texture, wallX, wallY, 200.0f, 167.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Crack4:
        DrawTextureMesh(crack4Mesh, crack4Texture, wallX, wallY, 10.0f, 119.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Drawing1:
        DrawTextureMesh(drawing1Mesh, drawing1Texture, wallX, wallY, 300.0f, 246.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Drawing2:
        DrawTextureMesh(drawing2Mesh, drawing2Texture, wallX, wallY, 400.0f, 300.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Drawing3:
        DrawTextureMesh(drawing3Mesh, drawing3Texture, wallX, wallY, 300.0f, 181.0f, 1.0f);
        break;
    case ANOMALYID::Wall_LeftHand:
        DrawTextureMesh(leftHandMesh, leftHandTexture, wallX, wallY, 100.0f, 100.0f, 1.0f);
        break;
    case ANOMALYID::Wall_RightHand:
        DrawTextureMesh(rightHandMesh, rightHandTexture, wallX, wallY, 100.0f, 100.0f, 1.0f);
        break;

    case ANOMALYID::None:
    default:
        break;
    }
}

void Wall_Unload()
{
    FreeMeshSafe(wallBgMesh);

    FreeMeshSafe(crack1Mesh);
    FreeMeshSafe(crack2Mesh);
    FreeMeshSafe(crack3Mesh);
    FreeMeshSafe(crack4Mesh);

    FreeMeshSafe(drawing1Mesh);
    FreeMeshSafe(drawing2Mesh);
    FreeMeshSafe(drawing3Mesh);

    FreeMeshSafe(leftHandMesh);
    FreeMeshSafe(rightHandMesh);

    UnloadTextureSafe(wallBgTexture);

    UnloadTextureSafe(crack1Texture);
    UnloadTextureSafe(crack2Texture);
    UnloadTextureSafe(crack3Texture);
    UnloadTextureSafe(crack4Texture);

    UnloadTextureSafe(drawing1Texture);
    UnloadTextureSafe(drawing2Texture);
    UnloadTextureSafe(drawing3Texture);

    UnloadTextureSafe(leftHandTexture);
    UnloadTextureSafe(rightHandTexture);
}

std::vector<AEGfxTexture*> Wall_GetAnomalies()
{
    return {
        crack1Texture, crack2Texture, crack3Texture, crack4Texture,
        drawing1Texture, drawing2Texture, drawing3Texture,
        leftHandTexture, rightHandTexture
    };
}