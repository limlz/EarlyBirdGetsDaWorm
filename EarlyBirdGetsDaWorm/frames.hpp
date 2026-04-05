#pragma once

// ============================================================================
// ENUMS & DATA STRUCTURES
// ============================================================================

/******************************************************************************************
	@brief	State machine phases for the global frame glitch timing.
******************************************************************************************/
enum FRAME_TIMING_STATE
{
	STATE_NORMAL = 0,    // Normal painting, no anomalies active
	STATE_GLITCHING,     // Anomaly active (wrong texture + physical offsets)
	STATE_COOLDOWN       // Brief reset pause before returning to normal
};

/******************************************************************************************
	@brief	Identifiers for the entity type a frame is currently reacting to.
******************************************************************************************/
enum ENTITIES
{
	HUMAN = 0,
	GHOST_ENTITY         // Renamed from GHOST to avoid clashing with ILLNESSES::GHOST
};

/******************************************************************************************
	@brief	Holds the persistent state, positional data, and design mapping for a single frame.
******************************************************************************************/
struct FrameAnomaly
{
	ENTITIES entity = HUMAN;

	float posX = 0.0f;
	float posY = 0.0f;

	f32   width = 0.0f;
	f32   height = 0.0f;

	int   designID = 1;       // Base poster design (0..4 based on updated logic)
	int   currentState = 0;   // The specific texture state (0 = normal, 1+ = scary states)
};


// ============================================================================
// SYSTEM LIFECYCLE (LOAD / INIT / UNLOAD)
// ============================================================================

/******************************************************************************************
	@brief	Loads all textures and assets required for the Frames anomaly system.
******************************************************************************************/
void Frames_Load();

/******************************************************************************************
	@brief	Initializes the base state, positions, and standard designs for all frames.
******************************************************************************************/
void Frames_Initialize();

/******************************************************************************************
	@brief	Unloads all frame textures and frees memory to prevent leaks.
******************************************************************************************/
void Frames_Unload();


// ============================================================================
// UPDATE LOGIC
// ============================================================================

/******************************************************************************************
	@brief	Updates the state machine and physics for frame anomalies based on current illness.

	@param	dt
			The delta time for the current frame.
	@param	camX
			The current X position of the camera to check proximity for gaze/distance mechanics.
******************************************************************************************/
void Frames_Update(float dt, float camX);


// ============================================================================
// DRAWING
// ============================================================================

/******************************************************************************************
	@brief	Draws the frames for the current floor, applying current states, offsets, and scales.

	@param	currentLevel
			The index of the floor the player is currently on.
	@param	camX
			The current X position of the camera to correctly offset world rendering.
******************************************************************************************/
void Frames_Draw(int currentLevel, f32 camX);


// ============================================================================
// UTILITY / STATE MANAGEMENT
// ============================================================================

/******************************************************************************************
	@brief	Safely resets all frames across all floors back to their perfectly normal states.
******************************************************************************************/
void Frames_ResetAll();