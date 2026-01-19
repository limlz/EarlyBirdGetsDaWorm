#include "pch.hpp"

static AEGfxVertexList* squareMesh;

void MainMenu_Load()
{
	// Load resources for the main menu
	std::cout << "MainMenu: Load\n";
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
	if (AEInputCheckTriggered(AEVK_SPACE))
	{
		std::cout << "Space key pressed in MainMenu\n";
		next = GS_QUIT; // Example: Transition to quit state
		
	}
	if (AEInputCheckTriggered(AEVK_N)) {
		next = GAME_STATE;
	}
}

void MainMenu_Draw()
{
	// Background
	AEGfxSetBackgroundColor(0.2f, 0.0f, 0.0f);

	// Draw main menu elements
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEMtx33 scale, translate, transform;
	AEMtx33Scale(&scale, 300.0f, 60.0f);
	AEMtx33Trans(&translate, 0.0f, 0.0f);
	AEMtx33Concat(&transform, &translate, &scale);

	AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 0.30f);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);


	AEMtx33Scale(&scale, 300.0f, 60.0f);
	AEMtx33Trans(&translate, 0.0f, 100.0f);
	AEMtx33Concat(&transform, &translate, &scale);

	AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 0.30f);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
	//std::cout << "MainMenu: Draw\n";
}

void MainMenu_Free()
{
	// Free main menu resources
	std::cout << "MainMenu: Free\n";
}


void MainMenu_Unload()
{
	// Unload main menu resources
	std::cout << "MainMenu: Unload\n";
}