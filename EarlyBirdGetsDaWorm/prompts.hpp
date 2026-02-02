// Prompts.hpp
#pragma once

void Prompts_Load();

// Update signature changed: Added 'float dt'
void Prompts_Update(float dt, float camX, int doorNumAtPlayer, bool liftMenuOpen, bool isLiftRange);

// New function to trigger the "Wrong Room" message
void Prompts_TriggerWrongRoom();

void Prompts_Draw();
void Prompts_Unload();