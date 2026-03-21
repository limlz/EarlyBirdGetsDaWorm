#include "pch.hpp"

class Guide {
	public:
		// Mesh
		AEGfxVertexList* GuideMesh;

		// Textures 
		struct Textures {
			AEGfxTexture* Icon;
			AEGfxTexture* BackgroundTop;
			AEGfxTexture* BackgroundBottom;
			AEGfxTexture* ArrowLeft;
			AEGfxTexture* ArrowRight;

			AEGfxTexture* Category1;
			AEGfxTexture* Category2;
			AEGfxTexture* Category3;

			AEGfxTexture* Image1;
			AEGfxTexture* Image2;
			AEGfxTexture* Image3;
			AEGfxTexture* Image4;
		} GuideTexture;

		// Font
		struct Fonts {
			s8 Pixel;
			s8 Goth;

		} GuideFont;

		// State
		struct State {
			bool IsOpen;
			bool CanDisplay;
			f32 FadeSpeed;
			f32 PopUpTimer;
			f32 Alpha;
		} GuideState;

		// Small Icon
		struct Icon {
			f32 Width, Height;
			f32 X, Y;
			f32 Alpha;
			bool Hover;
		} GuideIcon;

		// Arrow
		struct Arrow {
			f32 Width, Height;
			f32 X, Y;
			f32 Alpha;
		} GuideArrow;

		// Categories 
		struct Category {
			s8 ID;
			f32 Width, X;
			f32 Alpha1, Alpha2, Alpha3;
			f32 Speed;

			bool Selected1, Selected2, Selected3;
			bool Page1Active, Page2Active;
		} GuideCategory;

	public:
		void Load();
		void LoadConfigFromFile(const std::string& Filename);
		void Initialize();
		void Update(bool LiftActive, f32 DT, bool PagerActive);
		void Draw();
		void Free();
		void BossFight();
};

static Guide TheGuide;

void Guide_Load() { TheGuide.Load(); }
void Guide_Initialize() { TheGuide.Initialize(); }
void Guide_Update(bool LiftActive, f32 DT, bool PagerActive) { TheGuide.Update(LiftActive, DT, PagerActive); }
void Guide_Draw() { TheGuide.Draw(); }
void Guide_Free() { TheGuide.Free(); }
void Guide_BossFight() { TheGuide.BossFight(); }
bool IsGuideActive() { return TheGuide.GuideState.IsOpen; }

void Guide::Load() {
	GuideFont.Goth = AEGfxCreateFont(Assets::Fonts::Handwriting1, 50);
	GuideFont.Pixel = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

	GuideTexture.Icon = LoadTextureChecked(Assets::Guide::GuideIcon);
	GuideTexture.BackgroundTop = LoadTextureChecked(Assets::Guide::GuideBGTop);
	GuideTexture.BackgroundBottom = LoadTextureChecked(Assets::Guide::GuideBGBack);
	GuideTexture.ArrowLeft = LoadTextureChecked(Assets::Guide::GuideArrowLeft);
	GuideTexture.ArrowRight = LoadTextureChecked(Assets::Guide::GuideArrowRight);

	GuideTexture.Category1 = LoadTextureChecked(Assets::Guide::Category1Text);
	GuideTexture.Category2 = LoadTextureChecked(Assets::Guide::Category2Text);
	GuideTexture.Category3 = LoadTextureChecked(Assets::Guide::Category3Text);

	GuideTexture.Image1 = LoadTextureChecked(Assets::Guide::Image1);
	GuideTexture.Image2 = LoadTextureChecked(Assets::Guide::Image2);
	GuideTexture.Image3 = LoadTextureChecked(Assets::Guide::Image3);
	GuideTexture.Image4 = LoadTextureChecked(Assets::Guide::Image4);
}

void Guide::LoadConfigFromFile(const std::string& Filename) {
	std::ifstream File(Filename);
	if (!File.is_open()) {
		printf("[FAIL] Guide configuration could not be opened.");
		return;
	}

	std::string Line;

	while (std::getline(File, Line)) {
		std::istringstream iss(Line);

		std::string Key;

		if (std::getline(iss, Key, '=')) {
			// State
			if (Key == "State") {
				if (!(iss >> GuideState.IsOpen >> GuideState.CanDisplay >> GuideState.FadeSpeed >> GuideState.PopUpTimer >> GuideState.Alpha)) {
					printf("[FAIL] Guide state configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Guide state configuration loaded successfully.\n");
				}
			}
			else if (Key == "Icon") {
				if (!(iss >> GuideIcon.Width >> GuideIcon.Height >> GuideIcon.Alpha >> GuideIcon.Hover)) {
					printf("[FAIL] Guide icon configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Guide icon configuration loaded successfully.\n");
				}
			}
			else if (Key == "Arrow") {
				if (!(iss >> GuideArrow.Width >> GuideArrow.Height >> GuideArrow.X >> GuideArrow.Y >> GuideArrow.Alpha)) {
					printf("[FAIL] Guide arrow configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Guide arrow configuration loaded successfully.\n");
				}
			}
			else if (Key == "Categories") {
				if (!(iss >> GuideCategory.ID >> GuideCategory.Width >> GuideCategory.X >> GuideCategory.Alpha1 >> GuideCategory.Alpha2 >> GuideCategory.Alpha3
					>> GuideCategory.Speed >> GuideCategory.Selected1 >> GuideCategory.Selected2 >> GuideCategory.Selected3
					>> GuideCategory.Page1Active >> GuideCategory.Page2Active)) {
					printf("[FAIL] Guide category configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Guide category configuration loaded successfully.\n");
				}
			}
		}
	}
	File.close();
}

void Guide::Initialize() {
	// Shared mesh for all guide quads
	GuideMesh = CreateSquareMesh(0xFFFFFFFF);

	// States
	GuideState.IsOpen = false;
	GuideState.CanDisplay = false;
	GuideState.FadeSpeed = 2.5f;
	GuideState.PopUpTimer = 0.0f;
	GuideState.Alpha = 0.0f;

	// Small Icon
	GuideIcon.Width = 100.0f;
	GuideIcon.Height = 63.0f;
	GuideIcon.X = AEGfxGetWinMinX() + 1350.0f;
	GuideIcon.Y = AEGfxGetWinMinY() + 750.0f;
	GuideIcon.Alpha = 1.0f;
	GuideIcon.Hover = false;

	// Arrow
	GuideArrow.Width = 62.0f;
	GuideArrow.Height = 62.0f;
	GuideArrow.X = 420.0f;
	GuideArrow.Y = -290.0f;
	GuideArrow.Alpha = 1.0f;

	// Categories
	GuideCategory.ID = 1;
	GuideCategory.Width = 172.0f;
	GuideCategory.X = 514.0f;
	GuideCategory.Alpha1 = 1.0f;
	GuideCategory.Alpha2 = 1.0f;
	GuideCategory.Alpha3 = 1.0f;
	GuideCategory.Speed = 10.0f;
	GuideCategory.Selected1 = true;
	GuideCategory.Selected2 = false;
	GuideCategory.Selected3 = false;
	GuideCategory.Page1Active = false;
	GuideCategory.Page2Active = false;

	// File reading
	LoadConfigFromFile("Assets/Guide/Config/GuideConfig.txt");
}

void Guide::Update(bool liftActive, f32 DT, bool PagerActive) {
	if (GuideState.IsOpen && GuideState.Alpha < 1.0f) {
		GuideState.Alpha += GuideState.FadeSpeed * DT;

		if (GuideState.Alpha > 1.0f) {
			GuideState.Alpha = 1.0f;
		}
	}

	if (!GuideState.IsOpen) {
		GuideState.Alpha -= GuideState.FadeSpeed * DT;
		if (GuideState.Alpha < 0.0f) {
			GuideState.Alpha = 0.0f;
		}
		GuideCategory.Alpha2 = 0.5;
		GuideCategory.Alpha3 = 0.5;

		GuideCategory.ID = 1;
	}

	// Small Icon Hover and Select
	GuideIcon.Hover = IsAreaHovered(GuideIcon.X, GuideIcon.Y, GuideIcon.Width, GuideIcon.Height);
	HoverAnimation(GuideIcon.Alpha, GuideIcon.Hover, GuideCategory.Speed, DT, 1.0f, 0.5f);
	bool OutsideGuide{ (IsAreaClickedByMouse(0.0f, 0.0f, 1200.0f, 756.0f)) ? false : true };

	if (((GuideIcon.Hover && AEInputCheckTriggered(AEVK_LBUTTON)) || AEInputCheckTriggered(AEVK_G)) && !liftActive && !PagerActive)
	{
		GuideState.IsOpen = !GuideState.IsOpen;
	}

	if (GuideState.IsOpen) {
		if (OutsideGuide && AEInputCheckTriggered(AEVK_LBUTTON)) { GuideState.IsOpen = false; }
	}



	// Category Select
	bool Cat1Hover{ IsAreaHovered(GuideCategory.X, 153.0f, GuideCategory.Width, 153.0f) },
		Cat2Hover{ IsAreaHovered(GuideCategory.X, 0.0f, GuideCategory.Width, 157.0f) },
		Cat3Hover{ IsAreaHovered(GuideCategory.X, -150.0f, GuideCategory.Width, 157.0f) };

	// Arrow Select
	bool ArrowHover{ IsAreaHovered(GuideArrow.X, GuideArrow.Y, GuideArrow.Width, GuideArrow.Height) };
	HoverAnimation(GuideArrow.Alpha, ArrowHover, 5.0f, DT, 1.0f, 0.5f);

	// Fade out when button hover 
	if (!GuideCategory.Selected1) {
		HoverAnimation(GuideCategory.Alpha1, Cat1Hover, GuideCategory.Speed, DT, 0.5f, 1.0f);
	}
	if (!GuideCategory.Selected2) {
		HoverAnimation(GuideCategory.Alpha2, Cat2Hover, GuideCategory.Speed, DT, 0.5f, 1.0f);
	}
	if (!GuideCategory.Selected3) {
		HoverAnimation(GuideCategory.Alpha3, Cat3Hover, GuideCategory.Speed, DT, 0.5f, 1.0f);
	}

	if (Cat1Hover && (!GuideCategory.Selected2 || !GuideCategory.Selected3)) {
		GuideCategory.Alpha1 = 1.0f;
	}
	if (Cat2Hover && (!GuideCategory.Selected1 || !GuideCategory.Selected3)) {
		GuideCategory.Alpha2 = 1.0f;
	}
	if (Cat3Hover && (!GuideCategory.Selected1 || !GuideCategory.Selected2)) {
		GuideCategory.Alpha3 = 1.0f;
	}


	if (Cat1Hover && AEInputCheckTriggered(AEVK_LBUTTON)) {
		GuideCategory.Alpha1 = 1.0f;
		GuideCategory.ID = 1;

	}
	if (Cat2Hover && AEInputCheckTriggered(AEVK_LBUTTON)) {
		GuideCategory.Alpha2 = 1.0f;
		GuideCategory.ID = 2;
	}
	if (Cat3Hover && AEInputCheckTriggered(AEVK_LBUTTON)) {
		GuideCategory.Alpha3 = 1.0f;
		GuideCategory.ID = 3;
	}

	// Arrow-Keys
	if (AEInputCheckTriggered(AEVK_UP)) {
		AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
		--GuideCategory.ID;
		if (GuideCategory.ID < 1) {
			GuideCategory.ID = 1;
		}
		return;
	}
	if (AEInputCheckTriggered(AEVK_DOWN)) {
		AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
		++GuideCategory.ID;
		if (GuideCategory.ID > 3) {
			GuideCategory.ID = 3;
		}
		return;
	}

	switch (GuideCategory.ID) {
	case 1:
		GuideCategory.Selected1 = true;
		GuideCategory.Selected2 = false;
		GuideCategory.Selected3 = false;
		break;
	case 2:
		GuideCategory.Selected1 = false;
		GuideCategory.Selected2 = true;
		GuideCategory.Selected3 = false;
		break;
	case 3:
		GuideCategory.Selected1 = false;
		GuideCategory.Selected2 = false;
		GuideCategory.Selected3 = true;

		break;
	}

	if (GuideCategory.Selected1) {
		GuideCategory.Alpha1 = 1.0f;
		GuideCategory.Page1Active = true;
		GuideCategory.Page2Active = false;
	}
	if (GuideCategory.Selected2) {
		GuideCategory.Alpha2 = 1.0f;

		if (GuideCategory.Page1Active && ((ArrowHover && AEInputCheckTriggered(AEVK_LBUTTON)) || AEInputCheckTriggered(AEVK_RIGHT))) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			GuideCategory.Page1Active = false;
			GuideCategory.Page2Active = true;
			GuideArrow.Alpha = 1.0f;
		}
		else if (GuideCategory.Page2Active && ((ArrowHover && AEInputCheckTriggered(AEVK_LBUTTON)) || AEInputCheckTriggered(AEVK_LEFT))) {
			AudioManager_PlaySFX(SFX_JOURNAL_SELECT, 0.5f);
			GuideCategory.Page1Active = true;
			GuideCategory.Page2Active = false;
			GuideArrow.Alpha = 1.0f;
		}
	}
	if (GuideCategory.Selected3) {
		GuideCategory.Alpha3 = 1.0f;
		GuideCategory.Page1Active = true;
		GuideCategory.Page2Active = false;
	}
}

// This will open the guide with the correct image automatically
void Guide::BossFight() {
	// Open the guide
	GuideState.IsOpen = true;
	GuideState.CanDisplay = true;

	// Select the boss fight category
	GuideCategory.ID = 3;
	GuideCategory.Selected1 = false;
	GuideCategory.Selected2 = false;
	GuideCategory.Selected3 = true;

	// Fade in smoothly
	GuideState.Alpha = 0.0f;     
	GuideState.PopUpTimer = 0.0f;

	// Reset category alphas
	GuideCategory.Alpha1 = 0.5f;
	GuideCategory.Alpha2 = 0.5f;
	GuideCategory.Alpha3 = 1.0f;

	// Reset pages
	GuideCategory.Page1Active = true;
	GuideCategory.Page2Active = false;
}

void Guide::Draw() {
	// Draws the guide icon in the corner
	//DrawTextureMesh(guideIconMesh, guideIconTexture, AEGfxGetWinMinX() + 220.0f, AEGfxGetWinMinY() + 750.0f, 130.0f, 63.0f, 1.0f);\

	// Draws the guide icon in the corner
	DrawTextureMesh(GuideMesh, GuideTexture.Icon, GuideIcon.X, GuideIcon.Y, GuideIcon.Width, GuideIcon.Height, GuideIcon.Alpha);

	if (GuideIcon.Hover && !GuideState.IsOpen) {
		AEGfxPrint(GuideFont.Pixel, "Guide", textPosition(-30.0f, -380.0f).first, textPosition(-30.0f, -380.0f).second, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (GuideState.IsOpen) {
		// Black BG
		/*u32 colorBG = (0x000000 << 8) | static_cast<u32>(GuideAlpha * 187.0f);
		DrawSquareMesh(GuideMesh, 0.0f, 0.0f, winW, winH, colorBG);*/

		// Draw back BG
		DrawTextureMesh(GuideMesh, GuideTexture.BackgroundBottom, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		DrawTextureMesh(GuideMesh, GuideTexture.Category3, GuideCategory.X, -150.0f, GuideCategory.Width, 157.0f, GuideCategory.Alpha3);
		DrawTextureMesh(GuideMesh, GuideTexture.Category2, GuideCategory.X, 0.0f, GuideCategory.Width, 157.0f, GuideCategory.Alpha2);
		DrawTextureMesh(GuideMesh, GuideTexture.Category1, GuideCategory.X, 153.0f, GuideCategory.Width, 153.0f, GuideCategory.Alpha1);

		AEGfxPrint(GuideFont.Goth, "CONTROLS", textPosition(465.0f, 150.0f).first, textPosition(465.0f, 150.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, GuideCategory.Alpha1);
		AEGfxPrint(GuideFont.Goth, "HOW TO", textPosition(465.0f, 5.0f).first, textPosition(465.0f, 5.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, GuideCategory.Alpha2);
		AEGfxPrint(GuideFont.Goth, "PLAY", textPosition(465.0f, -25.0f).first, textPosition(465.0f, -25.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, GuideCategory.Alpha2);
		AEGfxPrint(GuideFont.Goth, "BOSS FIGHT", textPosition(465.0f, -155.0f).first, textPosition(465.0f, -155.0f).second, 0.6f, 0.50f, 0.05f, 0.05f, GuideCategory.Alpha3);


		f32 InfoX{ textPosition(-220.0f, -410.0f).first },
			InfoY{ textPosition(-220.0f, -410.0f).second };
		AEGfxPrint(GuideFont.Pixel, "PRESS [G] TO CLOSE GUIDE", InfoX, InfoY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		DrawTextureMesh(GuideMesh, GuideTexture.BackgroundTop, -68.0f, 0.0f, 1061.0f, 756.0f, 1.0f);
		if (GuideCategory.Selected1) {
			DrawTextureMesh(GuideMesh, GuideTexture.Image1, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		}
		if (GuideCategory.Selected2) {
			if (GuideCategory.Page1Active) {
				DrawTextureMesh(GuideMesh, GuideTexture.Image2, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
				DrawTextureMesh(GuideMesh, GuideTexture.ArrowRight, GuideArrow.X, GuideArrow.Y, GuideArrow.Width, GuideArrow.Height, GuideArrow.Alpha);
			}
			else if (GuideCategory.Page2Active) {
				DrawTextureMesh(GuideMesh, GuideTexture.Image3, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
				DrawTextureMesh(GuideMesh, GuideTexture.ArrowLeft, GuideArrow.X, GuideArrow.Y, GuideArrow.Width, GuideArrow.Height, GuideArrow.Alpha);
			}
		}
		if (GuideCategory.Selected3) {
			DrawTextureMesh(GuideMesh, GuideTexture.Image4, 0.0f, 0.0f, 1200.0f, 756.0f, 1.0f);
		}
	}

}

void Guide::Free() {
	FreeMeshSafe(GuideMesh);

	UnloadTextureSafe(GuideTexture.Icon);
	UnloadTextureSafe(GuideTexture.BackgroundTop);
	UnloadTextureSafe(GuideTexture.BackgroundBottom);
	UnloadTextureSafe(GuideTexture.ArrowLeft);
	UnloadTextureSafe(GuideTexture.ArrowRight);

	UnloadTextureSafe(GuideTexture.Category1);
	UnloadTextureSafe(GuideTexture.Category2);
	UnloadTextureSafe(GuideTexture.Category3);

	UnloadTextureSafe(GuideTexture.Image1);
	UnloadTextureSafe(GuideTexture.Image2);
	UnloadTextureSafe(GuideTexture.Image3);
	UnloadTextureSafe(GuideTexture.Image4);

	AEGfxDestroyFont(GuideFont.Goth);
	AEGfxDestroyFont(GuideFont.Pixel);
}