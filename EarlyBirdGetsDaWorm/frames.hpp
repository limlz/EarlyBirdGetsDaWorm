#pragma once

void Frames_Load();
void Frames_Initialize();
void Frames_Update(float dt);
void Frames_Draw(int currentLevel, f32 camX);
void Frames_Unload();

enum ENTITIES {
	HUMAN,
	GHOST
};

enum ILLNESSES {
	PARANOIA,
	MANIA,
	DEPRESSION,
	DEMENTIA,
	ALL
};

struct FrameAnomaly {
	ENTITIES entity;
	ILLNESSES illness;

	float posX, posY;
	f32 width, height;
	int designID;

	int currentState;
};	