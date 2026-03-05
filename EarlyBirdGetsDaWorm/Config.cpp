#include "pch.hpp"
#include "Config.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>

namespace Config {
    // Gameplay Defaults
    float playerSpeed = 400.0f;
    float doorDetectionRange = 150.0f;
    float lightFlickerSpeed = 1.0f;

    // Lighting Defaults
    int floorNormalChance = 70;
    int floorFuckedChance = 60;
    float standaloneBrightness = 0.15f;

    // Lighting Tables (Size 11 to match your enum class exactly)
    FlickerTimes flickerTable[11] = {
        /* 0: NONE          */ { 10, 140,   5,  25 }, // Safe default
        /* 1: PARANOIA      */ { 10,  50,   5,  25 },
        /* 2: MANIA         */ {  5,  30,   2,  15 },
        /* 3: DEPRESSION    */ {100, 200,  50, 150 },
        /* 4: DEMENTIA      */ { 20, 300,   5,  25 },
        /* 5: SCHIZOPHRENIA */ {  5, 150,   5, 100 },
        /* 6: AIW_SYNDROME  */ { 10, 140,   5,  25 },
        /* 7: INSOMNIA      */ {200, 400,   5,   1 },
        /* 8: OCD           */ {100,   1, 100,   1 },
        /* 9: SCOTOPHOBIA   */ { 10, 140, 100, 200 },
        /* 10: GHOST        */ { 10, 140,   5,  25 }, // Uses mimic logic, but needs a safe placeholder
    };

    IllnessVisuals visualsTable[11] = {
        /* 0: NONE          */ { 0.95f, 0.92f, 0.85f, 0.10f, 1.20f },
        /* 1: PARANOIA      */ { 0.95f, 0.90f, 0.80f, 0.09f, 0.90f },
        /* 2: MANIA         */ { 1.00f, 0.90f, 0.75f, 0.11f, 1.20f },
        /* 3: DEPRESSION    */ { 0.85f, 0.88f, 0.98f, 0.08f, 1.18f },
        /* 4: DEMENTIA      */ { 0.98f, 0.90f, 0.80f, 0.09f, 1.20f },
        /* 5: SCHIZOPHRENIA */ { 0.90f, 0.95f, 0.92f, 0.10f, 1.20f },
        /* 6: AIW_SYNDROME  */ { 0.95f, 0.95f, 1.00f, 0.10f, 1.30f },
        /* 7: INSOMNIA      */ { 0.98f, 0.98f, 1.00f, 0.10f, 1.20f },
        /* 8: OCD           */ { 0.95f, 0.95f, 0.95f, 0.11f, 1.15f },
        /* 9: SCOTOPHOBIA   */ { 0.95f, 0.90f, 0.80f, 0.09f, 0.90f },
        /* 10: GHOST        */ { 0.95f, 0.92f, 0.85f, 0.10f, 1.20f }
    };

    // --- Helper function to parse all illness variants cleanly! ---
    bool ParseIllness(const std::string& key, float val, const std::string& prefix, ILLNESSES illness) {
        int idx = static_cast<int>(illness);
        if (key == prefix + "_R") { visualsTable[idx].r = val; return true; }
        if (key == prefix + "_G") { visualsTable[idx].g = val; return true; }
        if (key == prefix + "_B") { visualsTable[idx].b = val; return true; }
        if (key == prefix + "_BRIGHTNESS") { visualsTable[idx].baseBrightness = val; return true; }
        if (key == prefix + "_CONE") { visualsTable[idx].baseConeAngle = val; return true; }
        if (key == prefix + "_ON_MIN") { flickerTable[idx].onMin = (int)val; return true; }
        if (key == prefix + "_ON_RANGE") { flickerTable[idx].onRange = (int)val; return true; }
        if (key == prefix + "_OFF_MIN") { flickerTable[idx].offMin = (int)val; return true; }
        if (key == prefix + "_OFF_RANGE") { flickerTable[idx].offRange = (int)val; return true; }
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

                // --- ILLNESS SETTINGS ---
                else if (ParseIllness(key, val, "PARANOIA", ILLNESSES::PARANOIA)) {}
                else if (ParseIllness(key, val, "MANIA", ILLNESSES::MANIA)) {}
                else if (ParseIllness(key, val, "DEPRESSION", ILLNESSES::DEPRESSION)) {}
                else if (ParseIllness(key, val, "DEMENTIA", ILLNESSES::DEMENTIA)) {}
                else if (ParseIllness(key, val, "SCHIZOPHRENIA", ILLNESSES::SCHIZOPHRENIA)) {}
                else if (ParseIllness(key, val, "AIW_SYNDROME", ILLNESSES::AIW_SYNDROME)) {}
                else if (ParseIllness(key, val, "INSOMNIA", ILLNESSES::INSOMNIA)) {}
                else if (ParseIllness(key, val, "OCD", ILLNESSES::OCD)) {}
                else if (ParseIllness(key, val, "SCOTOPHOBIA", ILLNESSES::SCOTOPHOBIA)) {}

            }
            catch (...) {
                std::cout << "[ERROR] Invalid config value for key: " << key << "\n";
            }
        }
        std::cout << "[SUCCESS] Master Config Hot-Reloaded!\n";
    }
}