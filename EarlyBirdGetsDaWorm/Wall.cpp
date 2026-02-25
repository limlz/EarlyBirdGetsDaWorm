#include "pch.hpp"
#include <ctime>

// Mesh
static AEGfxVertexList* wallBgMesh = nullptr;

static AEGfxVertexList* crack1Mesh;
static AEGfxVertexList* crack2Mesh;
static AEGfxVertexList* crack3Mesh;
static AEGfxVertexList* crack4Mesh;

static AEGfxVertexList* drawing1Mesh;
static AEGfxVertexList* drawing2Mesh;
static AEGfxVertexList* drawing3Mesh;

static AEGfxVertexList* leftHandMesh;
static AEGfxVertexList* rightHandMesh;

// Textures
static AEGfxTexture* wallBgTexture = nullptr;

static AEGfxTexture* crack1Texture;
static AEGfxTexture* crack2Texture;
static AEGfxTexture* crack3Texture;
static AEGfxTexture* crack4Texture;

static AEGfxTexture* drawing1Texture;
static AEGfxTexture* drawing2Texture;
static AEGfxTexture* drawing3Texture;

static AEGfxTexture* leftHandTexture;
static AEGfxTexture* rightHandTexture;

// Variables
#define MAX_FLOORS 10
static s32 random[MAX_FLOORS], randomX[MAX_FLOORS], randomY[MAX_FLOORS];
static f32 wallX, wallY, camX;

void Wall_Load() {
	wallBgTexture = LoadTextureChecked("Assets/Background/WALL_BG.png");


	crack1Texture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK1.png");
	crack2Texture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK2.png");
	crack3Texture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK3.png");
	crack4Texture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK4.png");

	drawing1Texture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_DRAWING1.png");
	drawing2Texture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_DRAWING2.png");
	drawing3Texture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_DRAWING3.png");

	leftHandTexture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_LEFTHAND.png");
	rightHandTexture = LoadTextureChecked("Assets/Wall_Anomaly/WALL_ANOMALY_RIGHTHAND.png");

	return;
}

void Wall_Initialize() {

	// Mesh
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

	// Generate random numbers using time
	srand((unsigned)std::time(nullptr));

	// Loop to change anomaly for every floor
	for (int i{}; i < MAX_FLOORS; ++i) {
		// 50% chance of seeing anomaly, if u want to see an anomaly
		// for each refresh then change it to % 9
		random[i] = rand() % 9; 

		// Variables
		randomX[i] = rand() % 6000;
		randomY[i] = rand() % 200;

	}
	wallX = camX;
	wallY = 0.0f;
}

void Wall_Draw(f32 camX, s8 floorNum) {
	// Clamp floor index (prevents out-of-bounds crash)
	int idx = (int)floorNum;
	if (idx < 0) idx = 0;
	if (idx >= MAX_FLOORS) idx = MAX_FLOORS - 1;

	// ---- WALL BACKGROUND (world-anchored tiling) ----
	if (wallBgTexture && wallBgMesh)
	{
		const float TILE_W = 1600.0f;
		const float TILE_H = 900.0f;

		// Your visible width looks like 1600 in world units
		const float VIEW_W = 1600.0f;
		const float HALF_W = VIEW_W * 0.5f;

		// Convert screen edges to WORLD coordinates
		// screenX = worldX + camX  =>  worldX = screenX - camX
		const float worldLeft = (-HALF_W) - camX;
		const float worldRight = (HALF_W)-camX;

		// Find the first tile that covers worldLeft
		float firstTileWorldX = std::floor(worldLeft / TILE_W) * TILE_W;

		// Draw tiles across the visible world range
		for (float wx = firstTileWorldX; wx <= worldRight + TILE_W; wx += TILE_W)
		{
			// Convert world position back to screen position
			float sx = wx + camX;
			DrawTextureMesh(wallBgMesh, wallBgTexture, sx, 0.0f, TILE_W, TILE_H, 1.0f);
		}
	}

	// Position of anomalies
	wallX = camX + (f32)randomX[idx];
	wallY = (f32)randomY[idx];

	// Draws the anomaly
	switch (random[idx]) {
	case 0:
		DrawTextureMesh(crack1Mesh, crack1Texture, wallX, wallY, 200.0f, 179.0f, 1.0f);
		break;
	case 1:
		DrawTextureMesh(crack2Mesh, crack2Texture, wallX, wallY, 200.0f, 279.0f, 1.0f);
		break;
	case 2:
		DrawTextureMesh(crack3Mesh, crack3Texture, wallX, wallY, 200.0f, 167.0f, 1.0f);
		break;
	case 3:
		DrawTextureMesh(crack4Mesh, crack4Texture, wallX, wallY, 10.0f, 119.0f, 1.0f);
		break;
	case 4:
		DrawTextureMesh(drawing1Mesh, drawing1Texture, wallX, wallY, 300.0f, 246.0f, 1.0f);
		break;
	case 5:
		DrawTextureMesh(drawing2Mesh, drawing2Texture, wallX, wallY, 400.0f, 300.0f, 1.0f);
		break;
	case 6:
		DrawTextureMesh(drawing3Mesh, drawing3Texture, wallX, wallY, 300.0f, 181.0f, 1.0f);
		break;
	case 7:
		DrawTextureMesh(leftHandMesh, leftHandTexture, wallX, wallY, 100.0f, 100.0f, 1.0f);
		break;
	case 8:
		DrawTextureMesh(rightHandMesh, rightHandTexture, wallX, wallY, 100.0f, 100.0f, 1.0f);
		break;
	default:
		break;
	}
}

void Wall_Unload() {
    // Free meshes
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

    // Free textures
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
