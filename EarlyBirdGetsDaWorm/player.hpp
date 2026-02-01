#pragma once
struct AEGfxVertexList;

void Player_Load();
void Player_Unload();
void Player_Update(float dt, bool walkKey);
void Player_Draw(float x, float y);

void Player_SetFacing(int dir);

// ------------------------------
// PLAYER RANDOM
// ------------------------------
void Player_NewPatientRandom();     
bool Player_IsScaryPatient();       

// ------------------------------
// RESET PLAYER POSITION AFTER NEW DAY
// ------------------------------
//void Player_ResetToSpawn();

// ------------------------------
//
// ------------------------------
float Player_GetWidth();
float Player_GetHeight();
