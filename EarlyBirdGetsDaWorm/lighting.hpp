void Lighting_Load();

void Lighting_Initialize();

void Lighting_Update(s8 floorNum);

void Lighting_Draw(f32 camX, s8 floorNum);

void DrawConeLight(float lightWorldX, float lightY, float camX, bool right_left);

void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum);