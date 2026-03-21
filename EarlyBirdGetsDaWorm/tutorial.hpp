/*!*****************************************************************************
\brief
    Loads all resources required for the tutorial system.

    This includes creating the font used for tutorial text and loading
    the textures used for the tutorial prompt buttons.

    This function must be called before the tutorial is initialized
    or rendered.

\return
    None.
*******************************************************************************/
void Tutorial_Load();

/*!*****************************************************************************
\brief
    Initializes the tutorial system and resets all tutorial state.

    This function creates the meshes used by the tutorial UI and resets
    all tutorial flags, dialogue boxes, timers, and animation variables.
    It also sets up the dialogue box data used for each tutorial stage.

    This should be called when the game starts or when restarting
    the tutorial system.

\return
    None.
*******************************************************************************/
void Tutorial_Initialize();

/*!*****************************************************************************
\brief
    Updates the tutorial system state.

    This function controls the entire tutorial flow including:
    - Displaying the tutorial prompt at game start
    - Handling player input for tutorial selection (Yes/No)
    - Managing tutorial stage progression
    - Updating the dialogue boxes for each tutorial step

    The tutorial progresses through multiple stages based on player
    actions such as opening the pager, using the lift, collecting a
    patient, and delivering the patient.

\param[in] dt
    Delta time representing the time elapsed since the previous frame.

\return
    None.
*******************************************************************************/
void Tutorial_Update(f32 DT);

/*!*****************************************************************************
\brief
    Checks whether the tutorial mode is currently active.

\return
    True if the player chose to perform the tutorial, otherwise false.
*******************************************************************************/
bool Doing_Tutorial();

/*!*****************************************************************************
\brief
    Determines whether the player has answered the tutorial prompt.

    The tutorial prompt appears at the start of the game asking if
    the player wants to perform the tutorial.

\return
    True if the player has selected either Yes or No.
*******************************************************************************/
bool Tutorial_Prompt_Answered();

/*!*****************************************************************************
\brief
    Checks whether the tutorial prompt popup is currently being shown.

\return
    True if the tutorial selection prompt is visible on screen.
*******************************************************************************/
bool IsTutorialActive();
bool IsTutorialFinished();

/*!*****************************************************************************
\brief
    Renders the tutorial UI.

    This includes:
    - The tutorial prompt asking the player if they want a tutorial
    - The tutorial dialogue boxes for each tutorial stage
    - Tutorial instructions and on-screen prompts

    Only the dialogue box corresponding to the current tutorial stage
    will be rendered.

\return
    None.
*******************************************************************************/
void Tutorial_Draw();

/*!*****************************************************************************
\brief
    Frees all resources used by the tutorial system.

    This includes releasing meshes, unloading textures, and destroying
    the tutorial font.

    This function should be called when the game shuts down or when
    the tutorial system is no longer needed.

\return
    None.
*******************************************************************************/
void Tutorial_Free();
