// ---------------------------------------------------------------------------
// Main.cpp
// ---------------------------------------------------------------------------
#include "pch.hpp"
#include <crtdbg.h> 

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int gGameRunning = 1;

    // Initialize System
    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, true, NULL);
    AESysSetWindowTitle("The Porter: Ward 8");
    AESysReset();

    // Initialize Game State Manager (Start at Menu or Game)
    GSM_Initialize(START_UP);
    AESysSetFullScreen(1);

    // --- SAFE MAIN LOOP ---
    while (gGameRunning)
    {
        // 0. System Check
        if (0 == AESysDoesWindowExist())
            gGameRunning = 0;

        // 1. Update Function Pointers
        // We do this at the start to ensure fpLoad/fpInit match 'current'
        GSM_Update();

        // 2. Load Resources
        fpLoad();

        // 3. Initialize Variables
        fpInitialize();

        // 4. Inner Game Loop (Runs every frame)
        while (current == next)
        {
            AESysFrameStart();

            // Handle Input
            Input_Update();

            // Game Logic
            fpUpdate();

            // Rendering
            fpDraw();

            AESysFrameEnd();
        }

        // 5. Free Session Data (Variables, Vectors, etc.)
        fpFree();

        // 6. Handle State Transitions
        if (next == GS_QUIT) {
            // Unload the current state before quitting
            fpUnload();
            gGameRunning = 0;
            break; // BREAK IMMEDIATELY to stop the loop
        }
        else if (next == GS_RESTART) {
            fpUnload();

            next = current;
        }
        else {
            fpUnload(); // Unload Menu assets
            previous = current;
            current = next; // Switch to Game
        }
    }

    AESysExit();
    return 0;
}