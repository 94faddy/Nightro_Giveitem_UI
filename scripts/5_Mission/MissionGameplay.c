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
        string clientFile = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        // Check if file was updated
        static string lastUpdateTime = "";
        if (FileExist(clientFile))
        {
            ItemQueue queue = new ItemQueue();
            JsonFileLoader<ItemQueue>.JsonLoadFile(clientFile, queue);
            
            if (queue && queue.last_updated && queue.last_updated != lastUpdateTime)
            {
                lastUpdateTime = queue.last_updated;
                PrintFormat("[NIGHTRO CLIENT DEBUG] Detected file update: %1", queue.last_updated);
                
                // Refresh menu if it's open
                if (NightroItemGiverUIManager.IsMenuOpen())
                {
                    NightroItemGiverUIManager.RefreshMenuIfOpen();
                }
            }
        }
    }
    
    // ⭐ FIXED: Properly handle chat messages for sync
    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);
        
        if (eventTypeId == ChatMessageEventTypeID)
        {
            ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
            if (chatParams)
            {
                string message = chatParams.param3; // The actual message content
                
                // Check for sync messages
                if (message.IndexOf("[NIGHTRO_SYNC_DATA]") == 0)
                {
                    PrintFormat("[NIGHTRO CLIENT DEBUG] Intercepted sync message: %1", message.Substring(0, Math.Min(50, message.Length())));
                    NightroItemSyncClient.ProcessSyncMessage(message);
                    
                    // Prevent the message from showing in chat
                    chatParams.param3 = ""; // Clear the message
                }
                else if (message.IndexOf("[NIGHTRO SHOP]") == 0)
                {
                    // Let shop messages through but log them
                    PrintFormat("[NIGHTRO CLIENT DEBUG] Shop message: %1", message);
                }
            }
        }
    }
}