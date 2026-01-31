#pragma once

enum class Timer_EndChoice
{
    None,
    MainMenu,
    Restart
};

void Timer_Load();
void Timer_Reset();
void Timer_Update(float dt);
void Timer_Draw(float ndcX, float ndcY);

bool Timer_IsTimeUp();  
int  Timer_GetGameMinutes();    

void Timer_DrawShiftOverOverlay(AEGfxVertexList* squareMesh); // draws the overlay

// DEBUG: Skip timer to 5:58 AM
void Timer_DebugSetTime(float minutes);