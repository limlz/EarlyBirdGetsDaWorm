#include "AEEngine.h"

void Doors_Load();

void Doors_Initialize();

void Doors_Animate(float dt, int doorNearPlayer, float camX);

int  Doors_Update(f32 camX);

void Doors_Draw(f32 camX, s8 floorNum, f32 textXoffset, f32 textY, bool dementia = false);

void Doors_Unload();