#include "pch.hpp"
#include "pause_menu.hpp"

// --- Textures ---
static AEGfxTexture* pauseTitleTex = nullptr;
static AEGfxTexture* btnResumeTex = nullptr;
static AEGfxTexture* btnOptionsTex = nullptr;
static AEGfxTexture* btnQuitTex = nullptr;

// --- Variables ---
static bool gamePaused = false;
static bool isUnpausing = false; // Tracks the slide-out animation
static float pauseAlpha = 0.0f;
const float FADE_SPEED = 3.0f;

// --- Menu Animation Variables ---
static float pauseMenuTimer = 0.0f;
static float titleX = -1200.0f;
static float resumeX = -1200.0f;
static float optionsX = -1200.0f;
static float quitX = -1200.0f;

// --- HOVER SCALE VARIABLES ---
static float resumeScale = 1.0f;
static float optionsScale = 1.0f;
static float quitScale = 1.0f;

// --- Procedural Blood Splatter ---
struct BloodDrop {
	float x, y;
	float velX, velY;    // Directional speed
	float friction;      // How quickly it stops sliding
	float targetScaleX, targetScaleY;
	float currentScaleX, currentScaleY;
	float delayTimer;    // Wait this long before spawning (creates bursts)
	float flyTimer;      // Wait this long before hitting the screen and splattering
	bool isActive;
};

const int MAX_BLOOD_DROPS = 5000;
static BloodDrop bloodDrops[MAX_BLOOD_DROPS];

// ==========================================
// Load Textures
// ==========================================
void PauseMenu_Load()
{
	pauseTitleTex = LoadTextureChecked("Assets/Pause_Menu/pause_title.png");
	btnResumeTex = LoadTextureChecked("Assets/Pause_Menu/resume_button.png");
	btnOptionsTex = LoadTextureChecked("Assets/Pause_Menu/options_button.png");
	btnQuitTex = LoadTextureChecked("Assets/Pause_Menu/quit_button.png");
}

void PauseMenu_Initialize()
{
	gamePaused = false;
	isUnpausing = false;
	pauseAlpha = 0.0f;
	pauseMenuTimer = 0.0f;

	titleX = -1200.0f;
	resumeX = -1200.0f;
	optionsX = -1200.0f;
	quitX = -1200.0f;

	resumeScale = 1.0f;
	optionsScale = 1.0f;
	quitScale = 1.0f;
}

void PauseMenu_Update(float dt)
{
	float mx = Input_GetMouseX();
	float my = Input_GetMouseY();

	// --- ESCAPE KEY LOGIC ---
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		if (!gamePaused) {
			// Start Pause
			gamePaused = true;
			isUnpausing = false;
			pauseMenuTimer = 0.0f;

			// Reset UI positions back off-screen
			titleX = -1200.0f;
			resumeX = -1200.0f;
			optionsX = -1200.0f;
			quitX = -1200.0f;

			// Reset scales
			resumeScale = 1.0f;
			optionsScale = 1.0f;
			quitScale = 1.0f;

			// --- Generate Directional Splatter ---
			for (int i = 0; i < MAX_BLOOD_DROPS; i++) {
				bloodDrops[i].x = -1000.0f - (rand() % 400);
				bloodDrops[i].y = (rand() % 1100) - 550.0f;

				bloodDrops[i].velX = (float)((rand() % 3000) + 2000);
				bloodDrops[i].velY = (float)((rand() % 1400) - 700);

				bloodDrops[i].friction = (float)((rand() % 8) + 15);

				float baseScale = (float)((rand() % 5) + 5);

				bloodDrops[i].targetScaleX = baseScale * ((rand() % 4) + 1.0f);
				bloodDrops[i].targetScaleY = baseScale * 1.0f;

				bloodDrops[i].currentScaleX = 0.0f;
				bloodDrops[i].currentScaleY = 0.0f;

				int burstGroup = rand() % 4;
				bloodDrops[i].delayTimer = burstGroup * 0.10f;
				bloodDrops[i].flyTimer = (float)((rand() % 20) + 5) / 100.0f;
				bloodDrops[i].isActive = false;
			}
		}
		else if (!isUnpausing) {
			// Trigger slide out animation
			isUnpausing = true;

			// --- REVERSE BLOOD SPLATTER ---
			for (int i = 0; i < MAX_BLOOD_DROPS; i++) {
				bloodDrops[i].isActive = true;   // Force activate all waiting drops
				bloodDrops[i].flyTimer = 0.0f;   // Skip flying state
				bloodDrops[i].velX = -(float)((rand() % 4000) + 3000); // Shoot leftwards rapidly!
				bloodDrops[i].friction = 0.0f;   // Turn off friction so they don't stop
			}
		}
	}

	// --- ACTIVE PAUSE MENU LOGIC ---
	if (gamePaused) {
		pauseMenuTimer += dt;

		if (!isUnpausing) {
			// --- SLIDE IN ---
			if (pauseMenuTimer > 0.35f) titleX += (-475.0f - titleX) * 15.0f * dt;
			if (pauseMenuTimer > 0.45f) resumeX += (-600.0f - resumeX) * 15.0f * dt;
			if (pauseMenuTimer > 0.55f) optionsX += (-600.0f - optionsX) * 15.0f * dt;
			if (pauseMenuTimer > 0.65f) quitX += (-650.0f - quitX) * 15.0f * dt;

			pauseAlpha += FADE_SPEED * dt;
			if (pauseAlpha > 0.8f) pauseAlpha = 0.8f;
		}
		else {
			// --- SLIDE OUT (UNPAUSING) ---
			titleX += (-1200.0f - titleX) * 15.0f * dt;
			resumeX += (-1200.0f - resumeX) * 15.0f * dt;
			optionsX += (-1200.0f - optionsX) * 15.0f * dt;
			quitX += (-1200.0f - quitX) * 15.0f * dt;

			pauseAlpha -= FADE_SPEED * dt;

			// Once fully faded out, actually unpause the game!
			if (pauseAlpha <= 0.0f) {
				pauseAlpha = 0.0f;
				gamePaused = false;
				isUnpausing = false;
			}
		}

		// --- HOVER LOGIC & SCALING ---
		bool hoverResume = false, hoverOptions = false, hoverQuit = false;

		// Only interact if NOT unpausing and they've started sliding in
		if (!isUnpausing && pauseMenuTimer > 0.45f) {
			hoverResume = IsAreaClicked(resumeX, 100.0f, 300.0f, 60.0f, mx, my);
			hoverOptions = IsAreaClicked(optionsX, 0.0f, 300.0f, 60.0f, mx, my);
			hoverQuit = IsAreaClicked(quitX, -100.0f, 300.0f, 60.0f, mx, my);
		}

		float animSpeed = 15.0f;
		resumeScale += ((hoverResume ? 1.15f : 1.0f) - resumeScale) * animSpeed * dt;
		optionsScale += ((hoverOptions ? 1.15f : 1.0f) - optionsScale) * animSpeed * dt;
		quitScale += ((hoverQuit ? 1.15f : 1.0f) - quitScale) * animSpeed * dt;


		// --- Directional Splatter Physics ---
		for (int i = 0; i < MAX_BLOOD_DROPS; i++) {

			if (!bloodDrops[i].isActive) {
				bloodDrops[i].delayTimer -= dt;
				if (bloodDrops[i].delayTimer <= 0.0f) {
					bloodDrops[i].isActive = true;
				}
				continue;
			}

			if (bloodDrops[i].flyTimer > 0.0f) {
				bloodDrops[i].flyTimer -= dt;
				bloodDrops[i].x += bloodDrops[i].velX * dt;
				bloodDrops[i].y += bloodDrops[i].velY * dt;
				bloodDrops[i].currentScaleX = 2.0f;
				bloodDrops[i].currentScaleY = 2.0f;
			}
			else {
				bloodDrops[i].x += bloodDrops[i].velX * dt;
				bloodDrops[i].y += bloodDrops[i].velY * dt;
				bloodDrops[i].velX -= bloodDrops[i].velX * bloodDrops[i].friction * dt;
				bloodDrops[i].velY -= bloodDrops[i].velY * bloodDrops[i].friction * dt;

				float growSpeed = 30.0f;
				bloodDrops[i].currentScaleX += (bloodDrops[i].targetScaleX - bloodDrops[i].currentScaleX) * growSpeed * dt;
				bloodDrops[i].currentScaleY += (bloodDrops[i].targetScaleY - bloodDrops[i].currentScaleY) * growSpeed * dt;
			}
		}

		// --- Check for Button Clicks ---
		if (!isUnpausing && pauseMenuTimer > 0.45f) {
			if (hoverResume && AEInputCheckTriggered(AEVK_LBUTTON)) {
				isUnpausing = true; // Trigger slide out!

				// --- REVERSE BLOOD SPLATTER ---
				for (int i = 0; i < MAX_BLOOD_DROPS; i++) {
					bloodDrops[i].isActive = true;
					bloodDrops[i].flyTimer = 0.0f;
					bloodDrops[i].velX = -(float)((rand() % 4000) + 3000);
					bloodDrops[i].friction = 0.0f;
				}
			}
			if (hoverOptions && AEInputCheckTriggered(AEVK_LBUTTON)) {
				// Options Logic
			}
			if (hoverQuit && AEInputCheckTriggered(AEVK_LBUTTON)) {
				next = GS_QUIT;
			}
		}
	}
}

void PauseMenu_Draw(AEGfxVertexList* squareMesh)
{
	if (pauseAlpha <= 0.0f) return;

	// LAYER 1: The Darkening Overlay
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransparency(pauseAlpha);
	AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

	AEMtx33 scale, trans, transform;
	AEMtx33Scale(&scale, 1600.0f, 900.0f);
	AEMtx33Trans(&trans, 0.0f, 0.0f);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

	// LAYER 2: PIXELATED Procedural Blood Splatter
	if (gamePaused)
	{
		AEGfxSetColorToMultiply(0.6f, 0.0f, 0.0f, 1.0f);
		AEGfxSetTransparency(pauseAlpha);

		for (int i = 0; i < MAX_BLOOD_DROPS; i++) {
			AEMtx33 finalTransform;

			int pixelScaleX = (int)bloodDrops[i].currentScaleX;
			int pixelScaleY = (int)bloodDrops[i].currentScaleY;
			int pixelPosX = (int)bloodDrops[i].x;
			int pixelPosY = (int)bloodDrops[i].y;

			if (pixelScaleX <= 0) pixelScaleX = 1;
			if (pixelScaleY <= 0) pixelScaleY = 1;

			AEMtx33Scale(&scale, (float)pixelScaleX, (float)pixelScaleY);
			AEMtx33Trans(&trans, (float)pixelPosX, (float)pixelPosY);
			AEMtx33Concat(&finalTransform, &trans, &scale);
			AEGfxSetTransform(finalTransform.m);

			AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		}
	}

	// ==========================================
	// LAYER 3: The UI Textures (Staggered slide-in & Scaling)
	// ==========================================
	if (gamePaused && pauseMenuTimer > 0.35f)
	{
		// Draw Title (Static scale)
		if (pauseTitleTex) {
			DrawTextureMesh(squareMesh, pauseTitleTex, titleX, 250.0f, 600.0f, 180.0f, 1.0f);
		}

		// Draw Buttons (Multiply width and height by their respective scale variable!)
		if (btnResumeTex) {
			DrawTextureMesh(squareMesh, btnResumeTex, resumeX, 100.0f, 300.0f * resumeScale, 60.0f * resumeScale, 1.0f);
		}
		if (btnOptionsTex) {
			DrawTextureMesh(squareMesh, btnOptionsTex, optionsX, 0.0f, 300.0f * optionsScale, 60.0f * optionsScale, 1.0f);
		}
		if (btnQuitTex) {
			DrawTextureMesh(squareMesh, btnQuitTex, quitX, -100.0f, 200.0f * quitScale, 60.0f * quitScale, 1.0f);
		}
	}

	// Reset Global Engine State
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetTransparency(1.0f);
}

// ==========================================
// Unload Textures
// ==========================================
void PauseMenu_Unload()
{
	UnloadTextureSafe(pauseTitleTex);
	UnloadTextureSafe(btnResumeTex);
	UnloadTextureSafe(btnOptionsTex);
	UnloadTextureSafe(btnQuitTex);
}

bool PauseMenu_IsPaused()
{
	return gamePaused;
}

void PauseMenu_SetPaused(bool state)
{
	gamePaused = state;
}