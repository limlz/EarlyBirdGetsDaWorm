#pragma once
#include "pch.hpp"

// Define the structs here so other files can see what they look like
struct FlickerTimes {
    int onMin, onRange;
    int offMin, offRange;
};

struct IllnessVisuals {
    float r, g, b;
    float baseBrightness;
    float baseConeAngle;
};

namespace Config {
    // Gameplay Globals
    extern float playerSpeed;
    extern float doorDetectionRange;
    extern float lightFlickerSpeed;

    // Lighting Globals
    extern int floorNormalChance;
    extern int floorFuckedChance;
    extern float standaloneBrightness;

    // Lighting Data Arrays (Size 10 assuming 10 illnesses in your enum)
    extern FlickerTimes flickerTable[10];
    extern IllnessVisuals visualsTable[10];

    // The single master load function
    void Load();
}