#pragma once // Prevents this file from being included twice
#include "AEEngine.h" // Needed for AEGfxVertexList types

void InitMouseXandY();

bool CircleCollision(f32 x1, f32 y1, f32 r1, f32 x2, f32 y2, f32 r2);

bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);

void AEGfxPrintCentered(s8 fontId, const std::string& text, float centerX, float y, float scale, float r, float g, float b, float a, float offset = 0.025f);

AEGfxTexture* LoadTextureChecked(const char* path);
void UnloadTextureSafe(AEGfxTexture*& texture);
void FreeMeshSafe(AEGfxVertexList*& mesh);

bool IsColliding(float r1x, float r1y, float r1w, float r1h,
	float r2x, float r2y, float r2w, float r2h);

extern float worldMouseX;
extern float worldMouseY;
