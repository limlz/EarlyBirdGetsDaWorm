#pragma once
#include "AEEngine.h" // Or #include "pch.hpp" if that's where AEEngine is

// --- FUNCTIONS ---

// Call this once at the start of the level/game
void Particles_Initialize();

// Call this every frame in Game_Update
void Particles_Update();

// Call this in Game_Draw (Pass your global squareMesh and camera Position)
void Particles_Draw(AEGfxVertexList* mesh, float camX);

// Call this whenever you want an explosion (e.g., when a light breaks)
void Particles_Spawn(float startX, float startY, int count);

// Call this when quitting to clean up memory
void Particles_Free();