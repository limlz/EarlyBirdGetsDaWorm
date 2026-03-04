#include "pch.hpp"

// Tutorial Stages
enum TutorialStages {
	TUTORIAL_START,
	TUTORIAL_WAIT_FOR_PAGER_OPEN,
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
	f32 dialogueFadeInAlpha;
	f32 dialogueFadeOutAlpha;

	const char* tutFullText;	// The entire text

	f32 tutTextW;
	f32 tutTextH;

	f32 tutVisibleCharCount;			// Number of characters currently shown
	s8 tutLineCount;			// Number of lines currently visible
	s8 tutTypingSpeed;			// Number of lines currently visible


	f32 typingTimer;			// Accumulates time for typing speed
	bool finishedTyping; 
};
static DialogueBox tutStartedBox;
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
	tutStartedBox = { 500.0f, 100.0f, 450.0f, 50.0f, false , 0.0f, 2.5f, 0.0f, 0.0f, "Great!\nPress [Q] to open your pager."};
	tutSpacePress = false;
}

void DialogueBox_Update(DialogueBox& dialogue, f32 dt) {
	// Fading in from the right
	if (!dialogue.dialogueShown) {
		dialogue.dialogueTimer += dt;

		if (dialogue.dialogueTimer >= 0.5f) {
			dialogue.dialogueShown = true;
		}
	}

	if (dialogue.dialogueShown) {
		if (dialogue.dialogueFadeInAlpha < 1.0f) {
			dialogue.dialogueFadeInAlpha += dialogue.dialogueFadeSpeed * dt;

			if (dialogue.dialogueFadeInAlpha > 1.0f) {
				dialogue.dialogueFadeInAlpha = 1.0f;
			}
		}
		if (dialogue.tutBGX > 400.0f) {
			dialogue.tutBGX += (400.0f - dialogue.tutBGX) * 3.0f * dt;

			if (dialogue.tutBGX < 400.0f) {
				dialogue.tutBGX = 400.0f;
			}
		}

		// Text typing effect
		dialogue.tutVisibleCharCount += dialogue.tutTypingSpeed * dt;
		if (dialogue.tutVisibleCharCount >= strlen(dialogue.tutFullText)) {
			dialogue.tutVisibleCharCount = strlen(dialogue.tutFullText);
			dialogue.finishedTyping = true;
		}

		// Spacebar Logic
		if (tutSpacePress) {
			if (!dialogue.finishedTyping) {
				dialogue.tutVisibleCharCount = strlen(dialogue.tutFullText);
			}
			else {

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
				DialogueBox_Update(tutStartedBox, dt);
			}
		}
	}

	// -------------------------------- TUTORIAL STAGE: TUTORIAL_START --------------------------------
	
}

bool Doing_Tutorial() { return doTutorial; }
bool Tutorial_Prompt_Answered() { return promptAnswered; }
bool IsTutorialActive() { return tutPopUpShown; }

void Tutorial_Text_Draw(char const* str) {
	f32 winH{ static_cast<f32>(AEGfxGetWindowHeight()) };

	f32 tutTextX{ tutStartedBox.tutBGX + 280.0f }, tutTextY{ tutStartedBox.tutBGY - 260.0f};
	f32 lineHeight{ 100.0f / winH};

	s8 lineNum{};
	const char* lineStart{ str };

	for (int i{};; ++i) {
		if (str[i] == '\n' || str[i] == '\0') {
			std::string line(lineStart, &str[i]); // copy line
			AEGfxPrint(promptFontID, line.c_str(), textPosition(tutTextX, tutTextY).first, textPosition(tutTextX, tutTextY).second - lineHeight * lineNum, 700.0f / winH, 1.0f, 1.0f, 1.0f, tutStartedBox.dialogueFadeInAlpha);
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
		AEGfxPrint(promptFontID, "Do you need a tutorial?", 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, promptAlpha);

		// Draws the buttons
		DrawTextureMesh(yesButtonMesh, yesButtonTexture, -100.0f, -100.0f, buttonW * yesButtonScale, buttonH * yesButtonScale, promptAlpha);
		DrawTextureMesh(noButtonMesh, noButtonTexture, 200.0f, -100.0f, buttonW * noButtonScale, buttonH * noButtonScale, promptAlpha);
	}

	if (tutStarted) {
		//Tutorial_DialogueBox_Draw("const char* str");
		DrawTextureMesh(tutBGMesh, tutBGTexture, tutStartedBox.tutBGX, tutStartedBox.tutBGY, tutStartedBox.tutBGW, tutStartedBox.tutBGH, tutStartedBox.dialogueFadeInAlpha);
		Tutorial_Text_Draw(tutStartedBox.tutFullText);
		
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