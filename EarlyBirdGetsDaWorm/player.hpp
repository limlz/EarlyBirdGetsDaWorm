#pragma once
#include "central_pool.hpp"

struct AEGfxVertexList;

/******************************* PLAYER & PATIENT LOGIC **********************************/

/******************************************************************************************
    @brief  Resets the player/patient state, clears previous anomalies, and
            generates new pickup and destination rooms.
******************************************************************************************/
void Player_GenerateMission();

/******************************************************************************************
    @brief  Determines if the next patient is a ghost based on the day's
            specific probability pool.

    @param  day
            The current in-game day (1-5).
******************************************************************************************/
void SetPatientScaryByDay(int day);

/******************************************************************************************
    @brief  Handles player interaction with a door (picking up or delivering
            a patient).

    @param  currentFloor
            The floor the player is currently on.
    @param  doorNumAtPlayer
            The door number the player is standing in front of.
    @param  day
            The current in-game day.

    @return True if a successful pickup or delivery occurred, false otherwise.
******************************************************************************************/
bool Player_HandleInteraction(s8 currentFloor, s8 doorNumAtPlayer, int day);

/******************************************************************************************
    @brief  Resets the daily patient counter and shuffles the ghost/human pool
            for the specified day.

    @param  day
            The current in-game day.
******************************************************************************************/
void ResetPatientCounter(int day);

/********************************* CORE LIFECYCLE ****************************************/

/******************************************************************************************
    @brief  Initializes the player graphics, random seed, and loads all
            necessary textures/meshes.
******************************************************************************************/
void Player_Load();

/******************************************************************************************
    @brief  Updates player logic, including walking animations and dynamic
            audio playback.

    @param  dt
            The delta time for the current frame.
    @param  walkKey
            True if the player is pressing the movement keys, false otherwise.
******************************************************************************************/
void Player_Update(float dt, bool walkKey);

/******************************************************************************************
    @brief  Renders the player, the wheelchair, and any active visual bluffs
            to the screen.

    @param  x
            The world x-coordinate to draw the player.
    @param  y
            The world y-coordinate to draw the player.
******************************************************************************************/
void Player_Draw(float x, float y);

/******************************************************************************************
    @brief  Frees all allocated memory, unloads textures, and destroys meshes
            used by the player.
******************************************************************************************/
void Player_Unload();

/*************************************** MODIFIERS ***************************************/

/******************************************************************************************
    @brief  Safely forces the current patient to be a normal human and clears
            visual bluffs.
******************************************************************************************/
void Player_ForceHumanPatient();

/******************************************************************************************
    @brief  Safely forces the current patient to be a ghost patient and uses
            visual bluffs (used for tutorials).
******************************************************************************************/
void Player_ForceGhostPatient();

/******************************************************************************************
    @brief  Directly sets the patient's illness (typically used for debug cheats).

    @param  illness
            The ILLNESSES enum value to apply.
******************************************************************************************/
void Player_SetIllness(ILLNESSES illness);

/******************************************************************************************
    @brief  Sets the visual facing direction of the player sprite.

    @param  dir
            A positive number to face right, or a negative number to face left.
******************************************************************************************/
void Player_SetFacing(int dir);

/*************************************** ACCESSORS ***************************************/

/******************************************************************************************
    @brief  Checks the hidden truth value of the carried patient.

    @return True if the patient is a ghost, false if they are human.
******************************************************************************************/
bool IsGhostPatient();

/******************************************************************************************
    @brief  Retrieves the illness currently driving the game's anomalies.

    @return The active illness, or ILLNESSES::GHOST if the patient is a ghost.
******************************************************************************************/
ILLNESSES Player_GetCurrentIllness();

/******************************************************************************************
    @brief  Retrieves the base illness the patient has (or the illness the
            ghost is mimicking).

    @return The mimicked or actual illness.
******************************************************************************************/
ILLNESSES Player_GetMimicIllness();

/******************************************************************************************
    @brief  Retrieves the extra anomaly count modifier triggered by a ghost.

    @return 1 if the patient is a ghost, 0 otherwise.
******************************************************************************************/
int Player_GetGhostExtraAnomalies();

/******************************************************************************************
    @brief  Checks if the player's wheelchair is currently occupied by a patient.

    @return True if a patient is picked up, false otherwise.
******************************************************************************************/
bool Player_HasPatient();

/******************************************************************************************
    @brief  Retrieves the total number of patients handled in the current day.

    @return Integer count of patients handled.
******************************************************************************************/
int Player_GetPatientsHandled();

/******************************************************************************************
    @brief  Retrieves the target room coordinates for the current mission.

    @param  patientFloor
            Reference to store the pickup floor.
    @param  patientDoor
            Reference to store the pickup door.
    @param  destFloor
            Reference to store the destination floor.
    @param  destDoor
            Reference to store the destination door.
******************************************************************************************/
void Player_GetTargetRoom(s8& patientFloor, s8& patientDoor, s8& destFloor, s8& destDoor);

/******************************************************************************************
    @brief  Gets the rendering width of the player sprite.

    @return The player's width as a float.
******************************************************************************************/
float Player_GetWidth();

/******************************************************************************************
    @brief  Gets the rendering height of the player sprite.

    @return The player's height as a float.
******************************************************************************************/
float Player_GetHeight();