modded class MissionGameplay
{
    private bool m_NightroCtrlPressed = false;
    private bool m_NightroIPressed = false;
    private bool m_NightroKeyComboTriggered = false;
    
    // ⭐ CRITICAL FIX: Add safety flags to prevent excessive file operations
    private static float m_LastSafeUpdateCheck = 0;
    private static bool m_IsCheckingUpdates = false;
    private static int m_ConsecutiveFailures = 0;
    private static const int MAX_CONSECUTIVE_FAILURES = 5;
    private static const float UPDATE_CHECK_INTERVAL = 15000; // Increased from 5000 to 15000 (15 seconds)
    
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
        
        // ⭐ CRITICAL FIX: DISABLE ALL AUTO-UPDATE CHECKING TO PREVENT CRASHES
        // Updates will only happen when user manually opens the menu
        // This eliminates all file I/O operations during normal gameplay
        
        // NO MORE AUTOMATIC FILE CHECKING - PREVENTS ALL HEAP CORRUPTION ISSUES
    }
    
    // ⭐ CRITICAL FIX: REMOVED - All automatic update checking functions disabled
    // CheckForSyncUpdatesSafe() - REMOVED
    // CheckFileUpdatesSafer() - REMOVED  
    // RefreshMenuSafe() - REMOVED
    
    // Updates now only happen when:
    // 1. Player manually opens menu (Ctrl+I)
    // 2. Player clicks refresh button
    // 3. Player performs an action in the menu
    
    // ⭐ CRITICAL FIX: SIMPLIFIED - Only process delivery confirmations, no file operations
    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);
        
        if (eventTypeId == ChatMessageEventTypeID)
        {
            ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
            if (chatParams)
            {
                string message = chatParams.param3;
                
                // Only process successful delivery messages to refresh UI
                if (message.IndexOf("[NIGHTRO SHOP SUCCESS]") == 0)
                {
                    PrintFormat("[NIGHTRO CLIENT DEBUG] Delivery success message received");
                    // Only refresh menu if it's currently open
                    if (NightroItemGiverUIManager.IsMenuOpen())
                    {
                        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TriggerMenuRefresh, 2000, false);
                    }
                }
                // Let all messages show in chat normally
            }
        }
    }
    
    // ⭐ NEW: Simple menu refresh trigger
    void TriggerMenuRefresh()
    {
        if (NightroItemGiverUIManager.IsMenuOpen())
        {
            NightroItemGiverUIManager.RefreshMenuIfOpen();
        }
    }
}