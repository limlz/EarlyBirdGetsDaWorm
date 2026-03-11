#pragma once

/***************************************** LOAD ******************************************/

/******************************************************************************************
	@brief	Loads all textures, fonts, and assets required for the Lift system.
******************************************************************************************/
void Lift_Load();

/*************************************** INITIALIZE **************************************/

/******************************************************************************************
	@brief	Initializes the lift's state machine, resets timers, and creates necessary meshes.
******************************************************************************************/
void Lift_Initialize();

/***************************************** UPDATE ****************************************/

/******************************************************************************************
	@brief	Updates the lift's interaction proximity, toggle logic, and auto-closing.

	@param	dt 
			The delta time for the current frame.
	@param	camX 
			The current X position of the camera to check proximity.
	@param	maxDist 
			The maximum distance of the hallway.
******************************************************************************************/
void Lift_Update(float dt, float camX, float maxDist);

/******************************************************************************************
	@brief	Processes mouse input for the interactive lift panel and manages the state machine.

	@param	floorNum 
			Reference to the actual floor variable to modify when transit completes.
******************************************************************************************/
void Lift_HandleInput(s8& floorNum);

/***************************************** DRAW ******************************************/

/******************************************************************************************
	@brief	Draws the wall textures behind the elevator box in the 2D game world.

	@param	mesh 
			The mesh to draw the texture onto.
	@param	x 
			The X position in world space.
 	@param	y 
			The Y position in world space.
 	@param	w 
			The width of the background.
 	@param	h 
			The height of the background.
 	@param	floor 
			The current floor to determine the specific background texture.
******************************************************************************************/
void Lift_DrawBackground(AEGfxVertexList* mesh, float x, float y, float w, float h, s8 floor);

/******************************************************************************************
 	@brief	Draws the lift exterior in the game world, including the glowing digital floor number.

 	@param	squareMesh 
			The generic square mesh used for drawing solid colored boxes.
 	@param	x 
			The X position in world space.
 	@param	y	
			The Y position in world space.
 	@param	w	
			The width of the lift exterior.
 	@param	h 
			The height of the lift exterior.
 	@param	floorNum 
			The current floor number to display on the panel.
 	@param	textXoffset 
			Horizontal offset for the digital text.
 	@param	textY 
			Vertical coordinate for the floor number panel.
******************************************************************************************/
void Lift_DrawWorld(AEGfxVertexList* squareMesh, float x, float y, float w, float h, s8 floorNum, float textXoffset, float textY);

/******************************************************************************************
 	@brief	Draws the full-screen interactive Lift UI overlay (sliding doors and button panel).

 	@param	squareMesh 
			The generic square mesh used for drawing the UI elements and borders.
******************************************************************************************/
void Lift_Draw(AEGfxVertexList* squareMesh);

/**************************************** UNLOAD *****************************************/

/******************************************************************************************
	@brief	Unloads all lift textures, frees memory, and destroys fonts to prevent leaks.
******************************************************************************************/
void Lift_Unload();

/*************************************** ACCESSORS ***************************************/

/******************************************************************************************
	@brief	Checks if the interactive lift UI overlay is currently open and active.

	@return true if the lift UI is actively being used by the player.
******************************************************************************************/
bool Lift_IsActive();

/******************************************************************************************
	@brief	Checks if the player is currently standing close enough to a lift to interact.

	@return true if the player is within range of the left or right hallway lift.
******************************************************************************************/
bool Lift_IsNear();

/******************************* LIFT LOGIC & ANOMALIES **********************************/

/******************************************************************************************
 	@brief	Calculates the bounding box and applies visual anomalies (AIW, OCD) for a button slot.

 	@param	slotIndex 
			The physical button slot on the panel (0-9).
 	@param	x 
			Reference to store the calculated X position.
 	@param	y 
			Reference to store the calculated Y position.
 	@param	w 
			Reference to store the calculated width.
 	@param	h 
			Reference to store the calculated height.
******************************************************************************************/
static void GetButtonRect(int slotIndex, float& x, float& y, float& w, float& h);

/******************************************************************************************
	@brief	Randomizes button mappings and sets up target anomalies (Dementia, OCD) if active.
******************************************************************************************/
static void Lift_GenerateAnomalies();