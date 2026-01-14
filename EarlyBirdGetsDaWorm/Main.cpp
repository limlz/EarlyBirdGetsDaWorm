// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h>
#include <stdio.h>
#include "AEEngine.h"
#include "mesh_creation.hpp" 
#include "utils.hpp"

f32 playerX = 0.0f;
f32 playerY = 0.0f;

// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 1. Initialize System
    if (AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL) == 0)
        return 0;

    AESysSetWindowTitle("BurdBurd!");
    AESysReset();

	// Variables
	f32 maxHealth = 980.0f;
	f32 damageDealt = 0.0f;
	f32 damagePerSecond = 100.0f;
    // Create Circle Mesh (Radius 0.5, 40 steps)
    AEGfxVertexList* circleMesh = CreateCircleMesh(0.5f, 40, 0xFFFFFFFF);

	// Create Square Mesh for Health Bar 
	AEGfxVertexList* squareMesh = CreateSquareMesh(0xFFFFFFFF);
    int gGameRunning = 1;

    // 3. Game Loop
    while (gGameRunning)
    {
        AESysFrameStart();

        // Input & Exit Logic
        if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
            gGameRunning = 0;

        f64 deltaTime = AEFrameRateControllerGetFrameTime();

        // Player Movement
        f32 speed = 200.0f;
        if (AEInputCheckCurr(AEVK_W)) playerY += speed * (f32)deltaTime;
        if (AEInputCheckCurr(AEVK_S)) playerY -= speed * (f32)deltaTime;
        if (AEInputCheckCurr(AEVK_A)) playerX -= speed * (f32)deltaTime;
        if (AEInputCheckCurr(AEVK_D)) playerX += speed * (f32)deltaTime;

		// Damage / Heal Logic
        if (CircleCollision(playerX, playerY, 50.0f, -400.0f, 0.0f, 225.0f)) {
            damageDealt += damagePerSecond * (f32)deltaTime;
			if (damageDealt > maxHealth) damageDealt = maxHealth;
        } else if (CircleCollision(playerX, playerY, 50.0f, 400.0f, 0.0f, 225.0f)) {
            damageDealt -= damagePerSecond * (f32)deltaTime;
			if (damageDealt < 0.0f) damageDealt = 0.0f;
		}

        // Render

        AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        AEMtx33 scale, translate, transform;

		// --- Draw Health Bar Background ---
        AEMtx33Scale(&scale, 1000.0f, 60.0f);
        AEMtx33Trans(&translate, 0.0f, 400.0f);
        AEMtx33Concat(&transform, &translate, &scale);

        AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 1.0f);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --- Draw Health Bar  ---
        AEMtx33Scale(&scale, maxHealth - damageDealt, 40.0f);
        AEMtx33Trans(&translate, 0.0f - damageDealt / 2, 400.0f);
        AEMtx33Concat(&transform, &translate, &scale);

        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --- Mini Health Bar ---
        s8 mini_bar = (maxHealth - damageDealt) / 98;
        AEMtx33Scale(&scale, 52.63157f, 40.0f);

        for (s8 bars{}; bars < mini_bar; bars++) {
            AEMtx33Trans(&translate, -473.684215f + bars * (2 * 52.63157f), 330.0f);
            AEMtx33Concat(&transform, &translate, &scale);

            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
        }

        // --- Left Circle (Red) ---
        AEMtx33Scale(&scale, 450.0f, 450.0f);
        AEMtx33Trans(&translate, -400.0f, 0.0f);
        AEMtx33Concat(&transform, &translate, &scale);

        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --- Right Circle (Green) ---
        AEMtx33Scale(&scale, 450.0f, 450.0f);
        AEMtx33Trans(&translate, 400.0f, 0.0f);
        AEMtx33Concat(&transform, &translate, &scale);

        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --- Player Circle (Blue) ---
        AEMtx33Scale(&scale, 100.0f, 100.0f);
        AEMtx33Trans(&translate, playerX, playerY);
        AEMtx33Concat(&transform, &translate, &scale);

        AEGfxSetColorToMultiply(0.0f, 0.0f, 1.0f, 1.0f);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        AESysFrameEnd();
    }

    // 4. Cleanup
    if (circleMesh) AEGfxMeshFree(circleMesh);

    AESysExit();
    return 1;
}