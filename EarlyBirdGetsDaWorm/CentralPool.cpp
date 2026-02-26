#include "pch.hpp"

void AllAnomalies_Load() {
	Wall_Load();
	Frames_Load();
}

void AllAnomalies_Initialize() {
	Frames_Initialize();
	Lighting_Initialize(7);
	Wall_Initialize();
}

