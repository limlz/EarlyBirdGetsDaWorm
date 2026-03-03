// ============================================================
// JUMPSCARE SYSTEM
// - Press J to test trigger (in JumpScare_Update)
// - Screen shake offsets you can apply to ALL draws
// - Draw overlay last (JumpScare_Draw)
// ============================================================

#pragma once
#include "pch.hpp"

void JumpScare_Init();
void JumpScare_Load();
void JumpScare_Initialize();
void JumpScare_Unload();

void JumpScare_Start();
bool JumpScare_Update(float dt);
void JumpScare_GetShakeOffset(float& outX, float& outY);
void JumpScare_Draw();