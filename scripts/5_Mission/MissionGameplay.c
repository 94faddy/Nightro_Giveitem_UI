modded class MissionGameplay
{
    private bool m_NightroCtrlPressed = false;
    private bool m_NightroIPressed = false;
    private bool m_NightroKeyComboTriggered = false;
    
    void MissionGameplay()
    {
        NightroItemGiverUIManager.Init();
    }
    
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);
        
        // Track key states for Ctrl+I combination
        if (key == KeyCode.KC_LCONTROL || key == KeyCode.KC_RCONTROL)
        {
            m_NightroCtrlPressed = true;
        }
        else if (key == KeyCode.KC_I && m_NightroCtrlPressed && !m_NightroKeyComboTriggered)
        {
            PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
            if (player && player.IsAlive() && !player.IsUnconscious() && !player.IsRestrained())
            {
                NightroItemGiverUIManager.ToggleMenu();
                m_NightroKeyComboTriggered = true; // Prevent multiple triggers
            }
        }
        
        // Close menu with Escape
        if (key == KeyCode.KC_ESCAPE && NightroItemGiverUIManager.IsMenuOpen())
        {
            NightroItemGiverUIManager.CloseMenu();
        }
    }
    
    override void OnKeyRelease(int key)
    {
        super.OnKeyRelease(key);
        
        // Reset key states
        if (key == KeyCode.KC_LCONTROL || key == KeyCode.KC_RCONTROL)
        {
            m_NightroCtrlPressed = false;
            m_NightroKeyComboTriggered = false; // Reset combo trigger when Ctrl is released
        }
        else if (key == KeyCode.KC_I)
        {
            m_NightroKeyComboTriggered = false; // Reset combo trigger when I is released
        }
    }
    
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        
        // Additional input handling if needed
        // Currently everything is handled in OnKeyPress/OnKeyRelease
    }
}