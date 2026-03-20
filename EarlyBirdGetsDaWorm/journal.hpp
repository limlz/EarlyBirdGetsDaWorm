#pragma once
#include <vector>

enum class ANOMALYID : int;  

struct AEGfxVertexList;

void Journal_Load();

void Journal_Initialize();

void Journal_DrawSmallIcon(AEGfxVertexList* squareMesh);

void Journal_ReportAnomaly(ANOMALYID id);

bool Journal_HasObserved(ANOMALYID id);

bool Journal_EvidenceHasUnread(ANOMALYID id);

bool Journal_IllnessHasUnread(ILLNESSES illness);

bool Journal_PageHasUnread(ILLNESSES illness, int page);

bool Journal_HasNewContent();

void Journal_MarkAsRead(ANOMALYID id);

void Journal_Update(bool liftActive, bool guideActive, bool pagerActive, f32 dt);

void Journal_Draw(AEGfxVertexList* squareMesh);

void Journal_Unload();

bool Journal_IsOpen();

void Journal_Clear();