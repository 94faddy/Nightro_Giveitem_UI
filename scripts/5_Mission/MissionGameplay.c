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
        
        // Check for sync messages via file system every 2 seconds
        static float lastSyncCheck = 0;
        if (GetGame().GetTime() - lastSyncCheck > 2000)
        {
            lastSyncCheck = GetGame().GetTime();
            CheckForSyncUpdates();
        }
    }
    
    void CheckForSyncUpdates()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity()) return;
        
        string steamID = player.GetIdentity().GetPlainId();
        string serverFile = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        if (FileExist(serverFile))
        {
            // Read file to check if it has been updated recently
            ItemQueue queue = new ItemQueue();
            JsonFileLoader<ItemQueue>.JsonLoadFile(serverFile, queue);
            
            if (queue && queue.last_updated && queue.last_updated.IndexOf("server_sync_") == 0)
            {
                PrintFormat("[NIGHTRO CLIENT DEBUG] Detected server sync file update: %1", queue.last_updated);
                
                // Refresh menu if it's open
                if (NightroItemGiverUIManager.IsMenuOpen())
                {
                    NightroItemGiverUIManager.RefreshMenuIfOpen();
                }
            }
        }
    }
    
    // CRITICAL: Handle sync messages from server
    void SendNightroNotification(string message, string color = "ColorWhite")
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
        {
            // Check if this is a sync message
            if (message.IndexOf("[NIGHTRO_SYNC_FILE]") == 0)
            {
                PrintFormat("[NIGHTRO CLIENT DEBUG] Received sync message from SendNightroNotification: %1", message);
                NightroItemSyncClient.ProcessSyncMessage(message);
                return; // Don't display sync messages as regular chat
            }
            
            GetGame().ChatMP(player, message, color);
        }
    }
}