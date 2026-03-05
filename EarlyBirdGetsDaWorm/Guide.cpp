#include "pch.hpp"

// Mesh
static AEGfxVertexList* guideIconMesh;
static AEGfxVertexList* guideBGMesh;
static AEGfxVertexList* guideCategoryMesh;
static AEGfxVertexList* guideSeperatorMesh;
//Image Meshes
static AEGfxVertexList* guideImage1Mesh;
static AEGfxVertexList* guideImage2Mesh;
static AEGfxVertexList* guideImage3Mesh;
static AEGfxVertexList* guideImage4Mesh;
static AEGfxVertexList* guideImage5Mesh;

// Textures
static AEGfxTexture* guideIconTexture;
static AEGfxTexture* guideCategoryTexture;
// Image Textures
static AEGfxTexture* guideImage1Texture;
static AEGfxTexture* guideImage2Texture;
static AEGfxTexture* guideImage3Texture;
static AEGfxTexture* guideImage4Texture;
static AEGfxTexture* guideImage5Texture;

// Variables
static bool isGuideOpen;
static f32 fadeSpeed;
static f32 guidePopTimer;
static f32 guideAlpha;

// Seperator
static f32 seperateH;
static f32 centerY;
static f32 topY;

// Category
static f32 categoryTopX;
static f32 categoryTopY;
static f32 categorySpacing;
static f32 categoryW;
static f32 categoryH;
static f32 cat1Alpha;
static f32 cat2Alpha;
static f32 cat3Alpha;
static f32 catSpeed;
static bool cat1Selected;
static bool cat2Selected;
static bool cat3Selected;

void Guide_Load() {
	guideIconTexture = LoadTextureChecked(Assets::Guide::guideIcon);
	guideCategoryTexture = LoadTextureChecked(Assets::Guide::categoryText);
	guideImage1Texture = LoadTextureChecked(Assets::Guide::image1);
	guideImage2Texture = LoadTextureChecked(Assets::Guide::image2);
	guideImage3Texture = LoadTextureChecked(Assets::Guide::image3);
	guideImage4Texture = LoadTextureChecked(Assets::Guide::image4);
	guideImage5Texture = LoadTextureChecked(Assets::Guide::image5);
}

void Guide_Initialize() {
	// Mesh
	guideIconMesh = CreateSquareMesh(0xFFFFFFFF);
	guideBGMesh = CreateSquareMesh(0xFFFFFFFF);
	guideCategoryMesh = CreateSquareMesh(0xFFFFFFFF);
	guideSeperatorMesh = CreateSquareMesh(0xFFFFFFFF);
	guideImage1Mesh = CreateSquareMesh(0xFFFFFFFF);
	guideImage2Mesh = CreateSquareMesh(0xFFFFFFFF);
	guideImage3Mesh = CreateSquareMesh(0xFFFFFFFF);
	guideImage4Mesh = CreateSquareMesh(0xFFFFFFFF);
	guideImage5Mesh = CreateSquareMesh(0xFFFFFFFF);

	// Variabls
	isGuideOpen = false;

	// Fade In Animation
	guidePopTimer = 0.0f;
	fadeSpeed = 2.5f;
	guideAlpha = 0.0f;

	// Seperator
	seperateH = 700.0f;
	centerY = (AEGfxGetWinMinY() + AEGfxGetWinMaxY()) * 0.5f;
	topY = centerY + (seperateH * 0.5f);

	// Category
	categoryTopX = AEGfxGetWinMinX() + 220.0f;
	categoryTopY = topY - (50.0f * 0.5f);
	categorySpacing = 70.0f;
	categoryW = 250.0f;
	categoryH = 50.0f;
	catSpeed = 10.0f;
	cat1Alpha = 1.0f;
	cat2Alpha = 1.0f;
	cat3Alpha = 1.0f;
	cat1Selected = true;
	cat2Selected = false;
	cat3Selected = false;
}

void Guide_Update(bool liftActive, f32 dt, bool pagerActive) {
	if (isGuideOpen && guideAlpha < 1.0f) {
		guideAlpha += fadeSpeed * dt;

		if (guideAlpha > 1.0f) {
			guideAlpha = 1.0f;
		}
	}

	if (!isGuideOpen) {
		guideAlpha -= fadeSpeed * dt;
		if (guideAlpha < 0.0f) {
			guideAlpha = 0.0f;
		}
		cat1Alpha = 0.5;
		cat1Selected = true;
		cat2Selected = false;
		cat3Selected = false;
	}

	if (AEInputCheckTriggered(AEVK_G) && !liftActive && !pagerActive)
	{
		isGuideOpen = !isGuideOpen;
	}

	// Category Select
	bool cat1Hover{ IsAreaClicked(categoryTopX, categoryTopY, categoryW, categoryH, Input_GetMouseX(), Input_GetMouseY()) },
		cat2Hover{ IsAreaClicked(categoryTopX, categoryTopY - (categorySpacing), categoryW, categoryH, Input_GetMouseX(), Input_GetMouseY()) },
		cat3Hover{ IsAreaClicked(categoryTopX, categoryTopY - (categorySpacing * 2.0f), categoryW, categoryH, Input_GetMouseX(), Input_GetMouseY()) };
	// Fade out when button hover 
	f32 catTargetAlpha = cat1Hover ? 0.5f : 1.0f;
	cat1Alpha += (catTargetAlpha - cat1Alpha) * catSpeed * dt;
	catTargetAlpha = cat2Hover ? 0.5f : 1.0f;
	cat2Alpha += (catTargetAlpha - cat2Alpha) * catSpeed * dt;
	catTargetAlpha = cat3Hover ? 0.5f : 1.0f;
	cat3Alpha += (catTargetAlpha - cat3Alpha) * catSpeed * dt;

	if (cat1Alpha < 0.5f) { cat1Alpha = 0.5f; }
	if (cat1Alpha > 1.0f) { cat1Alpha = 1.0f; }
	if (cat2Alpha < 0.5f) { cat2Alpha = 0.5f; }
	if (cat2Alpha > 1.0f) { cat2Alpha = 1.0f; }
	if (cat3Alpha < 0.5f) { cat3Alpha = 0.5f; }
	if (cat3Alpha > 1.0f) { cat3Alpha = 1.0f; }

	if (cat1Hover && AEInputCheckCurr(AEVK_LBUTTON)) {
		cat1Alpha = 0.5;
		cat1Selected = true;
		cat2Selected = false;
		cat3Selected = false;
	}
	if (cat2Hover && AEInputCheckCurr(AEVK_LBUTTON)) {
		cat2Alpha = 0.5f;
		cat1Selected = false;
		cat2Selected = true;
		cat3Selected = false;
	}
	if (cat3Hover && AEInputCheckCurr(AEVK_LBUTTON)) {
		cat3Alpha = 0.5f;
		cat1Selected = false;
		cat2Selected = false;
		cat3Selected = true;
	}

	if (cat1Selected) {
		cat1Alpha = 0.5;
	}
	if (cat2Selected) {
		cat2Alpha = 0.5f;
	}
	if (cat3Selected) {
		cat3Alpha = 0.5f;
	}
}

bool IsGuideActive() { return isGuideOpen; }

void Guide_DrawSmallIcon() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	DrawTextureMesh(guideIconMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);

}

void Guide_Draw() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	//DrawTextureMesh(guideIconMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);

	if (isGuideOpen) {
		// Black BG
		u32 black = (0x000000 << 8) | static_cast<u32>(guideAlpha * 187.0f);
		DrawSquareMesh(guideBGMesh, 0.0f, 0.0f, winW, winH, black);

		// Line that seperates the category and info
		u32 white = 0xFFFFFFFF;
		DrawSquareMesh(guideSeperatorMesh, AEGfxGetWinMinX() + 400.0f, centerY, 1.0f, 700.0f, white);

		// Category Text 
		DrawTextureMesh(guideCategoryMesh, guideCategoryTexture, categoryTopX, categoryTopY, categoryW, categoryH, cat1Alpha);
		DrawTextureMesh(guideCategoryMesh, guideCategoryTexture, categoryTopX, categoryTopY - (categorySpacing), categoryW, categoryH, cat2Alpha);
		DrawTextureMesh(guideCategoryMesh, guideCategoryTexture, categoryTopX, categoryTopY - (categorySpacing * 2.0f), categoryW, categoryH, cat3Alpha);

		if (cat1Selected) {
			DrawTextureMesh(guideImage1Mesh, guideImage1Texture, 180.0f, centerY, categoryW + 800.0f, 700.0f, 1.0f);
		}
		if (cat2Selected) {
			DrawTextureMesh(guideImage2Mesh, guideImage2Texture, 180.0f, centerY, categoryW + 800.0f, 700.0f, 1.0f);
		}
		if (cat3Selected) {
			DrawTextureMesh(guideImage3Mesh, guideImage3Texture, 180.0f, centerY, categoryW + 800.0f, 700.0f, 1.0f);
		}

	}

}

void Guide_Free() {
	FreeMeshSafe(guideIconMesh);
	FreeMeshSafe(guideBGMesh);
	FreeMeshSafe(guideCategoryMesh);
	FreeMeshSafe(guideSeperatorMesh);
	FreeMeshSafe(guideImage1Mesh);
	FreeMeshSafe(guideImage2Mesh);
	FreeMeshSafe(guideImage3Mesh);
	FreeMeshSafe(guideImage4Mesh);
	FreeMeshSafe(guideImage5Mesh);

	UnloadTextureSafe(guideIconTexture);
	UnloadTextureSafe(guideCategoryTexture);
	UnloadTextureSafe(guideImage1Texture);
	UnloadTextureSafe(guideImage2Texture);
	UnloadTextureSafe(guideImage3Texture);
	UnloadTextureSafe(guideImage4Texture);
	UnloadTextureSafe(guideImage5Texture);
}