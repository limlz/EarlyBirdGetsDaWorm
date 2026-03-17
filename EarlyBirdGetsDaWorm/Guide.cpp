#include "pch.hpp"

// Shared mesh for all guide quads (all are identical white squares)
static AEGfxVertexList* GuideMesh;

// Textures
static AEGfxTexture* GuideIconTexture;
static AEGfxTexture* GuideBGTopTexture;
static AEGfxTexture* GuideBGBackTexture;
static AEGfxTexture* GuideArrowLeftTexture;
static AEGfxTexture* GuideArrowRightTexture;

static AEGfxTexture* GuideCategory1Texture;
static AEGfxTexture* GuideCategory2Texture;
static AEGfxTexture* GuideCategory3Texture;
// Image Textures
static AEGfxTexture* GuideImage1Texture;
static AEGfxTexture* GuideImage2Texture;
static AEGfxTexture* GuideImage3Texture;
static AEGfxTexture* GuideImage4Texture;

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

// Arrow Dimensions
static f32 ArrowW;
static f32 ArrowH;
static f32 ArrowX;
static f32 ArrowY;
static f32 ArrowAlpha;

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

	GuideIconTexture = LoadTextureChecked(Assets::Guide::GuideIcon);
	GuideBGTopTexture = LoadTextureChecked(Assets::Guide::GuideBGTop);
	GuideBGBackTexture = LoadTextureChecked(Assets::Guide::GuideBGBack);
	GuideArrowLeftTexture = LoadTextureChecked(Assets::Guide::GuideArrowLeft);
	GuideArrowRightTexture = LoadTextureChecked(Assets::Guide::GuideArrowRight);

	GuideCategory1Texture = LoadTextureChecked(Assets::Guide::Category1Text);
	GuideCategory2Texture = LoadTextureChecked(Assets::Guide::Category2Text);
	GuideCategory3Texture = LoadTextureChecked(Assets::Guide::Category3Text);

	GuideImage1Texture = LoadTextureChecked(Assets::Guide::Image1);
	GuideImage2Texture = LoadTextureChecked(Assets::Guide::Image2);
	GuideImage3Texture = LoadTextureChecked(Assets::Guide::Image3);
	GuideImage4Texture = LoadTextureChecked(Assets::Guide::Image4);
}

void Guide_Initialize() {
	// Shared mesh for all guide quads
	GuideMesh = CreateSquareMesh(0xFFFFFFFF);

	// Variables
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
	ArrowW = 62.0f;
	ArrowH = 62.0f;
	ArrowX = 420.0f;
	ArrowY = -290.0f;
	ArrowAlpha = 1.0f;

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

	// Arrow Select
	bool ArrowHover{ IsAreaHovered(ArrowX, ArrowY, ArrowW, ArrowH) };
	HoverAnimation(ArrowAlpha, ArrowHover, CatSpeed, dt, 1.0f, 0.5f);

	// Fade out when button hover 
	if (!Cat1Selected) {
		HoverAnimation(Cat1Alpha, Cat1Hover, CatSpeed, dt, 0.5f, 1.0f);
	}
	if (!Cat2Selected) {
		HoverAnimation(Cat2Alpha, Cat2Hover, CatSpeed, dt, 0.5f, 1.0f);
	}
	if (!Cat3Selected) {
		HoverAnimation(Cat3Alpha, Cat3Hover, CatSpeed, dt, 0.5f, 1.0f);
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
		Cat2Page2 = false;
		Cat2Page1 = true;
	}
	if (Cat2Selected) {
		Cat2Alpha = 1.0f;

		if (Cat2Page1 && ((ArrowHover && AEInputCheckTriggered(AEVK_LBUTTON)) || AEInputCheckTriggered(AEVK_RIGHT))) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			Cat2Page1 = false;
			Cat2Page2 = true;
			ArrowAlpha = 1.0f;
		}
		else if (Cat2Page2 && ((ArrowHover && AEInputCheckTriggered(AEVK_LBUTTON)) || AEInputCheckTriggered(AEVK_LEFT))) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			Cat2Page2 = false;
			Cat2Page1 = true;
			ArrowAlpha = 1.0f;
		}
	}
	if (Cat3Selected) {
		Cat3Alpha = 1.0f;
		Cat2Page2 = false;
		Cat2Page1 = true;
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
	DrawTextureMesh(GuideMesh, GuideIconTexture, AEGfxGetWinMinX() + 1350.0f, AEGfxGetWinMinY() + 750.0f, 100.0f, 63.0f, 1.0f);

}

void Guide_Draw() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	// Draws the guide icon in the corner
	//DrawTextureMesh(guideIconMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);

	if (IsGuideOpen) {
		// Black BG
		u32 colorBG = (0x000000 << 8) | static_cast<u32>(GuideAlpha * 187.0f);
		DrawSquareMesh(GuideMesh, 0.0f, 0.0f, winW, winH, colorBG);

		// Draw back BG
		DrawTextureMesh(GuideMesh, GuideBGBackTexture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		DrawTextureMesh(GuideMesh, GuideCategory3Texture, CategoryX, -150.0f, CategoryW, 157.0f, Cat3Alpha);
		DrawTextureMesh(GuideMesh, GuideCategory2Texture, CategoryX, 0.0f, CategoryW, 157.0f, Cat2Alpha);
		DrawTextureMesh(GuideMesh, GuideCategory1Texture, CategoryX, 153.0f, CategoryW, 153.0f, Cat1Alpha);	

		AEGfxPrint(GuideFontID, "CONTROLS", textPosition(465.0f, 150.0f).first, textPosition(465.0f, 150.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat1Alpha);
		AEGfxPrint(GuideFontID, "HOW TO", textPosition(465.0f, 5.0f).first, textPosition(465.0f, 5.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat2Alpha);
		AEGfxPrint(GuideFontID, "PLAY", textPosition(465.0f, -25.0f).first, textPosition(465.0f, -25.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat2Alpha);
		AEGfxPrint(GuideFontID, "BOSS FIGHT", textPosition(465.0f, -155.0f).first, textPosition(465.0f, -155.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, Cat3Alpha);

		DrawTextureMesh(GuideMesh, GuideBGTopTexture, -68.0f, 0.0f, 1061.0f, 756.0f, 1.0f);
		if (Cat1Selected) {
			DrawTextureMesh(GuideMesh, GuideImage1Texture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		}
		if (Cat2Selected) {
			if (Cat2Page1) {
				DrawTextureMesh(GuideMesh, GuideImage2Texture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
				DrawTextureMesh(GuideMesh, GuideArrowRightTexture, ArrowX, ArrowY, ArrowW, ArrowH, ArrowAlpha);
			}
			else if (Cat2Page2) {
				DrawTextureMesh(GuideMesh, GuideImage3Texture, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
				DrawTextureMesh(GuideMesh, GuideArrowLeftTexture, ArrowX, ArrowY, ArrowW, ArrowH, ArrowAlpha);
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
	UnloadTextureSafe(GuideBGTopTexture);
	UnloadTextureSafe(GuideBGBackTexture);
	UnloadTextureSafe(GuideArrowLeftTexture);
	UnloadTextureSafe(GuideArrowRightTexture);

	UnloadTextureSafe(GuideCategory1Texture);
	UnloadTextureSafe(GuideCategory2Texture);
	UnloadTextureSafe(GuideCategory3Texture);

	UnloadTextureSafe(GuideImage1Texture);
	UnloadTextureSafe(GuideImage2Texture);
	UnloadTextureSafe(GuideImage3Texture);
	UnloadTextureSafe(GuideImage4Texture);

	AEGfxDestroyFont(GuideFontID);
}