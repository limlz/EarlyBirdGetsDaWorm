#include "pch.hpp"
#include <filesystem>
#include <map>

namespace Config {
    int posterGlitch[11][5] = {};
    static std::filesystem::file_time_type s_lastWriteTime{};
    static std::map<std::string, float> s_prevValues;
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
        /* 9: SCOTOPHOBIA   */ { 300, 500, 500, 1000 },
        /* 10: GHOST        */ { 10, 140,   5,  25 }, // Placeholder
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

    // Frame/poster glitch cycle timing per illness (index matches ILLNESSES enum)
    FrameTimes frameTimesTable[11] = {
        /* 0: NONE          */ { 3.0f, 3.0f, 0.0f }, // Default: 3s glitch, 3s wait, fixed
        /* 1: PARANOIA      */ { 3.0f, 5.0f, 0.0f }, // 3s glitch, always 5s wait
        /* 2: MANIA         */ { 2.0f, 2.0f, 5.0f }, // 2s glitch, 2-7s random wait
        /* 3: DEPRESSION    */ { 4.0f, 6.0f, 4.0f }, // 4s glitch, 6-10s random wait
        /* 4: DEMENTIA      */ { 4.0f, 6.0f, 4.0f }, // 4s glitch, 6-10s random wait
        /* 5: SCHIZOPHRENIA */ { 2.0f, 2.0f, 5.0f }, // 2s glitch, 2-7s random wait
        /* 6: AIW_SYNDROME  */ { 5.0f, 5.0f, 0.0f }, // Fixed 5s cycle both phases
        /* 7: INSOMNIA      */ { 8.0f, 8.0f, 0.0f }, // Fixed 8s cycle both phases
        /* 8: OCD           */ { 6.0f, 6.0f, 0.0f }, // Fixed 6s cycle both phases
        /* 9: SCOTOPHOBIA   */ { 3.0f, 3.0f, 0.0f }, // Default fallback
        /* 10: GHOST        */ { 3.0f, 3.0f, 0.0f }, // Default; ghost modifier applied separately
    };

    // --- FRAME BEHAVIOR CONSTANTS ---
    // Paranoia
    float frameParanoiaCloseRange     = 250.0f;
    float frameParanoiaPanicRange     = 350.0f;
    float frameParanoiaIntensityScale = 10.0f;
    int   frameParanoiaMinIntensity   = 2;
    // Mania
    float frameManiaDriftSpeed   = 30.0f;
    int   frameManiaSutterChance = 30;
    int   frameManiaSutterRange  = 8;
    // Depression
    float frameDepressionStretchDuration   = 4.0f;
    float frameDepressionMaxHeightIncrease = 100.0f;
    float frameDepressionYOffset           = 50.0f;
    // Dementia
    int frameDementiaJitterRange    = 6;
    // Schizophrenia
    int frameSchizophreniJitterRange = 14;
    // AIW
    float frameAIWOscillationFreq = 2.0f;
    float frameAIWOscillationAmp  = 15.0f;
    // Insomnia
    float frameInsomniaFarDist     = 300.0f;
    float frameInsomniaStretchMult = 1.5f;
    // OCD
    float frameOCDOffsetXEven = 14.0f;
    float frameOCDOffsetXOdd  = -11.0f;
    float frameOCDOffsetYEven = -9.0f;
    float frameOCDOffsetYOdd  = 13.0f;
    // Scotophobia
    float frameScotophobiaCloseRange  = 200.0f;
    int   frameScotophobiaJitterRange = 12;
    // Ghost overlay
    int   frameGhostStutterChance   = 15;
    int   frameGhostStutterRange    = 8;
    float frameGhostGlitchSpeedMult = 0.85f;
    float frameGhostWaitSpeedMult   = 0.90f;
    float frameGhostMinTimer        = 0.05f;

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

    // --- Helper: map illness name string to enum index ---
    static int IllnessIndexFromName(const std::string& name) {
        if (name == "NONE")          return 0;
        if (name == "PARANOIA")      return 1;
        if (name == "MANIA")         return 2;
        if (name == "DEPRESSION")    return 3;
        if (name == "DEMENTIA")      return 4;
        if (name == "SCHIZOPHRENIA") return 5;
        if (name == "AIW_SYNDROME")  return 6;
        if (name == "INSOMNIA")      return 7;
        if (name == "OCD")           return 8;
        if (name == "SCOTOPHOBIA")   return 9;
        if (name == "GHOST")         return 10;
        return -1;
    }
    static int PosterIndexFromName(const std::string& name) {
        if (name == "EVERYDAY") return 0;
        if (name == "HOPE")     return 1;
        if (name == "RECOVERY") return 2;
        if (name == "SIGNS")    return 3;
        if (name == "COLOR")    return 4;
        return -1;
    }

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
        if (key == prefix + "_ON_MIN")    { flickerTable[idx].onMin   = static_cast<int>(val); return true; }
        if (key == prefix + "_ON_RANGE")  { flickerTable[idx].onRange  = static_cast<int>(val); return true; }
        if (key == prefix + "_OFF_MIN")   { flickerTable[idx].offMin   = static_cast<int>(val); return true; }
        if (key == prefix + "_OFF_RANGE") { flickerTable[idx].offRange  = static_cast<int>(val); return true; }
        if (key == prefix + "_OVERLAY_ALPHA") { overlayAlphaTable[idx] = val; return true; }
        if (key == prefix + "_FRAME_GLITCH_DUR") { frameTimesTable[idx].glitchDuration = val; return true; }
        if (key == prefix + "_FRAME_WAIT_MIN")   { frameTimesTable[idx].waitMin        = val; return true; }
        if (key == prefix + "_FRAME_WAIT_RAND")  { frameTimesTable[idx].waitRandMax    = val; return true; }
        return false; // Key didn't match this illness
    }

    // --- Illness name-to-enum lookup for DRY parsing ---
    static const struct { const char* name; ILLNESSES illness; } illnessMap[] = {
        {"NONE",          ILLNESSES::NONE},
        {"PARANOIA",      ILLNESSES::PARANOIA},
        {"MANIA",         ILLNESSES::MANIA},
        {"DEPRESSION",    ILLNESSES::DEPRESSION},
        {"DEMENTIA",      ILLNESSES::DEMENTIA},
        {"SCHIZOPHRENIA", ILLNESSES::SCHIZOPHRENIA},
        {"AIW_SYNDROME",  ILLNESSES::AIW_SYNDROME},
        {"INSOMNIA",      ILLNESSES::INSOMNIA},
        {"OCD",           ILLNESSES::OCD},
        {"SCOTOPHOBIA",   ILLNESSES::SCOTOPHOBIA},
        {"GHOST",         ILLNESSES::GHOST},
    };

    // --- Validation helpers ---
    static void ValidateRange(const char* keyName, float value, float lo, float hi) {
        if (value < lo || value > hi) {
            std::cout << "[WARNING] '" << keyName << "' value " << value
                      << " out of expected range [" << lo << ", " << hi << "]\n";
        }
    }

    static void ValidateAll() {
        ValidateRange("PLAYER_SPEED",          playerSpeed,          0.0f, 2000.0f);
        ValidateRange("DOOR_DETECTION_RANGE",  doorDetectionRange,   0.0f, 1000.0f);
        ValidateRange("LIGHT_FLICKER_SPEED",   lightFlickerSpeed,    0.0f, 100.0f);
        ValidateRange("FLOOR_NORMAL_CHANCE",   (float)floorNormalChance, 0.0f, 100.0f);
        ValidateRange("FLOOR_FUCKED_CHANCE",   (float)floorFuckedChance, 0.0f, 100.0f);
        ValidateRange("STANDALONE_BRIGHTNESS", standaloneBrightness, 0.0f, 1.0f);
        ValidateRange("JUMPSCARE_DURATION",       jumpscareDuration,      0.0f, 10.0f);
        ValidateRange("JUMPSCARE_FLASH_DURATION", jumpscareFlashDuration, 0.0f, 5.0f);

        for (int i = 0; i < 11; ++i) {
            ValidateRange("visualsTable.r",         visualsTable[i].r,              0.0f, 1.0f);
            ValidateRange("visualsTable.g",         visualsTable[i].g,              0.0f, 1.0f);
            ValidateRange("visualsTable.b",         visualsTable[i].b,              0.0f, 1.0f);
            ValidateRange("visualsTable.brightness", visualsTable[i].baseBrightness, 0.0f, 1.0f);
            ValidateRange("overlayAlphaTable",       overlayAlphaTable[i],           0.0f, 1.0f);
            ValidateRange("soundTable.volume",       soundTable[i].volume,           0.0f, 1.0f);
            ValidateRange("soundTable.minInterval",  soundTable[i].minInterval,      0.0f, 600.0f);
            ValidateRange("soundTable.maxInterval",  soundTable[i].maxInterval,      0.0f, 600.0f);
            ValidateRange("frameTimesTable.glitchDuration", frameTimesTable[i].glitchDuration, 0.0f, 60.0f);
            ValidateRange("frameTimesTable.waitMin",        frameTimesTable[i].waitMin,        0.0f, 60.0f);
            for (int p = 0; p < 5; ++p) {
                ValidateRange("posterGlitch", (float)posterGlitch[i][p], 0.0f, 4.0f);
            }
        }

        ValidateRange("FRAME_MANIA_STUTTER_CHANCE",     (float)frameManiaSutterChance,      0.0f, 100.0f);
        ValidateRange("FRAME_GHOST_STUTTER_CHANCE",     (float)frameGhostStutterChance,     0.0f, 100.0f);
        ValidateRange("FRAME_DEMENTIA_JITTER_RANGE",    (float)frameDementiaJitterRange,    0.0f, 100.0f);
        ValidateRange("FRAME_SCHIZO_JITTER_RANGE",      (float)frameSchizophreniJitterRange, 0.0f, 100.0f);
        ValidateRange("FRAME_SCOTOPHOBIA_JITTER_RANGE", (float)frameScotophobiaJitterRange, 0.0f, 100.0f);
        ValidateRange("FRAME_GHOST_GLITCH_SPEED_MULT",  frameGhostGlitchSpeedMult,         0.0f, 10.0f);
        ValidateRange("FRAME_GHOST_WAIT_SPEED_MULT",    frameGhostWaitSpeedMult,            0.0f, 10.0f);
        ValidateRange("FRAME_GHOST_MIN_TIMER",          frameGhostMinTimer,                 0.0f, 10.0f);
    }

    void Load() {
        std::memset(posterGlitch, 0, sizeof(posterGlitch));
        std::ifstream file("Assets/config.txt");

        if (!file.is_open()) {
            std::cout << "[WARNING] Could not open Assets/config.txt\n";
            return;
        }

        std::map<std::string, float> newValues;
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
                newValues[key] = val;

                // --- GAMEPLAY SETTINGS ---
                if (key == "PLAYER_SPEED") playerSpeed = val;
                else if (key == "DOOR_DETECTION_RANGE") doorDetectionRange = val;
                else if (key == "LIGHT_FLICKER_SPEED") lightFlickerSpeed = val;

                // --- LIGHTING GLOBALS ---
                else if (key == "FLOOR_NORMAL_CHANCE") floorNormalChance = static_cast<int>(val);
                else if (key == "FLOOR_FUCKED_CHANCE") floorFuckedChance = static_cast<int>(val);
                else if (key == "STANDALONE_BRIGHTNESS") standaloneBrightness = val;

                // --- FRAME BEHAVIOR CONSTANTS ---
                else if (key == "FRAME_PARANOIA_CLOSE_RANGE")       frameParanoiaCloseRange     = val;
                else if (key == "FRAME_PARANOIA_PANIC_RANGE")        frameParanoiaPanicRange     = val;
                else if (key == "FRAME_PARANOIA_INTENSITY_SCALE")    frameParanoiaIntensityScale = val;
                else if (key == "FRAME_PARANOIA_MIN_INTENSITY")      frameParanoiaMinIntensity   = static_cast<int>(val);
                else if (key == "FRAME_MANIA_DRIFT_SPEED")           frameManiaDriftSpeed        = val;
                else if (key == "FRAME_MANIA_STUTTER_CHANCE")        frameManiaSutterChance      = static_cast<int>(val);
                else if (key == "FRAME_MANIA_STUTTER_RANGE")         frameManiaSutterRange       = static_cast<int>(val);
                else if (key == "FRAME_DEPRESSION_STRETCH_DURATION") frameDepressionStretchDuration   = val;
                else if (key == "FRAME_DEPRESSION_MAX_HEIGHT")       frameDepressionMaxHeightIncrease = val;
                else if (key == "FRAME_DEPRESSION_Y_OFFSET")         frameDepressionYOffset           = val;
                else if (key == "FRAME_DEMENTIA_JITTER_RANGE")       frameDementiaJitterRange         = static_cast<int>(val);
                else if (key == "FRAME_SCHIZO_JITTER_RANGE")         frameSchizophreniJitterRange     = static_cast<int>(val);
                else if (key == "FRAME_AIW_OSCILLATION_FREQ")        frameAIWOscillationFreq          = val;
                else if (key == "FRAME_AIW_OSCILLATION_AMP")         frameAIWOscillationAmp           = val;
                else if (key == "FRAME_INSOMNIA_FAR_DIST")           frameInsomniaFarDist             = val;
                else if (key == "FRAME_INSOMNIA_STRETCH_MULT")       frameInsomniaStretchMult         = val;
                else if (key == "FRAME_OCD_OFFSET_X_EVEN")           frameOCDOffsetXEven              = val;
                else if (key == "FRAME_OCD_OFFSET_X_ODD")            frameOCDOffsetXOdd               = val;
                else if (key == "FRAME_OCD_OFFSET_Y_EVEN")           frameOCDOffsetYEven              = val;
                else if (key == "FRAME_OCD_OFFSET_Y_ODD")            frameOCDOffsetYOdd               = val;
                else if (key == "FRAME_SCOTOPHOBIA_CLOSE_RANGE")     frameScotophobiaCloseRange       = val;
                else if (key == "FRAME_SCOTOPHOBIA_JITTER_RANGE")    frameScotophobiaJitterRange      = static_cast<int>(val);
                else if (key == "FRAME_GHOST_STUTTER_CHANCE")        frameGhostStutterChance          = static_cast<int>(val);
                else if (key == "FRAME_GHOST_STUTTER_RANGE")         frameGhostStutterRange           = static_cast<int>(val);
                else if (key == "FRAME_GHOST_GLITCH_SPEED_MULT")     frameGhostGlitchSpeedMult        = val;
                else if (key == "FRAME_GHOST_WAIT_SPEED_MULT")       frameGhostWaitSpeedMult          = val;
                else if (key == "FRAME_GHOST_MIN_TIMER")             frameGhostMinTimer               = val;

                // --- JUMPSCARE SETTINGS ---
                else if (key == "JUMPSCARE_DURATION")       jumpscareDuration = val;
                else if (key == "JUMPSCARE_SCREEN_SHAKE")   jumpscareScreenShake = val;
                else if (key == "JUMPSCARE_IMAGE_SHAKE")    jumpscareImageShake = val;
                else if (key == "JUMPSCARE_FLASH_DURATION") jumpscareFlashDuration = val;

                // --- ILLNESS + SOUND + POSTER GLITCH (loop over all illnesses) ---
                else {
                    bool matched = false;

                    // ALL_ prefix: apply to every illness as a baseline
                    if (key.rfind("ALL_", 0) == 0) {
                        for (const auto& [name, ill] : illnessMap) {
                            if (ParseIllness(key, val, "ALL", ill)) { matched = true; }
                        }
                    }

                    if (!matched) {
                        for (const auto& [name, ill] : illnessMap) {
                            if (ParseIllness(key, val, name, ill))      { matched = true; break; }
                            if (ParseSoundIllness(key, val, name, ill)) { matched = true; break; }
                        }
                    }
                    if (!matched && key.rfind("POSTER_GLITCH_", 0) == 0) {
                        std::string rest = key.substr(14);
                        size_t split = rest.rfind('_');
                        if (split != std::string::npos) {
                            int illIdx    = IllnessIndexFromName(rest.substr(0, split));
                            int posterIdx = PosterIndexFromName(rest.substr(split + 1));
                            if (illIdx >= 0 && posterIdx >= 0)
                                posterGlitch[illIdx][posterIdx] = static_cast<int>(val);
                            else
                                std::cout << "[WARNING] Unknown poster glitch key: " << key << "\n";
                        }
                        matched = true;
                    }
                    if (!matched) {
                        std::cout << "[WARNING] Unrecognized config key: " << key << "\n";
                    }
                }

            }
            catch (...) {
                std::cout << "[ERROR] Invalid config value for key: " << key << "\n";
            }
        }

        // --- Print changed values on reload ---
        if (!s_prevValues.empty()) {
            bool anyChange = false;
            for (const auto& [k, v] : newValues) {
                auto it = s_prevValues.find(k);
                if (it == s_prevValues.end()) {
                    std::cout << std::flush << "[CONFIG] NEW  " << k << " = " << v << "\n";
                    anyChange = true;
                } else if (std::abs(it->second - v) > 0.0001f) {
                    std::cout << std::flush << "[CONFIG] " << k << ": " << it->second << " -> " << v << "\n";
                    anyChange = true;
                }
            }
            for (const auto& [k, v] : s_prevValues) {
                if (newValues.find(k) == newValues.end()) {
                    std::cout << std::flush << "[CONFIG] REMOVED " << k << " (was " << v << ")\n";
                    anyChange = true;
                }
            }
            if (!anyChange) {
                std::cout << std::flush << "[CONFIG] File touched but no values changed.\n";
            }
        }
        s_prevValues = std::move(newValues);

        // --- Validate all ranges ---
        ValidateAll();

        std::cout << "[SUCCESS] Master Config Hot-Reloaded!" << std::endl;
    }

    void PollAndReload() {
        namespace fs = std::filesystem;
        static const fs::path configPath = "Assets/config.txt";
        std::error_code ec;
        auto currentTime = fs::last_write_time(configPath, ec);
        if (ec) return;   // file temporarily locked during save — retry next frame
        if (currentTime != s_lastWriteTime) {
            s_lastWriteTime = currentTime;
            Load();
        }
    }
}