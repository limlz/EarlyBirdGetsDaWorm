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

// Defines the sound event timing and volume for a sound anomaly per illness.
struct SoundTimes {
    float minInterval;  // minimum seconds between sound events
    float maxInterval;  // maximum seconds between sound events
    float volume;       // playback volume (0.0 - 1.0)
    float ghostMult;    // multiplier applied to the interval when patient is a ghost (< 1.0 = more frequent)
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
    extern FlickerTimes   flickerTable[11];
    extern IllnessVisuals visualsTable[11];
    extern float          overlayAlphaTable[11]; // Full-screen darkness overlay per illness
    extern SoundTimes     soundTable[11];        // Sound anomaly timing and volume per illness

    // --- JUMPSCARE SETTINGS ---
    extern float jumpscareDuration;
    extern float jumpscareScreenShake;
    extern float jumpscareImageShake;
    extern float jumpscareFlashDuration;

    // --- CORE FUNCTIONS ---
    void Load();
}