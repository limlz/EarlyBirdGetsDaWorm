#include "pch.hpp"

namespace Analytics {

    // Generated once per application launch; format: YYYYMMDD-HHMMSS-XXXX
    // The 4-char hex suffix guards against duplicate IDs on rapid relaunches within the same second.
    static std::string GenerateSessionId() {
        std::time_t now = std::time(nullptr);
        struct tm t;
        localtime_s(&t, &now);
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << (t.tm_year + 1900)
            << std::setw(2) << (t.tm_mon + 1)
            << std::setw(2) << t.tm_mday << "-"
            << std::setw(2) << t.tm_hour
            << std::setw(2) << t.tm_min
            << std::setw(2) << t.tm_sec;
        // Append a 4-digit hex random suffix for uniqueness
        unsigned suffix = static_cast<unsigned>(RandInt(0, 0xFFFE));
        oss << "-" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << suffix;
        return oss.str();
    }

    static std::string gSessionId = GenerateSessionId();

    // Converts illness enum index to its name; default catches any unmapped future values
    static std::string GetIllnessName(int illness) {
        switch (illness) {
        case 0: return "NONE";
        case 1: return "PARANOIA";
        case 2: return "MANIA";
        case 3: return "DEPRESSION";
        case 4: return "DEMENTIA";
        case 5: return "SCHIZOPHRENIA";
        case 6: return "AIW_SYNDROME";
        case 7: return "INSOMNIA";
        case 8: return "OCD";
        case 9: return "SCOTOPHOBIA";
        case 10: return "GHOST";
        default: return "UNKNOWN";
        }
    }

    // Single source of truth for end-reason strings; keeps call sites free of hand-written literals
    std::string GetEndReasonName(EndGameReason reason) {
        switch (reason) {
        case EndGameReason::REASON_SURVIVED_5_DAYS:        return "Victory - Survived 5 Days";
        case EndGameReason::REASON_DIED_TO_BOSS:           return "Death - Killed By Boss";
        case EndGameReason::REASON_SENT_HUMAN_TO_BASEMENT: return "Murder - Wrong Basement Delivery";
        case EndGameReason::REASON_SENT_GHOST_TO_ROOM:     return "Fired - Ghost Sent To Room";
        case EndGameReason::REASON_QUIT_FROM_MENU:         return "Quit From Menu";
        case EndGameReason::REASON_QUOTA_NOT_MET:          return "Fired - Quota Not Met";
        default:                            return "UNKNOWN";
        }
    }

    void LogRun(EndGameReason reason, int dayReached, int patientsHandled,
                bool wasCarryingGhost, int TrueDiagnosis, int AnomalyIllness,
                float timeSurvived) {

        std::string filename = "Assets/analytics.csv";

        // ifstream::good() returns false if the file is missing, so we use that to gate header writing
        std::ifstream checkFile(filename);
        bool needsHeader = !checkFile.good();
        checkFile.close();

        // ios::app moves the write cursor to EOF each time, preserving all previous run data
        std::ofstream outFile(filename, std::ios::app);

        if (!outFile.is_open()) {
            std::cout << "[ERROR] Could not open analytics.csv for writing!\n";
            return;
        }

        // Only runs once on first launch - subsequent runs skip this and append directly
        if (needsHeader) {
            outFile << "Session_ID,Date_Time,Day_Reached,End_Reason,Illness_Active,Mimic_Illness,Was_Carrying_Ghost,Patients_Handled,Time_Survived_Seconds\n";
        }

        // localtime_s is the MSVC-safe alternative to localtime; avoids deprecation warning
        std::time_t now = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);

        std::ostringstream ossTime;
        ossTime << std::setfill('0')
                << std::setw(4) << (timeinfo.tm_year + 1900) << "-"
                << std::setw(2) << (timeinfo.tm_mon + 1) << "-"
                << std::setw(2) << timeinfo.tm_mday << " "
                << std::setw(2) << timeinfo.tm_hour << ":"
                << std::setw(2) << timeinfo.tm_min << ":"
                << std::setw(2) << timeinfo.tm_sec;
        std::string timeStr = ossTime.str();

        outFile << gSessionId << ","
            << timeStr << ","
            << dayReached << ","
            << GetEndReasonName(reason) << ","
            << GetIllnessName(TrueDiagnosis) << ","
            << GetIllnessName(AnomalyIllness) << ","
            << (wasCarryingGhost ? "YES" : "NO") << ","
            << patientsHandled << ","
            << timeSurvived << "\n";

        outFile.close();
        std::cout << "[SUCCESS] Run metrics logged to analytics.csv\n";
    }

    // Call once at shutdown to release the session-ID heap buffer before the CRT leak check runs.
    // static const std::string would have been freed by the static destructor, but that fires
    // after _CrtDumpMemoryLeaks, causing a false-positive in the leak report.
    void Shutdown()
    {
        std::string().swap(gSessionId);
    }
}
