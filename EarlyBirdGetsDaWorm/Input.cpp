#include "pch.hpp"

#include <windows.h> // Needed for ScreenToClient

// Internal variables to store mouse state
static s32 g_MouseX = 0;
static s32 g_MouseY = 0;

// Helper to access screen dimensions
// If these are not defined in a header, replace them with 1600 and 900
extern const int SCREEN_W;
extern const int SCREEN_H;

void Input_Update()
{
    // 1. Get Global Mouse Position (Monitor Space)
    POINT pt;
    GetCursorPos(&pt);

    // 2. Convert to Game Window Space
    // We use GetActiveWindow() to find the game window automatically
    ScreenToClient(GetActiveWindow(), &pt);

    g_MouseX = pt.x;
    g_MouseY = pt.y;
}

// --- KEYBOARD WRAPPERS ---

bool Input_IsKeyHeld(u8 key)
{
    return AEInputCheckCurr(key);
}

bool Input_IsKeyTriggered(u8 key)
{
    return AEInputCheckTriggered(key);
}

// --- MOUSE POSITION LOGIC ---

s32 Input_GetMouseWindowX()
{
    return g_MouseX;
}

s32 Input_GetMouseWindowY()
{
    return g_MouseY;
}

f32 Input_GetMouseWorldX()
{
    // Convert 0..1600 range to -800..800 range
    // Formula: MouseX - (Screen Width / 2)
    return (f32)g_MouseX - (SCREEN_W / 2.0f);
}

f32 Input_GetMouseWorldY()
{
    // Convert 0..900 range to 450..-450 range
    // Formula: (Screen Height / 2) - MouseY
    // We flip the subtraction because Screen Y goes DOWN, but World Y goes UP.
    return (SCREEN_H / 2.0f) - (f32)g_MouseY;
}

// --- MOUSE BUTTON WRAPPERS ---

bool Input_IsMouseHeld(u8 button)
{
    return AEInputCheckCurr(button);
}

bool Input_IsMouseTriggered(u8 button)
{
    return AEInputCheckTriggered(button);
}