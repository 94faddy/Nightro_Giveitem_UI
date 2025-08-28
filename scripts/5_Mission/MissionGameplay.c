modded class MissionGameplay
{
    private bool m_NightroCtrlPressed = false;
    private bool m_NightroIPressed = false;
    private bool m_NightroKeyComboTriggered = false;
    
    void MissionGameplay()
    {
        NightroItemGiverUIManager.Init();
        
        if (GetGame().IsClient())
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] MissionGameplay initialized on client");
        }
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
                PrintFormat("[NIGHTRO CLIENT DEBUG] Ctrl+I pressed, toggling menu");
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
    
    // CRITICAL: Handle sync messages from server
    void SendNightroNotification(string message, string color = "ColorWhite")
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
        {
            // Check if this is a sync message
            if (message.IndexOf("[NIGHTRO_SYNC_") == 0)
            {
                PrintFormat("[NIGHTRO CLIENT DEBUG] Received sync message from server");
                NightroItemSyncClient.ProcessSyncMessage(message);
                return; // Don't display sync messages as regular chat
            }
            
            GetGame().ChatMP(player, message, color);
        }
    }
}