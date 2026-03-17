#include "pch.hpp"

// Shared mesh for all guide quads (all are identical white squares)
static AEGfxVertexList* GuideMesh;

// Textures
static AEGfxTexture* GuideIconTexture;
static AEGfxTexture* GuideBGBackTexture;
static AEGfxTexture* GuideBGTopTexture;
static AEGfxTexture* GuideCategory1Texture;
static AEGfxTexture* GuideCategory2Texture;
static AEGfxTexture* GuideCategory3Texture;
// Image Textures
static AEGfxTexture* GuideImage1Texture;
static AEGfxTexture* GuideImage2Texture;
static AEGfxTexture* GuideImage3Texture;
static AEGfxTexture* GuideImage4Texture;
static AEGfxTexture* GuideImage5Texture;

// Variables
static s8 GuideFontID = -1;
static bool IsGuideOpen;
static f32 FadeSpeed;
static f32 GuidePopTimer;
static f32 GuideAlpha;
static f32 Cat2ScrollY;
static f32 Cat2ScrollSpeed;
static bool Cat2Page1;
static bool Cat2Page2;

// Category Dimensions
static f32 CategoryW;
static f32 CategoryX;

// Category
static int CategoryID;
static f32 Cat1Alpha;
static f32 Cat2Alpha;
static f32 Cat3Alpha;
static f32 CatSpeed;
static bool Cat1Selected;
static bool Cat2Selected;
static bool Cat3Selected;

void Guide_Load() {
	GuideFontID = AEGfxCreateFont(Assets::Fonts::Handwriting1, 50);

	GuideIconTexture = LoadTextureChecked(Assets::Guide::guideIcon);
	GuideBGBackTexture = LoadTextureChecked(Assets::Guide::guideBGBack);
	GuideBGTopTexture = LoadTextureChecked(Assets::Guide::guideBGTop);

	GuideCategory1Texture = LoadTextureChecked(Assets::Guide::category1Text);
	GuideCategory2Texture = LoadTextureChecked(Assets::Guide::category2Text);
	GuideCategory3Texture = LoadTextureChecked(Assets::Guide::category3Text);
	GuideImage1Texture = LoadTextureChecked(Assets::Guide::image1);
	GuideImage2Texture = LoadTextureChecked(Assets::Guide::image2);
	GuideImage3Texture = LoadTextureChecked(Assets::Guide::image3);
	GuideImage4Texture = LoadTextureChecked(Assets::Guide::image4);
	GuideImage5Texture = LoadTextureChecked(Assets::Guide::image5);
}

void Guide_Initialize() {
	// Shared mesh for all guide quads
	GuideMesh = CreateSquareMesh(0xFFFFFFFF);

	// Variabls
	IsGuideOpen = false;
	Cat2ScrollY = 0.0f;
	Cat2ScrollSpeed = 500.0f;
	Cat2Page1 = true;
	Cat2Page2 = false;

	// Fade In Animation
	GuidePopTimer = 0.0f;
	FadeSpeed = 2.5f;
	GuideAlpha = 0.0f;

	// Category Dimensions
	CategoryW = 172.0f;
	CategoryX = 514.0f;

	// Category
	CatSpeed = 10.0f;
	Cat1Alpha = 1.0f;
	Cat2Alpha = 1.0f;
	Cat3Alpha = 1.0f;
	CategoryID = 1;
	Cat1Selected = true;
	Cat2Selected = false;
	Cat3Selected = false;
}

void Guide_Update(bool liftActive, f32 dt, bool pagerActive) {
	if (IsGuideOpen && GuideAlpha < 1.0f) {
		GuideAlpha += FadeSpeed * dt;

		if (GuideAlpha > 1.0f) {
			GuideAlpha = 1.0f;
		}
	}

	if (!IsGuideOpen) {
		GuideAlpha -= FadeSpeed * dt;
		if (GuideAlpha < 0.0f) {
			GuideAlpha = 0.0f;
		}
		Cat2Alpha = 0.5;
		Cat3Alpha = 0.5;
		Cat1Selected = true;
		Cat2Selected = false;
		Cat3Selected = false;
	}

	if (AEInputCheckTriggered(AEVK_G) && !liftActive && !pagerActive)
	{
		IsGuideOpen = !IsGuideOpen;
	}

	// Category Select
	bool Cat1Hover{ IsAreaHovered(CategoryX, 153.0f, CategoryW, 153.0f) },
		Cat2Hover{ IsAreaHovered(CategoryX, 0.0f, CategoryW, 157.0f) },
		Cat3Hover{ IsAreaHovered(CategoryX, -150.0f, CategoryW, 157.0f) };

	// Fade out when button hover 
	if (!Cat1Selected) {
		HoverAnimation(Cat1Alpha, Cat1Hover, CatSpeed, dt);
	}
	if (!Cat2Selected) {
		HoverAnimation(Cat2Alpha, Cat2Hover, CatSpeed, dt);
	}
	if (!Cat3Selected) {
		HoverAnimation(Cat3Alpha, Cat3Hover, CatSpeed, dt);
	}

	if (Cat1Hover && (!Cat2Selected || !Cat3Selected)) {
		Cat1Alpha = 1.0f;
	}
	if (Cat2Hover && (!Cat1Selected || !Cat3Selected)) {
		Cat2Alpha = 1.0f;
	}
	if (Cat3Hover && (!Cat1Selected || !Cat2Selected)) {
		Cat3Alpha = 1.0f;
	}
	

	if (Cat1Hover && AEInputCheckTriggered(AEVK_LBUTTON)) {
		Cat1Alpha = 1.0f;
		CategoryID = 1;
		
	}
	if (Cat2Hover && AEInputCheckTriggered(AEVK_LBUTTON)) {
		Cat2Alpha = 1.0f;
		CategoryID = 2;
		//cat2ScrollY = 0.0f;
	}
	if (Cat3Hover && AEInputCheckTriggered(AEVK_LBUTTON)) {
		Cat3Alpha = 1.0f;
		CategoryID = 3;
	}

	// Arrow-Keys
	if (AEInputCheckTriggered(AEVK_UP)) {
		AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
		--CategoryID;
		if (CategoryID < 1) {
			CategoryID = 1;
		}
	}
	if (AEInputCheckTriggered(AEVK_DOWN)) {
		AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
		++CategoryID;
		if (CategoryID > 3) {
			CategoryID = 3;
		}
	}

	switch (CategoryID) {
	case 1:
		Cat1Selected = true;
		Cat2Selected = false;
		Cat3Selected = false;
		break;
	case 2:
		Cat1Selected = false;
		Cat2Selected = true;
		Cat3Selected = false;
		break;
	case 3:
		Cat1Selected = false;
		Cat2Selected = false;
		Cat3Selected = true;
		
		break;
	}

	if (Cat1Selected) {
		Cat1Alpha = 1.0f;
	}
	if (Cat2Selected) {
		Cat2Alpha = 1.0f;
		if (Cat2Page1 && AEInputCheckTriggered(AEVK_RIGHT)) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			Cat2Page1 = false;
			Cat2Page2 = true;
		}
		else if (Cat2Page2 && AEInputCheckTriggered(AEVK_LEFT)) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			Cat2Page2 = false;
			Cat2Page1 = true;
		}
	}
	if (Cat3Selected) {
		Cat3Alpha = 1.0f;
	}
}

// This will open the guide with the correct image automatically
void Guide_BossFight() {
	// Open the guide
	IsGuideOpen = true;

	// Select the boss fight category
	CategoryID = 3;
	Cat1Selected = false;
	Cat2Selected = false;
	Cat3Selected = true;

	// Fade in smoothly
	GuideAlpha = 0.0f;       // start fade from 0
	GuidePopTimer = 0.0f;

	// Reset category alphas
	Cat1Alpha = 0.5f;
	Cat2Alpha = 0.5f;
	Cat3Alpha = 1.0f;        // non-selected category should be dimmed

	// Reset scroll & pages if needed
	Cat2ScrollY = 0.0f;
	Cat2Page1 = true;
	Cat2Page2 = false;
}

bool IsGuideActive() { return IsGuideOpen; }

void Guide_DrawSmallIcon() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	DrawTextureMesh(GuideMesh, GuideIconTexture, AEGfxGetWinMinX() + 1300.0f, AEGfxGetWinMinY() + 750.0f, 100.0f, 63.0f, 1.0f);

}

void Guide_Draw() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	//DrawTextureMesh(guideIconMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);

	if (IsGuideOpen) {
		// Draw back BG
		DrawTextureMesh(GuideMesh, GuideBGBackTexture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		DrawTextureMesh(GuideMesh, GuideCategory3Texture, CategoryX, -150.0f, CategoryW, 157.0f, Cat3Alpha);
		DrawTextureMesh(GuideMesh, GuideCategory2Texture, CategoryX, 0.0f, CategoryW, 157.0f, Cat2Alpha);
		DrawTextureMesh(GuideMesh, GuideCategory1Texture, CategoryX, 153.0f, CategoryW, 153.0f, Cat1Alpha);



		//// Black BG
		////u32 colorBG = (0x000000 << 8) | static_cast<u32>(guideAlpha * 187.0f);
		//u32 colorBG = (0x880000 << 8) | static_cast<u32>(guideAlpha * 187.0f);
		//DrawSquareMesh(guideMesh, 0.0f, 0.0f, winW, winH, colorBG);

		//// Line that seperates the category and info
		//u32 white = 0xFFFFFFFF;
		//DrawSquareMesh(guideMesh, AEGfxGetWinMinX() + 400.0f, centerY, 1.0f, 700.0f, white);

		//// Category Text
		//DrawTextureMesh(guideMesh, guideCategory1Texture, categoryTopX, categoryTopY, categoryW, categoryH, cat1Alpha);
		//DrawTextureMesh(guideMesh, guideCategory2Texture, categoryTopX, categoryTopY - (categorySpacing), categoryW, categoryH, cat2Alpha);
		//DrawTextureMesh(guideMesh, guideCategory3Texture, categoryTopX, categoryTopY - (categorySpacing * 2.0f), categoryW, categoryH, cat3Alpha);
		//DrawTextureMesh(guideMesh, guideImage5Texture, categoryTopX, (categoryTopY - (categorySpacing * 9.0f)) - 21.5f, categoryW, categoryH, 1.0f);

		AEGfxPrint(GuideFontID, "CONTROLS", textPosition(465.0f, 150.0f).first, textPosition(465.0f, 150.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat1Alpha);
		AEGfxPrint(GuideFontID, "HOW TO", textPosition(465.0f, 10.0f).first, textPosition(465.0f, 10.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat2Alpha);
		AEGfxPrint(GuideFontID, "PLAY", textPosition(465.0f, -10.0f).first, textPosition(465.0f, -10.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat2Alpha);
		AEGfxPrint(GuideFontID, "BOSS FIGHT", textPosition(465.0f, -150.0f).first, textPosition(465.0f, -150.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat3Alpha);

		DrawTextureMesh(GuideMesh, GuideBGTopTexture, -68.0f, 0.0f, 1061.0f, 756.0f, 1.0f);
		if (Cat1Selected) {
			DrawTextureMesh(GuideMesh, GuideImage1Texture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		}
		if (Cat2Selected) {
			if (Cat2Page1) {
				DrawTextureMesh(GuideMesh, GuideImage2Texture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
			}
			else if (Cat2Page2) {
				DrawTextureMesh(GuideMesh, GuideImage3Texture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
			}
		}
		if (Cat3Selected) {
			DrawTextureMesh(GuideMesh, GuideImage4Texture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		}

	}

}

void Guide_Free() {
	FreeMeshSafe(GuideMesh);

	UnloadTextureSafe(GuideIconTexture);
	UnloadTextureSafe(GuideBGBackTexture);
	UnloadTextureSafe(GuideBGTopTexture);
	UnloadTextureSafe(GuideCategory1Texture);
	UnloadTextureSafe(GuideCategory2Texture);
	UnloadTextureSafe(GuideCategory3Texture);
	UnloadTextureSafe(GuideImage1Texture);
	UnloadTextureSafe(GuideImage2Texture);
	UnloadTextureSafe(GuideImage3Texture);
	UnloadTextureSafe(GuideImage4Texture);
	UnloadTextureSafe(GuideImage5Texture);

	AEGfxDestroyFont(GuideFontID);
}