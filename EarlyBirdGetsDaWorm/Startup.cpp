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
	digipenLogo = AEGfxTextureLoad("Assets/digipen_white.png");
	if(digipenLogo == nullptr)
	{
		std::cout << "Failed to load DigiPen logo texture!\n";
	}
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

		// Normalize the timer to 0.0 - 1.0 range
		// At start (3.0), this is 1.0. At end (0.0), this is 0.0.
		float percent = (float)splash_screen_timer / maxDuration;

		// Map to Radians (0 to PI)
		// PI is approx 3.14159f. 
		// sin(PI) is 0, sin(PI/2) is 1, sin(0) is 0.
		float alpha = sinf(percent * 3.14159f);

		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);

		// Use the calculated Alpha
		AEGfxSetTransparency(alpha);

		AEGfxTextureSet(digipenLogo, 0.0f, 0.0f);

		AEMtx33 scale, trans, transform;
		AEMtx33Scale(&scale, 1500.0f, 420.0f);
		AEMtx33Trans(&trans, 0.0f, 0.0f);
		AEMtx33Concat(&transform, &trans, &scale);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(splashMesh, AE_GFX_MDM_TRIANGLES);
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
	// Unload main menu resources
	std::cout << "Startup: Unload\n";
}