#include <vector>

struct Particle {
    float x, y;          // World Position
    float velX, velY;    // Velocity
    float life;          // Life timer (1.0 = full, 0.0 = dead)
    float scale;         // Size in pixels
    float r, g, b;       // Color
};

// Global List of Particles
static std::vector<Particle> g_ParticleSystem;

// Function Prototypes
void Particle_Create(float startX, float startY, int count);
void Particle_Update();
void Particle_Draw(AEGfxVertexList* mesh, float camX);