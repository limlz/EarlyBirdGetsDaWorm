#pragma once
#include "AEEngine.h"

/******************************************************************************************
    @brief  Creates a circle mesh built from triangle fan segments.

    @param  radius  Size of the circle (0.5f creates a diameter of 1.0f matching the unit quad).
    @param  steps   Number of segments around the circumference (30-40 is usually good).
    @param  color   Vertex color in 0xAARRGGBB hex format.
    @return A new vertex list representing the circle mesh. Caller must free with AEGfxMeshFree.
******************************************************************************************/
AEGfxVertexList* CreateCircleMesh(f32 radius, int steps, u32 color);

/******************************************************************************************
    @brief  Creates a unit square mesh (1x1) centered at the origin.

    @param  color   Vertex color in 0xAARRGGBB hex format.
    @return A new vertex list representing the square mesh. Caller must free with AEGfxMeshFree.
******************************************************************************************/
AEGfxVertexList* CreateSquareMesh(u32 color);

/******************************************************************************************
    @brief  Draws a colored square at the specified world position and dimensions.

    @param  mesh    The square mesh to render.
    @param  x       Center X position in world space.
    @param  y       Center Y position in world space.
    @param  width   Width in pixels.
    @param  height  Height in pixels.
    @param  color   Fill color in 0xAARRGGBB hex format.
******************************************************************************************/
void DrawSquareMesh(AEGfxVertexList* mesh, f32 x, f32 y, f32 width, f32 height, u32 color);

/******************************************************************************************
    @brief  Draws a colored circle at the specified world position and radius.

    @param  mesh    The circle mesh to render.
    @param  x       Center X position in world space.
    @param  y       Center Y position in world space.
    @param  radius  Radius in pixels.
    @param  color   Fill color in 0xAARRGGBB hex format.
******************************************************************************************/
void DrawCircleMesh(AEGfxVertexList* mesh, f32 x, f32 y, f32 radius, u32 color);

/******************************************************************************************
    @brief  Draws a textured quad at the specified world position with opacity control.

    @param  mesh    The square mesh to render.
    @param  texture The texture to apply.
    @param  x       Center X position in world space.
    @param  y       Center Y position in world space.
    @param  width   Width in pixels (negative flips horizontally).
    @param  height  Height in pixels.
    @param  opacity Transparency from 0.0f (invisible) to 1.0f (fully opaque).
******************************************************************************************/
void DrawTextureMesh(AEGfxVertexList* mesh, AEGfxTexture* texture, f32 x, f32 y, f32 width, f32 height, f32 opacity);

/******************************************************************************************
    @brief  Draws a textured quad with rotation at the specified world position.

    @param  mesh    The square mesh to render.
    @param  texture The texture to apply.
    @param  x       Center X position in world space.
    @param  y       Center Y position in world space.
    @param  width   Width in pixels.
    @param  height  Height in pixels.
    @param  angle   Rotation angle in radians.
    @param  opacity Transparency from 0.0f (invisible) to 1.0f (fully opaque).
******************************************************************************************/
void DrawTextureMeshRotated(AEGfxVertexList* mesh, AEGfxTexture* texture, f32 x, f32 y, f32 width, f32 height, f32 angle, f32 opacity);
