#include <iostream>
#include <fstream>
#include <string>
#include "AEEngine.h"

#include "state_list.hpp"
#include "gsm.hpp"

#include "utils.hpp"
#include "input.hpp"
#include "mesh_creation.hpp"
#include "lights.hpp"
#include "frames.hpp"

#include "startup.hpp"
#include "main_menu.hpp"
#include "game.hpp"
#include "quit.hpp"

// --- MACROS --- //
// Colors
#define COLOR_WHITE         0xFFFFFFFF
#define COLOR_BLACK         0x000000FF
#define COLOR_NIGHT_BLUE    0x191970FF
#define COLOR_DOOR_BROWN    0x8B4513FF
#define COLOR_LIFT_GREY     0x696969FF
#define COLOR_LIFT_BG       0x2F4F4FFF
#define COLOR_LIFT_CONSOLE  0xC0C0C0FF
#define COLOR_LIFT_BUTTON   0xFFD700FF

const int SCREEN_W = 1600;
const int SCREEN_H = 900;