#pragma once

#include "AEEngine.h"

// Call this at the start of your game loop
void Input_Update();

// --- KEYBOARD ---
bool Input_IsKeyHeld(u8 key);      // True while holding down
bool Input_IsKeyTriggered(u8 key); // True only once when pressed

// --- MOUSE POSITION ---
// Returns raw pixel coordinate (0 to 1600)
s32 Input_GetMouseWindowX();
s32 Input_GetMouseWindowY();

// Returns coordinate relative to center (-800 to 800)
// Perfect for your Raycaster rotation or placing objects in the world
f32 Input_GetMouseWorldX();
f32 Input_GetMouseWorldY();

// --- MOUSE BUTTONS ---
bool Input_IsMouseHeld(u8 button);      // AEVK_LBUTTON, AEVK_RBUTTON
bool Input_IsMouseTriggered(u8 button);