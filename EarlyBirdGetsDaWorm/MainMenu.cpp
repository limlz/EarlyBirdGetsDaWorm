#include "pch.hpp"

static AEGfxVertexList* squareMesh;

static AEGfxTexture* main_menu_bg = nullptr;
static AEGfxTexture* admit_patient_tag = nullptr;
static AEGfxTexture* admit_patient_text = nullptr;
static AEGfxTexture* options_text = nullptr;
static AEGfxTexture* options_tag = nullptr;
static AEGfxTexture* quit_tag = nullptr;
static AEGfxTexture* quit_text = nullptr;
static AEGfxTexture* title_bg = nullptr;
static AEGfxTexture* title = nullptr;


s8 menuFontId = 0;
s32 mouseX, mouseY;
f32 worldMouseX, worldMouseY;

bool first_load = true;

const float FADE_IN_DURATION = 0.3f;
static float fade_in_timer = FADE_IN_DURATION;// Fade Out Animation tracking
static bool isFadingOut = false;
static float fade_out_alpha = 0.0f; 
const float FADE_OUT_SPEED = 3.0f; // Multiplier. Higher is faster.
static int pendingNextState = 0;   // Stores where we want to go after the fade
float menuLightX = -200.0f;
float menuLightY = 500.0f;
// Hover animation scales
static float admit_scale = 1.0f;
static float options_scale = 1.0f;
static float quit_scale = 1.0f;

//title ani
const int NUM_TITLE_FRAMES = 13; 
static AEGfxTexture* title_frames[NUM_TITLE_FRAMES];
static int currentTitleFrame = 0;
static float titleAnimTimer = 0.0f;
const float TITLE_ANIM_SPEED = 0.2f;

void MainMenu_Load()
{
	// Load resources for the main menu
	main_menu_bg = LoadTextureChecked("Assets/Main_Menu/main_menu_bg.jpg");
	admit_patient_tag = LoadTextureChecked("Assets/Main_Menu/admit_patient_tag.png");
	admit_patient_text = LoadTextureChecked("Assets/Main_Menu/admit_patient_text.png");
	options_text = LoadTextureChecked("Assets/Main_Menu/options_text.png");
	options_tag = LoadTextureChecked("Assets/Main_Menu/options_tag.png");
	quit_tag = LoadTextureChecked("Assets/Main_Menu/discharge_tag.png");
	quit_text = LoadTextureChecked("Assets/Main_Menu/discharge_text.png");
	title_bg = LoadTextureChecked("Assets/Main_Menu/title_bg.png");
	for (int i = 0; i < NUM_TITLE_FRAMES; i++)
	{
		// Load each frame of the title animation
		std::string filePath = "Assets/Main_Menu/title_sheets/title_" + std::to_string(i + 1) + ".PNG";

		title_frames[i] = LoadTextureChecked(filePath.c_str());
	}


	std::cout << "MainMenu: Load\n";
	menuFontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
}

void MainMenu_Initialize()
{
	Lighting_Initialize(0);
	Particles_Initialize();

	fade_in_timer = FADE_IN_DURATION;
	isFadingOut = false;
	fade_out_alpha = 0.0f;
	pendingNextState = 0;

	float dt = (f32)AEFrameRateControllerGetFrameTime();
	int count = 3;
	squareMesh = CreateSquareMesh(0xFFFFFFFF);
	std::cout << "MainMenu: Initialize\n";
}

void MainMenu_Update()
{
	float dt = (f32)AEFrameRateControllerGetFrameTime();

	if (isFadingOut) {
		fade_out_alpha += FADE_OUT_SPEED * dt;

		if (fade_out_alpha >= 1.0f) {
			fade_out_alpha = 1.0f;
			next = pendingNextState; 
		}

		return;
	}

	if (fade_in_timer > 0.0f) {
		fade_in_timer -= dt;
		if (fade_in_timer < 0.0f) {
			fade_in_timer = 0.0f; // Lock it at 0 when finished
		}
	}

	if (currentTitleFrame < NUM_TITLE_FRAMES - 1) // If we haven't reached the last frame
	{
		titleAnimTimer += dt;
		if (titleAnimTimer >= TITLE_ANIM_SPEED)
		{
			titleAnimTimer = 0.0f; // Reset timer
			currentTitleFrame++;   // Go to the next drawing step
			
		}
	}

	Particles_Update();
	Update_StandaloneLight(dt, menuLightX, menuLightY);

	bool hoverAdmit = IsAreaClicked(510.0f, 270.0f, 490.0f, 290.0f, Input_GetMouseX(), Input_GetMouseY());
	bool hoverOptions = IsAreaClicked(530.0f, 15.0f, 500.0f, 200.0f, Input_GetMouseX(), Input_GetMouseY());
	bool hoverQuit = IsAreaClicked(520.0f, -210.0f, 490.0f, 190.0f, Input_GetMouseX(), Input_GetMouseY());

	float animationSpeed = 15.0f;
	admit_scale += ((hoverAdmit ? 1.15f : 1.0f) - admit_scale) * animationSpeed * dt;
	options_scale += ((hoverOptions ? 1.15f : 1.0f) - options_scale) * animationSpeed * dt;
	quit_scale += ((hoverQuit ? 1.15f : 1.0f) - quit_scale) * animationSpeed * dt;

	if (AEInputCheckTriggered(AEVK_LBUTTON))
	{
		if (hoverAdmit) {
			pendingNextState = GAME_STATE;
			isFadingOut = true; // Start the fade 
		}
		else if (hoverOptions) {
			pendingNextState = GS_QUIT;
			isFadingOut = true; // Start the fade
		}
		else if (hoverQuit) {
			pendingNextState = GS_QUIT;
			isFadingOut = true; // Start the fade
		}
	}
}

void MainMenu_Draw()
{
	DrawTextureMesh(squareMesh, main_menu_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

	DrawTextureMesh(squareMesh, title_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);
	// --- DRAW CURRENT ANIMATION FRAME ---
	if (title_frames[currentTitleFrame]) {
		DrawTextureMesh(squareMesh, title_frames[currentTitleFrame], 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);
	}

	Draw_StandaloneConeLight(menuLightX, menuLightY);

	Particles_Draw(squareMesh, 0.0f);

	DrawTextureMesh(squareMesh, admit_patient_tag, 520.0f, 280.0f, 500.0f * admit_scale, 300.0f * admit_scale, 1.0f);
	DrawTextureMesh(squareMesh, admit_patient_text, 500.0f, 280.0f, 400.0f * admit_scale, 200.0f * admit_scale, 1.0f);

	DrawTextureMesh(squareMesh, options_tag, 530.0f, 15.0f, 500.0f * options_scale, 200.0f * options_scale, 1.0f);
	DrawTextureMesh(squareMesh, options_text, 530.0f, 15.0f, 400.0f * options_scale, 150.0f * options_scale, 1.0f);

	DrawTextureMesh(squareMesh, quit_tag, 530.0f, -220.0f, 500.0f * quit_scale, 200.0f * quit_scale, 1.0f);
	DrawTextureMesh(squareMesh, quit_text, 545.0f, -220.0f, 400.0f * quit_scale, 150.0f * quit_scale, 1.0f);


	float currentBlackAlpha = 0.0f;

	if (fade_in_timer > 0.0f) {
		currentBlackAlpha = fade_in_timer / FADE_IN_DURATION;
	}
	else if (isFadingOut) {
		currentBlackAlpha = fade_out_alpha;
	}

	if (currentBlackAlpha > 0.0f)
	{
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);

		AEGfxSetTransparency(currentBlackAlpha);
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // Pure Black

		AEMtx33 scale, trans, transform;
		AEMtx33Scale(&scale, 1600.0f, 900.0f);
		AEMtx33Trans(&trans, 0.0f, 0.0f);
		AEMtx33Concat(&transform, &trans, &scale);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
	}
}

void MainMenu_Free()
{
	std::cout << "MainMenu: Free\n";
}

void MainMenu_Unload()
{
    FreeMeshSafe(squareMesh);

	UnloadTextureSafe(admit_patient_tag);
	UnloadTextureSafe(admit_patient_text);
	UnloadTextureSafe(quit_tag);
	UnloadTextureSafe(quit_text);
	UnloadTextureSafe(title_bg); 
	for (int i = 0; i < NUM_TITLE_FRAMES; i++) {
		UnloadTextureSafe(title_frames[i]);
	}

    if (menuFontId >= 0) {
        AEGfxDestroyFont(menuFontId);
        menuFontId = -1;
    }
	Particles_Free();
    std::cout << "MainMenu: Unload\n";
}