#include "pch.hpp"

// Mesh
static AEGfxVertexList* iconMesh;
static AEGfxVertexList* pagerMesh;
static AEGfxVertexList* leftArrowMesh;
static AEGfxVertexList* rightArrowMesh;

// Textures
static AEGfxTexture* iconTexture;
static AEGfxTexture* pagerTexture;
static AEGfxTexture* leftArrow;
static AEGfxTexture* rightArrow;

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

void Notifications_Load()
{
	iconTexture = AEGfxTextureLoad("Assets/pager.png");
	pagerTexture = AEGfxTextureLoad("Assets/pager.png");
	leftArrow = AEGfxTextureLoad("Assets/pager-arrow-left.png");
	rightArrow = AEGfxTextureLoad("Assets/pager-arrow-right.png");

	lineID = AEGfxCreateFont("Assets/buggy-font.ttf", 20);

	return;
}

void Notifications_Initialize()
{
	// Pager Positions and Size
	pagerX = AEGfxGetWinMinX() + 100.0f;
	pagerY = AEGfxGetWinMinY() + 750.0f;
	pagerWidth = 100.0f;
	pagerHeight = 63.0f;

	// Misc
	smallClicked = false;
	currentPage = 0;
	mouseX = static_cast<s32>(0.0f);
	mouseY = static_cast <s32>(0.0f);
	prevFloor = -1;

	// Mesh
	iconMesh = CreateSquareMesh(0xFFFFFFFF);
	pagerMesh = CreateSquareMesh(0xFFFFFFFF);
	leftArrowMesh = CreateSquareMesh(0xFFFFFFFF);
	rightArrowMesh = CreateSquareMesh(0xFFFFFFFF);

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

// Converts pager-local pixel offset to normalized screen coordinates for AEGfxPrint
std::pair<f32, f32> textPosition(float adjustX, float adjustY)
{
	f32 textX = (-500.0f + adjustX) / (AEGfxGetWindowWidth() * 0.5f);
	f32 textY = (317.5f + adjustY) / (AEGfxGetWindowWidth() * 0.5f);
	return {textX, textY};
}

void Notifications_Update(bool liftActive)
{
	// Gets the current x and y position of mouse
	AEInputGetCursorPosition(&initialX, &initialY);
	mouseX = static_cast<s32>(initialX - (AEGfxGetWindowWidth() / 2.0f));
	mouseY = static_cast<s32>((AEGfxGetWindowHeight() / 2.0f) - initialY);


	// Toggles the pager when user press Q, pager will not be displayed if lift is active
	if (AEInputCheckTriggered(AEVK_Q) && !liftActive)
	{
		isPagerOpen = !isPagerOpen;
	}

	// Resets popup state when player moves to a new floor
	/*if (floorNum != prevFloor)
	{
		popUp = false;
		prevFloor = floorNum;
	}*/

	//switch (floorNum)
	//{
	//	// If user is in the basement, pop up will display and the
	//	// page with the objective will be displayed. The page where the
	//	// next objective will be displayed so user does not need to toggle
	//	case 5:
	//		if (!popUp)
	//		{
	//			currentPage = 2;
	//			smallClicked = true;
	//			popUp = true;
	//		}
	//		break;
	//	case 3:
	//		if (!popUp)
	//		{
	//			currentPage = 1;
	//			smallClicked = true;
	//			popUp = true;
	//		}
	//		break;
	//	case 7:
	//		if (!popUp)
	//		{
	//			currentPage = 0;
	//			smallClicked = true;
	//			popUp = true;
	//		}
	//		break;
	//}

	// Diplayer pager if Q is pressed
	/*if (smallClicked)
	{
		Notifications_Draw();
	}*/


	return;
}


void Notifications_Draw()
{
	// Draws the icon pager in the corner
	DrawTextureMesh(iconMesh, iconTexture, pagerX, pagerY, pagerWidth, pagerHeight, 1.0f);

	if (isPagerOpen)
	{
		// Draws Pager
		DrawTextureMesh(pagerMesh, pagerTexture, 0.0f, 0.0f, 1000.0f, 635.0f, 1.0f);

		// Text
		char pagetextBuffer[32], textBuffer[32];
		sprintf_s(pagetextBuffer, "%02d/03", currentPage + 1);

		AEGfxPrint(lineID, pagetextBuffer, textPosition(140.0f, -40.0f).first, textPosition(140.0f, -40.0f).second, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		switch (currentPage + 1)
		{
		case 1:
			sprintf_s(textBuffer, "Bring patient to room #03-09");
			break;
		case 2:
			sprintf_s(textBuffer, "Bring patient to room #05-06");
			break;
		case 3:
			sprintf_s(textBuffer, "Bring patient to room #07-03");
			break;
		}
		AEGfxPrint(lineID, textBuffer, textPosition(140.0f, -100.0f).first, textPosition(140.0f, -100.0f).second, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);


		// Arrows
		DrawTextureMesh(leftArrowMesh, leftArrow, -100.0f, -245.0f, 100.0f, 95.0f, 1.0f);
		DrawTextureMesh(rightArrowMesh, rightArrow, 100.0f, -245.0f, 100.0f, 95.0f, 1.0f);


		// Clicking of left arrow
		if (AEInputCheckTriggered(AEVK_LBUTTON) && IsAreaClicked(-100.0f, -245.0f, 100.0f, 95.0f, static_cast<float>(mouseX), static_cast<float>(mouseY)))
		{
			if (currentPage > 0)
			{
				--currentPage;
			}
		}

		// Clicking of Right arrow
		if (AEInputCheckTriggered(AEVK_LBUTTON) && IsAreaClicked(100.0f, -245.0f, 100.0f, 95.0f, static_cast<float>(mouseX), static_cast<float>(mouseY)))
		{
			if (currentPage < 2)
			{
				++currentPage;
			}
		}
	}

}
