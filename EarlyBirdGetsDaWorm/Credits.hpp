#pragma once
#include "pch.hpp"

// Resource Management
void Credits_Load();
void Credits_Unload();

// State Management
void Credits_Initialize();
void Credits_Update(float dt);

// Draw
void Credits_DrawCharacter(AEGfxVertexList* mesh); // side character on main menu
void Credits_DrawOverlay(AEGfxVertexList* mesh);   // scrolling credits overlay

// API
bool Credits_IsOpen();
bool Credits_IsHovering();
void Credits_Open();
