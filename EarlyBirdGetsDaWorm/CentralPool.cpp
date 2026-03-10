#include "pch.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

// ============================================================
// GLOBALS
// ============================================================

// Illnesses selected for the current run
static std::vector<ILLNESSES>   currentRunILLNESSES;

static std::vector<bool>        currentPatientANOMALYFILES((int)AnomalyFiles::Count, false);

// Random generator
static std::mt19937             gRng((unsigned)std::time(nullptr));

// Maximum illnesses in a run
static const int                MAX_ILLNESSES_PER_RUN = 6;

static const int				MAX_ANOMALIES_PER_ILLNESS = 3;


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
    SoundAnomaly_Initialize();
}


// ============================================================
// GENERATE RUN (random illnesses)
// ============================================================

void AllAnomalies_GenerateRun()
{
    currentRunILLNESSES.clear();

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
        currentRunILLNESSES.push_back(illnessPool[i]);
    }
}


// ============================================================
// GET CURRENT RUN
// ============================================================

const std::vector<ILLNESSES>& AllAnomalies_CurrentRun()
{
    if (currentRunILLNESSES.empty())
    {
        AllAnomalies_GenerateRun();
    }

    return currentRunILLNESSES;
}

void CentralPool_GeneratePatientAnomalies(ILLNESSES illness, int ghostExtraAnomalies)
{
    std::fill(currentPatientANOMALYFILES.begin(), currentPatientANOMALYFILES.end(), false);

    if (illness == ILLNESSES::NONE) return;

    int totalSystemsToActivate = MAX_ANOMALIES_PER_ILLNESS + ghostExtraAnomalies;

    if (totalSystemsToActivate > (int)AnomalyFiles::Count) {
        totalSystemsToActivate = (int)AnomalyFiles::Count;
    }

    std::vector<int> availableSystems;
    for (int i = 0; i < (int)AnomalyFiles::Count; ++i) {
        availableSystems.push_back(i);
    }

    // Shuffle the systems randomly
    unsigned seed = (unsigned)std::time(nullptr) + rand();
    std::shuffle(availableSystems.begin(), availableSystems.end(), std::default_random_engine(seed));

    for (int i = 0; i < totalSystemsToActivate; ++i) {
        int chosenSystem = availableSystems[i];
        currentPatientANOMALYFILES[chosenSystem] = true;
    }
}
void CentralPool_ClearPatientAnomalies()
{
    std::fill(currentPatientANOMALYFILES.begin(), currentPatientANOMALYFILES.end(), false);
}

// The function other files will call to check their status
bool CentralPool_IsSystemActive(AnomalyFiles systemFile)
{
    return currentPatientANOMALYFILES[static_cast<int>(systemFile)];
}