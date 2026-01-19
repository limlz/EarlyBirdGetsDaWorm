#include "pch.hpp"

int current = 0, previous = 0, next = 0;

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
	std::cout << "GSM:Initialize\n";
	// some additional code
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
		// Assign Level 1 state functions
		fpLoad = Startup_Load;
		fpInitialize = Startup_Initialize;
		fpUpdate = Startup_Update;
		fpDraw = Startup_Draw;
		fpFree = Startup_Free;
		fpUnload = Startup_Unload;
		break;

	case MAIN_MENU:
		// Assign Level 2 state functions
		fpLoad = MainMenu_Load;
		fpInitialize = MainMenu_Initialize;
		fpUpdate = MainMenu_Update;
		fpDraw = MainMenu_Draw;
		fpFree = MainMenu_Free;
		fpUnload = MainMenu_Unload;
		break;

	case GAME_STATE:
		fpLoad = Game_Load;
		fpInitialize = Game_Initialize;
		fpUpdate = Game_Update;
		fpDraw = Game_Draw;
		fpFree = Game_Free;
		fpUnload = Game_Unload;
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
