#include "AEEngine.h"

bool CircleCollision(f32 x1, f32 y1, f32 r1, f32 x2, f32 y2, f32 r2)
{
	f32 dx = x2 - x1;
	f32 dy = y2 - y1;
	f32 distanceSquared = dx * dx + dy * dy;
	f32 radiusSum = r1 + r2;
	return distanceSquared <= (radiusSum * radiusSum);
}