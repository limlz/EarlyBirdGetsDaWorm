#pragma once

namespace Analytics {
    // Call this exactly once right before switching to the Game Over screen
    void LogRun(int dayReached, const std::string& deathReason, int currentIllness, float timeSurvived);
}