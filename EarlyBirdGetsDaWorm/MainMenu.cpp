#include "pch.hpp"

static AEGfxVertexList* squareMesh;
s8 menuFontId = 0;
s32 mouseX, mouseY;
f32 worldMouseX, worldMouseY;

void MainMenu_Load()
{
	// Load resources for the main menu
	std::cout << "MainMenu: Load\n";
	menuFontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
}

void MainMenu_Initialize()
{
	// Initialize main menu variables
	int count = 3;
	squareMesh = CreateSquareMesh(0xFFFFFFFF);
	std::cout << "MainMenu: Initialize\n";
}

void MainMenu_Update()
{
	AEInputGetCursorPosition(&mouseX, &mouseY);
	mouseX = mouseX - 800 ; // Convert to NDC
	mouseY = 450 - mouseY; // Convert to NDC
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		std::cout << "Space key pressed in MainMenu\n";
		next = GS_QUIT; // Example: Transition to quit state
		
	}
	if (IsAreaClicked(0.0f, 100.0f, 300.0f, 60.0f, mouseX, mouseY) && AEInputCheckTriggered(AEVK_LBUTTON)) {
		next = GAME_STATE;
	}
	if (IsAreaClicked(0.0f, 0.0f, 300.0f, 60.0f, mouseX, mouseY) && AEInputCheckTriggered(AEVK_LBUTTON)) {
		next = GS_QUIT;
	}
}

void MainMenu_Draw()
{
	// Background
	AEGfxSetBackgroundColor(0.2f, 0.0f, 0.0f);
	DrawSquareMesh(squareMesh, 0.0f, 100.0f, 300.0f, 60.0f, COLOR_WHITE);
	DrawSquareMesh(squareMesh, 0.0f, 0.0f, 300.0f, 60.0f, COLOR_WHITE);

	// Passing a std::string variable
	std::string titleMsg = "echoes of the ward";
	AEGfxPrintCentered(menuFontId, titleMsg, 0.0f, 0.5f, 1.2f, 1.0f, 0.0f, 0.0f, 1.0f);
	
	std::string myMsg = "START";
	std::cout << mouseX << " " << mouseY << "\n";
	AEGfxPrintCentered(menuFontId, myMsg, 0.0f, 0.2f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	myMsg = "QUIT";
	AEGfxPrintCentered(menuFontId, myMsg, 0.0f, -0.03f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);


	//std::cout << "MainMenu: Draw\n";
}

void MainMenu_Free()
{
	// Free main menu resources
	std::cout << "MainMenu: Free\n";
}


void MainMenu_Unload()
{
    if (squareMesh) { AEGfxMeshFree(squareMesh); squareMesh = nullptr; }

    if (menuFontId >= 0) {
        AEGfxDestroyFont(menuFontId);
        menuFontId = -1;
    }

    std::cout << "MainMenu: Unload\n";
}