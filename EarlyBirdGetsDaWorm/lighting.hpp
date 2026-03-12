#pragma once

void Lighting_Initialize(int fucked_floor);
void Lighting_Update(s8 floorNum, float camX, bool dementia);

void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum, bool dementia);

void Update_StandaloneLight(float dt, float lightX, float lightY);
void Draw_StandaloneConeLight(float x, float y);

void Lighting_Unload();

// Returns the opacity for the full-screen dark overlay in Game_Draw(), scaled per active illness
float Lighting_GetOverlayAlpha();

// Returns true when the nearest overhead light to the player is currently off or mid-flicker-off
bool Lighting_IsPlayerInDarkness(float camX, s8 floorNum);