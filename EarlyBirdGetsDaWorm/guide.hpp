/*!*****************************************************************************
\brief
    Loads all textures required for the in-game guide system.

    This function loads the guide icon, category labels, and guide
    images that will be displayed inside the guide menu. These
    resources must be loaded before the guide can be rendered.

\return
    None.
*******************************************************************************/
void Guide_Load();

/*!*****************************************************************************
\brief
    Initializes the guide system and resets all guide variables.

    This function creates the mesh used for rendering guide UI
    elements and resets all guide states such as category selection,
    page states, scrolling values, and fade animation variables.

    It prepares the guide interface so it starts in a closed and
    consistent state when the game begins.

\return
    None.
*******************************************************************************/
void Guide_Initialize();

/*!*****************************************************************************
\brief
    Updates the guide system and handles player interaction.

    This function manages opening and closing the guide, category
    selection, page navigation, and UI animations such as fading.
    It also handles mouse hovering and keyboard navigation for
    switching between guide categories.

    The guide cannot be opened while the lift or pager UI is active.

\param[in] liftActive
    Indicates whether the lift interface is currently open.

\param[in] dt
    Delta time representing the time elapsed since the last frame.

\param[in] pagerActive
    Indicates whether the pager interface is currently open.

\return
    None.
*******************************************************************************/
void Guide_Update(bool LiftActive, f32 DT, bool PagerActive);

/*!*****************************************************************************
\brief
    Opens the guide directly to the boss fight information.

    This function forces the guide to open and automatically selects
    the boss fight category. It resets the guide fade animation,
    category states, and page values so the correct information is
    displayed to the player.

    This is typically triggered during a boss encounter.

\return
    None.
*******************************************************************************/
void Guide_BossFight();

/*!*****************************************************************************
\brief
    Restores the visibility of the guide corner icon.

    Re-enables the icon that appears on the screen, typically after
    a boss fight has ended where the icon was previously hidden.

\return
    None.
*******************************************************************************/
void Guide_ShowIcon();

/*!*****************************************************************************
\brief
    Checks whether the guide menu is currently open.

\return
    True if the guide is open, otherwise false.
*******************************************************************************/
bool IsGuideActive();

/*!*****************************************************************************
\brief
    Renders the guide UI.

    This includes:
    - The background overlay when the guide is open
    - The category selection panel
    - The separator between categories and guide content
    - The images or pages corresponding to the selected category

    Only the content belonging to the currently selected category
    will be rendered.

\return
    None.
*******************************************************************************/
void Guide_Draw();

/*!*****************************************************************************
\brief
    Frees all resources used by the guide system.

    This function releases the mesh and unloads all textures used
    by the guide interface to prevent memory leaks when the guide
    system is no longer needed.

\return
    None.
*******************************************************************************/
void Guide_Free();

/*!*****************************************************************************
\brief
    Draws only the guide clipboard icon at a specific world-space position.
    Used to anchor the icon beside the boss HP bar during the boss fight.
*******************************************************************************/
void Guide_DrawIconAt(float worldX, float worldY);