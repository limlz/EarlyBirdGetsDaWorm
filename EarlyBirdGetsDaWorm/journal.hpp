#pragma once
#include <vector>

enum class ANOMALYID : int;  

struct AEGfxVertexList;

void Journal_Load();
void Journal_Unload();

// Core
void Journal_Clear();
void Journal_ReportAnomaly(ANOMALYID id);
bool Journal_HasObserved(ANOMALYID id);
bool Journal_TryDeduceHumanIllness(ILLNESSES& outIllness);
bool Journal_IsGhostEvidence();

// UI (the “book”)
void Journal_Update();  // handles I toggle + selection
void Journal_Draw(AEGfxVertexList* squareMesh);
bool Journal_IsOpen();