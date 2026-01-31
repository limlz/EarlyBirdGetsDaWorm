#pragma once
struct AEGfxVertexList;

void Player_Load();
void Player_Unload();
void Player_Update(float dt, bool walkKey);
void Player_Draw(float x, float y);
void Player_SetFacing(int dir);

float Player_GetWidth();
float Player_GetHeight();

// NEW:
void Player_NewPatientRandom();     // call when new game / enter room
bool Player_IsScaryPatient();       // optional (if you want UI/logic)
