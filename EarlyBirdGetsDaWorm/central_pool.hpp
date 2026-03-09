#pragma once

// --- ANONMALY TYPES --- //
enum class ILLNESSES : int
{
    NONE,           // no patient / no evidence yet
    //ALL,			// ghost (matches no illness exactly)
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
    Wall_Crack1,
    Wall_Crack2,
    Wall_Crack3,
    Wall_Crack4,
    Wall_Drawing1,
    Wall_Drawing2,
    Wall_Drawing3,
    Wall_LeftHand,
    Wall_RightHand,

    // ---- DOOR ----
    Door_HandSlam,
    Door_LightChange,
    Door_Knock,

    // ---- LIGHTING ----
    Light_Mania,
    Light_Depression,
    Light_Dementia,
    Light_Schizophrenia,
    Light_AIW,
    Light_Insomnia,
    Light_Scotophobia,

    // ---- FRAMES ----
    Frame_Glitch,
    Frame_Shift,
    Frame_Distort,

    None,
};

enum class AnomalyFiles : int
{
    Frames_isActive,
    Doors_isActive,
    Lighting_isActive,
    Wall_isActive,
	//Sound_isActive,
	//Lift_isActive,
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

