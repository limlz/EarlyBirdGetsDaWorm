#include "pch.hpp"



// Create the boss instance

// Helper to reset the boss
void Boss_Initialize(Boss &myBoss) {
    myBoss.x = 400.0f;  // Right side of screen
    myBoss.y = -360.0f; // On the ground (Same as player ground)
    myBoss.w = 150.0f;
    myBoss.h = 150.0f;
    myBoss.health = 100; // 5 hits to kill (if 20 dmg per hit)
    myBoss.active = true;
}

void Boss_TakeDamage(Boss &myBoss, int damage) {
    myBoss.health -= damage;
    if (myBoss.health <= 0) {
        myBoss.active = false; // Boss defeated
    }
}
