#pragma once

/******************************************************************************************
    @brief  Clears the particle list for a fresh start at the beginning of a level.
******************************************************************************************/
void Particles_Initialize();

/******************************************************************************************
    @brief  Updates all active particles: applies gravity, air resistance, movement,
            aging, floor bounce, and removes dead particles. Call every frame.
******************************************************************************************/
void Particles_Update();

/******************************************************************************************
    @brief  Renders all active particles with additive blending for a glowing effect.

    @param  mesh    The square mesh used for drawing each particle.
    @param  camX    Current camera X offset for world-to-screen conversion.
******************************************************************************************/
void Particles_Draw(AEGfxVertexList* mesh, float camX);

/******************************************************************************************
    @brief  Spawns a burst of particles at the given position (e.g., when a light breaks).

    @param  startX  World X position of the explosion origin.
    @param  startY  World Y position of the explosion origin.
    @param  count   Number of particles to spawn.
******************************************************************************************/
void Particles_Spawn(float startX, float startY, int count);

/******************************************************************************************
    @brief  Deallocates the particle list memory. Call when quitting to clean up.
******************************************************************************************/
void Particles_Free();
