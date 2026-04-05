#ifndef INPUT_HPP
#define INPUT_HPP

/******************************************************************************************
    @brief  Polls the raw mouse position from the engine and converts it to NDC
            screen-space coordinates. Call once per frame in the main update loop.
******************************************************************************************/
void Input_Update();

/******************************************************************************************
    @brief  Returns the current mouse X coordinate in NDC screen-space.

    @return Mouse X position where 0 is screen center, negative is left, positive is right.
******************************************************************************************/
float Input_GetMouseX();

/******************************************************************************************
    @brief  Returns the current mouse Y coordinate in NDC screen-space.

    @return Mouse Y position where 0 is screen center, negative is bottom, positive is top.
******************************************************************************************/
float Input_GetMouseY();

#endif
