// ---------------------------------------------------------------------------
// includes
#include "pch.hpp"
#include <crtdbg.h> // To check for memory leaks




// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	
	int gGameRunning = 1;

	// Initialization of your own variables go here
	
	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, true, NULL);

	// Changing the window title
	AESysSetWindowTitle("My New Demo!");

	// reset the system modules
	AESysReset();

	printf("Hello World\n");

	// Game Loop

	GSM_Initialize(START_UP);
	AESysSetFullScreen(1);
	while (gGameRunning)
	{
		// Informing the system about the loop'stt start

		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;

		// Your own update logic goes here
		if (current == GS_RESTART) {
			current = previous;
			next = previous;
		}
		else {
			// Update the game state manager and assign function pointers
			GSM_Update();

			// Load the current state's resources
			fpLoad();
		}

		// Initialize the current state
		fpInitialize();

		// --------------------------------------------------------------------
		// Game loop
		// Runs as long as the current state does not change
		// --------------------------------------------------------------------
		while (current == next)
		{
			AESysFrameStart();
			// Handle user input
			Input_Update();
			// Update game logic for the current state
			fpUpdate();

			// Draw the current state
			fpDraw();
			AESysFrameEnd();
		}
		
		// Free resources related to the current state
		fpFree();



		// Unload state resources unless the game is restarting the same state
		if (next != GS_RESTART) {
			fpUnload();
		}

		// Temp Space to Quit
		if (next == GS_QUIT) {
			gGameRunning = 0;
		}

		// Update state tracking variables
		previous = current;
		current = next;
		std::cout << previous << current << next;


		// Informing the system about the loop's end

	}


	// free the system
	AESysExit();
}