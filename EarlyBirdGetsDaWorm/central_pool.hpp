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
    GHOST			// evidence matches illness + 1 extra anomaly (impossible for a human patient)
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
    Door_ShadowMove,
    Door_Knock,

    // ---- LIGHTING ----
    Light_Flicker,
    Light_Dim,
    Light_Off,

    // ---- FRAMES ----
    Frame_Glitch,
    Frame_Shift,
    Frame_Distort,

    None,
};

void AllAnomalies_Load();
void AllAnomalies_Initialize();

void AllAnomalies_GenerateRun();
const std::vector<ILLNESSES>& AllAnomalies_CurrentRun();

