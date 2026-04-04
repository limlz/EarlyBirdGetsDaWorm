

void Game_Load();

void Game_Initialize();

void Game_Update();

void Game_Draw();

void Game_Free();

void Game_Unload();

void Game_ResetSession();

int   Game_GetCurrentDay();
float Game_GetTimeSurvived();

bool  Game_IsPlaytestMode();
void  Game_SetPlaytestMode(bool enabled);

bool  Game_IsPresentationMode();
void  Game_SetPresentationMode(bool enabled);