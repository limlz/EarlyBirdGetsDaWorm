#pragma once

void Frames_Load();
void Frames_Initialize();
void Frames_Update();
void Frames_Draw(int currentLevel, f32 camX);
void Frames_Unload();

enum ENTITY {
	HUMAN,
	GHOST
};
   
struct FrameAnomaly {
	ENTITY type;
	float posX;
	float posY;
	f32 width;
	f32 height;
	int textureID;
};	