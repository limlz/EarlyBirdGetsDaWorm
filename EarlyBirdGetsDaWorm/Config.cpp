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

    float overlayAlphaTable[11] = {
        /* 0: NONE          */ 0.50f,
        /* 1: PARANOIA      */ 0.65f,
        /* 2: MANIA         */ 0.35f,
        /* 3: DEPRESSION    */ 0.70f,
        /* 4: DEMENTIA      */ 0.55f,
        /* 5: SCHIZOPHRENIA */ 0.60f,
        /* 6: AIW_SYNDROME  */ 0.40f,
        /* 7: INSOMNIA      */ 0.40f,
        /* 8: OCD           */ 0.45f,
        /* 9: SCOTOPHOBIA   */ 0.75f,
        /* 10: GHOST        */ 0.55f,
    };

    // Sound anomaly timing and volume per illness (index matches ILLNESSES enum)
    SoundTimes soundTable[11] = {
        /* 0: NONE          */ {  8.0f, 15.0f, 0.30f, 0.70f },
        /* 1: PARANOIA      */ {  6.0f, 14.0f, 0.40f, 0.60f }, // Hushed whispers, moderately frequent
        /* 2: MANIA         */ {  3.0f,  8.0f, 0.70f, 0.50f }, // Frequent loud screams
        /* 3: DEPRESSION    */ { 10.0f, 20.0f, 0.30f, 0.80f }, // Rare slow muffled knocks
        /* 4: DEMENTIA      */ {  5.0f, 12.0f, 0.50f, 0.60f }, // Mid-frequency disorienting thuds
        /* 5: SCHIZOPHRENIA */ {  4.0f,  9.0f, 0.60f, 0.50f }, // Frequent clashing voice bursts
        /* 6: AIW_SYNDROME  */ {  7.0f, 16.0f, 0.40f, 0.70f }, // Occasional wildly pitched whispers
        /* 7: INSOMNIA      */ {  5.0f, 10.0f, 0.35f, 0.65f }, // Ticking/flicker bursts
        /* 8: OCD           */ {  4.0f,  5.0f, 0.50f, 0.80f }, // Tight interval = metronomic knocking
        /* 9: SCOTOPHOBIA   */ {  8.0f, 18.0f, 0.30f, 0.60f }, // Rare low breathing whispers
        /* 10: GHOST        */ {  6.0f, 12.0f, 0.25f, 1.0f  }, // Ghost-layer volume; ghostMult unused here
    };

    // --- JUMPSCARE SETTINGS ---
    float jumpscareDuration      = 1.0f;
    float jumpscareScreenShake   = 35.0f;
    float jumpscareImageShake    = 18.0f;
    float jumpscareFlashDuration = 0.08f;

    // --- Helper: parse sound anomaly keys for one illness ---
    // Keys: PREFIX_SOUND_MIN, PREFIX_SOUND_MAX, PREFIX_SOUND_VOL, PREFIX_SOUND_GHOST_MULT
    bool ParseSoundIllness(const std::string& key, float val, const std::string& prefix, ILLNESSES illness) {
        int idx = static_cast<int>(illness);
        if (key == prefix + "_SOUND_MIN")        { soundTable[idx].minInterval = val; return true; }
        if (key == prefix + "_SOUND_MAX")        { soundTable[idx].maxInterval = val; return true; }
        if (key == prefix + "_SOUND_VOL")        { soundTable[idx].volume      = val; return true; }
        if (key == prefix + "_SOUND_GHOST_MULT") { soundTable[idx].ghostMult   = val; return true; }
        return false;
    }

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
        if (key == prefix + "_OVERLAY_ALPHA") { overlayAlphaTable[idx] = val; return true; }
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

                // --- JUMPSCARE SETTINGS ---
                else if (key == "JUMPSCARE_DURATION")       jumpscareDuration = val;
                else if (key == "JUMPSCARE_SCREEN_SHAKE")   jumpscareScreenShake = val;
                else if (key == "JUMPSCARE_IMAGE_SHAKE")    jumpscareImageShake = val;
                else if (key == "JUMPSCARE_FLASH_DURATION") jumpscareFlashDuration = val;

                // --- ILLNESS SETTINGS ---
                else if (ParseIllness(key, val, "NONE", ILLNESSES::NONE)) {}
                else if (ParseIllness(key, val, "PARANOIA", ILLNESSES::PARANOIA)) {}
                else if (ParseIllness(key, val, "MANIA", ILLNESSES::MANIA)) {}
                else if (ParseIllness(key, val, "DEPRESSION", ILLNESSES::DEPRESSION)) {}
                else if (ParseIllness(key, val, "DEMENTIA", ILLNESSES::DEMENTIA)) {}
                else if (ParseIllness(key, val, "SCHIZOPHRENIA", ILLNESSES::SCHIZOPHRENIA)) {}
                else if (ParseIllness(key, val, "AIW_SYNDROME", ILLNESSES::AIW_SYNDROME)) {}
                else if (ParseIllness(key, val, "INSOMNIA", ILLNESSES::INSOMNIA)) {}
                else if (ParseIllness(key, val, "OCD", ILLNESSES::OCD)) {}
                else if (ParseIllness(key, val, "SCOTOPHOBIA", ILLNESSES::SCOTOPHOBIA)) {}

                // --- SOUND ANOMALY SETTINGS ---
                else if (ParseSoundIllness(key, val, "NONE", ILLNESSES::NONE)) {}
                else if (ParseSoundIllness(key, val, "PARANOIA", ILLNESSES::PARANOIA)) {}
                else if (ParseSoundIllness(key, val, "MANIA", ILLNESSES::MANIA)) {}
                else if (ParseSoundIllness(key, val, "DEPRESSION", ILLNESSES::DEPRESSION)) {}
                else if (ParseSoundIllness(key, val, "DEMENTIA", ILLNESSES::DEMENTIA)) {}
                else if (ParseSoundIllness(key, val, "SCHIZOPHRENIA", ILLNESSES::SCHIZOPHRENIA)) {}
                else if (ParseSoundIllness(key, val, "AIW_SYNDROME", ILLNESSES::AIW_SYNDROME)) {}
                else if (ParseSoundIllness(key, val, "INSOMNIA", ILLNESSES::INSOMNIA)) {}
                else if (ParseSoundIllness(key, val, "OCD", ILLNESSES::OCD)) {}
                else if (ParseSoundIllness(key, val, "SCOTOPHOBIA", ILLNESSES::SCOTOPHOBIA)) {}
                else if (ParseSoundIllness(key, val, "GHOST", ILLNESSES::GHOST)) {}

            }
            catch (...) {
                std::cout << "[ERROR] Invalid config value for key: " << key << "\n";
            }
        }
        std::cout << "[SUCCESS] Master Config Hot-Reloaded!\n";
    }
}