#pragma once

void Lift_Initialize();
void Lift_Update(float dt, float camX, float maxDist);
void Lift_HandleInput(s8& floorNum);
void Lift_Unload();

void Lift_Draw(AEGfxVertexList* squareMesh);

bool Lift_IsActive();
bool Lift_IsNear();