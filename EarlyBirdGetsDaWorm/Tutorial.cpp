#include "pch.hpp"

// Tutorial Stages
enum TutorialStages {
	TUTORIAL_START,
	TUTORIAL_WAIT_FOR_PAGER_CLOSE,
	TUTORIAL_WAIT_FOR_LIFT_OPEN,
	TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION,
	TUTORIAL_WAIT_FOR_PATIENT_COLLECTION,
	TUTORIAL_WAIT_FOR_PATIENT_DELIVERY,
	TUTORIAL_END
};
static s8 curTutStage{};
static bool tutStarted{ false };
static bool tutPagerOpened{ false };
static bool tutPagerClosed{ false };
static bool tutLiftOpened{ false };
static bool tutLiftFloorSelected{ false };
static bool tutPatientCollected{ false };
static bool tutPatientDelivered{ false };
static bool tutFinished{ false };

// Tutorial Variables
struct DialogueBox {
	f32 tutBGW;
	f32 tutBGH;
	f32 tutBGX;
	f32 tutBGY;

	// Pop Up
	bool dialogueShown;
	f32 dialogueTimer;

	// Fade Animation
	f32 dialogueFadeSpeed;
	f32 dialogueFadeAlpha;

	const char* tutFullText;	// The entire 

	f32 tutTypingSpeed;			// Number of lines currently visible
	bool tutSoundPlayed;		// T check if sound is being played
	f32 tutVisibleCharCount;	// Number of characters currently shown
	s8 tutLineCount;			// Number of lines currently visible

	f32 tutTextW;
	f32 tutTextH;


	f32 typingTimer;			// Accumulates time for typing speed
	bool finishedTyping; 
};
static DialogueBox tutStartedBox, tutPagerOpenedBox, tutPagerClosedBox, tutLiftOpenedBox,
tutLiftFloorSelectedBox, tutPatientCollectedBox, tutPatientDeliveredBox;
static bool tutSpacePress{ false };

// Mesh
static AEGfxVertexList* TutorialMesh;

// Textures
static AEGfxTexture* yesButtonTexture;
static AEGfxTexture* yesTextTexture;
static AEGfxTexture* noButtonTexture;
static AEGfxTexture* noTextTexture;
static AEGfxTexture* tutBGTexture;
static AEGfxTexture* TutSpeakerTexture;

// Text
static s8 promptFontID;

// Button
static f32 buttonW{ 250.0f };
static f32 buttonH{ 100.0f };
static bool doTutorial{ false };
static bool promptAnswered{ false };

// Button Animation
static bool buttonHover{ false };
static f32 buttonHoverSpeed{ 15.0f };
static f32 yesButtonScale{ 1.0f };
static f32 noButtonScale{ 1.0f };

// Prompt Pop Up
static bool tutPopUpShown{ false };
static f32 tutPopupTimer{ 0.0f };

// Fade In Animation
static f32 fadeSpeed{ 2.5f };
static f32 promptAlpha{ 0.0f };
static f32 textSpaceAlpha{ 0.0f };

void Tutorial_Load() {
	promptFontID = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
	yesButtonTexture = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
	yesTextTexture = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
	noButtonTexture = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
	noTextTexture = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
	yesButtonTexture = LoadTextureChecked(Assets::Tutorial::YesButton);
	noButtonTexture = LoadTextureChecked(Assets::Tutorial::NoButton);
	tutBGTexture = LoadTextureChecked(Assets::Tutorial::TutBG);
	TutSpeakerTexture = LoadTextureChecked(Assets::Tutorial::TutSpeaker);
}

void Tutorial_Initialize() {
	// Mesh
	TutorialMesh = CreateSquareMesh(0xFFFFFFFF);

	// Button
	yesButtonScale = 1.0f;
	noButtonScale = 1.0f;
	doTutorial = false;
	promptAnswered = false;

	// Prompt Pop Up
	tutPopupTimer = 0.0f;
	tutPopUpShown = false;

	// Fade In Animation
	fadeSpeed = 2.5f;
	promptAlpha = 0.0f;	
	textSpaceAlpha = 0.0f;

	// Tutorial
	tutStarted = false;
	tutPagerOpened = false;
	tutPagerClosed = false;
	tutLiftOpened = false;
	tutLiftFloorSelected = false;
	tutPatientCollected = false;
	tutPatientDelivered = false;
	tutFinished = false;

	// Tutorial Dialogue Box
	tutStartedBox = { 520.0f, 120.0f, 450.0f, 50.0f, false , 0.0f, 2.5f, 0.0f, "Great!\nPress [TAB] to open your pager.", 25.0f, false};
	tutPagerOpenedBox = { 600.0f, 310.0f, 400.0f, -150.0f, false , 0.0f, 2.5f, 0.0f, 
		"This is your pager.\nIt shows :\n1. Patient Collection Room\n2. Patient Delivery Room\nPress [TAB] again to close the pager.", 25.0f, false };
	tutPagerClosedBox = { 380.0f, 120.0f, 450.0f, 50.0f, false , 0.0f, 2.5f, 0.0f, "Now, go to the lift.\nPress [L] to use it.", 25.0f, false };
	tutLiftOpenedBox = { 550.0f, 220.0f, 530.0f, -150.0f, false , 0.0f, 2.5f, 0.0f,
		"This lift can travel from\nBasement 1 to Level 9.\nTo go to a level,\nClick [B1-9] to select a floor.", 25.0f, false };
	tutLiftFloorSelectedBox = { 385.0f, 180.0f, 450.0f, -150.0f, false , 0.0f, 2.5f, 0.0f, "Good! Now collect the\npatient from the room\nshown on your pager.", 25.0f, false };
	tutPatientCollectedBox = { 580.0f, 420.0f, 530.0f, -150.0f, false , 0.0f, 2.5f, 0.0f,
		"There are two types of patients:\nNormal and Ghost\nNormal patients must be delivered\nto the room shown on your pager,\nand Ghosts to the basement.\nGhosts will have an additional\nsymptom on top of illness.\nWrong delivery will end the game.", 25.0f, false };
	tutPatientDeliveredBox = { 625.0f, 290.0f, 450.0f, -170.0f, false , 0.0f, 2.5f, 0.0f, "Well done!\nYour shift begins now.\nDeliver at least 5 patients.\nPress [G] anytime to view the guide.\nPress [I] anytime to view the journal.\nStay alert and watch out.", 25.0f, false };

	tutSpacePress = false;
}

/*!*****************************************************************************
\brief
	Updates a tutorial dialogue box.

	Handles the animation and behaviour of a dialogue box including:
	- Fade-in animation
	- Sliding movement to its destination position
	- Typing text effect
	- Typing sound playback
	- Spacebar logic to instantly reveal the full text

\param[in,out] dialogue
	Reference to the dialogue box being updated.

\param[in] dt
	Delta time representing the time elapsed since the previous frame.

\param[in] destX
	The target x-position the dialogue box will slide toward.

\return
	None.
*******************************************************************************/
void DialogueBox_Update(DialogueBox& dialogue, f32 dt, f32 destX) {
	// Fading in from the right
	if (!dialogue.dialogueShown) { // basically waits till 0.5 f til the box shows
		dialogue.dialogueTimer += dt;

		if (dialogue.dialogueTimer >= 0.5f) {
			dialogue.dialogueShown = true;
		}
	}

	if (dialogue.dialogueShown) { // if the dhown is true, the alpha will increase
		if (dialogue.dialogueFadeAlpha < 1.0f) {
			dialogue.dialogueFadeAlpha += dialogue.dialogueFadeSpeed * dt;

			if (dialogue.dialogueFadeAlpha > 1.0f) {
				dialogue.dialogueFadeAlpha = 1.0f; // clamp the alpha
			}
		}
		if (dialogue.tutBGX > destX) { // currently i put their initial X value to be more than the destination X so itll move from righ tto left
			dialogue.tutBGX += (destX - dialogue.tutBGX) * 3.0f * dt;

			if (dialogue.tutBGX < destX) {
				dialogue.tutBGX = destX;
			}
		}

		// Sound play
		if (!dialogue.finishedTyping && !dialogue.tutSoundPlayed) {
			AudioManager_PlaySFX(SFX_TUTORIAL_TYPING, 0.2f, 1.0f, -1);
			dialogue.tutSoundPlayed = true;
		}
		if (dialogue.finishedTyping && dialogue.tutSoundPlayed) {
			AudioManager_StopSFX();
			dialogue.tutSoundPlayed = false;
		}

		// Text typing effect
		dialogue.tutVisibleCharCount += dialogue.tutTypingSpeed * dt;
		if (dialogue.tutVisibleCharCount >= static_cast<f32>(strlen(dialogue.tutFullText))) {
			dialogue.tutVisibleCharCount = static_cast<f32>(strlen(dialogue.tutFullText));
			dialogue.finishedTyping = true;
		}

		tutSpacePress = AEInputCheckTriggered(AEVK_SPACE);

		// Spacebar Logic
		if (tutSpacePress) {
			if (!dialogue.finishedTyping) { // Basiclly check sif the dialogue box has finished typing, if not then itll be forced to be true
				dialogue.tutVisibleCharCount = static_cast<f32>(strlen(dialogue.tutFullText));
				dialogue.finishedTyping = true;
				dialogue.tutSoundPlayed = true;
				//textSpaceAlpha = 0.0f;
			}
		}

		/*if (dialogue.finishedTyping) {
			textSpaceAlpha = 0.0f;
		}*/
	}
}

void Tutorial_Update(f32 dt) {
	// Game start delayed pop up
	if (!promptAnswered && !tutPopUpShown) {
		tutPopupTimer += dt;

		if (tutPopupTimer >= 0.5f) {
			tutPopUpShown = true;
		}
	}

	if (tutPopUpShown && promptAlpha < 1.0f) {
		promptAlpha += fadeSpeed * dt;

		if (promptAlpha > 1.0f) {
			promptAlpha = 1.0f;
		}
	}

	
	// -------------------------------- TUTORIAL PROMPT: YES OR NO --------------------------------
	bool yesButtonHover{ IsAreaClicked(-100.0f, -100.0f, buttonW, buttonH, Input_GetMouseX(), Input_GetMouseY()) },
		noButtonHover{ IsAreaClicked(200.0f, -100.0f, buttonW, buttonH, Input_GetMouseX(), Input_GetMouseY()) };

	UpdateButtonScale(yesButtonScale, yesButtonHover, 1.2f, buttonHoverSpeed, dt);	// Function to scale the buttons when hover
	UpdateButtonScale(noButtonScale, noButtonHover, 1.2f, buttonHoverSpeed, dt);	// Function to scale the buttons when hover

	// if user doesnt want to do tutorial, tutorial prompt will close and pager will be displayed
	if (tutPopUpShown) {
		if (noButtonHover && AEInputCheckTriggered(AEVK_LBUTTON)) {
			tutPopUpShown = false;
			promptAnswered = true;
			doTutorial = false;
		}

		if (yesButtonHover && AEInputCheckTriggered(AEVK_LBUTTON)) {
			tutPopUpShown = false;
			promptAnswered = true;
			doTutorial = true;
			curTutStage = TUTORIAL_START;
		}
	}

	// -------------------------------- TUTORIAL: YES --------------------------------
	if (doTutorial && !tutFinished) {
		textSpaceAlpha = 1.0f;
		Timer_SetPaused(true); // so player doesnt lose the game if they take too long
		switch (curTutStage) {
			case TUTORIAL_START: {
				tutStarted = true;
				DialogueBox_Update(tutStartedBox, dt, 400.0f); // draws the dialogue box
				if (curTutStage == TUTORIAL_START && AEInputCheckTriggered(AEVK_TAB)) { // if player opens the pager, next stage of tutorial will appaer 
					curTutStage = TUTORIAL_WAIT_FOR_PAGER_CLOSE; // nxet stage
					tutStarted = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PAGER_CLOSE: {
				tutPagerOpened = true;
				DialogueBox_Update(tutPagerOpenedBox, dt, 350.0f);
				if (curTutStage == TUTORIAL_WAIT_FOR_PAGER_CLOSE && AEInputCheckTriggered(AEVK_TAB)) {
					curTutStage = TUTORIAL_WAIT_FOR_LIFT_OPEN;
					tutPagerOpened = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_LIFT_OPEN: {
				tutPagerClosed = true;
				DialogueBox_Update(tutPagerClosedBox, dt, 400.0f);
				if (curTutStage == TUTORIAL_WAIT_FOR_LIFT_OPEN && AEInputCheckTriggered(AEVK_L)) {
					curTutStage = TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION;
					tutPagerClosed = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION: {
				tutLiftOpened = true;
				DialogueBox_Update(tutLiftOpenedBox, dt, 480.0f);
				if ((curTutStage == TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION) && (Lift_IsFloorPressed())) {
					curTutStage = TUTORIAL_WAIT_FOR_PATIENT_COLLECTION;
					tutLiftOpened = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PATIENT_COLLECTION: {
				tutLiftFloorSelected = true;
				DialogueBox_Update(tutLiftFloorSelectedBox, dt, 400.0f);
				if (curTutStage == TUTORIAL_WAIT_FOR_PATIENT_COLLECTION && Player_HasPatient()) {
					Player_ForceHumanPatient();
					curTutStage = TUTORIAL_WAIT_FOR_PATIENT_DELIVERY;
					tutLiftFloorSelected = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PATIENT_DELIVERY: {
				tutPatientCollected = true;
				DialogueBox_Update(tutPatientCollectedBox, dt, 500.0f);
				if (curTutStage == TUTORIAL_WAIT_FOR_PATIENT_DELIVERY && !Player_HasPatient()) {
					curTutStage = TUTORIAL_END;
					tutPatientCollected = false;
				}
				break;
			}
			case TUTORIAL_END: {
				tutPatientDelivered = true;
				DialogueBox_Update(tutPatientDeliveredBox, dt, 400.0f);
				if (curTutStage == TUTORIAL_END && AEInputCheckTriggered(AEVK_TAB)) {
					tutFinished = true;
					Timer_SetPaused(false); // UNPAUSE TIMER
				}
				break;
			}

		}
	}
}

bool Doing_Tutorial() { return doTutorial; }
bool Tutorial_Prompt_Answered() { return promptAnswered; }
bool IsTutorialActive() { return tutPopUpShown; }

// Helper function for typing effect
/*!*****************************************************************************
\brief
	Draws tutorial text with support for multiple lines.

	The function processes newline characters in the string and
	renders each line individually with proper vertical spacing.

\param[in] str
	The text string to render.

\param[in] tutTextX
	The x-position where the text should be drawn.

\param[in] tutTextY
	The y-position where the text should be drawn.

\param[in] alpha
	Transparency value used when rendering the text.

\return
	None.
*******************************************************************************/
void Tutorial_Text_Draw(char const* str, f32 tutTextX, f32 tutTextY, f32 alpha) {
	f32 winH{ static_cast<f32>(AEGfxGetWindowHeight()) };
	f32 lineHeight{ 100.0f / winH};
	s8 lineNum{};
	const char* lineStart{ str };

	for (int i{};; ++i) {
		// This basically checks the end of the line, if it reaches the end, line will be copied and printed
		if (str[i] == '\n' || str[i] == '\0') {
			std::string line(lineStart, &str[i]); 
			AEGfxPrint(promptFontID, line.c_str(), textPosition(tutTextX, tutTextY).first, textPosition(tutTextX, tutTextY).second - lineHeight * lineNum, 700.0f / winH, 1.0f, 1.0f, 1.0f, alpha);
			++lineNum;
			if (str[i] == '\0') {
				break;
			}
			lineStart = &str[i + 1];
		}
		
	}
}

// Helper function for dialogue
/*!*****************************************************************************
\brief
	Renders the background for a tutorial dialogue box.

	The background uses a semi-transparent colored rectangle and
	fades in based on the dialogue box alpha value.

\param[in] dialogue
	Reference to the dialogue box whose background will be drawn.

\return
	None.
*******************************************************************************/
//void DrawDialogueBoxBackground(DialogueBox& dialogue) {
//	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
//		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };
//
//	// Apply red fade like the guide
//	// Moves the color bits left to make space for the transparency value.   | gets the fade amount by multiplying it to 200.0f
//	/*u32 colorBG = (0x880000 << 8) | static_cast<u32>(dialogue.dialogueFadeAlpha * 200.0f);
//	DrawSquareMesh(tutBGMesh, dialogue.tutBGX, dialogue.tutBGY, dialogue.tutBGW, dialogue.tutBGH, 1.0f);*/
//	DrawTextureMesh(tutBGMesh, tutBGTexture, dialogue.tutBGX, dialogue.tutBGY, dialogue.tutBGW, dialogue.tutBGH, dialogue.dialogueFadeAlpha);
//}

void Tutorial_Draw() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	if (tutPopUpShown) {
		// Draws prompt background
		u32 black = (0x000000 << 8) | static_cast<u32>(promptAlpha * 187.0f);
		DrawSquareMesh(TutorialMesh, 0.0f, 0.0f, winW, winH, black);

		// Draws prompt text
		f32 promptX{ textPosition(-220.0f, 0.0f).first },
			promptY{ textPosition(-220.0f, 0.0f).second };
		AEGfxPrint(promptFontID, "Do you need a tutorial?", promptX, promptY, 1.0f, 1.0f, 1.0f, 1.0f, promptAlpha);

		// Draws the buttons
		DrawTextureMesh(TutorialMesh, yesButtonTexture, -180.0f, -100.0f, buttonW * yesButtonScale, buttonH * yesButtonScale, promptAlpha);
		DrawTextureMesh(TutorialMesh, noButtonTexture, 180.0f, -100.0f, buttonW * noButtonScale, buttonH * noButtonScale, promptAlpha);
	}

	if (doTutorial && !tutFinished) {
		const char* infoText{ "PRESS [SPACE] TO SKIP TEXT" };
		// Change text when tutorial is ending
		if (curTutStage == TUTORIAL_END) {
			infoText = "PRESS [TAB] TO END TUTORIAL";
		}
		f32 infoX{ textPosition(-220.0f, -380.0f).first },
			infoY{ textPosition(-220.0f, -380.0f).second };
		AEGfxPrint(promptFontID, infoText, infoX, infoY, 1.0f, 1.0f, 1.0f, 1.0f, textSpaceAlpha);

		if (tutStarted) {
			//DrawDialogueBoxBackground(tutStartedBox);
			DrawTextureMesh(TutorialMesh, tutBGTexture, tutStartedBox.tutBGX, tutStartedBox.tutBGY, tutStartedBox.tutBGW, tutStartedBox.tutBGH, tutStartedBox.dialogueFadeAlpha);
			DrawTextureMesh(TutorialMesh, TutSpeakerTexture, tutStartedBox.tutBGX + 225.0f, tutStartedBox.tutBGY + 60.0f, 64.0f, 64.0f, tutStartedBox.dialogueFadeAlpha);

			std::string visibleText(
				tutStartedBox.tutFullText,
				(size_t)tutStartedBox.tutVisibleCharCount
			); // It copies only the first few characters of the full text, c_str converts c++ to c -> const char* 
			Tutorial_Text_Draw(visibleText.c_str(), -240.0f + tutStartedBox.tutBGX, 15.0f + tutStartedBox.tutBGY, tutStartedBox.dialogueFadeAlpha);
		}
		if (tutPagerOpened) {
			//DrawDialogueBoxBackground(tutPagerOpenedBox);
			DrawTextureMesh(TutorialMesh, tutBGTexture, tutPagerOpenedBox.tutBGX, tutPagerOpenedBox.tutBGY, tutPagerOpenedBox.tutBGW, tutPagerOpenedBox.tutBGH, tutPagerOpenedBox.dialogueFadeAlpha);
			DrawTextureMesh(TutorialMesh, TutSpeakerTexture, tutPagerOpenedBox.tutBGX + 225.0f, tutPagerOpenedBox.tutBGY + 60.0f, 64.0f, 64.0f, tutPagerOpenedBox.dialogueFadeAlpha);

			std::string visibleText(
				tutPagerOpenedBox.tutFullText,
				(size_t)tutPagerOpenedBox.tutVisibleCharCount
			);
			Tutorial_Text_Draw(visibleText.c_str(), -280.0f + tutPagerOpenedBox.tutBGX, 95.0f + tutPagerOpenedBox.tutBGY, tutPagerOpenedBox.dialogueFadeAlpha);
		}
		if (tutPagerClosed) {
			//DrawDialogueBoxBackground(tutPagerClosedBox);
			DrawTextureMesh(TutorialMesh, tutBGTexture, tutPagerClosedBox.tutBGX, tutPagerClosedBox.tutBGY, tutPagerClosedBox.tutBGW, tutPagerClosedBox.tutBGH, tutPagerClosedBox.dialogueFadeAlpha);
			DrawTextureMesh(TutorialMesh, TutSpeakerTexture, tutPagerClosedBox.tutBGX + 225.0f, tutPagerClosedBox.tutBGY + 60.0f, 64.0f, 64.0f, tutPagerClosedBox.dialogueFadeAlpha);

			std::string visibleText(
				tutPagerClosedBox.tutFullText,
				(size_t)tutPagerClosedBox.tutVisibleCharCount
			);
			Tutorial_Text_Draw(visibleText.c_str(), -165.0f + tutPagerClosedBox.tutBGX, 15.0f + tutPagerClosedBox.tutBGY, tutPagerClosedBox.dialogueFadeAlpha);
		}
		if (tutLiftOpened) {
			//DrawDialogueBoxBackground(tutLiftOpenedBox);
			DrawTextureMesh(TutorialMesh, tutBGTexture, tutLiftOpenedBox.tutBGX, tutLiftOpenedBox.tutBGY, tutLiftOpenedBox.tutBGW, tutLiftOpenedBox.tutBGH, tutLiftOpenedBox.dialogueFadeAlpha);
			DrawTextureMesh(TutorialMesh, TutSpeakerTexture, tutLiftOpenedBox.tutBGX + 225.0f, tutLiftOpenedBox.tutBGY + 60.0f, 64.0f, 64.0f, tutLiftOpenedBox.dialogueFadeAlpha);

			std::string visibleText(
				tutLiftOpenedBox.tutFullText,
				(size_t)tutLiftOpenedBox.tutVisibleCharCount
			);
			Tutorial_Text_Draw(visibleText.c_str(), -230.0f + tutLiftOpenedBox.tutBGX, 65.0f + tutLiftOpenedBox.tutBGY, tutLiftOpenedBox.dialogueFadeAlpha);
		}
		if (tutLiftFloorSelected) {
			//DrawDialogueBoxBackground(tutLiftFloorSelectedBox);
			DrawTextureMesh(TutorialMesh, tutBGTexture, tutLiftFloorSelectedBox.tutBGX, tutLiftFloorSelectedBox.tutBGY, tutLiftFloorSelectedBox.tutBGW, tutLiftFloorSelectedBox.tutBGH, tutLiftFloorSelectedBox.dialogueFadeAlpha);
			DrawTextureMesh(TutorialMesh, TutSpeakerTexture, tutLiftFloorSelectedBox.tutBGX + 225.0f, tutLiftFloorSelectedBox.tutBGY + 60.0f, 64.0f, 64.0f, tutLiftFloorSelectedBox.dialogueFadeAlpha);

			std::string visibleText(
				tutLiftFloorSelectedBox.tutFullText,
				(size_t)tutLiftFloorSelectedBox.tutVisibleCharCount
			);
			Tutorial_Text_Draw(visibleText.c_str(), -170.0f + tutLiftFloorSelectedBox.tutBGX, 45.0f + tutLiftFloorSelectedBox.tutBGY, tutLiftFloorSelectedBox.dialogueFadeAlpha);
		}
		if (tutPatientCollected) {
			//DrawDialogueBoxBackground(tutPatientCollectedBox);
			DrawTextureMesh(TutorialMesh, tutBGTexture, tutPatientCollectedBox.tutBGX, tutPatientCollectedBox.tutBGY, tutPatientCollectedBox.tutBGW, tutPatientCollectedBox.tutBGH, tutPatientCollectedBox.dialogueFadeAlpha);
			DrawTextureMesh(TutorialMesh, TutSpeakerTexture, tutPatientCollectedBox.tutBGX + 225.0f, tutPatientCollectedBox.tutBGY + 60.0f, 64.0f, 64.0f, tutPatientCollectedBox.dialogueFadeAlpha);

			std::string visibleText(
				tutPatientCollectedBox.tutFullText,
				(size_t)tutPatientCollectedBox.tutVisibleCharCount
			);
			Tutorial_Text_Draw(visibleText.c_str(), -265.0f + tutPatientCollectedBox.tutBGX, 165.0f + tutPatientCollectedBox.tutBGY, tutPatientCollectedBox.dialogueFadeAlpha);
		}
		if (tutPatientDelivered) {
			//DrawDialogueBoxBackground(tutPatientDeliveredBox);
			DrawTextureMesh(TutorialMesh, tutBGTexture, tutPatientDeliveredBox.tutBGX, tutPatientDeliveredBox.tutBGY, tutPatientDeliveredBox.tutBGW, tutPatientDeliveredBox.tutBGH, tutPatientDeliveredBox.dialogueFadeAlpha);
			DrawTextureMesh(TutorialMesh, TutSpeakerTexture, tutPatientDeliveredBox.tutBGX + 225.0f, tutPatientDeliveredBox.tutBGY + 60.0f, 64.0f, 64.0f, tutPatientDeliveredBox.dialogueFadeAlpha);

			std::string visibleText(
				tutPatientDeliveredBox.tutFullText,
				(size_t)tutPatientDeliveredBox.tutVisibleCharCount
			);
			Tutorial_Text_Draw(visibleText.c_str(), -290.0f + tutPatientDeliveredBox.tutBGX, 110.0f + tutPatientDeliveredBox.tutBGY, tutPatientDeliveredBox.dialogueFadeAlpha);
		}
	}
}

void Tutorial_Free() {
	FreeMeshSafe(TutorialMesh);

	UnloadTextureSafe(yesButtonTexture);
	UnloadTextureSafe(noButtonTexture);
	UnloadTextureSafe(tutBGTexture);
	UnloadTextureSafe(TutSpeakerTexture);

	// Destroy font if valid
	if (promptFontID >= 0) {
		AEGfxDestroyFont(promptFontID);
		promptFontID = -1;
	}
}