#pragma once

// ============================================================
// LOAD / INITIALIZE / UNLOAD
// ============================================================

void Lift_Load();          // Load textures
void Lift_Initialize();    // Create meshes + reset state
void Lift_Unload();        // Free textures + meshes


// ============================================================
// UPDATE
// ============================================================

void Lift_Update(float dt, float camX, float maxDist);
void Lift_HandleInput(s8& floorNum);


// ============================================================
// DRAW
// ============================================================

// Draw lift in world (replaces grey hallway rectangle)
void Lift_DrawWorld(AEGfxVertexList* squareMesh,float x, float y, float w, float h);

// Draw lift UI / door animation overlay
void Lift_Draw(AEGfxVertexList* squareMesh);


// ============================================================
// STATE GETTERS
// ============================================================

bool Lift_IsActive();
bool Lift_IsNear();
