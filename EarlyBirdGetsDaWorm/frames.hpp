#pragma once

void Frames_Load();
void Frames_Initialize();
void Frames_Update();
void Frames_Draw(int currentLevel, f32 camX);
void Frames_Unload();

enum ANOMALY {
	NORMAL,
	TIER1,
	TIER2
};
   
struct FrameAnomaly {
	float posX;
	float posY;
	f32 width;
	f32 height;
	ANOMALY type;
	int textureID;
};	