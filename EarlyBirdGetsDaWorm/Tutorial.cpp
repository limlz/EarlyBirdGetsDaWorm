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
	f32 tutVisibleCharCount;			// Number of characters currently shown
	s8 tutLineCount;			// Number of lines currently visible

	f32 tutTextW;
	f32 tutTextH;


	f32 typingTimer;			// Accumulates time for typing speed
	bool finishedTyping; 
};
static DialogueBox tutStartedBox, tutPagerOpenedBox, tutPagerClosedBox, tutLiftOpenedBox,
tutLiftFloorSelectedBox, tutPatientCollectedBox, tutPatientDeliveredBox;
static f32 tutSpacePress{ false };

// Mesh
static AEGfxVertexList* tutPromptMesh;
static AEGfxVertexList* yesButtonMesh;
static AEGfxVertexList* noButtonMesh;
static AEGfxVertexList* tutBGMesh;

// Textures
static AEGfxTexture* yesButtonTexture;
static AEGfxTexture* noButtonTexture;
static AEGfxTexture* tutBGTexture;

// Text
static s8 promptFontID;
//static u32 black = 0x000000BB;

// Button
static f32 buttonW{ 200.0f };
static f32 buttonH{ 50.0f };
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

void Tutorial_Load() {
	promptFontID = AEGfxCreateFont(Assets::Fonts::Buggy, 20);
	yesButtonTexture = LoadTextureChecked(Assets::Tutorial::YesButton);
	noButtonTexture = LoadTextureChecked(Assets::Tutorial::NoButton);
	tutBGTexture = LoadTextureChecked(Assets::Tutorial::tutBG);
}

void Tutorial_Initialize() {
	// Mesh
	tutPromptMesh = CreateSquareMesh(0xFFFFFFFF);
	yesButtonMesh = CreateSquareMesh(0xFFFFFFFF);
	noButtonMesh = CreateSquareMesh(0xFFFFFFFF);
	tutBGMesh = CreateSquareMesh(0xFFFFFFFF);

	// Button
	yesButtonScale = 1.0f;
	noButtonScale = 1.0f;
	doTutorial = false;
	//promptAnswered = false; removed so that user is only prompted tut once

	// Prompt Pop Up
	tutPopupTimer = 0.0f;
	tutPopUpShown = false;

	// Fade In Animation
	fadeSpeed = 2.5f;
	promptAlpha = 0.0f;	

	// Tutorial
	tutStarted = false;
	tutPagerOpened = false;
	tutPagerClosed = false;
	tutLiftOpened = false;
	tutLiftFloorSelected = false;
	tutPatientCollected = false;
	tutPatientDelivered = false;

	// Tutorial Dialogue Box
	tutStartedBox = { 520.0f, 120.0f, 450.0f, 50.0f, false , 0.0f, 2.5f, 0.0f, "Great!\nPress [Q] to open your pager.", 10.0f };
	tutPagerOpenedBox = { 580.0f, 310.0f, 400.0f, -150.0f, false , 0.0f, 2.5f, 0.0f,
		"This is your pager.\nIt shows :\n1. Patient Collection Room\n2. Patient Delivery Room\nPress [Q] again to close the pager.", 10.0f};
	tutPagerClosedBox = { 520.0f, 120.0f, 450.0f, 50.0f, false , 0.0f, 2.5f, 0.0f, "Now, go to the lift.\nPress [L] to use it.", 10.0f };
	tutLiftOpenedBox = { 580.0f, 310.0f, 400.0f, -150.0f, false , 0.0f, 2.5f, 0.0f,
		"This lift can travel from Basement 1 (0) to Level 9.\nTo go to a level, press the number on your keyboard.\nTo exit the lift, press[L] again.\n", 10.0f };

	tutSpacePress = false;
}

void DialogueBox_Update(DialogueBox& dialogue, f32 dt, f32 destX) {
	// Fading in from the right
	if (!dialogue.dialogueShown) {
		dialogue.dialogueTimer += dt;

		if (dialogue.dialogueTimer >= 0.5f) {
			dialogue.dialogueShown = true;
		}
	}

	if (dialogue.dialogueShown) {
		if (dialogue.dialogueFadeAlpha < 1.0f) {
			dialogue.dialogueFadeAlpha += dialogue.dialogueFadeSpeed * dt;

			if (dialogue.dialogueFadeAlpha > 1.0f) {
				dialogue.dialogueFadeAlpha = 1.0f;
			}
		}
		if (dialogue.tutBGX > destX) {
			dialogue.tutBGX += (destX - dialogue.tutBGX) * 3.0f * dt;

			if (dialogue.tutBGX < destX) {
				dialogue.tutBGX = destX;
			}
		}

		// Text typing effect
		dialogue.tutVisibleCharCount += dialogue.tutTypingSpeed * dt;
		if (dialogue.tutVisibleCharCount >= strlen(dialogue.tutFullText)) {
			dialogue.tutVisibleCharCount = strlen(dialogue.tutFullText);
			dialogue.finishedTyping = true;
		}

		tutSpacePress = AEInputCheckTriggered(AEVK_SPACE);

		// Spacebar Logic
		if (tutSpacePress) {
			if (!dialogue.finishedTyping) {
				dialogue.tutVisibleCharCount = strlen(dialogue.tutFullText);
				dialogue.finishedTyping = true;
			}
		}
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
	bool yesButtonHover{ IsAreaClicked(-100.0f, -100.0f, 200.0f, 50.0f, Input_GetMouseX(), Input_GetMouseY()) },
		noButtonHover{ IsAreaClicked(200.0f, -100.0f, 200.0f, 50.0f, Input_GetMouseX(), Input_GetMouseY()) };

	f32 hoverTarget{ yesButtonHover ? 1.2f : 1.0f };
	yesButtonScale += (hoverTarget - yesButtonScale) * buttonHoverSpeed * dt;
	hoverTarget = noButtonHover ? 1.2f : 1.0f;
	noButtonScale += (hoverTarget - noButtonScale) * buttonHoverSpeed * dt;

	// if user doesnt want to do tutorial, tutorial prompt will close and pager will be displayed
	if (noButtonHover && AEInputCheckCurr(AEVK_LBUTTON)) {
		tutPopUpShown = false;
		promptAnswered = true;
		doTutorial = false;
	}

	if (yesButtonHover && AEInputCheckCurr(AEVK_LBUTTON)) {
		tutPopUpShown = false;
		promptAnswered = true;
		doTutorial = true;
		curTutStage = TUTORIAL_START;
	}

	// -------------------------------- TUTORIAL: YES --------------------------------
	if (doTutorial) {
		switch (curTutStage) {
			case TUTORIAL_START: {
				tutStarted = true;
				DialogueBox_Update(tutStartedBox, dt, 400.0f);
				if (AEInputCheckTriggered(AEVK_Q)) {
					curTutStage = TUTORIAL_WAIT_FOR_PAGER_CLOSE;
					tutStarted = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PAGER_CLOSE: {
				tutPagerOpened = true;
				DialogueBox_Update(tutPagerOpenedBox, dt, 350.0f);
				if (AEInputCheckTriggered(AEVK_Q)) {
					curTutStage = TUTORIAL_WAIT_FOR_LIFT_OPEN;
					tutPagerOpened = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_LIFT_OPEN: {
				tutPagerClosed = true;
				DialogueBox_Update(tutPagerClosedBox, dt, 400.0f);
				if (AEInputCheckTriggered(AEVK_L)) {
					curTutStage = TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION;
					tutPagerClosed = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_LIFT_FLOOR_SELECTION: {
				tutLiftOpened = true;
				DialogueBox_Update(tutLiftOpenedBox, dt, 350.0f);
				if (AEInputCheckTriggered(AEVK_Q)) {
					curTutStage = TUTORIAL_WAIT_FOR_PATIENT_COLLECTION;
					tutLiftOpened = false;
				}
				break;
			}
			case TUTORIAL_WAIT_FOR_PATIENT_COLLECTION: {
				/*tutLiftOpened = true;
				DialogueBox_Update(tutLiftOpenedBox, dt, 350.0f);
				if (AEInputCheckTriggered(AEVK_Q)) {
					curTutStage = TUTORIAL_WAIT_FOR_PATIENT_COLLECTION;
					tutLiftOpened = false;
				}
				break;*/
			}

		}
	}

	// -------------------------------- TUTORIAL STAGE: TUTORIAL_START --------------------------------
	
}

bool Doing_Tutorial() { return doTutorial; }
bool Tutorial_Prompt_Answered() { return promptAnswered; }
bool IsTutorialActive() { return tutPopUpShown; }

void Tutorial_Text_Draw(char const* str, f32 tutTextX, f32 tutTextY) {
	f32 winH{ static_cast<f32>(AEGfxGetWindowHeight()) };
	f32 lineHeight{ 100.0f / winH};
	s8 lineNum{};
	const char* lineStart{ str };

	for (int i{};; ++i) {
		if (str[i] == '\n' || str[i] == '\0') {
			std::string line(lineStart, &str[i]); // copy line
			AEGfxPrint(promptFontID, line.c_str(), textPosition(tutTextX, tutTextY).first, textPosition(tutTextX, tutTextY).second - lineHeight * lineNum, 700.0f / winH, 1.0f, 1.0f, 1.0f, tutStartedBox.dialogueFadeAlpha);
			++lineNum;
			if (str[i] == '\0') {
				break;
			}
			lineStart = &str[i + 1];
		}
		
	}
}

void Tutorial_Draw() {
	// Position
	f32 winW{ static_cast<f32>(AEGfxGetWindowWidth()) },
		winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	if (tutPopUpShown) {
		// Draws prompt background
		u32 black = (0x000000 << 8) | static_cast<u32>(promptAlpha * 187.0f);
		DrawSquareMesh(tutPromptMesh, 0.0f, 0.0f, winW, winH, black);

		// Draws prompt text
		f32 promptX{ textPosition(-220.0f, 0.0f).first },
			promptY{ textPosition(-220.0f, 0.0f).second };
		AEGfxPrint(promptFontID, "Do you need a tutorial?", promptX, promptY, 1.0f, 1.0f, 1.0f, 1.0f, promptAlpha);

		// Draws the buttons
		DrawTextureMesh(yesButtonMesh, yesButtonTexture, -180.0f, -100.0f, buttonW * yesButtonScale, buttonH * yesButtonScale, promptAlpha);
		DrawTextureMesh(noButtonMesh, noButtonTexture, 180.0f, -100.0f, buttonW * noButtonScale, buttonH * noButtonScale, promptAlpha);
	}

	if (tutStarted) {
		//Tutorial_DialogueBox_Draw("const char* str");
		DrawTextureMesh(tutBGMesh, tutBGTexture, tutStartedBox.tutBGX, tutStartedBox.tutBGY, tutStartedBox.tutBGW, tutStartedBox.tutBGH, tutStartedBox.dialogueFadeAlpha);
		std::string visibleText(
			tutStartedBox.tutFullText,
			(size_t)tutStartedBox.tutVisibleCharCount
		);
		Tutorial_Text_Draw(visibleText.c_str(), -230.0f + tutStartedBox.tutBGX, 10.0f + tutStartedBox.tutBGY);
	}
	if (tutPagerOpened) {
		DrawTextureMesh(tutBGMesh, tutBGTexture, tutPagerOpenedBox.tutBGX, tutPagerOpenedBox.tutBGY, tutPagerOpenedBox.tutBGW, tutPagerOpenedBox.tutBGH, tutPagerOpenedBox.dialogueFadeAlpha);
		std::string visibleText(
			tutPagerOpenedBox.tutFullText,
			(size_t)tutPagerOpenedBox.tutVisibleCharCount
		);
		Tutorial_Text_Draw(visibleText.c_str(), -260.0f + tutPagerOpenedBox.tutBGX, 95.0f + tutPagerOpenedBox.tutBGY);
	}
	if (tutPagerClosed) {
		DrawTextureMesh(tutBGMesh, tutBGTexture, tutPagerClosedBox.tutBGX, tutPagerClosedBox.tutBGY, tutPagerClosedBox.tutBGW, tutPagerClosedBox.tutBGH, tutPagerClosedBox.dialogueFadeAlpha);
		std::string visibleText(
			tutPagerClosedBox.tutFullText,
			(size_t)tutPagerClosedBox.tutVisibleCharCount
		);
		Tutorial_Text_Draw(visibleText.c_str(), -230.0f + tutPagerClosedBox.tutBGX, 10.0f + tutPagerClosedBox.tutBGY);
	}
	if (tutLiftOpened) {
		DrawTextureMesh(tutBGMesh, tutBGTexture, tutLiftOpenedBox.tutBGX, tutLiftOpenedBox.tutBGY, tutLiftOpenedBox.tutBGW, tutLiftOpenedBox.tutBGH, tutLiftOpenedBox.dialogueFadeAlpha);
		std::string visibleText(
			tutLiftOpenedBox.tutFullText,
			(size_t)tutLiftOpenedBox.tutVisibleCharCount
		);
		Tutorial_Text_Draw(visibleText.c_str(), -260.0f + tutLiftOpenedBox.tutBGX, 95.0f + tutLiftOpenedBox.tutBGY);
	}

}

void Tutorial_Free() {
	FreeMeshSafe(tutPromptMesh);
	FreeMeshSafe(yesButtonMesh);
	FreeMeshSafe(noButtonMesh);
	FreeMeshSafe(tutBGMesh);

	UnloadTextureSafe(yesButtonTexture);
	FreeMeshSafe(noButtonMesh);
	UnloadTextureSafe(tutBGTexture);

	// Destroy font if valid
	if (promptFontID >= 0) {
		AEGfxDestroyFont(promptFontID);
		promptFontID = -1;
	}
}