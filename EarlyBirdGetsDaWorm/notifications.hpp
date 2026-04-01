/*!*****************************************************************************
\brief
    Loads pager resources such as textures and fonts.

    This includes loading the pager icon, pager UI texture, and
    initializing the font used for rendering pager text.

    This function should be called once during the loading phase
    of the game.

\return
    None.
*******************************************************************************/
void Notifications_Load();

/*!*****************************************************************************
\brief
    Initializes the pager system with default values.

    Sets up mesh, input states, pager state, icon properties,
    pager behavior, and popup flags. Also loads configuration
    overrides from file if available.

    This function prepares the pager for use at the start
    of the game or when resetting the system.

\return
    None.
*******************************************************************************/
void Notifications_Initialize();

/*!*****************************************************************************
\brief
    Triggers the pager to open on the next update cycle.

    This is typically used by external systems (e.g. boss fights
    or task completion events) to notify the pager system to display.

\return
    None.
*******************************************************************************/
void Notifications_Trigger();

/*!*****************************************************************************
\brief
    Updates the pager system every frame.

    Handles user input, pager toggling, hover effects, popup logic,
    vibration animation, and task-related notifications.

    Also updates mouse position and manages timing for animations
    and delayed events.

\param LiftActive
    Indicates whether the lift is currently active, which may
    disable pager interaction.

\param DT
    Delta time representing the time elapsed since the last frame.

\return
    None.
*******************************************************************************/
void Notifications_Update(bool LiftActive, f32 DT);

/*!*****************************************************************************
\brief
    Checks whether the pager is currently active.

\return
    True if the pager is open, false otherwise.
*******************************************************************************/
bool IsPagerActive();

/*!*****************************************************************************
\brief
    Renders the pager UI and related elements.

    Draws the pager icon, handles hover text, displays the full
    pager interface when active, and renders task-related text
    such as collection and destination details.

    Also applies vibration effects and displays patient progress.

\param PatientFloorNumber
    Floor number of the patient to collect.

\param PatientDoorNumber
    Door number of the patient to collect.

\param DestinationFloorNumber
    Target floor number for delivery.

\param DestinationDoorNumber
    Target door number for delivery.

\return
    None.
*******************************************************************************/
void Notifications_Draw(s8 PatientFloorNumber, s8 PatientDoorNumber, s8 DestinationFloorNumber, s8 DestinationDoorNumber);

/*!*****************************************************************************
\brief
    Frees all resources used by the pager system.

    This includes stopping audio, releasing meshes, unloading
    textures, and destroying the pager font.

    This function should be called when the game shuts down or
    when the pager system is no longer needed.

\return
    None.
*******************************************************************************/
void Notifications_Free();

/*!*****************************************************************************
\brief
    Resets the pager task completion state.

    This function clears the TaskComplete flag in the pager system,
    ensuring that any previously completed task does not persist
    into future gameplay logic.

\return
    None.
*******************************************************************************/
void Notifications_ResetTaskComplete();