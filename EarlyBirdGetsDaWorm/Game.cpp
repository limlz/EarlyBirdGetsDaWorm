#include "pch.hpp"


static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;
s8 fontId = 0;
f32 playerX{}, playerY{};
f32 playerSpeed{ 1500 };
float doorWidth{ 150.0f }, doorHeight{ 300.0f }, distBetweenDoors{ 600.0f };
float liftWidth{ 200.0f }, liftHeight{ 300.0f };
float doorR{0.54f}, doorG{0.32f}, doorB{0.16f};
int numOfDoors{ 10 }, numOfFloors{ 10 };
float textXoffset{0.06f}, textY{ 50.0f };
int floorNum{}, doorNum{};
bool liftPromptActivated{}, liftActive{};
std::string doorID;




void Game_Load()
{
	// Load resources for the game
	fontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
	std::cout << "Startup: Load\n";
}

void Game_Initialize()
{
	squareMesh = CreateSquareMesh(0xFFFFFFFF);
	circleMesh = CreateCircleMesh(0.5f, 70, 0xFFFFFFFF);
	std::cout << "Startup: Initialize\n";
}

void Game_Update()
{
	if (AEInputCheckCurr(AEVK_SPACE)) {
		next = GS_QUIT;
	}
	if (AEInputCheckCurr(AEVK_A)) {
		playerX += playerSpeed * (f32)AEFrameRateControllerGetFrameTime();
	}
	if (AEInputCheckCurr(AEVK_D)) {
		playerX -= playerSpeed * (f32)AEFrameRateControllerGetFrameTime();
	}
	if (playerX > 0) {
		playerX = 0;
	}
	else if (playerX < -((numOfDoors + 1) * distBetweenDoors)) {
		playerX = -((numOfDoors + 1) * distBetweenDoors); // + 1 for right elevator
	}
	if (playerX > -5 || playerX < -((numOfDoors + 1) * distBetweenDoors) + 5) {
		liftPromptActivated = true;
		if (AEInputCheckTriggered(AEVK_L)) {
			liftActive = !liftActive;
		}
	}
	else {
		liftPromptActivated = false;
		liftActive = false;
	}
}

void Game_Draw()
{
	// Black Background
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
	float halfScreenW = 800.0f;
	float halfScreenH = 450.0f;
	// Draw main menu elements
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEMtx33 scale, translate, transform;
	AEMtx33Scale(&scale, 1600.0f, 500.0f);
	AEMtx33Trans(&translate, 0.0f, 0.0f);
	AEMtx33Concat(&transform, &translate, &scale);

	AEGfxSetColorToMultiply(0.9f, 0.9f, 0.9f, 1.0f);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

	// Draw doors
	for (int i{}; i < numOfDoors; i++) {
		float wallX = distBetweenDoors + playerX + (distBetweenDoors * i);
		if (wallX < distBetweenDoors || wallX > -(distBetweenDoors * 10)) {
			AEMtx33Scale(&scale, doorWidth, doorHeight);
			AEGfxSetColorToMultiply(doorR, doorG, doorB, 1.0f);
			AEMtx33Trans(&translate, wallX, -100.0f);
			AEMtx33Concat(&transform, &translate, &scale);
			AEGfxSetTransform(transform.m);
			AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);


			char textBuffer[32];
			sprintf_s(textBuffer, "%02d-%02d", floorNum + 1, i + 1);
			float textNDC_X = (wallX / halfScreenW) - textXoffset;
			float textNDC_Y = textY / halfScreenH;

			// Pass the calculated variables instead of fixed numbers
			AEGfxPrint(fontId, textBuffer, textNDC_X, textNDC_Y, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);

		}
	}
	
	if (playerX > -(2 * distBetweenDoors)) { // Draw left wall + lift
		AEMtx33Scale(&scale, 800, 900);
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
		AEMtx33Trans(&translate, - 600.0f + playerX, 0.0f);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

		AEMtx33Scale(&scale, liftWidth, liftHeight);
		AEGfxSetColorToMultiply(0.8f, 0.8f, 0.8f, 1.0f);
		AEMtx33Trans(&translate, playerX, -100.0f);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
	}
	else if (playerX < -(8 * distBetweenDoors)) { // Right wall + lift
		AEMtx33Scale(&scale, 800, 900);
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
		AEMtx33Trans(&translate, (12 * distBetweenDoors) + playerX, 0.0f);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

		AEMtx33Scale(&scale, liftWidth, liftHeight);
		AEGfxSetColorToMultiply(0.8f, 0.8f, 0.8f, 1.0f);
		AEMtx33Trans(&translate, (11 * distBetweenDoors) + playerX, -100.0f);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
	}

	if (liftPromptActivated && !liftActive) {
		AEGfxPrint(fontId, "Click L to access lift!", -0.5f, 0.8f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (liftActive) {
		//Background rectangle
		AEMtx33Scale(&scale, 400.0f, 700.0f);
		AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 1.0f);
		AEMtx33Trans(&translate, 0.0f, 0.0f);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		//Lift console
		AEMtx33Scale(&scale, 300.0f, 600.0f);
		AEGfxSetColorToMultiply(0.7f, 0.7f, 0.7f, 1.0f);
		AEMtx33Trans(&translate, 0.0f, 0.0f);
		AEMtx33Concat(&transform, &translate, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		//Buttons?

	}
}

void Game_Free()
{
	// Free main menu resources
	std::cout << "Startup: Free\n";
}


void Game_Unload()
{
	// Unload main menu resources
	std::cout << "Startup: Unload\n";
}