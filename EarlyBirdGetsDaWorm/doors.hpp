#include "AEEngine.h"
#pragma once

void Doors_Load();

void Doors_Initialize();

void Doors_Animate(float dt, int doorNearPlayer, float camX);

int Doors_Update(float camX);

void Doors_Draw(float camX, s8 floorNum, float textXoffset, float textY, bool dementia);

void Doors_Unload();

bool Doors_TryDisposal(int floorNum, int doorIdx, bool& outDidJumpscare);