# Echoes of the Ward

A psychological horror game in C++, built on DigiPen's AlphaEngine by team EarlyBirdGetsDaWorm.

You work a shift in a hospital ward. Patients present with different illnesses, the systems available to you change with them, and something in the building is wrong. Watch for anomalies, log what you find, and survive what is waiting at the end.

## Systems

| Area | Files |
| --- | --- |
| State machine | `GameStateManager.cpp`, `gsm.hpp`, `state_list.hpp` |
| Ward and navigation | `Lift.cpp`, `Doors.cpp`, `Wall.cpp`, `Frames.cpp` |
| Anomaly loop | `SoundAnomaly.cpp`, `Journal.cpp`, `Notifications.cpp`, `Prompts.cpp` |
| Boss encounter | `Boss.cpp`, `BossFight.cpp`, `Bullets.cpp`, `BossDebug.cpp` |
| Rendering | `Lighting.cpp`, `MeshCreation.cpp`, `ParticleEffects.cpp`, `Animation.hpp` |
| Audio | `AudioManager.cpp` |
| Player and scares | `Player.cpp`, `Jumpscare.cpp` |
| Onboarding | `Tutorial.cpp`, `Guide.cpp` |
| Infrastructure | `CentralPool.cpp`, `Config.cpp`, `Analytics.cpp`, `Debug.cpp`, `pch.hpp` |

Some details worth calling out:

- **Central object pool** (`CentralPool.cpp`) — shared allocation for runtime entities rather than per-system ownership
- **Hot-reloadable config** (`Config.cpp`) — tuning values reload without a rebuild, which cut iteration time during balancing
- **Presentation mode** — a separate endgame path (`PresentationEndgame.cpp`) used for demos
- Built at warning level 4, with a late pass dedicated to resolving memory leaks

## My contribution

- **Game state manager** (`GameStateManager.cpp`, `gsm.hpp`) — switches states through function pointers, and keeps the ward level loaded during the boss fight so the player returns to it without a reload
- **Config hot-reload** (`Config.cpp`) — watches `Assets/config.txt` for changes and applies new values while the game is running, printing what changed
- **Dynamic lighting** (`Lighting.cpp`) — per-floor flicker, a cone light, and checks for whether the player is standing in darkness, tied into the anomaly system

## Build

Open `EarlyBirdGetsDaWorm.sln` in Visual Studio and build. AlphaEngine is vendored under `Extern/`.

## Team

limlz, syadeeya, n4ddeer, a-abellera.

---

Coursework project at DigiPen Institute of Technology Singapore / Singapore Institute of Technology.
