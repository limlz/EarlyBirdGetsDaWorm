// Prompts.cpp
#include "pch.hpp"
#include "Prompts.hpp"

// Internal State
enum PromptType {
    PROMPT_NONE,
    PROMPT_LIFT,
    PROMPT_ENTER
};

static PromptType currentPrompt = PROMPT_NONE;
static s8 promptFontId = 0; // Dedicated font ID for prompts

void Prompts_Load() {
    // Load the font here so this module is self-contained
    promptFontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
}

void Prompts_Update(float camX, int doorNumAtPlayer, bool liftMenuOpen, bool isLiftRange) {
    currentPrompt = PROMPT_NONE;

    // Priority 1: Lift Prompt
    // Only show if we are in range AND the menu isn't already open
    if (isLiftRange && !liftMenuOpen) {
        currentPrompt = PROMPT_LIFT;
        return; // Return early so we don't show two prompts at once
    }

    // Priority 2: Door Prompt
    if (doorNumAtPlayer != -1) {
        currentPrompt = PROMPT_ENTER;
    }
}

void Prompts_Draw() {
    if (currentPrompt == PROMPT_NONE) return;

    // Use a switch to draw the correct text
    switch (currentPrompt) {
    case PROMPT_LIFT:
        AEGfxPrint(promptFontId, "Click L to access lift!", -0.20f, 0.6f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        break;
    case PROMPT_ENTER:
        AEGfxPrint(promptFontId, "Press E to enter the room", -0.20f, 0.6f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        break;
    }
}

void Prompts_Unload() {
    AEGfxDestroyFont(promptFontId);
}