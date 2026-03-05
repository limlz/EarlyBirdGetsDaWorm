#include "pch.hpp"

namespace Analytics {

    // Helper to convert the Illness enum into readable text for Excel
    static std::string GetIllnessName(int illness) {
        switch (illness) {
        case 0: return "PARANOIA";
        case 1: return "MANIA";
        case 2: return "DEPRESSION";
        case 3: return "DEMENTIA";
        case 4: return "SCHIZOPHRENIA";
        case 5: return "AIW_SYNDROME";
        case 6: return "INSOMNIA";
        case 7: return "OCD";
        case 8: return "SCOTOPHOBIA";
        default: return "UNKNOWN";
        }
    }

    void LogRun(int dayReached, const std::string& deathReason, int currentIllness, float timeSurvived) {
        std::string filename = "Assets/analytics.csv";

        // 1. Check if the file already exists
        // If it doesn't exist, we know we need to write the Header Row first.
        std::ifstream checkFile(filename);
        bool needsHeader = !checkFile.good();
        checkFile.close();

        // 2. Open the file in Append Mode (std::ios::app)
        // This ensures we add to the bottom of the file instead of wiping it!
        std::ofstream outFile(filename, std::ios::app);

        if (!outFile.is_open()) {
            std::cout << "[ERROR] Could not open analytics.csv for writing!\n";
            return;
        }

        // 3. Write the Header Row if this is a brand new file
        if (needsHeader) {
            outFile << "Date_Time,Day_Reached,Death_Reason,Illness_Active,Time_Survived_Seconds\n";
        }

        // 4. Generate a real-world timestamp safely for Visual Studio
        std::time_t now = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now); // MSVC safe version (2 arguments, no std::)

        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

        // 5. Write the Data Row (Values separated by commas)
        outFile << timeStr << ","
            << dayReached << ","
            << deathReason << ","
            << GetIllnessName(currentIllness) << ","
            << timeSurvived << "\n";

        outFile.close();
        std::cout << "[SUCCESS] Run metrics logged to analytics.csv\n";
    }
}