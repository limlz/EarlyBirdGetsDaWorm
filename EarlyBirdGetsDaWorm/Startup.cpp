#include "pch.hpp"


static f64 splash_screen_timer{};
static f64 title_screen_timer{};

void Startup_Load()
{
	// Load resources for the main menu
	std::cout << "Startup: Load\n";
}

void Startup_Initialize()
{
	// Initialize main menu variables
	splash_screen_timer = 1.0f;
	title_screen_timer = 1.0f;

	std::cout << "Timer: " << splash_screen_timer << "\n";
	std::cout << "Startup: Initialize\n";
}

void Startup_Update()
{

	if (splash_screen_timer > 0)
	{
		splash_screen_timer -= AEFrameRateControllerGetFrameTime();
		std::cout << "SplashScreen: " << splash_screen_timer << "\n";
		return;
	} 
	else if (title_screen_timer > 0)
	{
		title_screen_timer -= AEFrameRateControllerGetFrameTime();
		std::cout << "TitleScreen: " << title_screen_timer << "\n";
		return;
	}
	{
		next = MAIN_MENU; // Transition to main menu state
	}
}

void Startup_Draw()
{
	if (splash_screen_timer > 0)
	{
		AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);
	}
	else if (title_screen_timer > 0)
	{
		AEGfxSetBackgroundColor(1.0f, 0.0f, 1.0f);
	}
	// Draw main menu elements
	//std::cout << "MainMenu: Draw\n";
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