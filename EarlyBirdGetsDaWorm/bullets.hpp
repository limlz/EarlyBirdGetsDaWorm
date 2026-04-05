#pragma once

/******************************************************************************************
    @brief  Sets up bullet meshes, fonts, and resets ammo to full capacity.
******************************************************************************************/
void Bullets_Initialize();

/******************************************************************************************
    @brief  Spawns a new player bullet at the given position traveling in the specified
            direction. Consumes one ammo and triggers a reload when empty.

    @param  x       Starting X position of the bullet.
    @param  y       Starting Y position of the bullet.
    @param  dirX    Normalized X direction component.
    @param  dirY    Normalized Y direction component.
******************************************************************************************/
void Fire_Bullet(float x, float y, float dirX, float dirY);

/******************************************************************************************
    @brief  Advances all active player bullets, handles reload timing, and checks
            collisions against boss bullets and the boss itself.

    @param  dt      Frame delta time in seconds.
    @param  camX    Current camera X offset for screen-space culling.
    @param  myBoss  Reference to the boss entity for collision checks.
******************************************************************************************/
void Bullets_Update(float dt, float camX, Boss &myBoss);

/******************************************************************************************
    @brief  Renders all active player bullets as velocity-stretched water droplets.

    @param  camX    Current camera X offset for world-to-screen conversion.
******************************************************************************************/
void Bullets_Draw(float camX);

/******************************************************************************************
    @brief  Draws the horizontal ammo bar UI in the bottom-right corner of the screen.
******************************************************************************************/
void Bullets_DrawAmmoUI();

/******************************************************************************************
    @brief  Checks whether the player can currently fire.

    @return true if ammo is available and the gun is not reloading.
******************************************************************************************/
bool Bullets_HasAmmo();

/******************************************************************************************
    @brief  Releases bullet meshes and resets the bullet pool. Safe to call before
            the next Bullets_Initialize.
******************************************************************************************/
void Bullets_Free();
