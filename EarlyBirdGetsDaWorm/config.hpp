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

// Defines the glitch cycle timing for frame/poster anomalies per illness.
struct FrameTimes {
    float glitchDuration; // how long the glitching phase lasts (seconds)
    float waitMin;        // minimum idle time before the next glitch; also the fixed cycle length for AIW/Insomnia/OCD
    float waitRandMax;    // maximum extra random seconds added on top of waitMin (0 = fixed cycle, no randomness)
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
    extern FrameTimes     frameTimesTable[11];   // Frame/poster glitch cycle timing per illness

    // --- FRAME BEHAVIOR CONSTANTS ---
    // Paranoia (Weeping Angel effect)
    extern float frameParanoiaCloseRange;     // distance within which frames look completely normal
    extern float frameParanoiaPanicRange;     // distance range over which panic speed scales (added to closeRange)
    extern float frameParanoiaIntensityScale; // multiplier for jitter intensity at max distance
    extern int   frameParanoiaMinIntensity;   // minimum jitter intensity so it always moves at least 1px

    // Mania (hyperactive drift)
    extern float frameManiaDriftSpeed;   // pixels per second of continuous drift
    extern int   frameManiaSutterChance; // percent chance each frame of a rapid stutter (0-100)
    extern int   frameManiaSutterRange;  // pixel range of the stutter jitter (total, split ±half)

    // Depression (melting/sinking)
    extern float frameDepressionStretchDuration;   // seconds over which the frame fully stretches down
    extern float frameDepressionMaxHeightIncrease; // max extra pixels added to frame height at full stretch
    extern float frameDepressionYOffset;           // Y offset to keep the top edge anchored during stretch

    // Dementia (persistent confusion jitter)
    extern int frameDementiaJitterRange; // total jitter pixel range (split ±half)

    // Schizophrenia (aggressive hallucination jitter)
    extern int frameSchizophreniJitterRange; // total jitter pixel range (split ±half), wider than dementia

    // AIW Syndrome (spatial distortion floating)
    extern float frameAIWOscillationFreq; // oscillation cycles per second (radians/s divider)
    extern float frameAIWOscillationAmp;  // amplitude of the circular floating offset in pixels

    // Insomnia (exhaustion distortion)
    extern float frameInsomniaFarDist;     // distance beyond which frames stretch wildly
    extern float frameInsomniaStretchMult; // stretch multiplier applied when player is far away

    // OCD (misalignment offsets — alternates based on grid position)
    extern float frameOCDOffsetXEven; // X offset for even-column frames
    extern float frameOCDOffsetXOdd;  // X offset for odd-column frames
    extern float frameOCDOffsetYEven; // Y offset for even-row frames
    extern float frameOCDOffsetYOdd;  // Y offset for odd-row frames

    // Scotophobia (peripheral gaze mechanic)
    extern float frameScotophobiaCloseRange;  // distance within which frames look normal (center of screen)
    extern int   frameScotophobiaJitterRange; // jitter pixel range when in peripheral vision (split ±half)

    // Ghost overlay stutter (applied on top of all other illness logic)
    extern int   frameGhostStutterChance;   // percent chance each update of a ghost micro-stutter (0-100)
    extern int   frameGhostStutterRange;    // pixel range of the ghost stutter jitter (split ±half)
    extern float frameGhostGlitchSpeedMult; // multiplier on glitch phase duration when ghost is active
    extern float frameGhostWaitSpeedMult;   // multiplier on wait phase duration when ghost is active
    extern float frameGhostMinTimer;        // minimum allowed timer after ghost multiplier is applied

    // --- JUMPSCARE SETTINGS ---
    extern float jumpscareDuration;
    extern float jumpscareScreenShake;
    extern float jumpscareImageShake;
    extern float jumpscareFlashDuration;

    // --- CORE FUNCTIONS ---
    void Load();
}