#pragma once // Prevents this file from being included twice
#include "AEEngine.h" // Needed for AEGfxVertexList types

void InitMouseXandY();

bool CircleCollision(f32 x1, f32 y1, f32 r1, f32 x2, f32 y2, f32 r2);

bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);

extern float worldMouseX;
extern float worldMouseY;
