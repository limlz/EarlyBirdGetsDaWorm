#include "pch.hpp"

// Global Variables
static AEGfxVertexList* squareMesh;
static AEGfxVertexList* circleMesh;
static AEGfxTexture* lightingtest = nullptr;
static f32 camX{}, playerY{}, playerX{};

f32 textXoffset{ 0.06f }, textY{ 50.0f };
s8 floorNum{ 1 };           // Current floor the player is on
s8 demonFloorNum{ 0 };      // Floor where the demon is located
s8 demonRoomNum{ 3 };       // Room where the demon is located
s8 doorNumAtPlayer{ -1 };   // Door number the player is currently in front of

bool liftActive{}, left_right{ 1 };
bool dementia{ false };

// Day counter
static int CurrentDay = 1;
static bool gSessionStarted = false;

// player's spawn position
static float gSpawnX = 50.0f;
static float gSpawnY = 0.0f;

static float ComputeSpawnYFromBorder()
{
	float borderCenterY = -650.0f;
	float borderHeight = 800.0f;
	float borderTopY = borderCenterY + (borderHeight * 0.5f);
	return borderTopY + (Player_GetHeight() * 0.5f);
}

void Game_Load()
{
	std::cout << "Startup: Load\n";

	Debug_Load();
	Timer_Load();
	Wall_Load();
	Doors_Load();
	Frames_Load();
	Player_Load();
	Prompts_Load();
	Notifications_Load();

	// Day 1 setup
	if (!gSessionStarted)
	{
		CurrentDay = 1;
		Timer_Reset();
		Timer_StartDayOverlay(CurrentDay);
		camX = 0.0f;
		gSessionStarted = true;
	}
}

void Game_Initialize()
{
	std::cout << "Startup: Initialize\n";

	squareMesh = CreateSquareMesh(COLOR_WHITE);
	circleMesh = CreateCircleMesh(0.5f, 40, COLOR_WHITE);

	Doors_Initialize();
	Frames_Initialize();
	Lighting_Initialize(7);

	// Initialize the Randomized Balancing Pool and Mission
	Player_ResetPatientCounter(CurrentDay);
	Player_GenerateMission();
	Player_SetScaryByDay(CurrentDay);

	Notifications_Initialize();
	Wall_Initialize();
}

void Game_Update()
{
	float dt = (f32)AEFrameRateControllerGetFrameTime();
	Debug_Update();

	// 1) Update overlay + Freeze
	Timer_UpdateDayOverlay(dt);
	if (Timer_IsDayOverlayActive()) { return; }

	// 2) Update timer
	Timer_Update(dt);

	// 3) Day Transition Check
	if (Timer_IsTimeUp())
	{
		if (CurrentDay >= 5)
		{
			next = MAIN_MENU;
			return;
		}

		CurrentDay++;
		Timer_Reset();
		Timer_StartDayOverlay(CurrentDay);

		// Reset player for new day
		camX = 0.0f;
		floorNum = 1;
		liftActive = false;

		// Re-shuffle pools
		Player_ResetPatientCounter(CurrentDay);
		Player_GenerateMission();
		Player_SetScaryByDay(CurrentDay);

		return;
	}

	/************************************ INPUT HANDLING *************************************/
	if (AEInputCheckCurr(AEVK_A)) {
		camX += PLAYER_SPEED * dt;
		left_right = false;
	}
	if (AEInputCheckCurr(AEVK_D)) {
		camX -= PLAYER_SPEED * dt;
		left_right = true;
	}
	if (AEInputCheckTriggered(AEVK_O)) {
		dementia = !dementia;
	}
	if (AEInputCheckCurr(AEVK_M)) {
		camX -= 4000;
		left_right = true;
	}

	if (AEInputCheckTriggered(AEVK_K)) {
		Timer_DebugSetTime(5 * 60 + 58);
	}

	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		next = OTHERS_MENU;
		return;
	}

	/************************************ MOVEMENT & ANIMATION *******************************/
	bool moveRight = AEInputCheckCurr(AEVK_D);
	bool moveLeft = AEInputCheckCurr(AEVK_A);
	bool isWalking = moveRight || moveLeft;

	if (moveRight) Player_SetFacing(1);
	else if (moveLeft) Player_SetFacing(-1);

	Player_Update(dt, isWalking);
	doorNumAtPlayer = Doors_Update(camX);

	// Camera/World Bounds
	float maxDist = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;
	if (camX > 0) camX = 0;
	else if ((camX < -maxDist) && !dementia) camX = -maxDist;

	Lift_Update(dt, camX, maxDist);
	Lift_HandleInput(floorNum);
	Lighting_Update(floorNum, camX, dementia);
	Frames_Update(dt);

	/************************************ INTERACTION HANDLING *******************************/
	if (AEInputCheckTriggered(AEVK_E) && doorNumAtPlayer != -1)
	{
		// 1. Boss Room Special Case
		if (doorNumAtPlayer == demonRoomNum - 1 && floorNum == demonFloorNum)
		{
			Timer_SetPaused(true);
			next = BOSS_FIGHT_STATE;
			return;
		}

		// 2. Handle Pickup/Delivery Logic
		bool success = Player_HandleInteraction(floorNum, (s8)doorNumAtPlayer + 1, CurrentDay);

		if (success)
		{
			// Reset position only if a full delivery just finished
			if (!Player_HasPatient())
			{
				floorNum = 1;
				camX = 0.0f;
				liftActive = false;

				// Roll for the next mission's ghost status
				Player_SetScaryByDay(CurrentDay);
			}
		}
		else
		{
			Prompts_TriggerWrongRoom();
		}
	}

	Notifications_Update(liftActive);
	Prompts_Update(dt, camX, doorNumAtPlayer, Lift_IsActive(), Lift_IsNear());
}

void Game_Draw()
{
	Wall_Draw(camX, floorNum);
	Frames_Draw(floorNum, camX);

	// Background
	if (floorNum >= 1) AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);
	else AEGfxSetBackgroundColor(0.8f, 0.6f, 0.6f);

	// Floor Lines
	DrawSquareMesh(squareMesh, 0.0f, 650.0f, 1600.0f, 800.0f, COLOR_BLACK);
	DrawSquareMesh(squareMesh, 0.0f, -650.0f, 1600.0f, 800.0f, COLOR_BLACK);

	Doors_Draw(camX, floorNum, textXoffset, textY, dementia);

	// Start Lift
	if (camX > -(2 * DIST_BETWEEN_DOORS)) {
		DrawSquareMesh(squareMesh, -600.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
		DrawSquareMesh(squareMesh, camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
		if (floorNum != 0) DrawSquareMesh(squareMesh, -700.0f + camX - 100.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
	}

	// End Lift
	if ((camX < -((NUM_DOORS - 2) * DIST_BETWEEN_DOORS)) && !dementia) {
		float endOffset = (NUM_DOORS + 2) * DIST_BETWEEN_DOORS;
		float liftOffset = (NUM_DOORS + 1) * DIST_BETWEEN_DOORS;
		DrawSquareMesh(squareMesh, endOffset + camX + 100.0f, 0.0f, 800.0f, 900.0f, COLOR_BLACK);
		DrawSquareMesh(squareMesh, liftOffset + camX, -100.0f, LIFT_WIDTH, LIFT_HEIGHT, COLOR_LIFT_GREY);
		if (floorNum != 0) DrawSquareMesh(squareMesh, endOffset + camX + 200.0f, 0.0f, 800.0f, 900.0f, COLOR_NIGHT_BLUE);
	}

	// Player setup
	float playerY = -650.0f + (800.0f * 0.5f) + (Player_GetHeight() * 0.5f);

	// Global Darkness Overlay
	AEMtx33 scale;
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransparency(0.7f);
	AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
	AEMtx33Scale(&scale, 2000.0f, 2000.0f);
	AEGfxSetTransform(scale.m);
	AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

	Player_Draw(50.0f, playerY);
	Draw_and_Flicker(camX, left_right, floorNum, dementia);

	Prompts_Draw();

	// Dynamic UI Notifications
	s8 targetFloor, targetDoor;
	Player_GetTargetRoom(targetFloor, targetDoor);
	Notifications_Draw(targetDoor, targetFloor);

	Timer_Draw(0.0f, 0.85f);
	Timer_DrawDayOverlay(squareMesh);
	Lift_Draw(squareMesh);

	// Debug Overlay
	DebugInfo info;
	info.camX = camX;
	info.day = CurrentDay;
	info.floorNum = floorNum;
	info.doorNumAtPlayer = doorNumAtPlayer;
	info.patientDoorNum = targetDoor;
	info.patientFloorNum = targetFloor;
	Debug_Draw(info);
}

void Game_Free()
{
	std::cout << "Startup: Free\n";
	// Meshes are often freed in Unload, but can be added here if needed
}

void Game_Unload()
{
	Frames_Unload();
	Player_Unload();
	Prompts_Unload();
	Boss_Fight_Unload();
	Lighting_Unload();
	Doors_Unload();
	Debug_Unload();
	Notifications_Free();
	Wall_Unload();

	if (squareMesh) AEGfxMeshFree(squareMesh);
	if (circleMesh) AEGfxMeshFree(circleMesh);

	std::cout << "Startup: Unload\n";
}