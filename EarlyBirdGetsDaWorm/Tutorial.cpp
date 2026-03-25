#include "pch.hpp"

class Tutorial {
public:
	// Stages
	enum Stages {
		TUTORIAL_START,
		TUTORIAL_WAIT_FOR_PAGER_CLOSE,
		TUTORIAL_WAIT_FOR_LIFT_OPEN,
		TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION,
		TUTORIAL_WAIT_FOR_PATIENT_COLLECTION,
		TUTORIAL_WAIT_FOR_JOURNAL_OPEN,
		TUTORIAL_WAIT_FOR_JOURNAL_CLOSE,
		TUTORIAL_WAIT_FOR_PATIENT_DELIVERY,
		TUTORIAL_END
	} CurrentStage;

	// States
	struct State {

		bool Started;                        // tutStarted
		bool PagerOpened;                    // tutPagerOpened
		bool PagerClosed;                    // tutPagerClosed
		bool LiftOpened;                     // tutLiftOpened
		bool LiftFloorSelected;              // tutLiftFloorSelected
		bool PatientCollected;               // tutPatientCollected
		bool JournalOpened;
		bool JournalClosed;
		bool PatientDelivered;               // tutPatientDelivered
		bool Finished;                       // tutFinished
	} TutorialState;

	// Dialogue Box
	struct DialogueBox {
		f32 Width, Height;
		f32 X, Y;

		bool Shown;
		f32 Timer;

		f32 FadeSpeed;
		f32 FadeAlpha;

		const char* FullText;

		f32 TypingSpeed;
		bool SoundPlayed;
		f32 VisibleCharCount;
		s8 LineCount;

		f32 TypingTimer;
		bool FinishedTyping;
	};

	DialogueBox StartedBox;            // tutStartedBox
	DialogueBox PagerOpenedBox;        // tutPagerOpenedBox
	DialogueBox PagerClosedBox;        // tutPagerClosedBox
	DialogueBox LiftOpenedBox;         // tutLiftOpenedBox
	DialogueBox LiftFloorSelectedBox;  // tutLiftFloorSelectedBox
	DialogueBox PatientCollectedBox;   // tutPatientCollectedBox
	DialogueBox JournalOpenedBox;
	DialogueBox JournalClosedBox;
	DialogueBox PatientDeliveredBox;   // tutPatientDeliveredBox

	// Mesh
	AEGfxVertexList* TutorialMesh;

	// Font
	s8 Font; // promptFontID

	// Textures
	struct Textures {
		AEGfxTexture* Background;  // tutBGTexture
		AEGfxTexture* Speaker;  // TutSpeakerTexture
		AEGfxTexture* Skip; 
		AEGfxTexture* YesButton;
		AEGfxTexture* NoButton;
		AEGfxTexture* YesText;
		AEGfxTexture* NoText;
	} TutorialTexture;

	// Input
	struct Input {
		bool SpacePressed;     // tutSpacePress
		bool ButtonHover;      // buttonHover
		bool PromptAnswered;   // promptAnswered
		bool DoTutorial;       // doTutorial
	} TutorialInput;

	// Dimension 
	struct Size {
		f32 Width, Height;
		f32 X, Y;
		f32 DestinationY;
	} TutorialSize;

	// Pop Up
	struct Prompt {
		bool Shown;            // tutPopUpShown
		f32 Timer;              // tutPopupTimer

		f32 FadeSpeed;			 // fadeSpeed
		f32 Alpha;              // promptAlpha
		f32 TextAlpha;          // textSpaceAlpha
	} TutorialPrompt;

	// Button
	struct Button {
		f32 Width, Height;
		f32 HoverSpeed;
		f32 YesScale;
		f32 NoScale;
	} TutorialButton;

	// Skip 
	struct Skip {
		f32 Width, Height;
		f32 X, Y;
		f32 Timer;
		f32 Anim;
		f32 Offset;
	} TutorialSkip;

	public:
		void Load();
		void LoadConfigFromFile(const std::string& Filename);
		void Initialize();
		void Update(f32 DT);
		void Draw();
		void Free();
		void DialogueBoxUpdate(DialogueBox& Dialogue, f32 DT);
		void TextDraw(char const* Str, f32 TextX, f32 TextY, f32 Alpha);
		void DrawTextTopRight(DialogueBox& Dialogue);
		void DrawSkip(DialogueBox& Dialogue);
		void ResetDialogueBox(DialogueBox& Dialogue);
		void ResetTutorial();
};

static Tutorial TheTutorial;
void Tutorial_Load(){ TheTutorial.Load(); }
void Tutorial_Initialize(){ TheTutorial.Initialize(); }
void Tutorial_Update(f32 DT){ TheTutorial.Update(DT); }
void Tutorial_Draw() { TheTutorial.Draw(); }
void Tutorial_Free() { TheTutorial.Free(); }
bool Doing_Tutorial() { return TheTutorial.TutorialInput.DoTutorial; }
bool Tutorial_Prompt_Answered() { return TheTutorial.TutorialInput.PromptAnswered; }
bool IsTutorialActive() { return TheTutorial.TutorialPrompt.Shown; }
bool IsTutorialFinished() { return TheTutorial.TutorialState.Finished; }

void Tutorial::Load() {
	Font = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
	// Placeholder for now
	TutorialTexture.YesButton = LoadTextureChecked(Assets::Tutorial::YesButton);
	TutorialTexture.NoButton = LoadTextureChecked(Assets::Tutorial::NoButton);

	/*TutorialTexture.YesButton = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
	TutorialTexture.YesText = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
	TutorialTexture.NoButton = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
	TutorialTexture.NoText = LoadTextureChecked(Assets::Main_Menu::OptionsTag);*/
	TutorialTexture.Background = LoadTextureChecked(Assets::Tutorial::TutBG);
	TutorialTexture.Speaker = LoadTextureChecked(Assets::Tutorial::TutSpeaker);
	TutorialTexture.Skip = LoadTextureChecked(Assets::Boss_Fight::PlayerArmGun);
}

void Tutorial::LoadConfigFromFile(const std::string& Filename) {
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
			if (Key == "Input") {
				if (!(iss >> TutorialInput.SpacePressed >> TutorialInput.ButtonHover >> TutorialInput.PromptAnswered >> TutorialInput.DoTutorial)) {
					printf("[FAIL] Tutorial input configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Tutorial input configuration loaded successfully.\n");
				}
			}
			if (Key == "Dimension") {
				if (!(iss >> TutorialSize.Width >> TutorialSize.Height >> TutorialSize.X >> TutorialSize.Y >> TutorialSize.DestinationY)) {
					printf("[FAIL] Tutorial dimension configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Tutorial dimension configuration loaded successfully.\n");
				}
			}
			if (Key == "Prompt") {
				if (!(iss >> TutorialPrompt.Shown >> TutorialPrompt.Timer >> TutorialPrompt.FadeSpeed >> TutorialPrompt.Alpha >> TutorialPrompt.TextAlpha)) {
					printf("[FAIL] Tutorial prompt configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Tutorial prompt configuration loaded successfully.\n");
				}
			}
			if (Key == "Button") {
				if (!(iss >> TutorialButton.Width >> TutorialButton.Height >> TutorialButton.HoverSpeed >> TutorialButton.YesScale >> TutorialButton.NoScale)) {
					printf("[FAIL] Tutorial button configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Tutorial button configuration loaded successfully.\n");
				}
			}
			if (Key == "Skip") {
				if (!(iss >> TutorialSkip.Width >> TutorialSkip.Height >> TutorialSkip.X >> TutorialSkip.Y >> TutorialSkip.Timer >> TutorialSkip.Anim >> TutorialSkip.Offset)) {
					printf("[FAIL] Tutorial skip configuration failed to load.\n");
				}
				else {
					printf("[SUCCESS] Tutorial skip configuration loaded successfully.\n");
				}
			}
		}
	}
	File.close();
}

void Tutorial::Initialize() {
	// Mesh
	TutorialMesh = CreateSquareMesh(0xFFFFFFFF);

	// Stage 
	CurrentStage = TUTORIAL_START;

	// Input
	TutorialInput.SpacePressed = false;
	TutorialInput.ButtonHover = false;
	TutorialInput.PromptAnswered = false;
	TutorialInput.DoTutorial = false;

	// Dimension
	TutorialSize.Width = 1200.0f;
	TutorialSize.Height = 150.0f;
	TutorialSize.X = 0.0f;
	TutorialSize.Y = -380.0f;
	TutorialSize.DestinationY = -350.0f;

	// Prompt Pop Up
	TutorialPrompt.Shown = false;
	TutorialPrompt.Timer = 1.0f;

	TutorialPrompt.FadeSpeed = 2.5f;
	TutorialPrompt.Alpha = 0.0f;
	TutorialPrompt.TextAlpha = 0.0f;

	// Button
	TutorialButton.Width = 250.0f;
	TutorialButton.Height = 100.0f;
	TutorialButton.HoverSpeed = 15.0f;
	TutorialButton.YesScale = 1.0f;
	TutorialButton.NoScale = 1.0f;

	// Skip
	TutorialSkip.Width = 100.0f;
	TutorialSkip.Height = 29.0f;
	TutorialSkip.X = 700.0f;
	TutorialSkip.Y = 0.0f;
	TutorialSkip.Timer = 1.0f;
	TutorialSkip.Anim = 0.0f;
	TutorialSkip.Offset = 0.0f;

	// Dialogue Box
	// Width, Height, X, Y, Shown, Timer, FadeSpeed, FadeAlpha, FullText, TypingSpeed, SoundPlayed
	StartedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "Great!\nPress [TAB] to open your pager.", 30.0f, false };
	PagerOpenedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "This is your pager, it displays the:\n1. Patient Collection Room\n2. Patient Delivery Room\nPress [TAB] again to close the pager.", 30.0f, false };
	PagerClosedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "Now, go to the lift.\nPress [L] to use it.", 30.0f, false };
	LiftOpenedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "This lift can travel from Basement 1 to Level 9.\nClick [B1-9] to select a floor.", 30.0f, false };
	LiftFloorSelectedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "Good! Now collect the patient from the room shown on your pager.", 30.0f, false };
	PatientCollectedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "There are two types of patients:\nNormal and Ghost\nUsing the Journal, you must identify which illness the patient has\nor if they are a ghost.\nPress [I] to open the Journal.", 30.0f, false };
	JournalOpenedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "This is your journal. It records the anomalies you encounter.\nA normal patient will display 3 anomalies.\nA ghost will display an additional anomaly.\nMake sure to identify them correctly.\nPress [I] again to close the Journal.", 30.0f, false };
	JournalClosedBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f, "Good! Now bring the patient to their designated room.\nNormal patients must be delivered to the room shown on your pager.\nGhosts must be delivered to the basement (B1).\nBringing the wrong patient type will result in Game Over.", 30.0f, false };
	PatientDeliveredBox = { TutorialSize.Width , TutorialSize.Height , TutorialSize.X, TutorialSize.Y, false, 0.0f, 2.5f, 0.0f,"Well done! Your shift begins now.\nDeliver at least 5 patients before each day ends.\nIf you forget how to play, Press [G] anytime to view the guide.\nStay alert and watch out.\nPress [TAB] to end the tutorial.", 30.0f, false };

	if (Game_IsPlaytestMode()) {
		PatientDeliveredBox.FullText = "Well done! Your shift begins now.\nDeliver at least 2 patients before each day ends.\nIf you forget how to play, Press [G] anytime to view the guide.\nStay alert and watch out.\nPress [TAB] to end the tutorial.";
	}
	

	// File reading
	LoadConfigFromFile("Assets/Tutorial/Config/TutorialConfig.txt");
}

void Tutorial::DialogueBoxUpdate(DialogueBox& Dialogue, f32 DT) {
	// Fading in from the right
	if (!Dialogue.Shown) { // basically waits till 0.5 f til the box shows
		Dialogue.Timer += DT;

		if (Dialogue.Timer >= 0.5f) {
			Dialogue.Shown = true;
		}
	}

	if (Dialogue.Shown) { // if the dhown is true, the alpha will increase
		if (Dialogue.FadeAlpha < 0.8f) {
			Dialogue.FadeAlpha += Dialogue.FadeSpeed * DT;

			if (Dialogue.FadeAlpha > 0.8f) {
				Dialogue.FadeAlpha = 0.8f; // clamp the alpha
			}
		}
		if (Dialogue.Y < TutorialSize.DestinationY) { // down to up transition
			Dialogue.Y += (TutorialSize.DestinationY - Dialogue.Y) * 3.0f * DT;

			if (Dialogue.Y > TutorialSize.DestinationY) {
				Dialogue.Y = TutorialSize.DestinationY;
			}
		}

		// Sound play
		if (!Dialogue.FinishedTyping && !Dialogue.SoundPlayed) {
			AudioManager_PlaySFX(SFX_TUTORIAL_TYPING, 0.2f, 1.0f, -1);
			Dialogue.SoundPlayed = true;
		}
		if (Dialogue.FinishedTyping && Dialogue.SoundPlayed) {
			AudioManager_StopSFX();
			Dialogue.SoundPlayed = false;
		}

		// Text typing effect
		Dialogue.VisibleCharCount += Dialogue.TypingSpeed * DT;
		if (Dialogue.VisibleCharCount >= static_cast<f32>(strlen(Dialogue.FullText))) {
			Dialogue.VisibleCharCount = static_cast<f32>(strlen(Dialogue.FullText));
			Dialogue.FinishedTyping = true;
		}

		TutorialInput.SpacePressed = AEInputCheckTriggered(AEVK_SPACE);
		bool MouseClicked{ IsAreaClickedByMouse(Dialogue.X, Dialogue.Y, Dialogue.Width, Dialogue.Height)};

		// Spacebar Logic
		if (TutorialInput.SpacePressed || (MouseClicked && AEInputCheckTriggered(AEVK_LBUTTON))) {
			if (!Dialogue.FinishedTyping) { // Basiclly check sif the dialogue box has finished typing, if not then itll be forced to be true
				Dialogue.VisibleCharCount = static_cast<f32>(strlen(Dialogue.FullText));
				Dialogue.FinishedTyping = true;
				Dialogue.SoundPlayed = true;
			}
		}
	}
}

void Tutorial::Update(f32 DT) {
	// Game start delayed pop up
	if (!TutorialInput.PromptAnswered && !TutorialPrompt.Shown) {
		TutorialPrompt.Timer += DT;

		if (TutorialPrompt.Timer >= 0.5f) {
			TutorialPrompt.Shown = true;
		}
	}

	if (TutorialPrompt.Shown && TutorialPrompt.Alpha < 1.0f) {
		TutorialPrompt.Alpha += TutorialPrompt.FadeSpeed * DT;

		if (TutorialPrompt.Alpha > 1.0f) {
			TutorialPrompt.Alpha = 1.0f;
		}
	}

	// -------------------------------- TUTORIAL PROMPT: YES OR NO --------------------------------
	bool YesButtonHover{ IsAreaClicked(-100.0f, -100.0f, TutorialButton.Width, TutorialButton.Height, Input_GetMouseX(), Input_GetMouseY()) },
		NoButtonHover{ IsAreaClicked(200.0f, -100.0f, TutorialButton.Width, TutorialButton.Height, Input_GetMouseX(), Input_GetMouseY()) };

	UpdateButtonScale(TutorialButton.YesScale, YesButtonHover, 1.2f, TutorialButton.HoverSpeed, DT);	// Function to scale the buttons when hover
	UpdateButtonScale(TutorialButton.NoScale, NoButtonHover, 1.2f, TutorialButton.HoverSpeed, DT);	// Function to scale the buttons when hover
	
	// if user doesnt want to do tutorial, tutorial prompt will close and pager will be displayed
	if (TutorialPrompt.Shown) {
		if (NoButtonHover && AEInputCheckTriggered(AEVK_LBUTTON)) {
			TutorialPrompt.Shown = false;
			TutorialInput.PromptAnswered = true;
			TutorialInput.DoTutorial = false;
			AudioManager_StopSFX();
			Timer_SetPaused(false);
		}

		if (YesButtonHover && AEInputCheckTriggered(AEVK_LBUTTON)) {
			TutorialPrompt.Shown = false;
			TutorialInput.PromptAnswered = true;
			TutorialInput.DoTutorial = true;
			//CurrentStage = TUTORIAL_START;
			ResetTutorial();
		}
	}

	// -------------------------------- TUTORIAL: YES --------------------------------
	f32 actualDT{ static_cast<f32>(AEFrameRateControllerGetFrameTime()) };
	if (TutorialInput.DoTutorial && !TutorialState.Finished) {
		TutorialPrompt.TextAlpha = 1.0f;
		Timer_SetPaused(true); // so player doesnt lose the game if they take too long
		switch (CurrentStage) {
			case TUTORIAL_START: {
				TutorialState.Started = true;
				DialogueBoxUpdate(StartedBox, DT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_START && IsPagerActive()) { // if player opens the pager, next stage of tutorial will appaer 
					CurrentStage = TUTORIAL_WAIT_FOR_PAGER_CLOSE; // nxet stage
					TutorialState.Started = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PAGER_CLOSE: {
				TutorialState.PagerOpened = true;
				DialogueBoxUpdate(PagerOpenedBox, DT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_WAIT_FOR_PAGER_CLOSE && !IsPagerActive()) {
					CurrentStage = TUTORIAL_WAIT_FOR_LIFT_OPEN; // nxet stage
					TutorialState.PagerOpened = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_LIFT_OPEN: {
				TutorialState.PagerClosed = true;
				DialogueBoxUpdate(PagerClosedBox, DT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_WAIT_FOR_LIFT_OPEN && Lift_IsActive()) {
					CurrentStage = TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION; // nxet stage
					TutorialState.PagerClosed = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION: {
				TutorialState.LiftOpened = true;
				DialogueBoxUpdate(LiftOpenedBox, DT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION && Lift_IsFloorPressed()) {
					CurrentStage = TUTORIAL_WAIT_FOR_PATIENT_COLLECTION; // nxet stage
					TutorialState.LiftOpened = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PATIENT_COLLECTION: {
				TutorialState.LiftFloorSelected = true;
				DialogueBoxUpdate(LiftFloorSelectedBox, DT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_WAIT_FOR_PATIENT_COLLECTION && Player_HasPatient()) {
					Player_ForceHumanPatient();
					CurrentStage = TUTORIAL_WAIT_FOR_JOURNAL_OPEN; // nxet stage
					TutorialState.LiftFloorSelected = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_JOURNAL_OPEN: {
				TutorialState.PatientCollected = true;
				DialogueBoxUpdate(PatientCollectedBox, actualDT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_WAIT_FOR_JOURNAL_OPEN && Journal_IsOpen()) {
					CurrentStage = TUTORIAL_WAIT_FOR_JOURNAL_CLOSE; // nxet stage
					TutorialState.PatientCollected = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_JOURNAL_CLOSE: {
				TutorialState.JournalOpened = true;
				DialogueBoxUpdate(JournalOpenedBox, actualDT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_WAIT_FOR_JOURNAL_CLOSE && !Journal_IsOpen()) {
					CurrentStage = TUTORIAL_WAIT_FOR_PATIENT_DELIVERY; // nxet stage
					TutorialState.JournalOpened = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PATIENT_DELIVERY: {
				TutorialState.JournalClosed = true;
				DialogueBoxUpdate(JournalClosedBox, actualDT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_WAIT_FOR_PATIENT_DELIVERY && !Player_HasPatient()) {
					CurrentStage = TUTORIAL_END; // nxet stage
					TutorialState.JournalClosed = false;
				}
				break;
			}
			case TUTORIAL_END: {
				TutorialState.PatientDelivered = true;
				DialogueBoxUpdate(PatientDeliveredBox, actualDT); // draws the dialogue box
				if (CurrentStage == TUTORIAL_END && AEInputCheckTriggered(AEVK_TAB)) {
					TutorialState.PatientDelivered = false;
					TutorialState.Finished = true;
					AudioManager_StopSFX();
					Timer_SetPaused(false); // UNPAUSE TIMER
				}
				break;
			}
		}	
	}
}

// Helper function for printing by line
void Tutorial::TextDraw(char const* Str, f32 TextX, f32 TextY, f32 Alpha) {
	f32 WinH{ static_cast<f32>(AEGfxGetWindowHeight()) };
	f32 LineHeight{ 50.0f / WinH };
	s8 LineNum{};
	const char* LineStart{ Str };

	for (int i{};; ++i) {
		// This basically checks the end of the line, if it reaches the end, line will be copied and printed
		if (Str[i] == '\n' || Str[i] == '\0') {
			std::string Line(LineStart, &Str[i]);

			// Get Text Width
			AEGfxPrint(Font, Line.c_str(), textPosition(TextX, TextY).first, textPosition(TextX, TextY).second - LineHeight * LineNum, 700.0f / WinH, 1.0f, 1.0f, 1.0f, Alpha);
			++LineNum;
			if (Str[i] == '\0') {
				break;
			}
			LineStart = &Str[i + 1];
		}
	}
}

// Helper function to draw text on top right
void Tutorial::DrawTextTopRight(DialogueBox& Dialogue) {
	f32 PosX{ (Dialogue.X - (Dialogue.Width * 0.5f)) + 120.0f },
		PosY{ (Dialogue.Y + (Dialogue.Height * 0.5f)) - 30.0f };
	
	std::string visibleText( Dialogue.FullText, static_cast<size_t>(Dialogue.VisibleCharCount) );

	TextDraw(visibleText.c_str(), PosX, PosY, Dialogue.FadeAlpha);
}

void Tutorial::DrawSkip(DialogueBox& Dialogue) {
	f32 dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());
	if (!Dialogue.FinishedTyping) {
		TutorialSkip.Timer += dt;
		TutorialSkip.Anim = sinf(TutorialSkip.Timer * 3.0f + TutorialSkip.Offset) * 20.0f;
	}
	else {
		TutorialSkip.Anim = 0.0f;
	}
	DrawTextureMesh(TutorialMesh, TutorialTexture.Skip, static_cast<f32>((Dialogue.X + (Dialogue.Width * 0.5) - 90.0f) + TutorialSkip.Anim), static_cast<f32>((Dialogue.Y - (Dialogue.Height * 0.5) + 40.0f)), TutorialSkip.Width, TutorialSkip.Height, Dialogue.FadeAlpha);
}
void Tutorial::ResetDialogueBox(DialogueBox& Dialogue) {
	Dialogue.Shown = false;
	Dialogue.Timer = 0.0f;
	Dialogue.FadeAlpha = 0.0f;
	Dialogue.VisibleCharCount = 0.0f;
	Dialogue.TypingTimer = 0.0f;
	Dialogue.FinishedTyping = false;
	Dialogue.SoundPlayed = false;
	Dialogue.Y = TutorialSize.Y;
}

void Tutorial::ResetTutorial() {
	// Reset states
	TutorialState = {};
	CurrentStage = TUTORIAL_START;

	// Reset all dialogue boxes
	ResetDialogueBox(StartedBox);
	ResetDialogueBox(PagerOpenedBox);
	ResetDialogueBox(PagerClosedBox);
	ResetDialogueBox(LiftOpenedBox);
	ResetDialogueBox(LiftFloorSelectedBox);
	ResetDialogueBox(PatientCollectedBox);
	ResetDialogueBox(JournalOpenedBox);
	ResetDialogueBox(JournalClosedBox);
	ResetDialogueBox(PatientDeliveredBox);
}

void Tutorial::Draw() {
	// Prompt
	if (TutorialPrompt.Shown) {
		// Draws prompt background
		u32 black = (0x000000 << 8) | static_cast<u32>(TutorialPrompt.Alpha * 187.0f);
		DrawSquareMesh(TutorialMesh, 0.0f, 0.0f, static_cast<f32>(AEGfxGetWindowWidth()), static_cast<f32>(AEGfxGetWindowHeight()), black);

		// Draws prompt text
		f32 PromptX{ textPosition(-220.0f, 0.0f).first },
			PromptY{ textPosition(-220.0f, 0.0f).second };
		AEGfxPrint(Font, "Do you need a tutorial?", PromptX, PromptY, 1.0f, 1.0f, 1.0f, 1.0f, TutorialPrompt.Alpha);

		// Draws the buttons
		DrawTextureMesh(TutorialMesh, TutorialTexture.YesButton, -180.0f, -100.0f, TutorialButton.Width * TutorialButton.YesScale, TutorialButton.Height * TutorialButton.YesScale, TutorialPrompt.Alpha);
		DrawTextureMesh(TutorialMesh, TutorialTexture.NoButton, 180.0f, -100.0f, TutorialButton.Width * TutorialButton.NoScale, TutorialButton.Height * TutorialButton.NoScale, TutorialPrompt.Alpha);
	}

	if (TutorialInput.DoTutorial && !TutorialState.Finished) {
		if (TutorialState.Started) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, StartedBox.X, StartedBox.Y, StartedBox.Width, StartedBox.Height, StartedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (StartedBox.X - (StartedBox.Width * 0.5f)) + 50.0f, (StartedBox.Y + (StartedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, StartedBox.FadeAlpha);
			DrawTextTopRight(StartedBox);	
			DrawSkip(StartedBox);
		}
		if (TutorialState.PagerOpened) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, PagerOpenedBox.X, PagerOpenedBox.Y, PagerOpenedBox.Width, PagerOpenedBox.Height, PagerOpenedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (PagerOpenedBox.X - (PagerOpenedBox.Width * 0.5f)) + 50.0f, (PagerOpenedBox.Y + (PagerOpenedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, PagerOpenedBox.FadeAlpha);
			DrawTextTopRight(PagerOpenedBox);
			DrawSkip(PagerOpenedBox);
		}
		if (TutorialState.PagerClosed) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, PagerClosedBox.X, PagerClosedBox.Y, PagerClosedBox.Width, PagerClosedBox.Height, PagerClosedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (PagerClosedBox.X - (PagerClosedBox.Width * 0.5f)) + 50.0f, (PagerClosedBox.Y + (PagerClosedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, PagerClosedBox.FadeAlpha);
			DrawTextTopRight(PagerClosedBox);
			DrawSkip(PagerClosedBox);
		}
		if (TutorialState.LiftOpened) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, LiftOpenedBox.X, LiftOpenedBox.Y, LiftOpenedBox.Width, LiftOpenedBox.Height, LiftOpenedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (LiftOpenedBox.X - (LiftOpenedBox.Width * 0.5f)) + 50.0f, (LiftOpenedBox.Y + (LiftOpenedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, LiftOpenedBox.FadeAlpha);
			DrawTextTopRight(LiftOpenedBox);
			DrawSkip(LiftOpenedBox);
		}
		if (TutorialState.LiftFloorSelected) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, LiftFloorSelectedBox.X, LiftFloorSelectedBox.Y, LiftFloorSelectedBox.Width, LiftFloorSelectedBox.Height, LiftFloorSelectedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (LiftFloorSelectedBox.X - (LiftFloorSelectedBox.Width * 0.5f)) + 50.0f, (LiftFloorSelectedBox.Y + (LiftFloorSelectedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, LiftFloorSelectedBox.FadeAlpha);
			DrawTextTopRight(LiftFloorSelectedBox);
			DrawSkip(LiftFloorSelectedBox);
		}
		if (TutorialState.PatientCollected) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, PatientCollectedBox.X, PatientCollectedBox.Y, PatientCollectedBox.Width, PatientCollectedBox.Height, PatientCollectedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (PatientCollectedBox.X - (PatientCollectedBox.Width * 0.5f)) + 50.0f, (PatientCollectedBox.Y + (PatientCollectedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, PatientCollectedBox.FadeAlpha);
			DrawTextTopRight(PatientCollectedBox);
			DrawSkip(PatientCollectedBox);
		}
		if (TutorialState.JournalOpened) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, JournalOpenedBox.X, JournalOpenedBox.Y, JournalOpenedBox.Width, JournalOpenedBox.Height, JournalOpenedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (JournalOpenedBox.X - (JournalOpenedBox.Width * 0.5f)) + 50.0f, (JournalOpenedBox.Y + (JournalOpenedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, JournalOpenedBox.FadeAlpha);
			DrawTextTopRight(JournalOpenedBox);
			DrawSkip(JournalOpenedBox);
		}
		if (TutorialState.JournalClosed) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, JournalClosedBox.X, JournalClosedBox.Y, JournalClosedBox.Width, JournalClosedBox.Height, JournalClosedBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (JournalClosedBox.X - (JournalClosedBox.Width * 0.5f)) + 50.0f, (JournalClosedBox.Y + (JournalClosedBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, JournalClosedBox.FadeAlpha);
			DrawTextTopRight(JournalClosedBox);
			DrawSkip(JournalClosedBox);
		}
		if (TutorialState.PatientDelivered) {
			DrawTextureMesh(TutorialMesh, TutorialTexture.Background, PatientDeliveredBox.X, PatientDeliveredBox.Y, PatientDeliveredBox.Width, PatientDeliveredBox.Height, PatientDeliveredBox.FadeAlpha);
			DrawTextureMesh(TutorialMesh, TutorialTexture.Speaker, (PatientDeliveredBox.X - (PatientDeliveredBox.Width * 0.5f)) + 50.0f, (PatientDeliveredBox.Y + (PatientDeliveredBox.Height * 0.5f)) - 50.0f, 100.0f, 100.0f, PatientDeliveredBox.FadeAlpha);
			DrawTextTopRight(PatientDeliveredBox);
			DrawSkip(PatientDeliveredBox);
		}
	}

}

void Tutorial::Free() {
	FreeMeshSafe(TutorialMesh);

	UnloadTextureSafe(TutorialTexture.Background);
	UnloadTextureSafe(TutorialTexture.Speaker);
	UnloadTextureSafe(TutorialTexture.Skip);

	UnloadTextureSafe(TutorialTexture.YesButton);
	UnloadTextureSafe(TutorialTexture.NoButton);
	UnloadTextureSafe(TutorialTexture.YesText);
	UnloadTextureSafe(TutorialTexture.NoText);

	// Destroy font if valid
	if (Font >= 0) {
		AEGfxDestroyFont(Font);
		Font = -1;
	}
}