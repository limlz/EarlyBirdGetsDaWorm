#include "pch.hpp"
#include "AEEngine.h"

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