#include "pch.hpp"
#include <ctime>

// Mesh
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

	crack1Texture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK1.png");
	crack2Texture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK2.png");
	crack3Texture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK3.png");
	crack4Texture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_CRACK4.png");

	drawing1Texture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_DRAWING1.png");
	drawing2Texture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_DRAWING2.png");
	drawing3Texture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_DRAWING3.png");

	leftHandTexture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_LEFTHAND.png");
	rightHandTexture = AEGfxTextureLoad("Assets/Wall_Anomaly/WALL_ANOMALY_RIGHTHAND.png");

	return;
}

void Wall_Initialize() {

	// Mesh
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
		random[i] = rand() % 18; 

		// Variables
		randomX[i] = rand() % 6000;
		randomY[i] = rand() % 200;

	}
	wallX = camX;
	wallY = 0.0f;
}

void Wall_Draw(f32 camX, s8 floorNum) {

	// Position of anomalies
	wallX = camX + randomX[floorNum];
	wallY = static_cast<f32>(randomY[floorNum]);

	// Draws the anomaly
	switch (random[floorNum]) {
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
