// ---------------------------------------------------------------------------
// includes
#include <iostream>
#include <crtdbg.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <math.h> 
#include <Windows.h> // Required for SetCursorPos and GetCursorPos
#include "AEEngine.h"
#include "mesh_creation.hpp" 
#include "utils.hpp"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
const int SCREEN_W = 1600;
const int SCREEN_H = 900;

// Mouse sensitivity - adjust this to your liking
const float mouseSensitivity = 0.1f;

// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int        nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (AESysInit(hInstance, nCmdShow, SCREEN_W, SCREEN_H, 1, 60, false, NULL) == 0)
        return 0;

    AESysSetWindowTitle("Alpha Raycaster: FPS Controls");
    AESysReset();

    // Hide the cursor for better FPS feel
    ShowCursor(FALSE);

    // -----------------------------------------------------------------------
    // SETUP: DOOM-STYLE VECTORS
    // -----------------------------------------------------------------------
    f32 posX = 3.5f, posY = 3.5f;     // Position
    f32 dirX = -1.0f, dirY = 0.0f;    // Direction Vector
    f32 planeX = 0.0f, planeY = 0.66f; // Camera Plane (FOV)

    // Map (16x16)
    int mapHeight = 16;
    int mapWidth = 16;
    std::wstring map;
    map += L"################";
    map += L"#..............#";
    map += L"#....##.##.....#";
    map += L"#....#...#.....#";
    map += L"#....##.##.....#";
    map += L"#....#...#.....#";
    map += L"#....#...#.....#";
    map += L"#....#...#.....#";
    map += L"#....#...#.....#";
    map += L"#..####.####...#";
    map += L"#.#...#.#...#..#";
    map += L"#.....#.#......#";
    map += L"#..####.####...#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"################";

    AEGfxVertexList* pMeshWall = CreateSquareMesh(0xFFFFFFFF);
    int gGameRunning = 1;

    // Get screen center in screen coordinates for SetCursorPos
    int screenCenterX = GetSystemMetrics(SM_CXSCREEN) / 2;
    int screenCenterY = GetSystemMetrics(SM_CYSCREEN) / 2;

    // -----------------------------------------------------------------------
    // GAME LOOP
    // -----------------------------------------------------------------------
    while (gGameRunning)
    {
        AESysFrameStart();
        f64 deltaTime = AEFrameRateControllerGetFrameTime();
        f32 moveSpeed = 5.0f * (f32)deltaTime;

        if (AEInputCheckTriggered(AEVK_ESCAPE)) gGameRunning = 0;

        // --- 1. MOUSE LOOK (Non-constant Delta Movement) ---
        POINT currentMousePos;
        if (GetCursorPos(&currentMousePos))
        {
            // Calculate displacement from screen center
            float mouseDiffX = (float)(currentMousePos.x - screenCenterX);

            if (mouseDiffX != 0)
            {
                // Calculate rotation step
                f32 rotStep = mouseDiffX * mouseSensitivity * (f32)deltaTime;

                // Rotate Direction Vector
                f32 oldDirX = dirX;
                dirX = dirX * cosf(-rotStep) - dirY * sinf(-rotStep);
                dirY = oldDirX * sinf(-rotStep) + dirY * cosf(-rotStep);

                // Rotate Camera Plane Vector
                f32 oldPlaneX = planeX;
                planeX = planeX * cosf(-rotStep) - planeY * sinf(-rotStep);
                planeY = oldPlaneX * sinf(-rotStep) + planeY * cosf(-rotStep);

                // SNAP cursor back to center to prevent constant spinning
                SetCursorPos(screenCenterX, screenCenterY);
            }
        }

        // --- 2. WASD MOVEMENT & STRAFING ---
        // Forward/Backward (W/S)
        if (AEInputCheckCurr(AEVK_W)) {
            if (map[(int)posY * mapWidth + (int)(posX + dirX * moveSpeed)] != '#') posX += dirX * moveSpeed;
            if (map[(int)(posY + dirY * moveSpeed) * mapWidth + (int)posX] != '#') posY += dirY * moveSpeed;
        }
        if (AEInputCheckCurr(AEVK_S)) {
            if (map[(int)posY * mapWidth + (int)(posX - dirX * moveSpeed)] != '#') posX -= dirX * moveSpeed;
            if (map[(int)(posY - dirY * moveSpeed) * mapWidth + (int)posX] != '#') posY -= dirY * moveSpeed;
        }
        // Strafe Left/Right (A/D) - Uses the Plane vector
        if (AEInputCheckCurr(AEVK_A)) {
            if (map[(int)posY * mapWidth + (int)(posX - planeX * moveSpeed)] != '#') posX -= planeX * moveSpeed;
            if (map[(int)(posY - planeY * moveSpeed) * mapWidth + (int)posX] != '#') posY -= planeY * moveSpeed;
        }
        if (AEInputCheckCurr(AEVK_D)) {
            if (map[(int)posY * mapWidth + (int)(posX + planeX * moveSpeed)] != '#') posX += planeX * moveSpeed;
            if (map[(int)(posY + planeY * moveSpeed) * mapWidth + (int)posX] != '#') posY += planeY * moveSpeed;
        }

        // --- RENDER PREP ---
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // --- FLOOR & CEILING ---
        int halfHeight = SCREEN_H / 2;
        for (int y = 0; y < halfHeight; y += 4) {
            f32 intensity = ((f32)y / (f32)halfHeight) * 0.6f;
            AEGfxSetColorToMultiply(intensity, intensity, intensity, 1.0f);
            AEMtx33 scale, trans, transform;
            AEMtx33Scale(&scale, (f32)SCREEN_W, 4.0f);

            AEMtx33Trans(&trans, 0.0f, (f32)y + 2.0f); // Floor
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMeshWall, AE_GFX_MDM_TRIANGLES);

            AEMtx33Trans(&trans, 0.0f, -(f32)y - 2.0f); // Ceiling
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMeshWall, AE_GFX_MDM_TRIANGLES);
        }

        // --- RAYCASTING (DDA) ---
        for (int x = 0; x < SCREEN_W; x += 2) {
            f32 cameraX = 2.0f * x / (f32)SCREEN_W - 1.0f;
            f32 rayDirX = dirX + planeX * cameraX;
            f32 rayDirY = dirY + planeY * cameraX;

            int mapX = (int)posX, mapY = (int)posY;
            f32 deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
            f32 deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);
            f32 sideDistX, sideDistY, perpWallDist;
            int stepX, stepY;

            if (rayDirX < 0) { stepX = -1; sideDistX = (posX - mapX) * deltaDistX; }
            else { stepX = 1; sideDistX = (mapX + 1.0f - posX) * deltaDistX; }
            if (rayDirY < 0) { stepY = -1; sideDistY = (posY - mapY) * deltaDistY; }
            else { stepY = 1; sideDistY = (mapY + 1.0f - posY) * deltaDistY; }

            bool hitWall = false;
            int side = 0;
            bool hitWindow = false;
            f32 windowDist = 0.0f;

            while (!hitWall) {
                if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
                else { sideDistY += deltaDistY; mapY += stepY; side = 1; }

                if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) {
                    hitWall = true; perpWallDist = 20.0f;
                }
                else {
                    wchar_t tile = map[mapY * mapWidth + mapX];
                    if (tile == '#') hitWall = true;
                    else if (tile == 'w' && !hitWindow) {
                        hitWindow = true;
                        windowDist = (side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
                    }
                }
            }

            perpWallDist = (side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
            int lineHeight = (int)(SCREEN_H / perpWallDist);
            f32 shade = (1.0f - (perpWallDist / 16.0f)) * (side == 1 ? 0.7f : 1.0f);
            if (shade < 0) shade = 0;

            AEGfxSetBlendMode(AE_GFX_BM_NONE);
            AEGfxSetColorToMultiply(shade, shade, shade, 1.0f);
            AEMtx33 scale, trans, transform;
            f32 drawX = (f32)x - (SCREEN_W / 2.0f);
            AEMtx33Scale(&scale, 2.0f, (f32)lineHeight);
            AEMtx33Trans(&trans, drawX, 0.0f);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMeshWall, AE_GFX_MDM_TRIANGLES);

            if (hitWindow) {
                int winHeight = (int)(SCREEN_H / windowDist);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.0f, 1.0f, 1.0f, 0.3f);
                AEMtx33Scale(&scale, 2.0f, (f32)winHeight);
                AEMtx33Trans(&trans, drawX, 0.0f);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(pMeshWall, AE_GFX_MDM_TRIANGLES);
            }
        }
        AESysFrameEnd();
    }

    if (pMeshWall) AEGfxMeshFree(pMeshWall);
    AESysExit();
    return 1;
}