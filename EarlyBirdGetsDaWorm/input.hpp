#ifndef INPUT_HPP
#define INPUT_HPP

// Call this once per frame in your main update loop
void Input_Update();

// Call these from ANY file to get the current NDC mouse coordinates
float Input_GetMouseX();
float Input_GetMouseY();

#endif