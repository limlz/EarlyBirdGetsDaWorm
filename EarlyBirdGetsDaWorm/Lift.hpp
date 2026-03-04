#pragma once

void Lift_DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor);

void Lift_Load();          // Load textures

void Lift_Initialize();    // Create meshes + reset state

void Lift_Unload();        // Free textures + meshes

void Lift_Update(float dt, float camX, float maxDist);

void Lift_HandleInput(s8& floorNum);

// Draw lift UI / door animation overlay
void Lift_DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum,float textXoffset, float textY);

void Lift_Draw(AEGfxVertexList* squareMesh);

bool Lift_IsActive();

bool Lift_IsNear();
