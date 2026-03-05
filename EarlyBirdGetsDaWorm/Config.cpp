#include "pch.hpp"

namespace Config {
    // Gameplay Defaults
    float playerSpeed = 400.0f;
    float doorDetectionRange = 150.0f;
    float lightFlickerSpeed = 1.0f;

    // Lighting Defaults
    int floorNormalChance = 70;
    int floorFuckedChance = 60;
    float standaloneBrightness = 0.15f;

    // Lighting Tables
    FlickerTimes flickerTable[10] = {
        /* PARANOIA      */ { 10,  50,   5,  25 },
        /* MANIA         */ {  5,  30,   2,  15 },
        /* DEPRESSION    */ {100, 200,  50, 150 },
        /* DEMENTIA      */ { 20, 300,   5,  25 },
        /* SCHIZOPHRENIA */ {  5, 150,   5, 100 },
        /* AIW_SYNDROME  */ { 10, 140,   5,  25 },
        /* INSOMNIA      */ {200, 400,   5,   1 },
        /* OCD           */ {100,   1, 100,   1 },
        /* SCOTOPHOBIA   */ { 10, 140, 100, 200 },
        /* ALL           */ { 10, 140,   5,  25 },
    };

    IllnessVisuals visualsTable[10] = {
        /* PARANOIA      */ { 0.95f, 0.90f, 0.80f, 0.09f, 0.90f },
        /* MANIA         */ { 1.00f, 0.90f, 0.75f, 0.11f, 1.20f },
        /* DEPRESSION    */ { 0.85f, 0.88f, 0.98f, 0.08f, 1.18f },
        /* DEMENTIA      */ { 0.98f, 0.90f, 0.80f, 0.09f, 1.20f },
        /* SCHIZOPHRENIA */ { 0.90f, 0.95f, 0.92f, 0.10f, 1.20f },
        /* AIW_SYNDROME  */ { 0.95f, 0.95f, 1.00f, 0.10f, 1.30f },
        /* INSOMNIA      */ { 0.98f, 0.98f, 1.00f, 0.10f, 1.20f },
        /* OCD           */ { 0.95f, 0.95f, 0.95f, 0.11f, 1.15f },
        /* SCOTOPHOBIA   */ { 0.95f, 0.90f, 0.80f, 0.09f, 0.90f },
        /* ALL           */ { 0.95f, 0.92f, 0.85f, 0.10f, 1.20f }
    };

    // --- NEW: A helper function to parse all illness variants cleanly! ---
    bool ParseIllness(const std::string& key, float val, const std::string& prefix, ILLNESSES illness) {
        if (key == prefix + "_R") { visualsTable[illness].r = val; return true; }
        if (key == prefix + "_G") { visualsTable[illness].g = val; return true; }
        if (key == prefix + "_B") { visualsTable[illness].b = val; return true; }
        if (key == prefix + "_BRIGHTNESS") { visualsTable[illness].baseBrightness = val; return true; }
        if (key == prefix + "_CONE") { visualsTable[illness].baseConeAngle = val; return true; }
        if (key == prefix + "_ON_MIN") { flickerTable[illness].onMin = (int)val; return true; }
        if (key == prefix + "_ON_RANGE") { flickerTable[illness].onRange = (int)val; return true; }
        if (key == prefix + "_OFF_MIN") { flickerTable[illness].offMin = (int)val; return true; }
        if (key == prefix + "_OFF_RANGE") { flickerTable[illness].offRange = (int)val; return true; }
        return false; // Key didn't match this illness
    }

    void Load() {
        std::ifstream file("Assets/config.txt");

        if (!file.is_open()) {
            std::cout << "[WARNING] Could not open Assets/config.txt\n";
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            if (line.empty() || line[0] == '/' || line[0] == '#') continue;

            size_t delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) continue;

            std::string key = line.substr(0, delimiterPos);
            std::string valueStr = line.substr(delimiterPos + 1);

            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            valueStr.erase(0, valueStr.find_first_not_of(" \t"));
            valueStr.erase(valueStr.find_last_not_of(" \t") + 1);

            try {
                float val = std::stof(valueStr);

                // --- GAMEPLAY SETTINGS ---
                if (key == "PLAYER_SPEED") playerSpeed = val;
                else if (key == "DOOR_DETECTION_RANGE") doorDetectionRange = val;
                else if (key == "LIGHT_FLICKER_SPEED") lightFlickerSpeed = val;

                // --- LIGHTING GLOBALS ---
                else if (key == "FLOOR_NORMAL_CHANCE") floorNormalChance = (int)val;
                else if (key == "FLOOR_FUCKED_CHANCE") floorFuckedChance = (int)val;
                else if (key == "STANDALONE_BRIGHTNESS") standaloneBrightness = val;

                // --- ILLNESS SETTINGS (Using the Helper Function!) ---
                else if (ParseIllness(key, val, "PARANOIA", PARANOIA)) {}
                else if (ParseIllness(key, val, "MANIA", MANIA)) {}
                else if (ParseIllness(key, val, "DEPRESSION", DEPRESSION)) {}
                else if (ParseIllness(key, val, "DEMENTIA", DEMENTIA)) {}
                else if (ParseIllness(key, val, "SCHIZOPHRENIA", SCHIZOPHRENIA)) {}
                else if (ParseIllness(key, val, "AIW_SYNDROME", AIW_SYNDROME)) {}
                else if (ParseIllness(key, val, "INSOMNIA", INSOMNIA)) {}
                else if (ParseIllness(key, val, "OCD", OCD)) {}
                else if (ParseIllness(key, val, "SCOTOPHOBIA", SCOTOPHOBIA)) {}
                else if (ParseIllness(key, val, "ALL", ALL)) {}

            }
            catch (...) {
                std::cout << "[ERROR] Invalid config value for key: " << key << "\n";
            }
        }
        std::cout << "[SUCCESS] Master Config Hot-Reloaded!\n";
    }
}