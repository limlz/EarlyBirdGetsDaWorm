#include "pch.hpp"

AEGfxTexture* LoadTextureChecked(const char* path)
{
	AEGfxTexture* texture = AEGfxTextureLoad(path);
	if (!texture)
	{
		std::cout << "FAILED TO LOAD TEXTURE: " << path << "\n";
	}
	return texture;
}

void UnloadTextureSafe(AEGfxTexture*& texture)
{
	if (texture)
	{
		AEGfxTextureUnload(texture);
		texture = nullptr;
	}
}

void FreeMeshSafe(AEGfxVertexList*& mesh)
{
	if (mesh)
	{
		AEGfxMeshFree(mesh);
		mesh = nullptr;
	}
}

bool CircleCollision(f32 x1, f32 y1, f32 r1, f32 x2, f32 y2, f32 r2)
{
	f32 dx = x2 - x1;
	f32 dy = y2 - y1;
	f32 distanceSquared = dx * dx + dy * dy;
	f32 radiusSum = r1 + r2;
	return distanceSquared <= (radiusSum * radiusSum);
}

bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y)
{
	float area_width_half = area_width / 2;
	float area_height_half = area_height / 2;
	if (click_x < area_center_x + area_width_half &&
		click_x > area_center_x - area_width_half &&
		click_y < area_center_y + area_height_half &&
		click_y > area_center_y - area_height_half) {
		return true;
	}
	else {
		return false;
	}
}

bool IsAreaClickedByMouse(float area_center_x, float area_center_y, float area_width, float area_height)
{
	return IsAreaClicked(area_center_x, area_center_y, area_width, area_height, Input_GetMouseX(), Input_GetMouseY());
}

void AEGfxPrintCentered(s8 fontId, const std::string& text, float centerX, float y, float scale, float r, float g, float b, float a, float offset)
{
	// ESTIMATION: Assume average character width is roughly 0.03 NDC units at scale 1.0
	float estimatedCharWidth = offset * scale;

	// Calculate total width using .length()
	float totalWidth = text.length() * estimatedCharWidth;

	// Apply centering formula: Center - (Width / 2)
	float startX = centerX - (totalWidth / 2.0f);

	// Use .c_str() to convert the C++ string back to the C-style pointer the engine needs
	AEGfxPrint(fontId, text.c_str(), startX, y, scale, r, g, b, a);
}

void AEGfxPrintWithGlow(s8 fontId, const char* text, float x, float y, float scale,
	float textR, float textG, float textB, float textA,
	float glowR, float glowG, float glowB, float glowA,
	float glowOffset, bool includeDiagonals)
{
	AEGfxPrint(fontId, text, x - glowOffset, y, scale, glowR, glowG, glowB, glowA);
	AEGfxPrint(fontId, text, x + glowOffset, y, scale, glowR, glowG, glowB, glowA);
	AEGfxPrint(fontId, text, x, y - glowOffset, scale, glowR, glowG, glowB, glowA);
	AEGfxPrint(fontId, text, x, y + glowOffset, scale, glowR, glowG, glowB, glowA);

	if (includeDiagonals)
	{
		AEGfxPrint(fontId, text, x - glowOffset, y - glowOffset, scale, glowR, glowG, glowB, glowA);
		AEGfxPrint(fontId, text, x + glowOffset, y - glowOffset, scale, glowR, glowG, glowB, glowA);
		AEGfxPrint(fontId, text, x - glowOffset, y + glowOffset, scale, glowR, glowG, glowB, glowA);
		AEGfxPrint(fontId, text, x + glowOffset, y + glowOffset, scale, glowR, glowG, glowB, glowA);
	}

	AEGfxPrint(fontId, text, x, y, scale, textR, textG, textB, textA);
}

bool IsColliding(float r1x, float r1y, float r1w, float r1h,
	float r2x, float r2y, float r2w, float r2h)
{
	// AABB (Axis-Aligned Bounding Box) Collision
	// If ANY of these is true, they are NOT overlapping
	if (r1x - r1w / 2 > r2x + r2w / 2) return false; // R1 is right of R2
	if (r1x + r1w / 2 < r2x - r2w / 2) return false; // R1 is left of R2
	if (r1y - r1h / 2 > r2y + r2h / 2) return false; // R1 is above R2
	if (r1y + r1h / 2 < r2y - r2h / 2) return false; // R1 is below R2

	return true; // None were true, so they must be touching
}
