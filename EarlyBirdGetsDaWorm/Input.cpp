#include "pch.hpp" // Ensure this includes AEEngine.h
#include "Input.hpp"

// Private global variables to store the converted coordinates
static float g_mouseX = 0.0f;
static float g_mouseY = 0.0f;

void Input_Update()
{
    // AlphaEngine usually outputs to s32 (integers) for raw screen pixels
    s32 rawX, rawY;
    AEInputGetCursorPosition(&rawX, &rawY);

    // Convert to NDC and store as floats
    g_mouseX = (float)rawX - 800.0f;
    g_mouseY = 450.0f - (float)rawY;
}

float Input_GetMouseX()
{
    return g_mouseX;
}

float Input_GetMouseY()
{
    return g_mouseY;
}