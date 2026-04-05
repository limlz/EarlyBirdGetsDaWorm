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
		s32 InitialX, InitialY;
		s32 MouseX, MouseY;
	} PagerInput;


	// State
	struct State {
		bool IsActive;
		bool HasPatient;
		s8 PreviousFloor;
		s8 CurrentPage;
	} PagerState;


	// Icon 
	struct Icon {
		bool IsClicked;
		bool IsHovered;

		f32 Width, Height;
		f32 X, Y;

		f32 Alpha;
		f32 MinAlpha;
		f32 MaxAlpha;
		f32 FadeSpeed;
	} PagerIcon;


	// Pager
	struct PagerInfo {
		s8 Font;

		f32 Width, Height;

		bool IsVibrating;

		f32 VibrateTimer;
		f32 TaskTimer;
		f32 Duration;

		f32 Intensity;
		f32 Speed;
	} BigPager;


	// Pop Up
	struct PopUp {
		f32 Timer;

		bool IsActive;
		bool FirstPopUp;

		bool TutorialShown;
		bool TaskShown;
		bool WaitTaskDisplay;
		bool BeenShown;

		bool HadPatientPrev;
		bool TaskComplete;
	} PagerPopUp;

public:
	void Load();
	/*!*****************************************************************************
	\brief
		Loads pager configuration values from a file.

		Reads configuration data such as state, icon properties,
		pager settings, and popup behavior from the specified file.

		If the file cannot be opened or parsing fails, error messages
		will be printed to the console.

	\param Filename
		The path to the configuration file.

	\return
		None.
	*******************************************************************************/
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
void Notifications_ResetTaskComplete() { ThePager.PagerPopUp.TaskComplete = false; }

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
		std::cout << "[FAIL] Guide configuration could not be opened.";
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
					std::cout << "[FAIL] Pager state configuration failed to load.\n";
				}
				else {
					std::cout << "[SUCCESS] Pager state configuration loaded successfully.\n";
				}
			}
			else if (Key == "Icon") {
				if (!(iss >> PagerIcon.IsClicked >> PagerIcon.IsHovered >> PagerIcon.Width >> PagerIcon.Height
					>> PagerIcon.Alpha >> PagerIcon.MinAlpha >> PagerIcon.MaxAlpha >> PagerIcon.FadeSpeed)) {
					std::cout << "[FAIL] Pager icon configuration failed to load.\n";
				}
				else {
					std::cout << "[SUCCESS] Pager icon configuration loaded successfully.\n";
				}
			}
			else if (Key == "BigPager") {
				if (!(iss >> BigPager.Width >> BigPager.Height >> BigPager.IsVibrating >> BigPager.VibrateTimer
					>> BigPager.TaskTimer >> BigPager.Duration >> BigPager.Intensity >> BigPager.Speed)) {
					std::cout << "[FAIL] Big pager configuration failed to load.\n";
				}
				else {
					std::cout << "[SUCCESS] Big pager configuration loaded successfully.\n";
				}
			}
			else if (Key == "PopUp") {
				if (!(iss >> PagerPopUp.Timer >> PagerPopUp.IsActive >> PagerPopUp.FirstPopUp >> PagerPopUp.TutorialShown
					>> PagerPopUp.TaskShown >> PagerPopUp.WaitTaskDisplay >> PagerPopUp.BeenShown >> PagerPopUp.HadPatientPrev >> PagerPopUp.TaskComplete)) {
					std::cout << "[FAIL] Pager pop up configuration failed to load.\n";
				}
				else {
					std::cout << "[SUCCESS] Pager pop up configuration loaded successfully.\n";
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

void Pager::Update(bool LiftActive, f32 DT)
{
	// Small Icon Hover Animation
	PagerIcon.IsHovered = IsAreaHovered(PagerIcon.X, PagerIcon.Y, PagerIcon.Width, PagerIcon.Height);
	HoverAnimation(PagerIcon.Alpha, PagerIcon.IsHovered, PagerIcon.FadeSpeed, DT, PagerIcon.MinAlpha, PagerIcon.MaxAlpha);

	// ==========================================================
	// 1. HOLD-TO-VIEW LOGIC (MANUAL OPEN/CLOSE)
	// ==========================================================
	static bool wasTabHeld = false;
	bool isTabHeld = AEInputCheckCurr(AEVK_TAB);

	// Icon Click Fallback
	bool iconClicked = PagerIcon.IsHovered && AEInputCheckTriggered(AEVK_LBUTTON);

	if (iconClicked) {
		PagerState.IsActive = !PagerState.IsActive;
	}

	bool isTutorialMode = Doing_Tutorial() && !IsTutorialFinished();

	// Tab Logic
	if (isTutorialMode)
	{
		if (AEInputCheckTriggered(AEVK_TAB)) {
			PagerState.IsActive = !PagerState.IsActive;
		}
	}
	else
	{
		// ONLY run hold logic if NOT clicking this frame
		if (!iconClicked)
		{
			if (isTabHeld) {
				PagerState.IsActive = true;
			}
			else if (wasTabHeld && !isTabHeld) {
				PagerState.IsActive = false;
				PagerPopUp.FirstPopUp = false;
			}
		}
	}

	wasTabHeld = isTabHeld;

	// Click outside to close (Useful for when it pops up automatically)
	if (PagerState.IsActive && !isTabHeld && !isTutorialMode && !iconClicked) {
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
			AudioManager_PlaySFX(SoundID::SFX_PAGER_NOTIFICATIONS, 0.5f);
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
			AudioManager_PlaySFX(SoundID::SFX_PAGER_NOTIFICATIONS, 0.5f);

			PagerState.IsActive = true;
			BigPager.IsVibrating = true;
			BigPager.VibrateTimer = 0.0f;
			BigPager.Duration = 0.5f;
			PagerPopUp.WaitTaskDisplay = false;
		}
	}

	if (PagerPopUp.FirstPopUp) {
		if (AEInputCheckTriggered(AEVK_TAB)) {
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
	if (!Lift_IsActive())
	{
		// Draws the icon pager in the corner
		DrawTextureMesh(PagerMesh, PagerTexture.Icon, PagerIcon.X, PagerIcon.Y, PagerIcon.Width, PagerIcon.Height, PagerIcon.Alpha);
		if (PagerIcon.IsHovered && !PagerState.IsActive && !Doing_Tutorial()) {
			AEGfxPrint(BigPager.Font, "Pager", textPosition(-30.0f, -380.0f).first, textPosition(-30.0f, -380.0f).second, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	if (PagerState.IsActive) {
		f32 PosX{ 0.0f }, PosY{ 0.0f },
			CollectionX{ textPosition(-360.0f, 157.5f).first }, CollectionY{ textPosition(-360.0f, 157.5f).second },
			DestinationX{ textPosition(-360.0f, 97.5f).first }, DestinationY{ textPosition(-360.0f, 97.5f).second },
			CountX{ textPosition(50.0f, -40.0f).first }, CountY{ textPosition(70.0f, -100.0f).second };

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

			CountX += OffsetX;
			CountY += OffsetY;
		}

		// Draws Pager
		DrawTextureMesh(PagerMesh, PagerTexture.Pager, PosX, PosY, BigPager.Width, BigPager.Height, 1.0f);

		// Text
		std::ostringstream ossCollect;
		ossCollect << "Collect patient from room #"
			<< std::setw(2) << std::setfill('0') << static_cast<int>(PatientFloorNumber) << "-"
			<< std::setw(2) << std::setfill('0') << static_cast<int>(PatientDoorNumber);
		std::string CollectionText = ossCollect.str();

		std::ostringstream ossDest;
		ossDest << "Bring patient to room #"
			<< std::setw(2) << std::setfill('0') << static_cast<int>(DestinationFloorNumber) << "-"
			<< std::setw(2) << std::setfill('0') << static_cast<int>(DestinationDoorNumber);
		std::string DestinationText = ossDest.str();

		AEGfxPrint(BigPager.Font, CollectionText.c_str(), CollectionX, CollectionY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxPrint(BigPager.Font, DestinationText.c_str(), DestinationX, DestinationY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		// Patient count — bottom-right of pager content area
		const int requiredPatients = Game_IsPlaytestMode() ? 1 : REQUIRED_PATIENTS;
		std::ostringstream ossCount;
		ossCount << "Patients: " << Player_GetPatientsHandled() << " / " << requiredPatients;
		std::string CountText = ossCount.str();
		AEGfxPrint(BigPager.Font, CountText.c_str(), CountX, CountY, 1.0f, 1.0f, 0.85f, 0.45f, 1.0f);

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