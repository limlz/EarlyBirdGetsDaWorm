#include "pch.hpp"


void Quit_Load()
{
	// Load resources for the main menu
	std::cout << "Quit: Load\n";
}

void Quit_Initialize()
{
	// Initialize main menu variables
	int count = 3;
	std::cout << "Quit: Initialize\n";
}

void Quit_Update()
{
	// Update main menu logic
	//std::cout << "MainMenu: Update\n";
}

void Quit_Draw()
{
	// Draw main menu elements
	//std::cout << "MainMenu: Draw\n";
}

void Quit_Free()
{
	// Free main menu resources
	std::cout << "Quit: Free\n";
}


void Quit_Unload()
{
	AudioManager_Unload();
	// Unload main menu resources
	std::cout << "Quit: Unload\n";
}