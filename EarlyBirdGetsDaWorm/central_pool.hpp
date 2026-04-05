#pragma once

// ============================================================================
// ENUMS & DATA TYPES
// ============================================================================

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
    GHOST,          // evidence matches illness + 1 extra anomaly (impossible for a human patient)
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


// ============================================================================
// SYSTEM LIFECYCLE (LOAD / INIT / UNLOAD)
// ============================================================================

/******************************************************************************************
    @brief  Loads all textures, files, and initial memory for the overarching anomaly systems.
******************************************************************************************/
void AllAnomalies_Load();

/******************************************************************************************
    @brief  Initializes default states and starting parameters for all active anomaly systems.
******************************************************************************************/
void AllAnomalies_Initialize();

/******************************************************************************************
    @brief  Safely clears memory and vector buffers before the program closes.
******************************************************************************************/
void CentralPool_Shutdown();

/******************************************************************************************
    @brief  Unloads all loaded anomaly system assets to prevent memory leaks.
******************************************************************************************/
void AllAnomalies_Unload();


// ============================================================================
// GLOBAL RUN LOGIC
// ============================================================================

/******************************************************************************************
    @brief  Generates the core pool of illnesses that will be used for the current session.
******************************************************************************************/
void AllAnomalies_GenerateRun();

/******************************************************************************************
    @brief  Retrieves the list of active illnesses chosen for the current session.

    @return A constant reference to the vector containing the session's chosen illnesses.
******************************************************************************************/
const std::vector<ILLNESSES>& AllAnomalies_CurrentRun();


// ============================================================================
// PATIENT LEVEL LOGIC
// ============================================================================

/******************************************************************************************
    @brief  Generates a randomized truth table of which specific anomaly systems will activate
            for the current patient.

    @param  illness
            The illness assigned to the current patient.
    @param  ghostExtraAnomalies
            Additional anomaly modifiers applied if the patient is a ghost.
******************************************************************************************/
void CentralPool_GeneratePatientAnomalies(ILLNESSES illness, int ghostExtraAnomalies);

/******************************************************************************************
    @brief  Resets the truth table, disabling all active anomalies after a patient is processed.
******************************************************************************************/
void CentralPool_ClearPatientAnomalies();


// ============================================================================
// ACCESSORS & DEBUG
// ============================================================================

/******************************************************************************************
    @brief  Checks if a specific anomaly module is allowed to trigger logic for the current patient.

    @param  systemFile
            The anomaly module being queried (e.g., Frames, Lift, Sound).
    @return true if the system is flagged as active in the current patient's truth table.
******************************************************************************************/
bool CentralPool_IsSystemActive(AnomalyFiles systemFile);

/******************************************************************************************
    @brief  Forces a specific anomaly system to become active (Used exclusively for debugging).

    @param  systemFile
            The anomaly module to forcibly activate.
******************************************************************************************/
void CentralPool_ForceSystemActive(AnomalyFiles systemFile);

