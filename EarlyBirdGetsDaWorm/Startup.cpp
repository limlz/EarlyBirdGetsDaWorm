#include "pch.hpp"

static AEGfxVertexList* splashMesh = nullptr;
static AEGfxTexture* digipenLogo = nullptr;
static f32 splashTimer = 0.0f;
static f64 splash_screen_timer{};
static f64 title_screen_timer{};
static f64 wait_timer{};

void Startup_Load()
{
	// Load resources for the main menu
	digipenLogo = LoadTextureChecked("Assets/digipen_white.png");
	std::cout << "Startup: Load\n";
}

void Startup_Initialize()
{
	// Initialize main menu variables
	splashMesh = CreateSquareMesh(0xFFFFFFFF);
	wait_timer = 0.5f;
	splash_screen_timer = 3.0f;
	title_screen_timer = 2.4f;

	std::cout << "Timer: " << splash_screen_timer << "\n";
	std::cout << "Startup: Initialize\n";
}

void Startup_Update()
{
	if (wait_timer > 0)
	{
		wait_timer -= AEFrameRateControllerGetFrameTime();
		return;
	}
	if (splash_screen_timer > 0)
	{
		splash_screen_timer -= AEFrameRateControllerGetFrameTime();
		if (AEInputCheckTriggered(AEVK_SPACE))
		{
			splash_screen_timer = 0.0f;
		}
		return;
	} 
	else if (title_screen_timer > 0)
	{
		title_screen_timer -= AEFrameRateControllerGetFrameTime();
		std::cout << "TitleScreen: " << title_screen_timer << "\n";
		if (AEInputCheckTriggered(AEVK_SPACE))
		{
			title_screen_timer = 0.0f;
		}
		return;
	}
	{
		next = MAIN_MENU; // Transition to main menu state
	}
}

void Startup_Draw()
{
	if (wait_timer > 0)
	{
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		return;
	}
	if (splash_screen_timer > 0)
	{
		float maxDuration = 3.0f;
		float percent = (float)splash_screen_timer / maxDuration;
		float alpha = sinf(percent * 3.14159f);

		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		DrawTextureMesh(splashMesh, digipenLogo, 0.0f, 0.0f, 1500.0f, 420.0f, alpha);
		return;
	}
	if (title_screen_timer > 0)
	{
		AEGfxSetBackgroundColor(1.0f, 0.0f, 1.0f);
		return;
	}
}

void Startup_Free()
{
	// Free main menu resources
	std::cout << "Startup: Free\n";
}


void Startup_Unload()
{
	FreeMeshSafe(splashMesh);
	UnloadTextureSafe(digipenLogo);
	std::cout << "Startup: Unload\n";
}