#include "pch.hpp"

// Mesh
static AEGfxVertexList* guideIconMesh;
static AEGfxVertexList* guideBGMesh;

// Textures
static AEGfxTexture* guideIconTexture;

// Variables
static bool isGuideOpen{ false };
static f32 fadeSpeed{ 2.5f };
static f32 guidePopTimer{ 0.0f };
static f32 guideAlpha{ 0.0f };

void Guide_Load() {
	guideIconTexture = LoadTextureChecked(Assets::Guide::guideIcon);
}

void Guide_Initialize() {
	// Mesh
	guideIconMesh = CreateSquareMesh(0xFFFFFFFF);
	guideBGMesh = CreateSquareMesh(0xFFFFFFFF);

	// Variabls
	isGuideOpen = false;

	// Fade In Animation
	guidePopTimer = 0.0f;
	fadeSpeed = 2.5f;
	guideAlpha = 0.0f;
}

void Guide_Update(bool liftActive, f32 dt, bool pagerActive) {
	if (isGuideOpen && guideAlpha < 1.0f) {
		guideAlpha += fadeSpeed * dt;

		if (guideAlpha > 1.0f) {
			guideAlpha = 1.0f;
		}
	}

	if (!isGuideOpen) {
		guideAlpha = 0.0f;
	}

	if (AEInputCheckTriggered(AEVK_G) && !liftActive && !pagerActive)
	{
		isGuideOpen = !isGuideOpen;
	}
}

bool IsGuideActive() { return isGuideOpen; }

void Guide_Draw() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	DrawTextureMesh(guideIconMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);

	if (isGuideOpen) {
		u32 black = (0x000000 << 8) | static_cast<u32>(guideAlpha * 187.0f);
		DrawSquareMesh(guideBGMesh, 0.0f, 0.0f, winW, winH, black);

	}

}

void Guide_Free() {
	FreeMeshSafe(guideIconMesh);
	FreeMeshSafe(guideBGMesh);
	UnloadTextureSafe(guideIconTexture);
}