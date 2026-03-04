#pragma once

// Note: If you get a compiler error saying "ILLNESSES is undefined", 
// make sure to #include the header file where your ILLNESSES enum is defined, 
// or ensure it is included globally in your pch.hpp!
// Example: #include "Player.hpp" 

void Lighting_Load();
void Lighting_Initialize(int fucked_floor);

// --- Updated Core Lighting Functions ---
void Lighting_Update(s8 floorNum, float camX, bool dementia, bool isGhost, ILLNESSES illness);
void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum, bool dementia, bool isGhost, ILLNESSES illness);
void DrawConeLight(float lightWorldX, float lightY, float camX, bool right_left, bool isGhost, ILLNESSES illness);

// --- Standalone Light Functions ---
void Update_StandaloneLight(float dt, float lightX, float lightY);
void Draw_StandaloneConeLight(float x, float y);

void Lighting_Unload();