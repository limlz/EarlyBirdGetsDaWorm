#pragma once
#include <string>

enum EndGameReason : int;

namespace Analytics {
    // Maps an EndGameReason to its canonical CSV string; single source of truth for all call sites
    std::string GetEndReasonName(EndGameReason reason);

    // Call this exactly once right before switching to the endgame/menu screen
    void LogRun(EndGameReason reason, int dayReached, int patientsHandled,
                bool wasCarryingGhost, int currentIllness, int mimicIllness,
                float timeSurvived);
}
