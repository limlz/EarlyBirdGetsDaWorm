#include "pch.hpp"

// ============================================================================
// GLOBALS & CONSTANTS
// ============================================================================

// Stores the 6 specific illnesses chosen for the current full 5-day playthrough.
// This vector is populated once at the start of a run.
static std::vector<ILLNESSES>   currentRunILLNESSES;

// A boolean array acting as a "Truth Table" for the current patient.
// It maps directly to the AnomalyFiles enum (e.g., index 0 = Frames, index 2 = Lift).
// If an index is set to 'true', that specific game system is allowed to glitch.
static std::vector<bool>        currentPatientANOMALYFILES;

// ----------------------------------------------------------------------------
// GAME BALANCE CONSTANTS
// ----------------------------------------------------------------------------
static const int MAX_ILLNESSES_PER_RUN = 6;      // Total unique illnesses per playthrough
static const int MAX_ANOMALIES_PER_ILLNESS = 3;  // Base number of systems allowed to glitch per patient

// ============================================================================
// SYSTEM LIFECYCLE (LOAD / INIT / UNLOAD)
// ============================================================================

void AllAnomalies_Load()
{
    Wall_Load();
    Frames_Load();
    Doors_Load();
    Lift_Load();
}

void AllAnomalies_Initialize()
{
    if (currentPatientANOMALYFILES.size() != static_cast<size_t>(AnomalyFiles::Count)) {
        currentPatientANOMALYFILES.assign(static_cast<int>(AnomalyFiles::Count), false);
    }

    Doors_Initialize();
    Frames_Initialize();
    Lighting_Initialize(7);
    Wall_Initialize();
    Lift_Initialize();
    SoundAnomaly.Initialize();
}

void CentralPool_Shutdown()
{
    // The "Swap to Empty" idiom forces the vector to deallocate its heap memory immediately
    { std::vector<ILLNESSES> tmp; currentRunILLNESSES.swap(tmp); }
    { std::vector<bool>      tmp; currentPatientANOMALYFILES.swap(tmp); }
}

void AllAnomalies_Unload()
{
    Lift_Unload();
    Doors_Unload();
    Frames_Unload();
    Lighting_Unload();
    Wall_Unload();
}

// ============================================================================
// GLOBAL RUN LOGIC
// ============================================================================

// ----------------------------------------------------------------------------
// RUN GENERATION
// Generates the overarching "Rules" for the playthrough.
// Uses a "Deck Shuffle" algorithm to guarantee exactly 6 unique illnesses are chosen 
// without needing messy do-while collision checks or rerolls.
// ----------------------------------------------------------------------------
void AllAnomalies_GenerateRun()
{
    // Wipe any existing run data from a previous session
    currentRunILLNESSES.clear();

    // --- PRESENTATION MODE OVERRIDE ---
    // Forces a specific set of illnesses for the demo so the player sees the best anomalies.
    if (Game_IsPresentationMode()) {
        currentRunILLNESSES.push_back(ILLNESSES::SCOTOPHOBIA);
        currentRunILLNESSES.push_back(ILLNESSES::PARANOIA);
        currentRunILLNESSES.push_back(ILLNESSES::OCD);
        return;
    }

    // 1. Create the "Deck" containing every possible illness in the game
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

    // 2. Shuffle the deck perfectly using the global Mersenne Twister RNG
    std::shuffle(illnessPool.begin(), illnessPool.end(), GetGlobalRng());

    // 3. "Deal" the first N cards (illnesses) into the active run pool
    for (int i = 0; i < MAX_ILLNESSES_PER_RUN && i < static_cast<int>(illnessPool.size()); ++i)
    {
        currentRunILLNESSES.push_back(illnessPool[i]);
    }
}

const std::vector<ILLNESSES>& AllAnomalies_CurrentRun()
{
    // Safety Check: If a script asks for the run data before it has been 
    // explicitly generated, automatically generate one to prevent crashes.
    if (currentRunILLNESSES.empty())
    {
        AllAnomalies_GenerateRun();
    }
    return currentRunILLNESSES;
}

// ============================================================================
// PATIENT LEVEL LOGIC
// ============================================================================

// ----------------------------------------------------------------------------
// ANOMALY DISTRIBUTION
// Determines exactly WHICH game systems (Frames, Lift, Audio, etc.) are allowed to 
// actively glitch for the current patient being carried.
// ----------------------------------------------------------------------------
void CentralPool_GeneratePatientAnomalies(ILLNESSES illness, int ghostExtraAnomalies)
{
    // Safety check: if vector was somehow emptied, resize it before use
    if (currentPatientANOMALYFILES.empty()) {
        currentPatientANOMALYFILES.resize(static_cast<int>(AnomalyFiles::Count), false);
    }

    std::fill(currentPatientANOMALYFILES.begin(), currentPatientANOMALYFILES.end(), false);

    // If the patient is completely normal, abort early leaving all systems false.
    if (illness == ILLNESSES::NONE) return;

    // --- PRESENTATION MODE OVERRIDE ---
    // In demo mode, we force EVERY system to be active so all anomalies trigger simultaneously.
    if (Game_IsPresentationMode()) {
        std::fill(currentPatientANOMALYFILES.begin(), currentPatientANOMALYFILES.end(), true);
        return;
    }

    // --- DYNAMIC SCALING ---
    // Standard human patients get a base amount of anomalies (3). 
    // Ghosts mathematically scale up by adding the `ghostExtraAnomalies` modifier.
    int totalSystemsToActivate = MAX_ANOMALIES_PER_ILLNESS + ghostExtraAnomalies;

    // Safety clamp: Ensure we don't try to activate more systems than actually exist in the engine
    if (totalSystemsToActivate > static_cast<int>(AnomalyFiles::Count)) {
        totalSystemsToActivate = static_cast<int>(AnomalyFiles::Count);
    }

    // Build a temporary pool of all available system indices
    std::vector<int> availableSystems;
    for (int i = 0; i < static_cast<int>(AnomalyFiles::Count); ++i)
    {
        if (static_cast<AnomalyFiles>(i) == AnomalyFiles::Doors_isActive)
        {        
            if (illness == ILLNESSES::MANIA || illness == ILLNESSES::DEPRESSION ||
                illness == ILLNESSES::DEMENTIA || illness == ILLNESSES::AIW_SYNDROME)
            {
                continue;
            }
        }

        availableSystems.push_back(i);
    }

    // Deck-Shuffle the available systems to completely randomize which modules will break
    std::shuffle(availableSystems.begin(), availableSystems.end(), GetGlobalRng());

    // Flip the boolean to 'true' for the first N systems in our freshly shuffled deck
    for (int i = 0; i < totalSystemsToActivate; ++i) {
        int chosenSystem = availableSystems[i];
        currentPatientANOMALYFILES[chosenSystem] = true;
    }
}

void CentralPool_ClearPatientAnomalies()
{
    // Wipes the truth table clean when a patient is dropped off or the mission ends
    if (!currentPatientANOMALYFILES.empty()) {
        std::fill(currentPatientANOMALYFILES.begin(), currentPatientANOMALYFILES.end(), false);
    }
}

// ============================================================================
// ACCESSORS & MEMORY MANAGEMENT
// ============================================================================

bool CentralPool_IsSystemActive(AnomalyFiles systemFile)
{
    // Safety check: Ensure vector is valid and index is within bounds before accessing
    if (currentPatientANOMALYFILES.empty()) return false;

    int index = static_cast<int>(systemFile);
    if (index < 0 || index >= static_cast<int>(currentPatientANOMALYFILES.size())) return false;

    return currentPatientANOMALYFILES[index];
}

// Debug only: safely forces a specific anomaly system ON for the current patient during testing
void CentralPool_ForceSystemActive(AnomalyFiles systemFile)
{
    if (currentPatientANOMALYFILES.empty()) {
        currentPatientANOMALYFILES.resize(static_cast<int>(AnomalyFiles::Count), false);
    }
    currentPatientANOMALYFILES[static_cast<int>(systemFile)] = true;
}