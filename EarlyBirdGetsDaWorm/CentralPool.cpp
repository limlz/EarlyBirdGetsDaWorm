#include "pch.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

// ============================================================
// GLOBALS
// ============================================================

// Illnesses selected for the current run
static std::vector<ILLNESSES> gRunIllnesses;

// Random generator
static std::mt19937 gRng((unsigned)std::time(nullptr));

// Maximum illnesses in a run
static const int MAX_ILLNESSES_PER_RUN = 7;


// ============================================================
// LOAD
// ============================================================

void AllAnomalies_Load()
{
    Wall_Load();
    Frames_Load();
}


// ============================================================
// INITIALIZE
// ============================================================

void AllAnomalies_Initialize()
{
    Frames_Initialize();
    Lighting_Initialize(7);
    Wall_Initialize();
}


// ============================================================
// GENERATE RUN (random illnesses)
// ============================================================

void AllAnomalies_GenerateRun()
{
    gRunIllnesses.clear();

    // Pool of all illnesses
    std::vector<ILLNESSES> illnessPool =
    {
        ILLNESSES::PARANOIA,
        ILLNESSES::MANIA,
        ILLNESSES::DEPRESSION,
        ILLNESSES::DEMENTIA,
        ILLNESSES::SCHIZOPHRENIA,
        ILLNESSES::AIW_SYNDROME,   // Alice in Wonderland Syndrome
        ILLNESSES::INSOMNIA,
        ILLNESSES::OCD,
        ILLNESSES::SCOTOPHOBIA     // Fear of darkness
    };

    // Shuffle pool
    std::shuffle(illnessPool.begin(), illnessPool.end(), gRng);

    // Pick the first 7
    for (int i = 0; i < MAX_ILLNESSES_PER_RUN && i < (int)illnessPool.size(); ++i)
    {
        gRunIllnesses.push_back(illnessPool[i]);
    }
}


// ============================================================
// GET CURRENT RUN
// ============================================================

const std::vector<ILLNESSES>& AllAnomalies_CurrentRun()
{
    if (gRunIllnesses.empty())
    {
        AllAnomalies_GenerateRun();
    }

    return gRunIllnesses;
}