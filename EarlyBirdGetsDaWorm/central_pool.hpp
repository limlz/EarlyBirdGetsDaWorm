#pragma once

// --- ANONMALY TYPES --- //
enum class ILLNESSES : int
{
    NONE,           // no patient / no evidence yet
    PARANOIA,
    MANIA,
    DEPRESSION,
    DEMENTIA,
    SCHIZOPHRENIA,
    AIW_SYNDROME,
    INSOMNIA,
    OCD,
    SCOTOPHOBIA,
    GHOST,			// evidence matches illness + 1 extra anomaly (impossible for a human patient)
    COUNT
};

enum class ANOMALYID : int
{
    // ---- WALL ----
    Wall_Paranoia,
    Wall_Mania,
    Wall_Depression,
    Wall_Dementia,
    Wall_Schizophrenia,
    Wall_AIW,
    Wall_Insomnia,
    Wall_OCD,
    Wall_Scotophobia,

    // ---- DOOR ----
    Door_HandSlam,
    Door_LightChange,
    Door_Position,

    // ---- LIGHTING ----
    Light_Paranoia,
    Light_Mania,
    Light_Depression,
    Light_Dementia,
    Light_Schizophrenia,
    Light_AIW,
    Light_Insomnia,
    Light_OCD,
    Light_Scotophobia,

    // ---- FRAMES ----
	Frame_Shakes,   // paranoia, dementia, schizophrenia, scotophobia
    Frame_Mania,
    Frame_Depression,
	Frame_AIW,
	Frame_Insomnia,
	Frame_OCD,

    // ---- LIFT ----
    Lift_Mania,
    Lift_Depression,
    Lift_Dementia,
    Lift_OCD,
    Lift_AIW,
	Lift_Paranoia,
	Lift_Schizophrenia,
	Lift_Insomnia,
	Lift_Scotophobia,

	// ---- SOUND ----
    Sound_Mania,
    Sound_Depression,
    Sound_Dementia,
    Sound_OCD,
    Sound_AIW,
    Sound_Paranoia,
    Sound_Schizophrenia,
    Sound_Insomnia,
    Sound_Scotophobia,

    None,
};

enum class AnomalyFiles : int
{
    Frames_isActive,
    Doors_isActive,
    Lighting_isActive,
    Wall_isActive,
    Sound_isActive,
    Lift_isActive,
    Count
};

void AllAnomalies_Load();
void AllAnomalies_Initialize();

void AllAnomalies_GenerateRun();
const std::vector<ILLNESSES>& AllAnomalies_CurrentRun();
// Functions to interact with the boolean vector
void CentralPool_GeneratePatientAnomalies(ILLNESSES illness, int ghostExtraAnomalies);
void CentralPool_ClearPatientAnomalies();

// The function other files will call to check their status
bool CentralPool_IsSystemActive(AnomalyFiles systemFile);

// Debug only: forces a specific anomaly system on for the current patient
void CentralPool_ForceSystemActive(AnomalyFiles systemFile);

// Call once at program shutdown to release static vector buffers before the CRT leak check runs
void CentralPool_Shutdown();

