#pragma once

struct Animation {
    int currentFrame = 0;
    float timer = 0.0f;
    float speed = 0.2f;     // seconds per frame
    int frameCount = 1;
    bool loops = true;       // false = stop on last frame
    int direction = 1;       // 1 = forward, -1 = reverse (for ping-pong)
    bool pingPong = false;   // reverses direction at ends

    void Update(float dt) {
        timer += dt;
        if (timer < speed) return;
        timer -= speed;

        if (pingPong) {
            currentFrame += direction;
            if (currentFrame >= frameCount - 1) direction = -1;
            if (currentFrame <= 0) direction = 1;
        } else if (loops) {
            currentFrame = (currentFrame + 1) % frameCount;
        } else {
            if (currentFrame < frameCount - 1) currentFrame++;
        }
    }

    void Reset() { currentFrame = 0; timer = 0.0f; direction = 1; }
    bool IsFinished() const { return !loops && currentFrame >= frameCount - 1; }
};
