#include "pch.hpp"

class Pager {
	public:
		// Mesh
		AEGfxVertexList* PagerMesh;

		// Texture
		struct Textures {
			AEGfxTexture* Icon;
			AEGfxTexture* Pager;
		} PagerTexture;

		// Input 
		struct Input {
			s32 InitialX, InitialY;   // old: initialX, initialY
			s32 MouseX, MouseY;       // old: mouseX, mouseY
		} PagerInput;


		// State
		struct State {
			bool IsActive;        // old: isPagerOpen
			bool HasPatient;	  // old: currHasPatient
			s8 PreviousFloor;     // old: prevFloor
			s8 CurrentPage;       // old: currentPage
		} PagerState;


		// Icon 
		struct Icon {
			bool IsClicked;       // old: smallClicked
			bool IsHovered;       

			f32 Width, Height;    // old: pagerWidth, pagerHeight
			f32 X, Y;             // old: pagerX, pagerY

			f32 Alpha;
			f32 MinAlpha;
			f32 MaxAlpha;
			f32 FadeSpeed;
		} PagerIcon;


		// Pager
		struct PagerInfo {
			s8 Font;              // old: lineID

			f32 Width, Height;

			bool IsVibrating;     // old: pagerVibrating

			f32 VibrateTimer;            // old: vibrateTimer 
			f32 TaskTimer;            // old: taskTimer
			f32 Duration;         // old: vibrateDuration

			f32 Intensity;        // how strong the pager shakes
			f32 Speed;            // how fast the pager shakes
		} BigPager;


		// Pop Up
		struct PopUp {
			f32 Timer;            // old: popupTimer

			bool IsActive;        // old: popUp
			bool FirstPopUp;      // old: pagerFirstPop

			bool TutorialShown;
			bool TaskShown;       // old: taskPopUpShown
			bool WaitTaskDisplay; // old: waitTaskPopUp
			bool BeenShown;       // old: popUpShown

			bool HadPatientPrev;   // old: prevHadPatient
			bool TaskComplete; // old: openPagerAfterDoor
		} PagerPopUp;

	public:
		void Load();
		void LoadConfigFromFile(const std::string& Filename);
		void Initialize();
		void Trigger();
		void Update(bool LiftActive, f32 DT);
		void Draw(s8 PatientFloorNumber, s8 PatientDoorNumber, s8 DestinationFloorNumber, s8 DestinationDoorNumber);
		void Free();
};

static Pager ThePager;
void Notifications_Load() { ThePager.Load(); }
void Notifications_Initialize() { ThePager.Initialize(); }
void Notifications_Trigger() { ThePager.Trigger(); }
void Notifications_Update(bool LiftActive, f32 DT) { ThePager.Update(LiftActive, DT); }
void Notifications_Draw(s8 PatientFloorNumber, s8 PatientDoorNumber, s8 DestinationFloorNumber, s8 DestinationDoorNumber) { ThePager.Draw(PatientFloorNumber, PatientDoorNumber, DestinationFloorNumber, DestinationDoorNumber); }
void Notifications_Free() { ThePager.Free(); }
bool IsPagerActive() { return ThePager.PagerState.IsActive; }

void Pager::Load()
{
	PagerTexture.Icon = LoadTextureChecked(Assets::Equipment::PagerIcon);
	PagerTexture.Pager = LoadTextureChecked(Assets::Equipment::Pager);
	BigPager.Font = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

	return;
}

void Pager::LoadConfigFromFile(const std::string& Filename) {
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
				if (!(iss >> PagerState.IsActive >> PagerState.HasPatient >> PagerState.PreviousFloor >> PagerState.CurrentPage)) {
					printf("[FAIL] Pager state configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Pager state configuration loaded successfully.\n");
				}
			}
			else if (Key == "Icon") {
				if (!(iss >> PagerIcon.IsClicked >> PagerIcon.IsHovered >> PagerIcon.Width >> PagerIcon.Height
					>> PagerIcon.Alpha >> PagerIcon.MinAlpha >> PagerIcon.MaxAlpha >> PagerIcon.FadeSpeed)) {
					printf("[FAIL] Pager icon configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Pager icon configuration loaded successfully.\n");
				}
			}
			else if (Key == "BigPager") {
				if (!(iss >> BigPager.Width >> BigPager.Height >> BigPager.IsVibrating >> BigPager.VibrateTimer
					>> BigPager.TaskTimer >> BigPager.Duration >> BigPager.Intensity >> BigPager.Speed)) {
					printf("[FAIL] Big pager configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Big pager configuration loaded successfully.\n");
				}
			}
			else if (Key == "PopUp") {
				if (!(iss >> PagerPopUp.Timer >> PagerPopUp.IsActive >> PagerPopUp.FirstPopUp >> PagerPopUp.TutorialShown
					>> PagerPopUp.TaskShown >> PagerPopUp.WaitTaskDisplay >> PagerPopUp.BeenShown >> PagerPopUp.HadPatientPrev >> PagerPopUp.TaskComplete)) {
					printf("[FAIL] Pager pop up configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Pager pop up configuration loaded successfully.\n");
				}
			}
		}
	}
	File.close();
}

void Pager::Initialize()
{
	// Mesh
	PagerMesh = CreateSquareMesh(0xFFFFFFFF);

	// Input 
	PagerInput.MouseX = static_cast<s32>(0.0f);
	PagerInput.MouseY = static_cast<s32>(0.0f);

	// State
	PagerState.IsActive = false;	// Open pager after completing objective
	if (PagerPopUp.TaskComplete)
	{
		PagerState.IsActive = true;			// open big pager immediately
		PagerPopUp.TaskComplete = false;	// request handled
	}
	PagerState.HasPatient = false;
	PagerState.PreviousFloor = -1;
	PagerState.CurrentPage = 0;

	// Icon 
	PagerIcon.IsClicked = false;
	PagerIcon.IsHovered = false;
	PagerIcon.Width = 100.0f;
	PagerIcon.Height = 63.0f;
	PagerIcon.X = AEGfxGetWinMinX() + 100.0f;
	PagerIcon.Y = AEGfxGetWinMinY() + 750.0f;

	PagerIcon.Alpha = 1.0f;
	PagerIcon.MinAlpha = 1.0f;
	PagerIcon.MaxAlpha = 0.5f;
	PagerIcon.FadeSpeed = 20.0f;

	// Pager
	BigPager.Width = 1000.0f;
	BigPager.Height = 635.0f;
	BigPager.IsVibrating = false;
	BigPager.VibrateTimer = 0.0f;
	BigPager.TaskTimer = 0.0f;
	BigPager.Duration = 0.0f;
	BigPager.Intensity = 4.0f;
	BigPager.Speed = 100.0f;

	// Pop Up
	PagerPopUp.Timer = 0.0f;
	PagerPopUp.IsActive = false;
	PagerPopUp.FirstPopUp = false;

	PagerPopUp.TutorialShown = false;
	PagerPopUp.TaskShown = false;
	PagerPopUp.WaitTaskDisplay = false;
	PagerPopUp.BeenShown = false;

	PagerPopUp.HadPatientPrev = false;
	PagerPopUp.TaskComplete = false;

	// File reading
	LoadConfigFromFile("Assets/Equipment/logs/PagerConfig.txt");
}


// To call from boss fight
void Pager::Trigger()
{
	PagerPopUp.TaskComplete = true;   // request open on next state enter
}

void Notifications_ResetTaskComplete()
{
	PagerPopUp.TaskComplete = false;
}

void Pager::Update(bool LiftActive, f32 DT)
{
	// Toggles the pager when user press Q, pager will not be displayed if lift is active
	// Small Icon Hover and Select
	PagerIcon.IsHovered = IsAreaHovered(PagerIcon.X, PagerIcon.Y, PagerIcon.Width, PagerIcon.Height);
	HoverAnimation(PagerIcon.Alpha, PagerIcon.IsHovered, PagerIcon.FadeSpeed, DT, PagerIcon.MinAlpha, PagerIcon.MaxAlpha);

	if (!LiftActive) {
		if (PagerIcon.IsHovered && AEInputCheckTriggered(AEVK_LBUTTON)) {
			PagerState.IsActive = !PagerState.IsActive;
			return;
		}

		if (AEInputCheckTriggered(AEVK_TAB)) {
			PagerState.IsActive = !PagerState.IsActive;
			return;
		}
	}

	if (PagerState.IsActive) {
		bool OutsidePager{ (IsAreaClickedByMouse(0.0f, 0.0f, BigPager.Width, BigPager.Height)) ? false : true };

		if (OutsidePager && AEInputCheckTriggered(AEVK_LBUTTON))
		{
			PagerState.IsActive = false;
		}
	}	

	// Game start delayed pop up
	bool WantTutorial{ Doing_Tutorial() };
	bool AnsweredTutorial{ Tutorial_Prompt_Answered() };

	if (AnsweredTutorial && !WantTutorial && !PagerPopUp.BeenShown) {
		PagerPopUp.Timer += DT;

		if (PagerPopUp.Timer >= 1.3f) {
			AudioManager_PlaySFX(SFX_PAGER_NOTIFICATIONS, 0.5f);
			if (!IsTutorialActive() && !Doing_Tutorial()) { PagerPopUp.FirstPopUp = true; }

			PagerState.IsActive = true;
			BigPager.IsVibrating = true;
			BigPager.VibrateTimer = 0.0f;
			BigPager.Duration = 0.5f;
			PagerPopUp.BeenShown = true;
		}
	}

	// Pop Up after task complete
	PagerState.HasPatient = Player_HasPatient();

	if (PagerPopUp.HadPatientPrev && !PagerState.HasPatient) {
		PagerPopUp.WaitTaskDisplay = true;
		BigPager.TaskTimer = 0.0f;
	}
	PagerPopUp.HadPatientPrev = PagerState.HasPatient;

	if (PagerPopUp.WaitTaskDisplay) {
		BigPager.TaskTimer += DT;

		if (BigPager.TaskTimer >= 1.3f) {
			AudioManager_PlaySFX(SFX_PAGER_NOTIFICATIONS, 0.5f);

			PagerState.IsActive = true;
			BigPager.IsVibrating = true;
			BigPager.VibrateTimer = 0.0f;
			BigPager.Duration = 0.5f;
			PagerPopUp.WaitTaskDisplay = false;
		}
	}

	if (PagerPopUp.FirstPopUp) {
		if (AEInputCheckTriggered(AEVK_TAB) && !LiftActive) {
			PagerPopUp.FirstPopUp = false;
		}
	}

	// Animation
	if (BigPager.IsVibrating)
	{
		BigPager.VibrateTimer += DT;

		if (BigPager.VibrateTimer >= BigPager.Duration)
		{
			BigPager.IsVibrating = false;
		}
	}

	// Gets the current x and y position of mouse
	AEInputGetCursorPosition(&PagerInput.InitialX, &PagerInput.InitialY);
	PagerInput.MouseX = static_cast<s32>(PagerInput.InitialX - (AEGfxGetWindowWidth() / 2.0f));
	PagerInput.MouseY = static_cast<s32>((AEGfxGetWindowHeight() / 2.0f) - PagerInput.InitialY);
}

void Pager::Draw(s8 PatientFloorNumber, s8 PatientDoorNumber, s8 DestinationFloorNumber, s8 DestinationDoorNumber)
{
	// Draws the icon pager in the corner
	DrawTextureMesh(PagerMesh, PagerTexture.Icon, PagerIcon.X, PagerIcon.Y, PagerIcon.Width, PagerIcon.Height, PagerIcon.Alpha);
	if (PagerIcon.IsHovered && !PagerState.IsActive && !Doing_Tutorial()) {
		AEGfxPrint(BigPager.Font, "Pager", textPosition(-30.0f, -380.0f).first, textPosition(-30.0f, -380.0f).second, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (PagerState.IsActive) {
		f32 PosX{ 0.0f }, PosY{ 0.0f },
			CollectionX{ textPosition(-360.0f, 157.5f).first }, CollectionY{ textPosition(-360.0f, 157.5f).second },
			DestinationX{ textPosition(-360.0f, 97.5f).first }, DestinationY{ textPosition(-360.0f, 97.5f).second };

		if (BigPager.IsVibrating)
		{
			f32 OffsetX{ sinf(BigPager.VibrateTimer * BigPager.Speed) * BigPager.Intensity },
				OffsetY{ cosf(BigPager.VibrateTimer * BigPager.Speed) * BigPager.Intensity };

			PosX += OffsetX;
			PosY += OffsetY;

			CollectionX += OffsetX;
			CollectionY += OffsetY;

			DestinationX += OffsetX;
			DestinationY += OffsetY;
		}

		// Draws Pager
		DrawTextureMesh(PagerMesh, PagerTexture.Pager, PosX, PosY, BigPager.Width, BigPager.Height, 1.0f);

		// Text
		char CollectionText[40], DestinationText[40];
		sprintf_s(CollectionText, "Collect patient from room #%02d-%02d", PatientFloorNumber, PatientDoorNumber);
		sprintf_s(DestinationText, "Bring patient to room #%02d-%02d", DestinationFloorNumber, DestinationDoorNumber);
		AEGfxPrint(BigPager.Font, CollectionText, CollectionX, CollectionY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxPrint(BigPager.Font, DestinationText, DestinationX, DestinationY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		f32 InfoX{ textPosition(-220.0f, -410.0f).first },
			InfoY{ textPosition(-220.0f, -410.0f).second };
		if (IsTutorialFinished() || !Doing_Tutorial()) {
			AEGfxPrint(BigPager.Font, "PRESS [TAB] TO CLOSE PAGER", InfoX, InfoY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

	}
}

void Pager::Free()
{
	AudioManager_StopSFX();

	FreeMeshSafe(PagerMesh);

	UnloadTextureSafe(PagerTexture.Icon);
	UnloadTextureSafe(PagerTexture.Pager);

	// Destroy font if valid
	if (BigPager.Font >= 0) {
		AEGfxDestroyFont(BigPager.Font);
		BigPager.Font = -1;
	}
}