// --- INCLUDES --- //
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <cmath>
#include <vector>
#include <cstdlib> // For rand()
#include <algorithm> // Required for std::shuffle
#include <random>    // Required for std::default_random_engine
#include "AEEngine.h"

#include "debug.hpp"
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
#include "quit.hpp"
#include "player.hpp"
#include "timer.hpp"
#include "Lift.hpp"
#include "central_pool.hpp"
#include "assets_catalog.hpp"
#include "pause_menu.hpp"

// --- MACROS --- //
// Colors
#define COLOR_WHITE         0xFFFFFFFF
#define COLOR_BLACK         0x000000FF
#define COLOR_LIGHT_GREY    0x696969FF
#define COLOR_DARK_GREY		0x333333FF
#define COLOR_NIGHT_BLUE    0x191970FF
#define COLOR_DOOR_BROWN    0x8B4513FF
#define COLOR_LIFT_BG       0x2F4F4FFF
#define COLOR_LIFT_CONSOLE  0xC0C0C0FF
#define COLOR_LIFT_BUTTON   0xFFD700FF

// --- MACROS --- //
// Dimensions & Settings
#define PLAYER_SPEED        400.0f
#define PLAYER_WIDTH        220.0f
#define PLAYER_HEIGHT       200.0f

#define DOOR_WIDTH          200.0f
#define DOOR_HEIGHT         300.0f
#define DIST_BETWEEN_DOORS  600.0f

#define LIFT_WIDTH          200.0f
#define LIFT_HEIGHT         300.0f
#define LIFT_DOOR_WIDTH		500.0f
#define LIFT_DOOR_HEIGHT	800.0f

#define NUM_OF_FLOOR        10
#define NUM_DOORS           10
#define SCREEN_WIDTH_HALF   800.0f
#define SCREEN_HEIGHT_HALF  450.0f
#define FLOOR_CENTER_Y		650.0f
#define FLOOR_HEIGHT		800.0f

const int SCREEN_W = 1600;
const int SCREEN_H = 900;
