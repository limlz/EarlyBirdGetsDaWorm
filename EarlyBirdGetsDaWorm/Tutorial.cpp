#include "pch.hpp"

// Mesh
static AEGfxVertexList* tutPromptMesh;
static AEGfxVertexList* yesButtonMesh;
static AEGfxVertexList* noButtonMesh;

// Textures
static AEGfxTexture* yesButtonTexture;
static AEGfxTexture* noButtonTexture;

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
	promptFontID = AEGfxCreateFont("Assets/Fonts/buggy-font.ttf", 20);
	yesButtonTexture = LoadTextureChecked("Assets/Tutorial/yes-placeholder.jpg");
	noButtonTexture = LoadTextureChecked("Assets/Tutorial/no-placeholder.jpg");
}

void Tutorial_Initialize() {
	tutPromptMesh = CreateSquareMesh(0xFFFFFFFF);
	yesButtonMesh = CreateSquareMesh(0xFFFFFFFF);
	noButtonMesh = CreateSquareMesh(0xFFFFFFFF);

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

	bool yesButtonHover{ IsAreaClicked(-100.0f, -100.0f, 200.0f, 50.0f, Input_GetMouseX(), Input_GetMouseY()) },
		noButtonHover{ IsAreaClicked(200.0f, -100.0f, 200.0f, 50.0f, Input_GetMouseX(), Input_GetMouseY()) };

	f32 hoverTarget{ yesButtonHover ? 1.2f : 1.0f };
	yesButtonScale += (hoverTarget - yesButtonScale) * buttonHoverSpeed * dt;
	hoverTarget = noButtonHover ? 1.2f : 1.0f;
	noButtonScale += (hoverTarget - noButtonScale) * buttonHoverSpeed * dt;

	if (noButtonHover && AEInputCheckCurr(AEVK_LBUTTON)) {
		tutPopUpShown = false;
		promptAnswered = true;
		doTutorial = false;
	}

	if (yesButtonHover && AEInputCheckCurr(AEVK_LBUTTON)) {
		tutPopUpShown = false;
		promptAnswered = true;
		doTutorial = true;
	}

}

bool Doing_Tutorial() { return doTutorial; }
bool Tutorial_Prompt_Answered() { return promptAnswered; }
bool IsTutorialActive() { return tutPopUpShown; }

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
}

void Tutorial_Free() {
	FreeMeshSafe(tutPromptMesh);
	FreeMeshSafe(yesButtonMesh);
	FreeMeshSafe(noButtonMesh);

	UnloadTextureSafe(yesButtonTexture);
	UnloadTextureSafe(noButtonTexture);

	// Destroy font if valid
	if (promptFontID >= 0) {
		AEGfxDestroyFont(promptFontID);
		promptFontID = -1;
	}
}