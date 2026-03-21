/*!*****************************************************************************
\brief
    Loads all textures required for wall backgrounds and wall anomalies.

    This function initializes the texture resources used by the wall
    system, including the background wall texture and animation frames
    for each anomaly type.

    It must be called before any wall rendering occurs.
*******************************************************************************/
void Wall_Load();

/*!*****************************************************************************
\brief
    Initializes the wall system and prepares runtime variables.

    This includes:
    - Creating the mesh used for rendering wall textures
    - Seeding the random number generator
    - Generating random positions for anomalies on each floor
    - Resetting anomaly selections
    - Initializing animation timers and frame indices
*******************************************************************************/
void Wall_Initialize();

/*!*****************************************************************************
\brief
    Updates the wall anomaly animation state.

    This function advances the animation frame based on the elapsed
    time. When the animation timer exceeds the configured frame
    duration, the current image index is incremented and looped
    back to the start when necessary.

\param[in] dt
    Delta time representing the time elapsed since the last frame.

\return
    None.
*******************************************************************************/
void Wall_Update(f32 DT);

/*!*****************************************************************************
\brief
    Renders the wall background and the active wall anomaly.

    The wall background is drawn using world-anchored tiling so that it
    scrolls correctly with the camera. If wall anomalies are enabled and
    the player currently has a patient, the corresponding anomaly for the
    floor and illness will be rendered.

    The anomaly type is selected once per floor and remains consistent
    unless the illness changes.

\param[in] camX
    The current horizontal camera position.

\param[in] floorNum
    The index of the floor currently being rendered.

\return
    None.
*******************************************************************************/
void Wall_Draw(f32 CamX, s8 FloorNum);

/*!*****************************************************************************
\brief
    Releases all resources used by the wall system.

    This includes freeing the wall mesh and unloading all textures
    associated with the wall background and anomaly animations.

    This function should be called during shutdown or when the
    wall system is no longer needed.

\return
    None.
*******************************************************************************/
void Wall_Unload();