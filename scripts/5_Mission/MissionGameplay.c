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
        
        // ⭐ REDUCED: Check for file updates every 5 seconds (was 2 seconds)
        static float lastSyncCheck = 0;
        if (GetGame().GetTime() - lastSyncCheck > 5000)
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
        
        // ⭐ REDUCED LOG SPAM: Only log significant changes
        static string lastUpdateTime = "";
        if (FileExist(clientFile))
        {
            ItemQueue queue = new ItemQueue();
            JsonFileLoader<ItemQueue>.JsonLoadFile(clientFile, queue);
            
            if (queue && queue.last_updated && queue.last_updated != lastUpdateTime)
            {
                lastUpdateTime = queue.last_updated;
                
                // Only log if it's a significant update (not just sync updates)
                if (queue.last_updated.IndexOf("delivered_") >= 0 || queue.last_updated.IndexOf("created") >= 0)
                {
                    PrintFormat("[NIGHTRO CLIENT DEBUG] Detected significant file update: %1", queue.last_updated);
                }
                
                // Refresh menu if it's open
                if (NightroItemGiverUIManager.IsMenuOpen())
                {
                    NightroItemGiverUIManager.RefreshMenuIfOpen();
                }
            }
        }
    }
    
    // ⭐ REDUCED SPAM: Only process important shop notifications
    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);
        
        if (eventTypeId == ChatMessageEventTypeID)
        {
            ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
            if (chatParams)
            {
                string message = chatParams.param3; // The actual message content
                
                // Only process shop notifications that need action
                if (message.IndexOf("[NIGHTRO SHOP]") == 0)
                {
                    // Only log important messages, not routine sync notifications
                    bool isImportant = false;
                    if (message.IndexOf("You have items waiting!") > 0) isImportant = true;
                    if (message.IndexOf("Successfully received:") > 0) isImportant = true; 
                    if (message.IndexOf("ERROR") > 0) isImportant = true;
                    
                    if (isImportant)
                    {
                        PrintFormat("[NIGHTRO CLIENT DEBUG] Important shop message: %1", message);
                    }
                    
                    NightroItemSyncClient.ProcessSyncMessage(message);
                    // Let the message show in chat normally
                }
            }
        }
    }
}