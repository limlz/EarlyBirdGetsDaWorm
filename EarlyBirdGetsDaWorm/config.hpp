#pragma once
#include "pch.hpp"
#include <string>

// Defines how fast a light flickers for a given illness.
struct FlickerTimes {
    int onMin, onRange;
    int offMin, offRange;
};

// Defines the foundational visual look of an illness.
struct IllnessVisuals {
    float r, g, b;
    float baseBrightness;
    float baseConeAngle;
};

namespace Config {
    // --- GAMEPLAY SETTINGS ---
    extern float playerSpeed;
    extern float doorDetectionRange;
    extern float lightFlickerSpeed;

    // --- LIGHTING GLOBALS ---
    extern int floorNormalChance;
    extern int floorFuckedChance;
    extern float standaloneBrightness;

    // --- ILLNESS TABLES ---
    // Size 11 covers exactly: NONE (0) through GHOST (10)
    extern FlickerTimes flickerTable[11];
    extern IllnessVisuals visualsTable[11];

    // --- CORE FUNCTIONS ---
    void Load();
}