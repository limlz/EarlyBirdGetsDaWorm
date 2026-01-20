#pragma once // Prevents this file from being included twice
#include "AEEngine.h" // Needed for AEGfxVertexList types

// Function Declaration
// radius: Size of circle (0.5f creates a diameter of 1.0f, matching the unit quad)
// steps: How smooth the circle is (30-40 is usually good)
// color: Hex color (0xAARRGGBB)
AEGfxVertexList* CreateCircleMesh(f32 radius, int steps, u32 color);

AEGfxVertexList* CreateSquareMesh(u32 color);

void DrawSquareMesh(AEGfxVertexList* mesh, f32 x, f32 y, f32 width, f32 height, u32 color);

void DrawCircleMesh(AEGfxVertexList* mesh, f32 x, f32 y, f32 radius, u32 color);