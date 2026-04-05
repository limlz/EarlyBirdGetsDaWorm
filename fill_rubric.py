import openpyxl

wb = openpyxl.load_workbook('C:/Users/Liang Zhou/Downloads/project_rubrics_teamname.xlsm', keep_vba=True)

# === PRODUCT INFORMATION ===
pi = wb['Product Information']
pi['B7'] = 'Echoes of the Ward'
pi['B12'] = 'EarlyBirdGetsDaWorm'
pi['G7'] = (
    "DESCRIPTION: \n"
    "Echoes of the Ward is a 2D psychological horror game where the player takes on the role of a caretaker/nurse "
    "managing patients with different mental health conditions. The core mechanic involves identifying which mental "
    "illness a patient has by observing anomalies in the environment (frames, lighting, doors, walls, elevator, sound), "
    "then successfully delivering them to the correct destination before time runs out. Over 5 in-game days, the player "
    "must meet a patient quota while distinguishing ghost patients from human ones. A boss fight awaits after disposing "
    "of a ghost in the basement.\n\n"
    "CONTROLS:\n"
    "- A/D: Move left/right\n"
    "- E: Interact with doors / elevator\n"
    "- ESC: Pause menu\n"
    "- Tab: Open journal (evidence tracker)\n"
    "- Mouse Click: Navigate elevator panel, journal pages\n"
    "- Boss Fight: A/D move, Space jump, Ctrl crouch, Left Click shoot\n"
    "- Debug: F3 toggle debug overlay, F4 pause timer, H toggle ghost/human, 1-9 force illness\n\n"
    "ADDITIONAL NOTES:\n"
    "- Horror-less mode available in settings (disables jumpscares, enables persona characters outside elevator)\n"
    "- Presentation mode available for demo purposes (forces specific anomalies)\n"
    "- Playtest mode available (2x speed, 2 patients per day)\n"
    "- Config hot-reload: edit Assets/config.txt while running to tune parameters live\n"
    "- Known: Timer continues during boss fade transition (~4 in-game minutes lost)\n"
)

pi['B27'] = 'A'

roster = [
    ('BSGD', 'Lim Liang Zhou', 'limlz@digipen.edu'),
    ('BSGD', 'Nur Nadia Natasya Binte Mohd Taha', 'n.mohdtaha@digipen.edu'),
    ('BSGD', 'Aracel Joy Balbaira ABELLERA', 'a.abellera@digipen.edu'),
    ('BSGD', 'Syadeeya Binte Harith Fadhirlah', 'syadeeya.b@digipen.edu'),
]
for i, (degree, name, email) in enumerate(roster):
    row = 18 + i
    pi[f'B{row}'] = degree
    pi[f'C{row}'] = name
    pi[f'D{row}'] = email

# === RUBRIC ===
rb = wb['Rubric']

rb['D8'] = 'A'
rb['E8'] = 'Consistent naming conventions throughout (PascalCase functions, camelCase locals). Solution Explorer organized into Core, Gameplay, Anomalies, Boss Fight, UI, Audio, Rendering filters. Doxygen comments on all public function declarations. Systems clearly separated (CentralPool, Frames, Doors, Lighting, Wall, Lift, SoundAnomaly each in own file). Minimal global state; static file-scope variables used appropriately.'

rb['D14'] = 'A'
rb['E14'] = 'Reading: Assets/config.txt loaded at startup with hot-reload support (Config::PollAndReload every frame). Covers flicker timing, illness visuals, overlay alpha, sound timing, frame behavior constants, poster glitch mapping, jumpscare settings. Also reads Assets/credits.txt for credits screen. Writing: Analytics::LogRun writes complete game session data to CSV (end reason, day reached, patients handled, ghost status, diagnosis, anomaly illness, time survived).'

rb['D20'] = 1
rb['E20'] = 'F3 toggles in-game debug overlay showing: mouse coords (screen + world), camera X, current day, floor, illness, ghost/human status, all 6 anomaly system states (ON/--) from CentralPool, FPS, delta time, timer state. Controls: H toggle ghost/human, 1-9 force illness, F4 pause timer, F6 set quota, K next day, J jumpscare, M warp camera. Boss fight has separate BossDebug overlay with god mode. All drawn in-game, not console.'

rb['D24'] = 0.5
rb['E24'] = 'Config::PollAndReload() provides hot-reloading of Assets/config.txt. Designers can change illness parameters, flicker timing, frame behavior constants, poster glitch mapping, and jumpscare settings while the game is running. No visual level editor.'

rb['D33'] = 'A'
rb['E33'] = 'Stable throughout 5-day gameplay loop with no crashes. Cohesive horror hospital experience with consistent theme across all screens. Complete start-to-end loop: startup splash, main menu, 5 days of gameplay, boss fight, endgame screen, return to menu. Smooth state transitions with fade effects.'

rb['D44'] = 'A'
rb['E44'] = 'All buttons have hover animations (scale/alpha interpolation). Pager notification system guides player to pickup/delivery locations. Tutorial system on first play. Journal tracks observed anomalies with unread indicators. Contextual prompts (wrong room, interact hints). Consistent E key for all door/lift interactions, ESC for pause across all states.'

rb['D50'] = 1
rb['E50'] = 'The anomaly detection system is innovative: 10 distinct illness types each manifest differently across 6 independent systems (frames, doors, lighting, walls, elevator, sound). Ghost patients add extra anomalies. Journal evidence system lets players cross-reference observations to deduce diagnosis. Eye-blink transition defers state changes to when screen is fully black.'

rb['D58'] = 'A'
rb['E58'] = 'Consistent dark horror aesthetic across all screens. Custom fonts (Buggy, DigitalClock, Handwriting). Smooth fade transitions between all states. Color palette uses muted tones. UI elements properly aligned. Horror-less mode provides alternative friendly visual experience with persona characters.'

rb['D64'] = 1
rb['E64'] = 'Particle effects with gravity, air resistance, floor bounce, additive glow blending. Screen shake for jumpscares and boss fight. Eye-blink transition with sine-wave easing. Frame anomalies with 10 distinct visual behaviors (paranoia weeping angel, mania drift, depression melting, AIW oscillation, etc). Boss flicker/vanish/teleport effects. Day overlay fade transitions.'

rb['D68'] = 1
rb['E68'] = 'SoundAnomalySystem with 10 illness-specific audio behaviors: paranoia extra footsteps, mania spike/panning, depression slugged BGM, dementia stuttering, schizophrenia cuts, AIW big/small alternation, insomnia heartbeat acceleration, OCD repeating sounds with pitch variation, scotophobia darkness triggers. 3 isolated audio channels (BGM, SFX, Walking). 50+ sound effects. Dynamic boss BGM volume scaling.'

# === TCRs ===
tcr = wb['TCRs']
tcr['C3'] = 'Echoes of the Ward'
tcr['C4'] = 'EarlyBirdGetsDaWorm'
tcr['C5'] = 'CSD1451 / Section A / 2026'
tcr['C6'] = 'limlz@digipen.edu'

tcr_data = {
    14: ('Passed', 'DigiPen logo displayed in Startup state for 2+ seconds before transitioning to main menu'),
    15: ('Passed', 'DigiPen copyright displayed in credits screen, parsed from Assets/credits.txt'),
    16: ('Passed', 'Credits screen accessible from main menu, lists all 4 team members and instructors'),
    17: ('Passed', 'ESC opens pause menu with Resume, Options, Quit. Game pauses while menu active. ESC again resumes.'),
    18: ('Passed', 'In-game tutorial system guides first-time players. Guide system accessible via clipboard icon.'),
    19: ('Passed', 'Settings menu accessible from pause menu with BGM/SFX volume sliders and horror-less mode toggle.'),
    20: ('Passed', 'Main menu: Play, Options, Credits, Quit. Pause menu: Resume, Options, Quit.'),
    21: ('Passed', 'Pause menu allows returning to main menu which resets the session via Game_ResetSession()'),
    22: ('Not Tested', 'Quit from pause menu does not currently have explicit confirmation dialog'),
    23: ('Passed', 'Window opens immediately with DigiPen splash screen within 1 second'),
    24: ('Passed', 'Startup splash can be bypassed with any key press or mouse click'),
    26: ('Passed', 'All state transitions use fade effects, no visible freezes'),
    27: ('Passed', 'Fade transitions are animated (alpha interpolation), total transition under 2 seconds'),
    28: ('Passed', 'No loading screens needed; all assets load in under 1 second'),
    30: ('Passed', 'Full keyboard + mouse support for all gameplay and menus'),
    31: ('Passed', 'Mouse used for elevator panel, journal navigation, and boss fight aiming'),
    33: ('Passed', 'No crashes found during code review. Memory leaks resolved with explicit Shutdown calls.'),
    34: ('Passed', 'No soft-lock states found. Elevator can be cancelled with E. Timer prevents infinite loops.'),
    36: ('Passed', 'Frame rate controlled at 60 FPS via AESysInit'),
    37: ('Passed', 'Window remains responsive; AESysDoesWindowExist checked every frame in main loop'),
    39: ('Passed', 'Debug overlay (F3) off by default. Window title set to "Echoes of the Ward"'),
    40: ('Passed', 'PauseMenu_IsPaused() blocks all game updates. AudioManager_StopSFX on boss fight exit.'),
    41: ('Passed', 'Game exits only from main menu or pause menu quit, both transition through GS_QUIT'),
    43: ('Passed', '.gitignore excludes build artifacts (.pdb, .obj, .pch, bin/, .tmp/)'),
    44: ('Passed', 'All source files have doxygen comments. Code organized into logical filter categories.'),
    45: ('Passed', 'Project well under 500MB limit'),
    46: ('Not Applicable', 'No installer required for this submission format'),
    47: ('Not Tested', 'Screenshots not yet prepared'),
    48: ('Not Tested', 'Summary text file not yet prepared'),
    49: ('Passed', 'Folder structure follows submission guidelines'),
    50: ('Not Applicable', 'No installer used'),
    52: ('Not Applicable', 'No networking'),
    53: ('Not Applicable', 'No networking'),
    54: ('Not Applicable', 'No networking'),
    55: ('Not Applicable', 'No networking'),
}

for row_num, (status, comment) in tcr_data.items():
    tcr[f'D{row_num}'] = status
    tcr[f'E{row_num}'] = comment

wb.save('C:/Users/Liang Zhou/Downloads/project_rubrics_teamname.xlsm')
print("Rubrics filled successfully!")
