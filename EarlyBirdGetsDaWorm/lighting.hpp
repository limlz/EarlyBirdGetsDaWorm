void Lighting_Load();

void Lighting_Initialize(int fucked_floor);

void Lighting_Update(s8 floorNum, float camX, bool dementia);

void DrawConeLight(float lightWorldX, float lightY, float camX, bool right_left);

void Draw_and_Flicker(f32 camX, bool left_right, s8 floorNum, bool dementia = false);

void Lighting_Unload();