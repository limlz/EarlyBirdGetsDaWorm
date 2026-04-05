#pragma once
#include "AEEngine.h"

// ============================================================================
// COLLISION & INPUT DETECTION
// ============================================================================

/******************************************************************************************
    @brief  Tests circle-circle collision using squared distance comparison.

    @param  x1, y1  Center of the first circle.
    @param  r1      Radius of the first circle.
    @param  x2, y2  Center of the second circle.
    @param  r2      Radius of the second circle.
    @return true if the circles overlap.
******************************************************************************************/
bool CircleCollision(f32 x1, f32 y1, f32 r1, f32 x2, f32 y2, f32 r2);

/******************************************************************************************
    @brief  Tests if a click position falls within a rectangular area.

    @param  area_center_x, area_center_y  Center of the rectangle.
    @param  area_width, area_height       Dimensions of the rectangle.
    @param  click_x, click_y              Position of the click to test.
    @return true if the click is inside the rectangle.
******************************************************************************************/
bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);

/******************************************************************************************
    @brief  Tests if the current mouse position falls within a rectangular area.

    @param  area_center_x, area_center_y  Center of the rectangle.
    @param  area_width, area_height       Dimensions of the rectangle.
    @return true if the mouse cursor is inside the rectangle.
******************************************************************************************/
bool IsAreaClickedByMouse(float area_center_x, float area_center_y, float area_width, float area_height);

/******************************************************************************************
    @brief  Tests if the mouse cursor is hovering over a rectangular area.

    @param  areaX, areaY  Center of the rectangle.
    @param  areaW, areaH  Dimensions of the rectangle.
    @return true if the mouse cursor is inside the rectangle.
******************************************************************************************/
bool IsAreaHovered(f32 areaX, f32 areaY, f32 areaW, f32 areaH);

// ============================================================================
// RENDERING UTILITIES
// ============================================================================

/******************************************************************************************
    @brief  Prints text horizontally centered at the given X coordinate.

    @param  fontId   Font handle returned by AEGfxCreateFont.
    @param  text     The string to render.
    @param  centerX  NDC X coordinate to center the text around.
    @param  y        NDC Y coordinate for the text baseline.
    @param  scale    Text scale multiplier.
    @param  r, g, b, a  Text color and alpha (0.0-1.0 each).
    @param  offset   Estimated character width in NDC at scale 1.0 (default 0.025f).
******************************************************************************************/
void AEGfxPrintCentered(s8 fontId, const std::string& text, float centerX, float y, float scale, float r, float g, float b, float a, float offset = 0.025f);

/******************************************************************************************
    @brief  Prints text with a colored glow effect by rendering offset copies behind it.

    @param  fontId            Font handle.
    @param  text              The string to render.
    @param  x, y              NDC coordinates for the text.
    @param  scale             Text scale multiplier.
    @param  textR, textG, textB, textA  Foreground text color.
    @param  glowR, glowG, glowB, glowA  Glow color.
    @param  glowOffset        Pixel offset for glow copies.
    @param  includeDiagonals  Whether to render diagonal glow copies (default true).
******************************************************************************************/
void AEGfxPrintWithGlow(s8 fontId, const char* text, float x, float y, float scale,
	float textR, float textG, float textB, float textA,
	float glowR, float glowG, float glowB, float glowA,
	float glowOffset, bool includeDiagonals = true);

/******************************************************************************************
    @brief  Converts pixel offsets to normalized screen coordinates for AEGfxPrint.

    @param  adjustX  Pixel X offset from screen center.
    @param  adjustY  Pixel Y offset from screen center.
    @return A pair of (ndcX, ndcY) coordinates.
******************************************************************************************/
std::pair<f32, f32> textPosition(float adjustX, float adjustY);

// ============================================================================
// ASSET MANAGEMENT
// ============================================================================

/******************************************************************************************
    @brief  Loads a texture from disk with error reporting. Asserts in debug builds
            if the file is missing or fails to load.

    @param  path    File path to the texture asset.
    @return The loaded texture pointer, or nullptr on failure in release builds.
******************************************************************************************/
AEGfxTexture* LoadTextureChecked(const char* path);

/******************************************************************************************
    @brief  Safely unloads a texture and sets the pointer to nullptr.

    @param  texture  Reference to the texture pointer to unload.
******************************************************************************************/
void UnloadTextureSafe(AEGfxTexture*& texture);

/******************************************************************************************
    @brief  Safely frees a mesh and sets the pointer to nullptr.

    @param  mesh  Reference to the mesh pointer to free.
******************************************************************************************/
void FreeMeshSafe(AEGfxVertexList*& mesh);

// ============================================================================
// UI HELPERS
// ============================================================================

/******************************************************************************************
    @brief  Draws a full-screen black overlay with the specified alpha for fade effects.

    @param  mesh   The square mesh used for rendering.
    @param  alpha  Opacity from 0.0f (invisible) to 1.0f (fully black).
******************************************************************************************/
void DrawBlackOverlay(AEGfxVertexList* mesh, float alpha);

/******************************************************************************************
    @brief  Smoothly interpolates a button's scale toward a hover or default target.

    @param  scale        Reference to the current scale value to modify.
    @param  hovering     Whether the button is currently being hovered.
    @param  hoverTarget  The target scale when hovered.
    @param  animSpeed    Interpolation speed multiplier.
    @param  dt           Frame delta time in seconds.
******************************************************************************************/
void UpdateButtonScale(float& scale, bool hovering, float hoverTarget, float animSpeed, float dt);

/******************************************************************************************
    @brief  Smoothly interpolates an alpha value between min and max based on hover state.

    @param  alpha     Reference to the current alpha value to modify.
    @param  hovered   Whether the element is currently hovered.
    @param  speed     Interpolation speed multiplier.
    @param  dt        Frame delta time in seconds.
    @param  minAlpha  Alpha value when not hovered.
    @param  maxAlpha  Alpha value when hovered.
******************************************************************************************/
void HoverAnimation(f32& alpha, bool hovered, f32 speed, f32 dt, f32 minAlpha, f32 maxAlpha);

/******************************************************************************************
    @brief  Tests AABB collision between two axis-aligned rectangles.

    @param  r1x, r1y, r1w, r1h  Center and dimensions of rectangle 1.
    @param  r2x, r2y, r2w, r2h  Center and dimensions of rectangle 2.
    @return true if the rectangles overlap.
******************************************************************************************/
bool IsColliding(float r1x, float r1y, float r1w, float r1h,
	float r2x, float r2y, float r2w, float r2h);

extern float worldMouseX;
extern float worldMouseY;

// ============================================================================
// RNG UTILITIES
// ============================================================================

/******************************************************************************************
    @brief  Returns the global Mersenne Twister RNG instance (function-local static,
            avoiding the static initialization order fiasco).

    @return Reference to the shared mt19937 engine.
******************************************************************************************/
std::mt19937& GetGlobalRng();

/******************************************************************************************
    @brief  Generates a uniformly distributed random integer in [min, max].

    @param  min  Lower bound (inclusive).
    @param  max  Upper bound (inclusive).
    @return A random integer in the specified range.
******************************************************************************************/
int RandInt(int min, int max);

/******************************************************************************************
    @brief  Generates a uniformly distributed random float in [min, max].

    @param  min  Lower bound (inclusive).
    @param  max  Upper bound (inclusive).
    @return A random float in the specified range.
******************************************************************************************/
float RandFloat(float min, float max);
