#pragma once

// --- INCLUDES --- //
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <array>
#include <cmath>
#include <vector>
#include <ctime>
#include <cstdlib>   // For rand()
#include <algorithm> // Required for std::shuffle
#include <random>    // Required for std::default_random_engine
#include "AEEngine.h"


#include "analytics.hpp"
#include "debug.hpp"
#include "settings.hpp"
#include "config.hpp"
#include "prompts.hpp"
#include "boss.hpp"
#include "bullets.hpp"
#include "state_list.hpp"
#include "gsm.hpp"
#include "utils.hpp"
#include "input.hpp"
#include "mesh_creation.hpp"
#include "frames.hpp"
#include "doors.hpp"
#include "lighting.hpp"
#include "particle_effects.hpp"
#include "notifications.hpp"
#include "wall.hpp"
#include "startup.hpp"
#include "main_menu.hpp"
#include "game.hpp"
#include "boss_fight.hpp"
#include "endgame.hpp"
#include "audio_manager.hpp"
#include "quit.hpp"
#include "player.hpp"
#include "timer.hpp"
#include "Lift.hpp"
#include "central_pool.hpp"
#include "assets_catalog.hpp"
#include "pause_menu.hpp"
#include "jumpscare.hpp"
#include "tutorial.hpp"
#include "guide.hpp"
#include "journal.hpp"
#include "sound_anomaly.hpp"

// --- ENDGAME FLAGS ---
enum class EndGameReason : int {
    REASON_SURVIVED_5_DAYS,
    REASON_DIED_TO_BOSS,
    REASON_SENT_HUMAN_TO_BASEMENT,
	REASON_SENT_GHOST_TO_ROOM,
	REASON_QUIT_FROM_MENU,
    REASON_QUOTA_NOT_MET
};

// Declare it as extern so every file shares the EXACT same variable
extern EndGameReason currentEndReason;


// --- CONSTANTS --- //
// Colors
inline constexpr unsigned int COLOR_WHITE        = 0xFFFFFFFF;
inline constexpr unsigned int COLOR_BLACK        = 0x000000FF;
inline constexpr unsigned int COLOR_RED          = 0xFFFF0000;
inline constexpr unsigned int COLOR_LIFT_RED     = 0xFF5C5CFF;
inline constexpr unsigned int COLOR_LIGHT_GREY   = 0x696969FF;
inline constexpr unsigned int COLOR_DARK_GREY    = 0x333333FF;
inline constexpr unsigned int COLOR_NIGHT_BLUE   = 0x191970FF;
inline constexpr unsigned int COLOR_DOOR_BROWN   = 0x8B4513FF;
inline constexpr unsigned int COLOR_LIFT_BG      = 0x2F4F4FFF;
inline constexpr unsigned int COLOR_LIFT_CONSOLE = 0xC0C0C0FF;
inline constexpr unsigned int COLOR_LIFT_BUTTON  = 0xFFD700FF;

// --- CONSTANTS --- //
// Dimensions & Settings
inline constexpr int   SCREEN_W           = 1600;
inline constexpr int   SCREEN_H           = 900;

inline constexpr float PLAYER_SPEED       = 400.0f;
inline constexpr float PLAYER_WIDTH       = 220.0f;
inline constexpr float PLAYER_HEIGHT      = 200.0f;

inline constexpr float DOOR_WIDTH         = 200.0f;
inline constexpr float DOOR_HEIGHT        = 300.0f;
inline constexpr float DIST_BETWEEN_DOORS = 600.0f;

inline constexpr float LIFT_WIDTH         = 200.0f;
inline constexpr float LIFT_HEIGHT        = 300.0f;
inline constexpr float LIFT_DOOR_WIDTH    = 500.0f;
inline constexpr float LIFT_DOOR_HEIGHT   = 800.0f;

inline constexpr int   NUM_OF_FLOOR       = 10;
inline constexpr int   NUM_DOORS          = 10;
inline constexpr int   REQUIRED_PATIENTS  = 5;
inline constexpr float SCREEN_WIDTH_HALF  = 800.0f;
inline constexpr float SCREEN_HEIGHT_HALF = 450.0f;
inline constexpr float FLOOR_CENTER_Y     = 650.0f;
inline constexpr float FLOOR_HEIGHT       = 800.0f;