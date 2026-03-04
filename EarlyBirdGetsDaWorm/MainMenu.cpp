#include "pch.hpp"
#include <fstream>   // lets us open and read text files from disk
#include <string>    // gives us std::string for text manipulation
#include <vector>    // gives us std::vector, a resizable list
#include <cstdlib>   // gives us rand() for random numbers

// A single line of text in the credits scroll.
// We store the raw text, how big to render it, and whether it's a header (title/section) or a normal line.
struct CreditLine {
    std::string text;
    float       scale;    // 2.0 = big title, 1.5 = section header, 0.8 = normal name
    bool        isHeader; // headers get extra vertical spacing above them
};

// How many seconds the black fade-in takes when the menu first appears
static constexpr float FADE_IN_DURATION = 0.3f;
// How fast overlays and fades animate - higher number = snappier
static constexpr float FADE_OUT_SPEED   = 3.0f;
// The title card plays through 13 frames like a mini cutscene
static constexpr int   NUM_TITLE_FRAMES = 13;
// How long each title frame stays on screen before advancing to the next one (in seconds)
static constexpr float TITLE_ANIM_SPEED = 0.2f;

// The quad mesh we use to draw every rectangle on screen (buttons, overlays, backgrounds, etc.)
static AEGfxVertexList* squareMesh;

// All the textures we need for this screen - loaded once, drawn every frame
static AEGfxTexture* main_menu_bg       = nullptr; // the full background image
static AEGfxTexture* admit_patient_tag  = nullptr; // the tag shape behind the admit button
static AEGfxTexture* admit_patient_text = nullptr; // the text sitting on top of the admit tag
static AEGfxTexture* options_text       = nullptr; // options button text layer
static AEGfxTexture* options_tag        = nullptr; // options button backing shape
static AEGfxTexture* quit_tag           = nullptr; // quit button backing shape
static AEGfxTexture* quit_text          = nullptr; // quit button text layer
static AEGfxTexture* title_bg           = nullptr; // logo/title background layer
// The little character in the corner has two poses - one for idle, one when the player hovers over it
static AEGfxTexture* credits_guy        = nullptr; // idle pose - looking down
static AEGfxTexture* credits_guy2       = nullptr; // hover pose - looking up
// Each frame of the animated title card stored as its own texture
static AEGfxTexture* title_frames[NUM_TITLE_FRAMES];

// Font handle used to draw all text in the credits overlay. -1 means not loaded yet.
s8   menuFontId = 0;
// Leftover from earlier code - kept so nothing breaks if it's referenced elsewhere
bool first_load = true;

// fade_in_timer counts DOWN from FADE_IN_DURATION to 0.
// While it's above 0, the screen is still fading in from black.
static float fade_in_timer    = FADE_IN_DURATION;
// Set this to true when a button is clicked - triggers the black fade-out before changing state
static bool  isFadingOut      = false;
// Tracks how opaque the black fade-out rectangle is (0 = invisible, 1 = fully black)
static float fade_out_alpha   = 0.0f;
// We save the target game state here so we can switch to it AFTER the fade finishes
static int   pendingNextState = 0;

// World position of the swinging ceiling light on the menu screen
static float menuLightX = -200.0f;
static float menuLightY =  500.0f;

// Each button has its own scale float.
// When hovered it lerps up to 1.15 (slightly bigger), and back down to 1.0 when not.
static float admit_scale   = 1.0f;
static float options_scale = 1.0f;
static float quit_scale    = 1.0f;

// True if the mouse cursor is currently inside the credits character's clickable area
static bool  isHoveringCredits       = false;

// These X/Y offsets get added to the credits character's draw position to fake a twitch/jitter effect
static float credits_jitter_x        = 0.0f;
static float credits_jitter_y        = 0.0f;

// How much longer the current idle twitch should last before stopping
static float credits_twitch_duration = 0.0f;

// Counts down between idle twitches. When it reaches 0 a new twitch fires.
static float credits_idle_timer      = 2.0f;

// Prevents us from picking a new random jitter offset every single frame - only updates when this hits 0
static float jitter_update_timer     = 0.0f;

// Which title animation frame we're currently showing (0 = first frame)
static int   currentTitleFrame = 0;

// Accumulates delta time each frame; when it exceeds TITLE_ANIM_SPEED we advance to the next frame
static float titleAnimTimer    = 0.0f;

// Whether the options panel is currently open
static bool  isOptionsOpen       = false;
// 0 = fully invisible, 0.8 = fully visible (we cap it at 0.8 so the menu is still faintly visible behind it)
static float optionsOverlayAlpha = 0.0f;
// Player's chosen volume, ranging from 0.0 (silent) to 1.0 (full volume)
static float masterVolume        = 1.0f;
// True while the player holds down the mouse button on the volume slider
static bool  isDraggingVolume    = false;

// The full list of parsed credit lines, filled once when the file is loaded
static std::vector<CreditLine> parsedCredits;
// Whether the credits overlay is currently visible
static bool  isCreditsOpen       = false;
// Same deal as options alpha - 0 = hidden, up to 0.9 when fully open
static float creditsOverlayAlpha = 0.0f;
// The Y position where the first credit line starts drawing.
// Starts below the screen and moves upward over time to create the scrolling effect.
static float creditsScrollY      = -500.0f;

// Generic helper that fades 'alpha' toward its target.
// If 'open' is true it fades IN (increases), if false it fades OUT (decreases).
// maxVal lets each overlay have its own maximum opacity.
static inline void FadeAlpha(float& alpha, bool open, float dt, float maxVal = 1.0f)
{
    // Move alpha in the right direction based on whether we're opening or closing
    alpha += (open ? 1.0f : -1.0f) * FADE_OUT_SPEED * dt;
    // Make sure it never goes below 0 (invisible) or above maxVal (fully opaque)
    alpha  = AEClamp(alpha, 0.0f, maxVal);
}

// Draws a solid black rectangle that covers the entire screen at the given opacity.
// Used for: fade-in on entry, fade-out on exit, and darkening the background behind overlays.
static void DrawBlackOverlay(float alpha)
{
    // Nothing to draw if it's fully transparent - skip the work entirely
    if (alpha <= 0.0f) return;

    // Set up blending so the black quad respects transparency
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR); // draw in flat color mode, no texture
    AEGfxSetTransparency(alpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // pure black

    // Build a transform matrix that stretches the unit quad to fill the 1600x900 viewport
    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, 1600.0f, 900.0f); // make it fill the screen
    AEMtx33Trans(&trans, 0.0f, 0.0f);      // centered at the origin
    AEMtx33Concat(&transform, &trans, &scale); // combine scale + translation into one matrix
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
}

// Strips an HTML-style open and close tag from a string, leaving just the inner text.
// Example: "<h1>Credits</h1>" becomes "Credits"
static void StripTag(std::string& s, const char* open, const char* close)
{
    // Find and erase the opening tag (e.g. "<h1>")
    auto pos = s.find(open);
    if (pos != std::string::npos) s.erase(pos, strlen(open));
    // Find and erase the closing tag (e.g. "</h1>")
    pos = s.find(close);
    if (pos != std::string::npos) s.erase(pos, strlen(close));
}

// Opens Assets/credits.txt, reads it line by line, and fills the parsedCredits list.
// The text file uses simple HTML-like tags (<h1>, <h2>, <p>) to control text size.
void LoadCreditsFile()
{
    // Clear any previously loaded credits in case this gets called more than once
    parsedCredits.clear();
    std::ifstream file("Assets/credits.txt");

    // If we can't open the file, push a visible error message so it's obvious something went wrong
    if (!file.is_open()) {
        parsedCredits.push_back({ "ERROR",                             2.0f, true  });
        parsedCredits.push_back({ "Could not find Assets/credits.txt", 0.8f, false });
        return;
    }

    // Tag lookup table - maps each tag to a text scale and whether it needs extra spacing
    struct TagDef { const char* open; const char* close; float scale; bool isHeader; };
    static constexpr TagDef kTags[] = {
        { "<h1>", "</h1>", 2.0f, true  }, // big title 
        { "<h2>", "</h2>", 1.5f, true  }, // section header 
        { "<p>",  "</p>",  0.8f, false }, // regular credit line
    };

    std::string line;
    while (std::getline(file, line)) {
        // Start each line with default paragraph styling in case no tag is found
        CreditLine cl{ line, 0.8f, false };

        // Check if this line contains any of our known tags and update the style if so
        for (const auto& tag : kTags) {
            if (line.find(tag.open) != std::string::npos) {
                cl.scale    = tag.scale;
                cl.isHeader = tag.isHeader;
                StripTag(line, tag.open, tag.close); // remove the tags so they don't show on screen
                cl.text = line;
                break; // a line can only have one tag type - stop checking once we find it
            }
        }

        parsedCredits.push_back(cl);
    }
}

void MainMenu_Load()
{
    // Load every texture we need - LoadTextureChecked prints a warning if the file is missing
    main_menu_bg       = LoadTextureChecked(Assets::Main_Menu::Background);
    admit_patient_tag  = LoadTextureChecked(Assets::Main_Menu::AdmitPatientTag);
    admit_patient_text = LoadTextureChecked(Assets::Main_Menu::AdmitPatientText);
    options_text       = LoadTextureChecked(Assets::Main_Menu::OptionsText);
    options_tag        = LoadTextureChecked(Assets::Main_Menu::OptionsTag);
    quit_tag           = LoadTextureChecked(Assets::Main_Menu::QuitTag);
    quit_text          = LoadTextureChecked(Assets::Main_Menu::QuitText);
    title_bg           = LoadTextureChecked(Assets::Main_Menu::TitleBg);
    credits_guy        = LoadTextureChecked(Assets::Main_Menu::CreditsDown);
    credits_guy2       = LoadTextureChecked(Assets::Main_Menu::CreditsUp);

    // Parse the credits text file now so it's ready instantly when the player clicks credits
    LoadCreditsFile();

    // The title animation frames are numbered files
    // Build the full path for each frame and load it
    for (int i = 0; i < NUM_TITLE_FRAMES; i++) {
        std::string path = std::string(Assets::Main_Menu::TitleFramePrefix)
                         + std::to_string(i + 1)           // frame numbers start at 1, not 0
                         + Assets::Main_Menu::TitleFrameSuffix;
        title_frames[i] = LoadTextureChecked(path.c_str());
    }

    menuFontId = AEGfxCreateFont(Assets::Fonts::Buggy, 20); // load the font at size 20
    std::cout << "MainMenu: Load\n";
}

void MainMenu_Initialize()
{	
	AudioManager_PlayBGM(BGM_MAIN_MENU, 0.05f);
	AudioManager_PlaySFX(SFX_MAIN_MENU_WRITING_SCRATCH, 0.05f);
    Lighting_Initialize(0); // set up the flickering ceiling lights (floor 0 = lobby/menu)
    Particles_Initialize(); // set up the dust/spark particle system
    squareMesh = CreateSquareMesh(0xFFFFFFFF); // create a plain white unit quad we reuse everywhere

    // Always start with the screen fading IN from black so the transition feels clean
    fade_in_timer    = FADE_IN_DURATION;
    isFadingOut      = false;
    fade_out_alpha   = 0.0f;
    pendingNextState = 0; // no destination state yet - will be set when a button is clicked

    // Make sure the credits overlay is fully closed and scrolled back to the start
    isCreditsOpen       = false;
    creditsOverlayAlpha = 0.0f;
    creditsScrollY      = -500.0f; // off the bottom of the screen

    // Reset the credits character jitter so it doesn't carry over from a previous session
    isHoveringCredits       = false;
    credits_jitter_x        = 0.0f;
    credits_jitter_y        = 0.0f;
    credits_twitch_duration = 0.0f;
    credits_idle_timer      = 2.0f; // wait 2 seconds before the first idle twitch

    std::cout << "MainMenu: Initialize\n";
}


void MainMenu_Free() { 
    AudioManager_StopBGM(); 
    AudioManager_StopSFX(); 
}

void MainMenu_Unload()
{
    // Free the quad mesh from GPU memory
    FreeMeshSafe(squareMesh);

    // Unload every texture - safe version checks for null so we don't crash on a failed load
    UnloadTextureSafe(main_menu_bg);
    UnloadTextureSafe(admit_patient_tag);
    UnloadTextureSafe(admit_patient_text);
    UnloadTextureSafe(options_tag);
    UnloadTextureSafe(options_text);
    UnloadTextureSafe(quit_tag);
    UnloadTextureSafe(quit_text);
    UnloadTextureSafe(title_bg);
    UnloadTextureSafe(credits_guy);
    UnloadTextureSafe(credits_guy2);

    // Unload all 13 title animation frames in a loop
    for (int i = 0; i < NUM_TITLE_FRAMES; i++) UnloadTextureSafe(title_frames[i]);

    // Destroy the font handle if it was successfully created
    if (menuFontId >= 0) { AEGfxDestroyFont(menuFontId); menuFontId = -1; }

    Particles_Free(); // release particle system memory
    std::cout << "MainMenu: Unload\n";
}

void MainMenu_Update()
{
    // Delta time = how many seconds passed since the last frame (e.g. 0.016 at 60fps)
    float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());

    // If we're fading out (a button was clicked), keep darkening the screen.
    // Once it's fully black, actually switch to the new game state.
    if (isFadingOut) {
        fade_out_alpha += FADE_OUT_SPEED * dt;
        if (fade_out_alpha >= 1.0f) { fade_out_alpha = 1.0f; next = pendingNextState; }
        return; // skip all other updates while fading out
    }

    // Count the fade-in timer down to zero. AEMax stops it going negative.
    if (fade_in_timer > 0.0f)
        fade_in_timer = AEMax(fade_in_timer - dt, 0.0f);

    // Advance the title card animation, but stop on the last frame (don't loop)
    if (currentTitleFrame < NUM_TITLE_FRAMES - 1) {
        titleAnimTimer += dt;
        if (titleAnimTimer >= TITLE_ANIM_SPEED) {
            titleAnimTimer = 0.0f;
            currentTitleFrame++; // move to the next frame
        }
    }

    Particles_Update(); // move and age all active dust/spark particles
    Update_StandaloneLight(dt, menuLightX, menuLightY); // flicker the hanging ceiling light

    // CREDITS OVERLAY - when open, block all other menu interaction
    if (isCreditsOpen) {
        FadeAlpha(creditsOverlayAlpha, true, dt, 0.9f); // fade the dark backdrop in
        creditsScrollY += 60.0f * dt; // scroll the text upward at 60 pixels per second
        // Close the credits if the player presses Escape or clicks anywhere
        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON))
            isCreditsOpen = false;
        return; // don't process menu buttons while credits are open
    }
    FadeAlpha(creditsOverlayAlpha, false, dt, 0.9f); // fade the backdrop back out when closed

    // OPTIONS OVERLAY - same pattern: block menu while open, fade out when closed
    if (isOptionsOpen) {
        FadeAlpha(optionsOverlayAlpha, true, dt, 0.8f); // fade the dark backdrop in (slightly less dark than credits)
        if (AEInputCheckTriggered(AEVK_ESCAPE)) isOptionsOpen = false; // Escape closes the panel

        // If the player clicks on the slider track area, start dragging
        if (AEInputCheckTriggered(AEVK_LBUTTON) && IsAreaClickedByMouse(-160.0f, 15.0f, 320.0f, 30.0f))
            isDraggingVolume = true;
        // Release the drag when the mouse button is lifted
        if (AEInputCheckReleased(AEVK_LBUTTON)) isDraggingVolume = false;

        // While dragging, convert the mouse's X position into a 0.0-1.0 volume value.
        // The slider track runs from X=-150 to X=150 (300 pixels wide).
        if (isDraggingVolume)
            masterVolume = AEClamp((Input_GetMouseX() + 150.0f) / 300.0f, 0.0f, 1.0f);

        return; // don't process menu buttons while options are open
    }
    FadeAlpha(optionsOverlayAlpha, false, dt, 0.8f); // fade the backdrop out when closed

    // Check whether the mouse is currently inside each button's hitbox
    bool hoverAdmit   = IsAreaClickedByMouse( 480.0f,  270.0f, 490.0f, 290.0f);
    bool hoverOptions = IsAreaClickedByMouse( 500.0f,   15.0f, 500.0f, 200.0f);
    bool hoverQuit    = IsAreaClickedByMouse( 490.0f, -210.0f, 490.0f, 190.0f);
    isHoveringCredits = IsAreaClickedByMouse(-600.0f, -350.0f, 300.0f, 100.0f);

    // Smoothly scale each button toward 1.15x when hovered, back to 1.0x when not.
    // The lerp speed of 15 makes it feel quick but not instant.
    float animSpeed = 15.0f;
    admit_scale   += ((hoverAdmit   ? 1.15f : 1.0f) - admit_scale)   * animSpeed * dt;
    options_scale += ((hoverOptions ? 1.15f : 1.0f) - options_scale) * animSpeed * dt;
    quit_scale    += ((hoverQuit    ? 1.15f : 1.0f) - quit_scale)    * animSpeed * dt;

    // CREDITS CHARACTER JITTER
    // When hovered: shake rapidly side to side (looks nervous/scared)
    // When idle: occasionally do a small random twitch, then go still again
    jitter_update_timer -= dt; // count down to the next jitter update

    if (isHoveringCredits) {
        if (jitter_update_timer <= 0.0f) {
            // Pick a random X offset between -12 and +12 pixels
            credits_jitter_x    = ((rand() % 200 - 100) / 100.0f) * 6.0f;
            credits_jitter_y    = 0.0f; // no vertical movement - just side to side
            jitter_update_timer = 0.06f; // update ~16 times per second for frantic jitter
        }
    } else {
        // Not hovering - countdown to the next idle twitch
        if (credits_idle_timer > 0.0f) {
            credits_idle_timer -= dt;
            credits_jitter_x = credits_jitter_y = 0.0f; // stay perfectly still while waiting
        } else {
            // Timer ran out - trigger a short twitch and reset the idle wait
            credits_twitch_duration = 0.3f;                            // twitch lasts 0.3 seconds
            credits_idle_timer = ((rand() % 300) / 100.0f) + 1.5f;    // next twitch in 1.5-4.5 seconds
        }

        if (credits_twitch_duration > 0.0f) {
            credits_twitch_duration -= dt;
            if (jitter_update_timer <= 0.0f) {
                // Smaller jitter than hover - just a subtle flinch
                credits_jitter_x    = ((rand() % 200 - 100) / 100.0f) * 5.0f; // ±5 pixels
                credits_jitter_y    = 0.0f;
                jitter_update_timer = 0.05f; // slower update rate than hover jitter
            }
        } else {
            // Twitch finished - go back to standing still
            credits_jitter_x = credits_jitter_y = 0.0f;
        }
    }

    // Handle button clicks - only react on the frame the button is first pressed
    if (AEInputCheckTriggered(AEVK_LBUTTON)) {
        if (hoverAdmit) { pendingNextState = GAME_STATE; isFadingOut = true; } // start the game
        else if (hoverOptions)      { isOptionsOpen = true; }                              // open options panel
        else if (hoverQuit)         { pendingNextState = GS_QUIT; isFadingOut = true; }    // quit the game
        else if (isHoveringCredits) { isCreditsOpen = true; creditsScrollY = -600.0f; }   // open credits, reset scroll to bottom
    }
}

void MainMenu_Draw()
{
    // Draw the background layers from back to front so they stack correctly
    DrawTextureMesh(squareMesh, main_menu_bg, 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f); // furthest back
    DrawTextureMesh(squareMesh, title_bg,     0.0f, 0.0f, 1600.0f, 900.0f, 1.0f); // logo layer on top of BG
    // Draw whichever title animation frame we're currently on (if it loaded successfully)
    if (title_frames[currentTitleFrame])
        DrawTextureMesh(squareMesh, title_frames[currentTitleFrame], 0.0f, 0.0f, 1600.0f, 900.0f, 1.0f);

    // Pick the right credits character texture based on hover state, then offset it by the jitter values
    if (AEGfxTexture* creditTex = isHoveringCredits ? credits_guy2 : credits_guy)
        DrawTextureMesh(squareMesh, creditTex,
            -680.0f + credits_jitter_x,  // base X position + random jitter
            -250.0f + credits_jitter_y,  // base Y position (Y jitter is always 0 but kept for flexibility)
            400.0f, 600.0f, 1.0f);

    Draw_StandaloneConeLight(menuLightX, menuLightY); // draw the cone of light from the ceiling lamp
    Particles_Draw(squareMesh, 0.0f);                 // draw any active dust/spark particles

    // Each button is two layered textures: a tag shape behind, and text on top.
    // Both use the same scale so they grow together on hover.
    DrawTextureMesh(squareMesh, admit_patient_tag,  490.0f,  280.0f, 500.0f * admit_scale,   300.0f * admit_scale,   1.0f);
    DrawTextureMesh(squareMesh, admit_patient_text, 470.0f,  280.0f, 400.0f * admit_scale,   200.0f * admit_scale,   1.0f);
    DrawTextureMesh(squareMesh, options_tag,        500.0f,   15.0f, 500.0f * options_scale, 200.0f * options_scale, 1.0f);
    DrawTextureMesh(squareMesh, options_text,       500.0f,   15.0f, 400.0f * options_scale, 150.0f * options_scale, 1.0f);
    DrawTextureMesh(squareMesh, quit_tag,           500.0f, -220.0f, 500.0f * quit_scale,    200.0f * quit_scale,    1.0f);
    DrawTextureMesh(squareMesh, quit_text,          515.0f, -220.0f, 400.0f * quit_scale,    150.0f * quit_scale,    1.0f);

    // Figure out which fade is active right now and draw ONE black overlay for it.
    // During fade-in: alpha goes from 1 down to 0 as fade_in_timer counts to zero.
    // During fade-out: alpha goes from 0 up to 1 as fade_out_alpha increases.
    float fadeAlpha = (fade_in_timer > 0.0f) ? (fade_in_timer / FADE_IN_DURATION)
                    : (isFadingOut           ?  fade_out_alpha : 0.0f);
    DrawBlackOverlay(fadeAlpha);

    // OPTIONS OVERLAY
    // First darken the background, then once it's dark enough, draw the slider UI on top
    DrawBlackOverlay(optionsOverlayAlpha);
    if (optionsOverlayAlpha >= 0.5f) { // wait until backdrop is half-visible before drawing controls
        AEGfxSetTransparency(1.0f); // draw the slider elements fully opaque

        float fillWidth  = 300.0f * masterVolume;          // how wide the filled (white) part of the bar is
        float fillStartX = -150.0f + (fillWidth / 2.0f);   // DrawSquareMesh draws from center, so offset accordingly
        float handleX    = -150.0f + (300.0f * masterVolume); // the draggable handle sits exactly at the volume position

        DrawSquareMesh(squareMesh, 0.0f,       0.0f, 300.0f,    20.0f, 0xFF444444); // dark grey track background
        DrawSquareMesh(squareMesh, fillStartX, 0.0f, fillWidth, 20.0f, COLOR_WHITE); // white fill showing current volume
        DrawSquareMesh(squareMesh, handleX,    0.0f, 15.0f,     40.0f, 0xFF0000FF);  // blue draggable handle
    }

    // CREDITS OVERLAY
    // Darken the background, then scroll the credit lines upward across the screen
    DrawBlackOverlay(creditsOverlayAlpha);
    if (menuFontId >= 0 && creditsOverlayAlpha >= 0.5f) { // only draw text once the backdrop is visible enough
        AEGfxSetTransparency(1.0f); // draw all text fully opaque
        float currentY = creditsScrollY; // start drawing from the current scroll position

        for (const auto& line : parsedCredits) {
            // Headers get a gap above them to visually separate sections
            if (line.isHeader) currentY -= 40.0f;

            // Only bother drawing lines that are actually on screen right now
            if (currentY >= -600.0f && currentY <= 600.0f && !line.text.empty()) {
                // Convert our pixel Y position into NDC (-1 to 1) that the font renderer expects
                AEGfxPrint(menuFontId, line.text.c_str(), -0.8f, currentY / 450.0f, line.scale, 1.0f, 1.0f, 1.0f, 1.0f);
            }

            // Move down by an amount proportional to the line's scale so bigger text takes more space
            currentY -= 40.0f * line.scale;
        }
    }
}