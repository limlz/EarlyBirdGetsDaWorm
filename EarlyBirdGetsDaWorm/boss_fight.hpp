void Boss_Fight_Load();

void Boss_Fight_Initialize();

void Boss_Fight_Update();

void Boss_Fight_Draw();

void Boss_Fight_Free();

void Boss_Fight_Unload();

// Called by Bullets.cpp when a player bullet actually hits the boss
// Rolls the dodge chance and moves the boss if the cooldown allows
void Boss_Fight_TryDodge();