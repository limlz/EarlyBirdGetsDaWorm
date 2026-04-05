#pragma once
#include <string>

enum class EndGameReason : int;

namespace Analytics {

    /******************************************************************************************
        @brief  Maps an EndGameReason enum value to its canonical CSV string representation.

        @param  reason
                The end-game reason to convert.
        @return A human-readable string for use in analytics logs.
    ******************************************************************************************/
    std::string GetEndReasonName(EndGameReason reason);

    /******************************************************************************************
        @brief  Logs a complete game session to the analytics CSV file. Call exactly once
                right before switching to the endgame or menu screen.

        @param  reason
                Why the run ended (e.g., time up, ghost sent to room, boss death).
        @param  dayReached
                The in-game day the player reached (1-5).
        @param  patientsHandled
                Total patients successfully delivered this session.
        @param  wasCarryingGhost
                Whether the player was carrying a ghost patient when the run ended.
        @param  TrueDiagnosis
                The true illness of the last patient (as int from ILLNESSES enum).
        @param  AnomalyIllness
                The anomaly illness displayed to the player (as int from ILLNESSES enum).
        @param  timeSurvived
                Total seconds the player survived during the session.
    ******************************************************************************************/
    void LogRun(EndGameReason reason, int dayReached, int patientsHandled,
                bool wasCarryingGhost, int TrueDiagnosis, int AnomalyIllness,
                float timeSurvived);

    /******************************************************************************************
        @brief  Releases the session-ID string before the CRT leak check runs.
                Call once at program shutdown.
    ******************************************************************************************/
    void Shutdown();
}
