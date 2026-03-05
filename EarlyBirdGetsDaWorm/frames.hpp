#pragma once

enum class ILLNESSES
{
    NONE,           // no patient / no evidence yet
	ALL,			// ghost (matches no illness exactly)
    PARANOIA,
    MANIA,
    DEPRESSION,
    DEMENTIA,
    SCHIZOPHRENIA,
    AIW_SYNDROME,
    INSOMNIA,
    OCD,
    SCOTOPHOBIA,
};

enum ENTITIES {
	HUMAN,
	GHOST
};

enum FRAME_TIMING_STATE {
	STATE_NORMAL,       // 0: Perfectly normal painting
	STATE_GLITCHING,    // 1: The anomaly is ACTIVE (wrong texture + offset)
	STATE_COOLDOWN      // 2: Brief pause before resetting logic
};

struct FrameAnomaly {
	ENTITIES entity;

	float posX, posY;
	f32 width, height;
	int designID;

	int currentState;
};	

void Frames_SyncToLight(s8 floor, int lightIndex, bool isLightOn);
void Frames_ResetAll();
int GetRandomStateByIllness(ILLNESSES illness);

void Frames_Load();
void Frames_Initialize();
void Frames_Update(float dt);
void Frames_Draw(int currentLevel, f32 camX);
void Frames_Unload();