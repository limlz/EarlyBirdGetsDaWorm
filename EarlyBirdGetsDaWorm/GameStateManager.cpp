#include "pch.hpp"

int current = 0, previous = 0, next = 0;
bool gameStateSuspendedForBoss = false;
bool resumeGameStateFromBoss = false;

// -----------------------------------------------------------------------------
// Function pointers used to call the appropriate state functions
// These pointers are updated based on the active game state
// -----------------------------------------------------------------------------
FP fpLoad = nullptr, fpInitialize = nullptr, fpUpdate = nullptr, fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;

// -----------------------------------------------------------------------------
// Initializes the Game State Manager
// Sets the current, previous, and next states to the starting state
// This function should be called once at the start of the program
// -----------------------------------------------------------------------------
void GSM_Initialize(int startingState)
{
	current = previous = next = startingState;
	gameStateSuspendedForBoss = false;
	resumeGameStateFromBoss = false;
	std::cout << "GSM:Initialize\n";
	// some additional code
}

bool GSM_ShouldPreserveCurrentStateOnExit()
{
	return current == GAME_STATE && next == BOSS_FIGHT_STATE;
}

bool GSM_ShouldRunLoadAndInitialize()
{
	if (resumeGameStateFromBoss && current == GAME_STATE && previous == BOSS_FIGHT_STATE) {
		resumeGameStateFromBoss = false;
		return false;
	}

	return true;
}

void GSM_OnStateTransition()
{
	if (previous == GAME_STATE && current == BOSS_FIGHT_STATE) {
		gameStateSuspendedForBoss = true;
	}
	else if (previous == BOSS_FIGHT_STATE && current == GAME_STATE) {
		if (gameStateSuspendedForBoss) {
			resumeGameStateFromBoss = true;
		}
		gameStateSuspendedForBoss = false;
	}
	else if (current != GAME_STATE) {
		gameStateSuspendedForBoss = false;
		resumeGameStateFromBoss = false;
	}
}

// -----------------------------------------------------------------------------
// Updates the Game State Manager
// Assigns function pointers based on the next game state
// This function is responsible for state transitions
// -----------------------------------------------------------------------------
void GSM_Update()
{
	// Determine which state functions to use based on the next state
	switch (next)
	{
	case START_UP:
		// Assign Start Up state functions
		fpLoad = Startup_Load;
		fpInitialize = Startup_Initialize;
		fpUpdate = Startup_Update;
		fpDraw = Startup_Draw;
		fpFree = Startup_Free;
		fpUnload = Startup_Unload;
		break;

	case MAIN_MENU:
		// Assign Main Menu state functions
		fpLoad = MainMenu_Load;
		fpInitialize = MainMenu_Initialize;
		fpUpdate = MainMenu_Update;
		fpDraw = MainMenu_Draw;
		fpFree = MainMenu_Free;
		fpUnload = MainMenu_Unload;
		break;

	case OTHERS_MENU:
		// Assign Others Menu state functions
		fpLoad = OthersMenu_Load;
		fpInitialize = OthersMenu_Initialize;
		fpUpdate = OthersMenu_Update;
		fpDraw = OthersMenu_Draw;
		fpFree = OthersMenu_Free;
		fpUnload = OthersMenu_Unload;
		break;

	case GAME_STATE:
		// Assign Main Game State functions
		fpLoad = Game_Load;
		fpInitialize = Game_Initialize;
		fpUpdate = Game_Update;
		fpDraw = Game_Draw;
		fpFree = Game_Free;
		fpUnload = Game_Unload;
		break;

	case BOSS_FIGHT_STATE:
		// Assign Main Game State functions
		fpLoad = Boss_Fight_Load;
		fpInitialize = Boss_Fight_Initialize;
		fpUpdate = Boss_Fight_Update;
		fpDraw = Boss_Fight_Draw;
		fpFree = Boss_Fight_Free;
		fpUnload = Boss_Fight_Unload;
		break;

	case GS_RESTART:
		// Restart logic is handled in the main game loop
		break;

	case GS_QUIT:
		fpLoad = Quit_Load;
		fpInitialize = Quit_Initialize;
		fpUpdate = Quit_Update;
		fpDraw = Quit_Draw;
		fpFree = Quit_Free;
		fpUnload = Quit_Unload;
		break;

	default:
		// Invalid state, no action taken
		break;
	}

	// Print update confirmation for the Game State Manager
	std::cout << "GSM:Update\n";
}
