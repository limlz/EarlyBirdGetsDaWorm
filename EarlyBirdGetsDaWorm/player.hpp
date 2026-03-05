#pragma once
#include "central_pool.hpp"   // enum class ILLNESSES (put your enum in one place)

// Forward declare to avoid including graphics headers here
struct AEGfxVertexList;

// ============================================================
// PLAYER MODULE
// ============================================================

// ------------------------------------------------------------
// Core lifecycle
// ------------------------------------------------------------
void Player_Load();
void Player_Unload();
void Player_Update(float dt, bool walkKey);
void Player_Draw(float x, float y);

// ------------------------------------------------------------
// Movement / Facing
// ------------------------------------------------------------
void Player_SetFacing(int dir);

// ------------------------------------------------------------
// Patient / mission logic
// ------------------------------------------------------------
void     Player_GenerateMission();
bool     Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day);

bool     Player_HasPatient();
void     Player_GetTargetRoom(s8& patientFloor, s8& patientDoor, s8& destFloor, s8& destDoor);

// ------------------------------------------------------------
// Illness / ghost state
// ------------------------------------------------------------

// "Truth" illness currently driving anomalies.
// Can be a real illness OR ILLNESSES::GHOST.
ILLNESSES Player_GetCurrentIllness();

// If current illness is GHOST, this is the real illness it mimics.
// If not ghost, you can return Player_GetCurrentIllness().
ILLNESSES Player_GetMimicIllness();

// For your "GHOST = +1 anomaly" rule.
// Suggested: return 1 when ghost, 0 otherwise.
int       Player_GetGhostExtraAnomalies();

// Direct setters (debug / control)
void      Player_SetIllness(ILLNESSES illness);

// This is your “truth scary” flag that you used earlier.
// Keep it if other modules rely on it.
void      Player_SetScary(bool scary);
bool      Player_IsScaryPatient();

// ------------------------------------------------------------
// Random patient/day pool support
// ------------------------------------------------------------
void Player_NewPatientRandom();
void Player_ResetPatientCounter(int day);
void Player_SetScaryByDay(int day);

// ------------------------------------------------------------
// Player size
// ------------------------------------------------------------
float Player_GetWidth();
float Player_GetHeight();