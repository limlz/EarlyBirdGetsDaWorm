#pragma once

void Lighting_Load();
void Lighting_Initialize(int fucked_floor);
void Lighting_Update(s8 floorNum, float camX, bool dementia);

void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum, bool dementia);

void Update_StandaloneLight(float dt, float lightX, float lightY);
void Draw_StandaloneConeLight(float x, float y);

void Lighting_Unload();