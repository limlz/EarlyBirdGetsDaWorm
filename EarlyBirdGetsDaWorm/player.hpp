#pragma once
struct AEGfxVertexList;

float Player_GetWidth();
float Player_GetHeight();
void Player_SetFacing(int dir);
void Player_Load();
void Player_Update(float dt, bool walkKey);
void Player_Draw(float x, float y);
void Player_Unload();