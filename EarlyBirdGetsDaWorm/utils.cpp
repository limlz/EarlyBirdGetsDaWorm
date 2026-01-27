#include "pch.hpp"



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