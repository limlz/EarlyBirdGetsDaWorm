#pragma once
struct AEGfxVertexList;

//DEBUG
void Player_SetScary(bool scary);
void Player_SetIllness(ILLNESSES illness);

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
void Player_ResetPatientCounter(int day);
void Player_SetScaryByDay(int day);
ILLNESSES Player_GetCurrentIllness();

// ------------------------------
// RESET PLAYER POSITION AFTER NEW DAY
// ------------------------------
//void Player_ResetToSpawn();

// ------------------------------
//
// ------------------------------
float Player_GetWidth();
float Player_GetHeight();

// Mission States
enum class MissionPhase { PICKUP, DELIVERY };

void Player_GenerateMission();
bool Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day);
bool Player_HasPatient();
void Player_GetTargetRoom(s8& floor, s8& door);