#include "pch.hpp"

namespace Analytics {

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

    void LogRun(int dayReached, const std::string& deathReason, int currentIllness, float timeSurvived) {
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

        // Only runs once on first launch — subsequent runs skip this and append directly
        if (needsHeader) {
            outFile << "Date_Time,Day_Reached,Death_Reason,Illness_Active,Time_Survived_Seconds\n";
        }

        // localtime_s is the MSVC-safe alternative to localtime; avoids C4996 deprecation warning
        std::time_t now = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);

        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

        outFile << timeStr << ","
            << dayReached << ","
            << deathReason << ","
            << GetIllnessName(currentIllness) << ","
            << timeSurvived << "\n";

        outFile.close();
        std::cout << "[SUCCESS] Run metrics logged to analytics.csv\n";
    }
}