
void Bullets_Initialize();

void Fire_Bullet(float startX, float startY, bool facingRight);

void Bullets_Update(float dt, float camX, Boss &myBoss);

void Bullets_Draw(float camX);
void Bullets_Free();
