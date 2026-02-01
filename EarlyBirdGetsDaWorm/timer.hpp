#pragma once

enum class Timer_EndChoice
{
    None,
    MainMenu,
    Restart
};

void Timer_Load();
void Timer_Update(float dt);
void Timer_Draw(float ndcX, float ndcY);

// ------------------------------
// TIMER STATUS 
// ------------------------------
bool Timer_IsTimeUp();  
int  Timer_GetGameMinutes();    

// ------------------------------
// TIMER PAUSE WHEN PLAYER ENTERS ROOM
// ------------------------------
void Timer_SetPaused(bool paused);
bool Timer_IsPaused();

// ------------------------------
// TIMER RESET WHEN NEW DAY STARTS
// ------------------------------
void Timer_Reset();

// ------------------------------
// DEBUG: Skip timer to 5:58 AM
// ------------------------------
void Timer_DebugSetTime(float minutes);

// ------------------------------
// DAY OVERLAY (FADE IN/OUT)
// ------------------------------
void Timer_StartDayOverlay(int dayNum);
bool Timer_IsDayOverlayActive();
void Timer_UpdateDayOverlay(float dt);
void Timer_DrawDayOverlay(AEGfxVertexList* squareMesh);