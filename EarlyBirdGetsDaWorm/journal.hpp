#pragma once
#include <vector>

enum class ANOMALYID : int;  

struct AEGfxVertexList;

void Journal_Load();

void Journal_Initialize();

void Journal_DrawSmallIcon(AEGfxVertexList* squareMesh);

void Journal_ReportAnomaly(ANOMALYID id);

bool Journal_HasObserved(ANOMALYID id);

void Journal_Update();  // handles I toggle + selection

void Journal_Draw(AEGfxVertexList* squareMesh);

void Journal_Unload();

bool Journal_IsOpen();

void Journal_Clear();