#include "pch.hpp"

// ============================================================
// GLOBALS & CONSTANTS
// ============================================================

// Stores the 6 specific illnesses chosen for the current full game run
static std::vector<ILLNESSES>   currentRunILLNESSES;

// A boolean array acting as a truth table. It maps to the AnomalyFiles enum.
// If index 2 (e.g., Lift_isActive) is true, the elevator is allowed to glitch for this patient.
static std::vector<bool>        currentPatientANOMALYFILES((int)AnomalyFiles::Count, false);

// --- ADVANCED RNG ---
// Mersenne Twister RNG provides a much higher quality, even distribution of randomness 
// compared to standard C rand(), preventing identical runs.
static std::mt19937             gRng((unsigned)std::time(nullptr));

// Game Balance Constants
static const int                MAX_ILLNESSES_PER_RUN = 6;
static const int                MAX_ANOMALIES_PER_ILLNESS = 3;


// ============================================================
// LIFECYCLE MANAGEMENT
// ============================================================

void AllAnomalies_Load()
{
    Wall_Load();
    Frames_Load();
}

void AllAnomalies_Initialize()
{
    Frames_Initialize();
    Lighting_Initialize(7);
    Wall_Initialize();
    SoundAnomaly.Initialize();
}


/******************************* GLOBAL RUN LOGIC ********************************/

/*****************************************************************************************/
// Generates the overarching "Rules" for the playthrough.
// Uses a "Deck Shuffle" algorithm to guarantee exactly 6 unique illnesses are chosen 
// without needing messy do-while collision checks.
/*****************************************************************************************/
void AllAnomalies_GenerateRun()
{
    currentRunILLNESSES.clear();

    // Create the "Deck" of all possible illnesses
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

    // Shuffle the deck perfectly using the Mersenne Twister RNG
    std::shuffle(illnessPool.begin(), illnessPool.end(), gRng);

    // "Deal" the first 6 cards (illnesses) into the active run pool
    for (int i = 0; i < MAX_ILLNESSES_PER_RUN && i < (int)illnessPool.size(); ++i)
    {
        currentRunILLNESSES.push_back(illnessPool[i]);
    }
}

const std::vector<ILLNESSES>& AllAnomalies_CurrentRun()
{
    // Safety Check: Generate a run immediately if one doesn't exist yet
    if (currentRunILLNESSES.empty())
    {
        AllAnomalies_GenerateRun();
    }
    return currentRunILLNESSES;
}


/****************************** PATIENT LEVEL LOGIC ******************************/

/*****************************************************************************************/
// Determines exactly WHICH game systems (Frames, Lift, Audio, etc.) are allowed to 
// actively glitch for the current patient being carried.
/*****************************************************************************************/
void CentralPool_GeneratePatientAnomalies(ILLNESSES illness, int ghostExtraAnomalies)
{
    // Instead of deleting and reallocating arrays, std::fill instantly recycles 
    // the existing vector back to false
    std::fill(currentPatientANOMALYFILES.begin(), currentPatientANOMALYFILES.end(), false);

    if (illness == ILLNESSES::NONE) return;

    // --- DYNAMIC SCALING ---
    // Standard patients get 3 systems. Ghosts mathematically scale up by adding `ghostExtraAnomalies`
    int totalSystemsToActivate = MAX_ANOMALIES_PER_ILLNESS + ghostExtraAnomalies;

    // Safety clamp to prevent out-of-bounds if we ask for more systems than actually exist
    if (totalSystemsToActivate > (int)AnomalyFiles::Count) {
        totalSystemsToActivate = (int)AnomalyFiles::Count;
    }

    // Build a temporary pool of all available system indices (0 through Count)
    std::vector<int> availableSystems;
    for (int i = 0; i < (int)AnomalyFiles::Count; ++i) {
        availableSystems.push_back(i);
    }

    // Deck-Shuffle the available systems
    unsigned seed = (unsigned)std::time(nullptr) + rand();
    std::shuffle(availableSystems.begin(), availableSystems.end(), std::default_random_engine(seed));

    // Activate the first N systems in the shuffled deck
    for (int i = 0; i < totalSystemsToActivate; ++i) {
        int chosenSystem = availableSystems[i];
        currentPatientANOMALYFILES[chosenSystem] = true;
    }
}

void CentralPool_ClearPatientAnomalies()
{
    // Fast boolean wipe
    std::fill(currentPatientANOMALYFILES.begin(), currentPatientANOMALYFILES.end(), false);
}


/*********************************** ACCESSORS ***********************************/

/*****************************************************************************************/
// --- DECOUPLING ARCHITECTURE ---
// Other files (Lift.cpp, Frames.cpp) DO NOT calculate their own RNG or patient logic.
// They simply ping this central function to ask, "Am I allowed to activate my anomalies?"
// This prevents spaghetti code and centralizes all game logic in one place.
/*****************************************************************************************/
bool CentralPool_IsSystemActive(AnomalyFiles systemFile)
{
    return currentPatientANOMALYFILES[static_cast<int>(systemFile)];
}

// Debug only: safely forces a specific anomaly system ON for the current patient during testing
void CentralPool_ForceSystemActive(AnomalyFiles systemFile)
{
    currentPatientANOMALYFILES[static_cast<int>(systemFile)] = true;
}

// Call once at program shutdown to release static vector backing buffers before the CRT leak check runs.
// Static destructors fire after _CrtDumpMemoryLeaks, so without this the vectors produce false positives.
void CentralPool_Shutdown()
{
    { std::vector<ILLNESSES> tmp; currentRunILLNESSES.swap(tmp); }
    { std::vector<bool>      tmp; currentPatientANOMALYFILES.swap(tmp); }
}