#include "pch.hpp"

AEGfxVertexList* CreateCircleMesh(f32 radius, int steps, u32 color)
{
    AEGfxMeshStart();

    // Calculate the angle step for each slice of the pizza
    f32 angleStep = (2.0f * 3.14159f) / (f32)steps;

    for (int i = 0; i < steps; ++i)
    {
        // Calculate angles for the current slice
        f32 theta1 = i * angleStep;
        f32 theta2 = (i + 1) * angleStep;

        // --- Vertex 1: Center ---
        f32 x0 = 0.0f;
        f32 y0 = 0.0f;
        f32 u0 = 0.5f;
        f32 v0 = 0.5f;

        // --- Vertex 2: Edge Point Current ---
        f32 x1 = cosf(theta1) * radius;
        f32 y1 = sinf(theta1) * radius;
        // Map Position (-0.5 to 0.5) to UV (0.0 to 1.0)
        f32 u1 = (x1 / (radius * 2)) + 0.5f;
        f32 v1 = 1.0f - ((y1 / (radius * 2)) + 0.5f); // Flip V for textures

        // --- Vertex 3: Edge Point Next ---
        f32 x2 = cosf(theta2) * radius;
        f32 y2 = sinf(theta2) * radius;
        f32 u2 = (x2 / (radius * 2)) + 0.5f;
        f32 v2 = 1.0f - ((y2 / (radius * 2)) + 0.5f);

        // Add the triangle (Center -> P1 -> P2)
        AEGfxTriAdd(
            x0, y0, color, u0, v0,
            x1, y1, color, u1, v1,
            x2, y2, color, u2, v2
        );
    }

    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateSquareMesh(u32 color)
{
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, color, 0.0f, 1.0f,
        0.5f, -0.5f, color, 1.0f, 1.0f,
        -0.5f, 0.5f, color, 0.0f, 0.0f
    );
    // Second Triangle (Bottom Left, Top Right, Top Left)
    AEGfxTriAdd(
        0.5f, -0.5f, color, 1.0f, 1.0f,
        0.5f, 0.5f, color, 1.0f, 0.0f,
        -0.5f, 0.5f, color, 0.0f, 0.0f
    );
    return AEGfxMeshEnd();
}

void DrawSquareMesh(AEGfxVertexList* mesh, f32 x, f32 y, f32 width, f32 height, u32 color)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEMtx33 scale, translate, transform;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);
    AEGfxSetColorToMultiply(
        ((color >> 24) & 0xFF) / 255.0f,
        ((color >> 16) & 0xFF) / 255.0f,
        ((color >> 8) & 0xFF) / 255.0f,
        (color & 0xFF) / 255.0f
    );
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void DrawCircleMesh(AEGfxVertexList* mesh, f32 x, f32 y, f32 radius, u32 color)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEMtx33 scale, translate, transform;
    AEMtx33Scale(&scale, radius * 2.0f, radius * 2.0f); // Diameter scaling
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);
    AEGfxSetColorToMultiply(
        ((color >> 24) & 0xFF) / 255.0f,
        ((color >> 16) & 0xFF) / 255.0f,
        ((color >> 8) & 0xFF) / 255.0f,
        (color & 0xFF) / 255.0f
    );
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void DrawTextureMesh(AEGfxVertexList* mesh, AEGfxTexture* texture, f32 x, f32 y, f32 width, f32 height, f32 opacity)
{
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxTextureSet(texture, 0.0f, 0.0f);
    AEGfxSetTransparency(opacity);
    AEMtx33 scale, translate, transform;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}
