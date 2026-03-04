#include "pch.hpp"

// Mesh
static AEGfxVertexList* iconMesh;
static AEGfxVertexList* pagerMesh;

// Textures
static AEGfxTexture* iconTexture;
static AEGfxTexture* pagerTexture;

// Variables
static s32 initialX, initialY, mouseX, mouseY;
static f32 pagerX, pagerY, pagerWidth, pagerHeight;
static s8 lineID;
static bool smallClicked;
static bool popUp;
static s8 prevFloor;
static s8 currentPage;
static bool isPagerOpen;		// controls big notif
static bool openPagerAfterDoor;	// request to open pager from room

static bool popUpShown{ false };
static bool prevHadPatient{ false };
static bool taskPopUpShown{ false };
static bool waitTaskPopUp{ false };
static f32 popupTimer{ 0.0f };
static f32 taskTimer{ 0.0f };

// Pager animation
static bool pagerVibrating{ false };
static f32 vibrateTimer{ 0.0f };
static f32 vibrateDuration{ 0.0f };

void Notifications_Load()
{
	iconTexture = LoadTextureChecked(Assets::Pager::Pager);
	pagerTexture = LoadTextureChecked(Assets::Pager::Pager);
	lineID = AEGfxCreateFont(Assets::Fonts::Buggy, 20);

	return;
}

void Notifications_Initialize()
{
	// Pop Up
	popupTimer = 0.0f;
	taskTimer = 0.0f;
	popUpShown = false;
	waitTaskPopUp = false;
	prevHadPatient = false;
	taskPopUpShown = false;

	// Pager Positions and Size
	pagerX = AEGfxGetWinMinX() + 100.0f;
	pagerY = AEGfxGetWinMinY() + 750.0f;
	pagerWidth = 100.0f;
	pagerHeight = 63.0f;

	// Misc
	smallClicked = false;
	pagerVibrating = false;
	vibrateDuration = 0.0f;
	vibrateTimer = 0.0f;

	currentPage = 0;
	mouseX = static_cast<s32>(0.0f);
	mouseY = static_cast <s32>(0.0f);
	prevFloor = -1;

	// Mesh
	iconMesh = CreateSquareMesh(0xFFFFFFFF);
	pagerMesh = CreateSquareMesh(0xFFFFFFFF);

	// Open pager after completing objective
	isPagerOpen = false;
	if (openPagerAfterDoor)
	{
		isPagerOpen = true;			// open big pager immediately
		openPagerAfterDoor = false;	// request handled
	}
}

// To call from boss fight
void Notifications_Trigger()
{
	openPagerAfterDoor = true;   // request open on next state enter
}

void Notifications_Update(bool liftActive, f32 dt)
{
	// Game start delayed pop up
	bool wantTutorial{ Doing_Tutorial() };
	bool answeredTutorial{ Tutorial_Prompt_Answered() };
	if (answeredTutorial && !wantTutorial && !popUpShown) {
		popupTimer += dt;

		if (popupTimer >= 0.5f) {
			isPagerOpen = true;
			pagerVibrating = true;
			vibrateTimer = 0.0f;
			vibrateDuration = 0.5f;
			popUpShown = true;
		}
	}

	// Pop Up after task complete
	bool currHasPatient{ Player_HasPatient() };

	if (prevHadPatient && !currHasPatient) {
		waitTaskPopUp = true;
		taskTimer = 0.0f;
	}
	prevHadPatient = currHasPatient;

	if (waitTaskPopUp) {
		taskTimer += dt;

		if (taskTimer >= 0.5f) {
			isPagerOpen = true;
			pagerVibrating = true;
			vibrateTimer = 0.0f;
			vibrateDuration = 0.5f;
			waitTaskPopUp = false;
		}
	}

	// Animation
	if (pagerVibrating)
	{
		vibrateTimer += dt;

		if (vibrateTimer >= vibrateDuration)
		{
			pagerVibrating = false;
		}
	}

	// Gets the current x and y position of mouse
	AEInputGetCursorPosition(&initialX, &initialY);
	mouseX = static_cast<s32>(initialX - (AEGfxGetWindowWidth() / 2.0f));
	mouseY = static_cast<s32>((AEGfxGetWindowHeight() / 2.0f) - initialY);


	// Toggles the pager when user press Q, pager will not be displayed if lift is active
	if (AEInputCheckTriggered(AEVK_Q) && !liftActive)
	{
		isPagerOpen = !isPagerOpen;
	}
	return;
}


void Notifications_Draw(s8 patientDoorNum, s8 patientFloorNum, s8 desDoorNum, s8 desFloorNum)
{
	// Draws the icon pager in the corner
	DrawTextureMesh(iconMesh, iconTexture, pagerX, pagerY, pagerWidth, pagerHeight, 1.0f);

	if (isPagerOpen)
	{
		f32 drawX{ 0.0f }, drawY{ 0.0f },
			collectTextX{ textPosition(-500.0f + 140.0f, 317.5f -160.0f).first }, collectTextY{ textPosition(-500.0f + 140.0f, 317.5f -160.0f).second },
			bringTextX{ textPosition(-500.0f + 140.0f, 317.5f -220.0f).first }, bringTextY{ textPosition(-500.0f + 140.0f, 317.5f -220.0f).second };

		if (pagerVibrating)
		{
			f32 intensity{ 4.0f },      // how strong pager shakes
				speed{ 100.0f };        // how fast pager shakes

			f32 offsetX{ sinf(vibrateTimer * speed) * intensity },
				offsetY{ cosf(vibrateTimer * speed) * intensity };

			drawX += offsetX;
			drawY += offsetY;

			collectTextX += offsetX;
			collectTextY += offsetY;

			bringTextX += offsetX;
			bringTextY += offsetY;
		}

		// Draws Pager
		DrawTextureMesh(pagerMesh, pagerTexture, drawX, drawY, 1000.0f, 635.0f, 1.0f);

		// Text
		char collecttextBuffer[40], bringBuffer[40];
		sprintf_s(collecttextBuffer, "Collect patient from room #%02d-%02d", patientFloorNum, patientDoorNum);
		sprintf_s(bringBuffer, "Bring patient to room #%02d-%02d", desDoorNum, desFloorNum);
		AEGfxPrint(lineID, collecttextBuffer, collectTextX, collectTextY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxPrint(lineID, bringBuffer, bringTextX, bringTextY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

}

void Notifications_Free()
{
	FreeMeshSafe(iconMesh);
	FreeMeshSafe(pagerMesh);

	UnloadTextureSafe(iconTexture);
	UnloadTextureSafe(pagerTexture);

	// Destroy font if valid
	if (lineID >= 0) {
		AEGfxDestroyFont(lineID);
		lineID = -1;
	}
}