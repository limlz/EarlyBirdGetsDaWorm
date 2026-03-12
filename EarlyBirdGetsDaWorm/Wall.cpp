#include "pch.hpp"
#include <ctime>
#include <cmath>
#include <vector>

// ============================================================
// MESHES
// ============================================================

static AEGfxVertexList* wallMesh = nullptr;

//static AEGfxVertexList* crack1Mesh = nullptr;
//static AEGfxVertexList* crack2Mesh = nullptr;
//static AEGfxVertexList* crack3Mesh = nullptr;
//static AEGfxVertexList* crack4Mesh = nullptr;
//
//static AEGfxVertexList* drawing1Mesh = nullptr;
//static AEGfxVertexList* drawing2Mesh = nullptr;
//static AEGfxVertexList* drawing3Mesh = nullptr;
//
//static AEGfxVertexList* leftHandMesh = nullptr;
//static AEGfxVertexList* rightHandMesh = nullptr;

// ============================================================
// TEXTURES
// ============================================================
#define MAX_IMG 4

static AEGfxTexture* wallBgTexture = nullptr;

static AEGfxTexture* paranoiaTexture[2];
static AEGfxTexture* maniaTexture[MAX_IMG];
static AEGfxTexture* depressionTexture[MAX_IMG];
static AEGfxTexture* dementiaTexture[MAX_IMG];
static AEGfxTexture* schizoTexture[MAX_IMG];
static AEGfxTexture* aiwTexture[MAX_IMG];
static AEGfxTexture* insomniaTexture[MAX_IMG];
static AEGfxTexture* ocdTexture[MAX_IMG];
static AEGfxTexture* scotoTexture[MAX_IMG];

//static AEGfxTexture* crack1Texture = nullptr;
//static AEGfxTexture* crack2Texture = nullptr;
//static AEGfxTexture* crack3Texture = nullptr;
//static AEGfxTexture* crack4Texture = nullptr;
//
//static AEGfxTexture* drawing1Texture = nullptr;
//static AEGfxTexture* drawing2Texture = nullptr;
//static AEGfxTexture* drawing3Texture = nullptr;
//
//static AEGfxTexture* leftHandTexture = nullptr;
//static AEGfxTexture* rightHandTexture = nullptr;

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

// ANimation
static s32 currentImage;
static f32 imageDuration;
static f32 wallTimer;

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
    static const std::vector<ANOMALYID> paranoiaPool{ ANOMALYID::Wall_Paranoia };
    static const std::vector<ANOMALYID> maniaPool{ ANOMALYID::Wall_Mania };
    static const std::vector<ANOMALYID> depressionPool{ ANOMALYID::Wall_Depression };
    static const std::vector<ANOMALYID> dementiaPool{ ANOMALYID::Wall_Dementia };

    static const std::vector<ANOMALYID> schizophreniaPool{ ANOMALYID::Wall_Schizophernia };
    static const std::vector<ANOMALYID> aiwPool{ ANOMALYID::Wall_AIW };
    static const std::vector<ANOMALYID> insomniaPool{ ANOMALYID::Wall_Insomnia };
    static const std::vector<ANOMALYID> ocdPool{ ANOMALYID::Wall_OCD };
    static const std::vector<ANOMALYID> scotophobiaPool{ ANOMALYID::Wall_Scotophobia };

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

    paranoiaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::PARANOIA_BACKGROUND);
    paranoiaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::PARANOIA_TOP);

    maniaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_1);
    maniaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_2);
    maniaTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_3);
    maniaTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::MANIA_4);

    depressionTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_1);
    depressionTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_2);
    depressionTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_3);
    depressionTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::DEPRESSION_4);

    dementiaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_1);
    dementiaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_2);
    dementiaTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_3);
    dementiaTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::DEMENTIA_4);

    schizoTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_1);
    schizoTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_2);
    schizoTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_3);
    schizoTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::SCHIZOPHRENIA_4);

    aiwTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_1);
    aiwTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_2);
    aiwTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_3);
    aiwTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::AIW_SYNDROME_4);

    insomniaTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_1);
    insomniaTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_2);
    insomniaTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_3);
    insomniaTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::INSOMNIA_4);

    ocdTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_1);
    ocdTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_2);
    ocdTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_3);
    ocdTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::OCD_4);

    scotoTexture[0] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_1);
    scotoTexture[1] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_2);
    scotoTexture[2] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_3);
    scotoTexture[3] = LoadTextureChecked(Assets::Wall_Anomaly::SCOTOPHOBIA_4);

    /*crack1Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack1);
    crack2Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack2);
    crack3Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack3);
    crack4Texture = LoadTextureChecked(Assets::Wall_Anomaly::Crack4);

    drawing1Texture = LoadTextureChecked(Assets::Wall_Anomaly::Drawing1);
    drawing2Texture = LoadTextureChecked(Assets::Wall_Anomaly::Drawing2);
    drawing3Texture = LoadTextureChecked(Assets::Wall_Anomaly::Drawing3);

    leftHandTexture = LoadTextureChecked(Assets::Wall_Anomaly::LeftHand);
    rightHandTexture = LoadTextureChecked(Assets::Wall_Anomaly::RightHand);*/
}

void Wall_Initialize()
{
    wallMesh = CreateSquareMesh(0xFFFFFFFF);
    /*crack1Mesh = CreateSquareMesh(0xFFFFFFFF);
    crack2Mesh = CreateSquareMesh(0xFFFFFFFF);
    crack3Mesh = CreateSquareMesh(0xFFFFFFFF);
    crack4Mesh = CreateSquareMesh(0xFFFFFFFF);

    drawing1Mesh = CreateSquareMesh(0xFFFFFFFF);
    drawing2Mesh = CreateSquareMesh(0xFFFFFFFF);
    drawing3Mesh = CreateSquareMesh(0xFFFFFFFF);

    leftHandMesh = CreateSquareMesh(0xFFFFFFFF);
    rightHandMesh = CreateSquareMesh(0xFFFFFFFF);*/

    srand((unsigned)std::time(nullptr));

    // placement randomness (KEEP SAME)
    for (int i = 0; i < MAX_FLOORS; ++i)
    {
        randomX[i] = rand() % 6000;
        randomY[i] = rand() % 200;

        gWallPick[i] = ANOMALYID::None;
        gWallPicked[i] = false;
    }

    currentImage = 0;
    imageDuration = 0.5f;
    wallTimer = 0.5f;
}

void Wall_Update(f32 dt) {
    wallTimer += dt;

    if (wallTimer >= imageDuration)
    {
        wallTimer = 0.0f;
        currentImage++;

        if (currentImage >= MAX_IMG)
            currentImage = 0; // loop animation
    }

}

void Wall_Draw(f32 camX, s8 floorNum)
{
    int idx = (int)floorNum;
    if (idx < 0) idx = 0;
    if (idx >= MAX_FLOORS) idx = MAX_FLOORS - 1;

    // ---- WALL BACKGROUND (world-anchored tiling) ----
    if (wallBgTexture && wallMesh)
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
            DrawTextureMesh(wallMesh, wallBgTexture, sx, 0.0f, TILE_W, TILE_H, 1.0f);
        }
    }

    if (!gWallEnabled) return;
    if (!Player_HasPatient()) return;

    ILLNESSES TrueDiagnosis = Player_GetTrueDiagnosis();

    // Position of anomalies (KEEP SAME)
    wallX = camX + (f32)randomX[idx];
    wallY = (f32)randomY[idx];

    // Pick once per floor based on current illness
    if (!gWallPicked[idx] || gLastIllness[idx] != TrueDiagnosis)
    {
        gWallPick[idx] = GetWallPickByIllness(TrueDiagnosis); 
        gWallPicked[idx] = true;
        gLastIllness[idx] = TrueDiagnosis;

        //// Sound
        //if (gWallPick[idx] == ANOMALYID::Wall_Drawing1 ||
        //    gWallPick[idx] == ANOMALYID::Wall_Drawing2 ||
        //    gWallPick[idx] == ANOMALYID::Wall_Drawing3)
        //{
        //    AudioManager_PlaySFX(SFX_MAIN_MENU_WRITING_SCRATCH, 0.5f);
        //}
    }

    switch (gWallPick[idx])
    {
    case ANOMALYID::Wall_Paranoia:
        //DrawTextureMesh(crack1Mesh, crack1Texture, wallX, wallY, 200.0f, 179.0f, 1.0f);
        /*for (int i{ ; i < 2; ++i) {
            DrawTextureMesh(wallMesh, paranoiaTexture[i], wallX, wallY, 200.0f, 179.0f, 1.0f);
                }*/
        break;
    case ANOMALYID::Wall_Mania:
        //DrawTextureMesh(crack2Mesh, crack2Texture, wallX, wallY, 200.0f, 279.0f, 1.0f);
            DrawTextureMesh(wallMesh, maniaTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Depression:
        //DrawTextureMesh(crack3Mesh, crack3Texture, wallX, wallY, 200.0f, 167.0f, 1.0f);
            DrawTextureMesh(wallMesh, depressionTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Dementia:
        //DrawTextureMesh(crack4Mesh, crack4Texture, wallX, wallY, 10.0f, 119.0f, 1.0f);
            DrawTextureMesh(wallMesh, dementiaTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Schizophernia:
        //DrawTextureMesh(drawing1Mesh, drawing1Texture, wallX, wallY, 300.0f, 246.0f, 1.0f);
            DrawTextureMesh(wallMesh, schizoTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_AIW:
        //DrawTextureMesh(drawing2Mesh, drawing2Texture, wallX, wallY, 400.0f, 300.0f, 1.0f);
            DrawTextureMesh(wallMesh, aiwTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Insomnia:
        //DrawTextureMesh(drawing3Mesh, drawing3Texture, wallX, wallY, 300.0f, 181.0f, 1.0f);
            DrawTextureMesh(wallMesh, insomniaTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_OCD:
        //DrawTextureMesh(leftHandMesh, leftHandTexture, wallX, wallY, 100.0f, 100.0f, 1.0f);
            DrawTextureMesh(wallMesh, ocdTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;
    case ANOMALYID::Wall_Scotophobia:
        //DrawTextureMesh(rightHandMesh, rightHandTexture, wallX, wallY, 100.0f, 100.0f, 1.0f);
            DrawTextureMesh(wallMesh, scotoTexture[currentImage], wallX, wallY, 200.0f, 279.0f, 1.0f);
        break;

    case ANOMALYID::None:
    default:
        break;
    }
}

void Wall_Unload()
{
    FreeMeshSafe(wallMesh);

    /*FreeMeshSafe(crack1Mesh);
    FreeMeshSafe(crack2Mesh);
    FreeMeshSafe(crack3Mesh);
    FreeMeshSafe(crack4Mesh);

    FreeMeshSafe(drawing1Mesh);
    FreeMeshSafe(drawing2Mesh);
    FreeMeshSafe(drawing3Mesh);

    FreeMeshSafe(leftHandMesh);
    FreeMeshSafe(rightHandMesh);*/

    UnloadTextureSafe(wallBgTexture);

    /*UnloadTextureSafe(crack1Texture);
    UnloadTextureSafe(crack2Texture);
    UnloadTextureSafe(crack3Texture);
    UnloadTextureSafe(crack4Texture);

    UnloadTextureSafe(drawing1Texture);
    UnloadTextureSafe(drawing2Texture);
    UnloadTextureSafe(drawing3Texture);

    UnloadTextureSafe(leftHandTexture);
    UnloadTextureSafe(rightHandTexture);*/

    for (int i{}; i < 2; ++i) {
        UnloadTextureSafe(paranoiaTexture[i]);
    }

    for (int i{}; i < MAX_IMG; ++i) {
        UnloadTextureSafe(maniaTexture[i]);
        UnloadTextureSafe(depressionTexture[i]);
        UnloadTextureSafe(dementiaTexture[i]);
        UnloadTextureSafe(schizoTexture[i]);
        UnloadTextureSafe(aiwTexture[i]);
        UnloadTextureSafe(insomniaTexture[i]);
        UnloadTextureSafe(ocdTexture[i]);
        UnloadTextureSafe(scotoTexture[i]);
    }

}

//std::vector<AEGfxTexture*> Wall_GetAnomalies()
//{
//    return {
//        //maniaTexture
//        crack1Texture, crack2Texture, crack3Texture, crack4Texture,
//        drawing1Texture, drawing2Texture, drawing3Texture,
//        leftHandTexture, rightHandTexture
//    };
//}