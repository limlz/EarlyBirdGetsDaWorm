#pragma once
#include "pch.hpp"

// ============================================================
// FRAMES MODULE (Frames.hpp)
// - Paintings/frames that glitch based on the carried patient's illness.
// - GHOST: illness can be ILLNESSES::GHOST (use mimic illness inside Frames.cpp).
// ============================================================

// Frame glitch timing state machine
enum FRAME_TIMING_STATE
{
    STATE_NORMAL = 0,    // normal painting
    STATE_GLITCHING,     // anomaly active (wrong texture + offsets)
    STATE_COOLDOWN       // brief reset pause
};

// What the frame "belongs to" (kept from your code)
enum ENTITIES
{
    HUMAN = 0,
    GHOST_ENTITY        // renamed from GHOST to avoid clashing with ILLNESSES::GHOST
};

// Per-frame data stored in levelMap[floor][frame]
struct FrameAnomaly
{
    ENTITIES entity = HUMAN;

    float posX = 0.0f;
    float posY = 0.0f;

    f32   width = 0.0f;
    f32   height = 0.0f;

    int   designID = 1;       // 1..3
    int   currentState = 0;   // 0..(FRAME_STATES-1)
};

// ============================================================
// PUBLIC API
// ============================================================

// Sync nearby frames when a specific light toggles on/off
void Frames_SyncToLight(s8 floor, int lightIndex, bool isLightOn);

// Resets all frames to normal (optional public; keep if other modules call it)
void Frames_ResetAll();

// Asset + lifetime
void Frames_Load();
void Frames_Initialize();
void Frames_Update(float dt);
void Frames_Draw(int currentLevel, f32 camX);
void Frames_Unload();