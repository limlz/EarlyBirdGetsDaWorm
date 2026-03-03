#pragma once

void PauseMenu_Load();     // <-- ADD THIS
void PauseMenu_Initialize();
void PauseMenu_Update(float dt);
void PauseMenu_Draw(AEGfxVertexList* squareMesh);
void PauseMenu_Unload();   // <-- ADD THIS

bool PauseMenu_IsPaused();
void PauseMenu_SetPaused(bool state);