#include "pch.hpp"

// Shared mesh for all guide quads (all are identical white squares)
static AEGfxVertexList* guideMesh;

// Textures
static AEGfxTexture* guideIconTexture;
static AEGfxTexture* guideCategory1Texture;
static AEGfxTexture* guideCategory2Texture;
static AEGfxTexture* guideCategory3Texture;
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
static f32 cat2ScrollY;
static f32 cat2ScrollSpeed;
static bool cat2Page1;
static bool cat2Page2;

// Seperator
static f32 seperateH;
static f32 centerY;
static f32 topY;

// Category
static f32 categoryTopX;
static f32 categoryTopY;
static f32 categorySpacing;
static int categoryID;
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
	guideCategory1Texture = LoadTextureChecked(Assets::Guide::category1Text);
	guideCategory2Texture = LoadTextureChecked(Assets::Guide::category2Text);
	guideCategory3Texture = LoadTextureChecked(Assets::Guide::category3Text);
	guideImage1Texture = LoadTextureChecked(Assets::Guide::image1);
	guideImage2Texture = LoadTextureChecked(Assets::Guide::image2);
	guideImage3Texture = LoadTextureChecked(Assets::Guide::image3);
	guideImage4Texture = LoadTextureChecked(Assets::Guide::image4);
	guideImage5Texture = LoadTextureChecked(Assets::Guide::image5);
}

void Guide_Initialize() {
	// Shared mesh for all guide quads
	guideMesh = CreateSquareMesh(0xFFFFFFFF);

	// Variabls
	isGuideOpen = false;
	cat2ScrollY = 0.0f;
	cat2ScrollSpeed = 500.0f;
	cat2Page1 = true;
	cat2Page2 = false;

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
	categoryID = 1;
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
		cat2Alpha = 0.5;
		cat3Alpha = 0.5;
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
	f32 catTargetAlpha = cat1Hover ? 1.0f : 0.5f;
	cat1Alpha += (catTargetAlpha - cat1Alpha) * catSpeed * dt;
	catTargetAlpha = cat2Hover ? 1.0f : 0.5f;
	cat2Alpha += (catTargetAlpha - cat2Alpha) * catSpeed * dt;
	catTargetAlpha = cat3Hover ? 1.0f : 0.5f;
	cat3Alpha += (catTargetAlpha - cat3Alpha) * catSpeed * dt;

	cat1Alpha = AEClamp(cat1Alpha, 0.5f, 1.0f);
	cat2Alpha = AEClamp(cat2Alpha, 0.5f, 1.0f);
	cat3Alpha = AEClamp(cat3Alpha, 0.5f, 1.0f);

	if (cat1Hover) {
		cat1Alpha = 1.0;
	}
	if (cat2Hover) {
		cat2Alpha = 1.0;
	}
	if (cat3Hover) {
		cat3Alpha = 1.0;
	}

	if (cat1Hover && AEInputCheckCurr(AEVK_LBUTTON)) {
		cat1Alpha = 1.0f;
		categoryID = 1;
		
	}
	if (cat2Hover && AEInputCheckCurr(AEVK_LBUTTON)) {
		cat2Alpha = 1.0f;
		categoryID = 2;
		//cat2ScrollY = 0.0f;
	}
	if (cat3Hover && AEInputCheckCurr(AEVK_LBUTTON)) {
		cat3Alpha = 1.0f;
		categoryID = 3;
	}

	// Arrow-Keys
	if (AEInputCheckTriggered(AEVK_UP) || AEInputCheckTriggered(AEVK_LEFT)) {
		AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
		--categoryID;
		if (categoryID < 1) {
			categoryID = 1;
		}
	}
	if (AEInputCheckTriggered(AEVK_DOWN) || AEInputCheckTriggered(AEVK_RIGHT)) {
		AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
		++categoryID;
		if (categoryID > 3) {
			categoryID = 3;
		}
	}

	switch (categoryID) {
	case 1:
		cat1Selected = true;
		cat2Selected = false;
		cat3Selected = false;
		break;
	case 2:
		cat1Selected = false;
		cat2Selected = true;
		cat3Selected = false;
		break;
	case 3:
		cat1Selected = false;
		cat2Selected = false;
		cat3Selected = true;
		
		break;
	}

	if (cat1Selected) {
		cat1Alpha = 1.0f;
	}
	if (cat2Selected) {
		cat2Alpha = 1.0f;
		if (cat2Page1 && AEInputCheckTriggered(AEVK_D)) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			cat2Page1 = false;
			cat2Page2 = true;
		}
		else if (cat2Page2 && AEInputCheckTriggered(AEVK_A)) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			cat2Page2 = false;
			cat2Page1 = true;
		}
	}
	if (cat3Selected) {
		cat3Alpha = 1.0f;
	}
}

// This will open the guide with the correct image automatically
void Guide_BossFight() {
	// Open the guide
	isGuideOpen = true;

	// Select the boss fight category
	categoryID = 3;
	cat1Selected = false;
	cat2Selected = false;
	cat3Selected = true;

	// Fade in smoothly
	guideAlpha = 0.0f;       // start fade from 0
	guidePopTimer = 0.0f;

	// Reset category alphas
	cat1Alpha = 0.5f;
	cat2Alpha = 0.5f;
	cat3Alpha = 1.0f;        // non-selected category should be dimmed

	// Reset scroll & pages if needed
	cat2ScrollY = 0.0f;
	cat2Page1 = true;
	cat2Page2 = false;
}

bool IsGuideActive() { return isGuideOpen; }

void Guide_DrawSmallIcon() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	DrawTextureMesh(guideMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);

}

void Guide_Draw() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	//DrawTextureMesh(guideIconMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);

	if (isGuideOpen) {
		// Black BG
		//u32 colorBG = (0x000000 << 8) | static_cast<u32>(guideAlpha * 187.0f);
		u32 colorBG = (0x880000 << 8) | static_cast<u32>(guideAlpha * 187.0f);
		DrawSquareMesh(guideMesh, 0.0f, 0.0f, winW, winH, colorBG);

		// Line that seperates the category and info
		u32 white = 0xFFFFFFFF;
		DrawSquareMesh(guideMesh, AEGfxGetWinMinX() + 400.0f, centerY, 1.0f, 700.0f, white);

		// Category Text
		DrawTextureMesh(guideMesh, guideCategory1Texture, categoryTopX, categoryTopY, categoryW, categoryH, cat1Alpha);
		DrawTextureMesh(guideMesh, guideCategory2Texture, categoryTopX, categoryTopY - (categorySpacing), categoryW, categoryH, cat2Alpha);
		DrawTextureMesh(guideMesh, guideCategory3Texture, categoryTopX, categoryTopY - (categorySpacing * 2.0f), categoryW, categoryH, cat3Alpha);
		DrawTextureMesh(guideMesh, guideImage5Texture, categoryTopX, (categoryTopY - (categorySpacing * 9.0f)) - 21.5f, categoryW, categoryH, 1.0f);

		if (cat1Selected) {
			DrawTextureMesh(guideMesh, guideImage1Texture, 180.0f, centerY, categoryW + 800.0f, 700.0f, 1.0f);
		}
		if (cat2Selected) {
			if (cat2Page1) {
				DrawTextureMesh(guideMesh, guideImage2Texture, 180.0f, centerY, categoryW + 800.0f, 700.0f, 1.0f);
			}
			else if (cat2Page2) {
				DrawTextureMesh(guideMesh, guideImage3Texture, 180.0f, centerY, categoryW + 800.0f, 700.0f, 1.0f);
			}
		}
		if (cat3Selected) {
			DrawTextureMesh(guideMesh, guideImage4Texture, 180.0f, centerY, categoryW + 800.0f, 700.0f, 1.0f);
		}

	}

}

void Guide_Free() {
	FreeMeshSafe(guideMesh);

	UnloadTextureSafe(guideIconTexture);
	UnloadTextureSafe(guideCategory1Texture);
	UnloadTextureSafe(guideCategory2Texture);
	UnloadTextureSafe(guideCategory3Texture);
	UnloadTextureSafe(guideImage1Texture);
	UnloadTextureSafe(guideImage2Texture);
	UnloadTextureSafe(guideImage3Texture);
	UnloadTextureSafe(guideImage4Texture);
	UnloadTextureSafe(guideImage5Texture);
}