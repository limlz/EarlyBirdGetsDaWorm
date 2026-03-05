#pragma once
#include "pch.hpp"

// Resource Management
void Settings_Load();
void Settings_Unload();

// State Management
void Settings_Initialize();
void Settings_Update(float dt);

// Draw the settings overlay
void Settings_Draw(AEGfxVertexList* squareMesh);

// API
bool Settings_IsOpen();
void Settings_Open();
void Settings_Close();
float Settings_GetMasterVolume();