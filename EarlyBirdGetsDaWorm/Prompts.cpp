// Prompts.cpp
#include "pch.hpp"

// Internal State
enum PromptType {
    PROMPT_NONE,
    PROMPT_WRONG_ROOM, // Priority High
    PROMPT_LIFT,       // Priority Medium
    PROMPT_ENTER       // Priority Low
};

static PromptType currentPrompt = PROMPT_NONE;
static s8 promptFontId = 0;

// Timer for feedback messages
static float messageTimer = 0.0f;
const float MESSAGE_DURATION = 2.0f; // Message stays for 2 seconds

void Prompts_Load() {
    promptFontId = AEGfxCreateFont("Assets/buggy-font.ttf", 20);
}

// Function called by Game.cpp when logic fails
void Prompts_TriggerWrongRoom() {
    messageTimer = MESSAGE_DURATION;
}

void Prompts_Update(float dt, float camX, int doorNumAtPlayer, bool liftMenuOpen, bool isLiftRange) {
    currentPrompt = PROMPT_NONE;

    // --- PRIORITY 0: Feedback Messages (Timed) ---
    // If the timer is running, show the feedback message and IGNORE everything else
    if (messageTimer > 0.0f) {
        messageTimer -= dt;
        currentPrompt = PROMPT_WRONG_ROOM;
        return;
    }

    // --- PRIORITY 1: Lift Prompt ---
    if (isLiftRange && !liftMenuOpen) {
        currentPrompt = PROMPT_LIFT;
        return;
    }

    // --- PRIORITY 2: Door Prompt ---
    if (doorNumAtPlayer != -1) {
        currentPrompt = PROMPT_ENTER;
    }
}

void Prompts_Draw() {
    if (currentPrompt == PROMPT_NONE) return;

    switch (currentPrompt) {
    case PROMPT_WRONG_ROOM:
        // Draw in Red (R, G, B, A) -> (1, 0, 0, 1) to indicate error
        AEGfxPrint(promptFontId, "That's not the right room...", -0.25f, 0.6f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
        break;
    case PROMPT_LIFT:
        AEGfxPrint(promptFontId, "Click L to access lift!", -0.20f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        break;
    case PROMPT_ENTER:
        AEGfxPrint(promptFontId, "Press E to enter the room", -0.20f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        break;
    }
}

void Prompts_Unload() {
    AEGfxDestroyFont(promptFontId);
}