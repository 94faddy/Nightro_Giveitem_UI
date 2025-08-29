class NightroItemGiverMenu extends UIScriptedMenu
{
    private Widget m_MainPanel;
    private Widget m_HeaderPanel;
    private ButtonWidget m_CloseButton;
    private ButtonWidget m_ReceiveAllButton;
    private ButtonWidget m_RefreshButton;
    private Widget m_ItemContainer;
    private Widget m_EmptyStatePanel;
    private TextWidget m_EmptyMessage;
    private ScrollWidget m_ItemScrollView;
    private TextWidget m_StatusText;
    
    private ref array<ref NightroItemWidget> m_ItemWidgets;
    private ref array<ref ItemInfo> m_PendingItems;
    private bool m_IsInitialized = false;
    
    void NightroItemGiverMenu()
    {
        m_ItemWidgets = new array<ref NightroItemWidget>;
        m_PendingItems = new array<ref ItemInfo>;
    }
    
    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("Nightro_Giveitem/gui/layouts/nightro_itemgiver_menu.layout");
        
        if (!layoutRoot)
        {
            ErrorEx("[NIGHTRO CLIENT ERROR] Failed to load menu layout!");
            return null;
        }
        
        // Find all widgets
        m_MainPanel = layoutRoot.FindAnyWidget("MainPanel");
        m_HeaderPanel = layoutRoot.FindAnyWidget("HeaderPanel");
        m_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));
        m_ReceiveAllButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ReceiveAllButton"));
        m_RefreshButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("RefreshButton"));
        m_ItemContainer = layoutRoot.FindAnyWidget("ItemContainer");
        m_EmptyStatePanel = layoutRoot.FindAnyWidget("EmptyStatePanel");
        m_EmptyMessage = TextWidget.Cast(layoutRoot.FindAnyWidget("EmptyMessage"));
        m_ItemScrollView = ScrollWidget.Cast(layoutRoot.FindAnyWidget("ItemScrollView"));
        m_StatusText = TextWidget.Cast(layoutRoot.FindAnyWidget("StatusText"));
        
        // Debug widget loading
        PrintFormat("[NIGHTRO CLIENT DEBUG] === Widget Loading Status ===");
        PrintFormat("[NIGHTRO CLIENT DEBUG] MainPanel: %1", m_MainPanel != null);
        PrintFormat("[NIGHTRO CLIENT DEBUG] CloseButton: %1", m_CloseButton != null);
        PrintFormat("[NIGHTRO CLIENT DEBUG] ItemContainer: %1", m_ItemContainer != null);
        PrintFormat("[NIGHTRO CLIENT DEBUG] EmptyStatePanel: %1", m_EmptyStatePanel != null);
        PrintFormat("[NIGHTRO CLIENT DEBUG] EmptyMessage: %1", m_EmptyMessage != null);
        PrintFormat("[NIGHTRO CLIENT DEBUG] ItemScrollView: %1", m_ItemScrollView != null);
        PrintFormat("[NIGHTRO CLIENT DEBUG] StatusText: %1", m_StatusText != null);
        
        m_IsInitialized = true;
        
        // ⭐ CRITICAL FIX: Delay loading to prevent LBmaster crashes
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SafeLoadPendingItems, 1500, false);
        
        return layoutRoot;
    }
    
    override void OnShow()
    {
        super.OnShow();
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Menu showing, setting up input focus");
        
        GetGame().GetInput().ChangeGameFocus(1);
        GetGame().GetUIManager().ShowUICursor(true);
        
        // ⭐ CRITICAL FIX: Only request sync, don't immediately load items
        RequestServerSync();
        
        // ⭐ CRITICAL FIX: Delay any LBmaster-related operations
        if (m_IsInitialized)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SafeLoadPendingItems, 2000, false);
        }
    }
    
    override void OnHide()
    {
        super.OnHide();
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Menu hiding, restoring input focus");
        
        GetGame().GetInput().ResetGameFocus();
        GetGame().GetUIManager().ShowUICursor(false);
    }
    
    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == m_CloseButton)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Close button clicked");
            Close();
            return true;
        }
        else if (w == m_ReceiveAllButton)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Receive all button clicked");
            OnReceiveAllClicked();
            return true;
        }
        else if (w == m_RefreshButton)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Refresh button clicked");
            RequestServerSync();
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SafeLoadPendingItems, 1500, false);
            return true;
        }
        
        return super.OnClick(w, x, y, button);
    }
    
    override bool OnKeyDown(Widget w, int x, int y, int key)
    {
        if (key == KeyCode.KC_ESCAPE)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Escape key pressed, closing menu");
            Close();
            return true;
        }
        
        return super.OnKeyDown(w, x, y, key);
    }
    
    void RequestServerSync()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity()) return;
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Requesting server sync for fresh data");
        
        // Request fresh data from server by creating a sync request file
        string steamID = player.GetIdentity().GetPlainId();
        string requestPath = "$profile:NightroItemGiverData/sync_requests/" + steamID + "_" + GetGame().GetTime() + ".txt";
        
        // Ensure directory exists
        string dirPath = "$profile:NightroItemGiverData/sync_requests/";
        if (!FileExist(dirPath))
        {
            MakeDirectory(dirPath);
        }
        
        // Create sync request file
        FileHandle requestFile = OpenFile(requestPath, FileMode.WRITE);
        if (requestFile)
        {
            FPrintln(requestFile, "sync_request");
            CloseFile(requestFile);
            PrintFormat("[NIGHTRO CLIENT DEBUG] Created sync request file");
        }
        
        UpdateStatusText("Refreshing data from server...");
    }
    
    // ⭐ CRITICAL FIX: Safe loading with minimal system calls
    void SafeLoadPendingItems()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity()) 
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] No valid player found!");
            UpdateEmptyState("No player data available");
            return;
        }
        
        // ⭐ CRITICAL FIX: Minimal safety checks - don't touch player inventory/state
        if (!player.IsAlive())
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Player not alive, delaying load");
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SafeLoadPendingItems, 2000, false);
            return;
        }
        
        // ⭐ CRITICAL FIX: Don't check player status or inventory here - just load file data
        LoadPendingItemsWithoutValidation();
    }
    
    void LoadPendingItemsWithoutValidation()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity()) 
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] No valid player found!");
            UpdateEmptyState("No player data available");
            return;
        }
        
        string steamID = player.GetIdentity().GetPlainId();
        string playerName = player.GetIdentity().GetName();
        
        // Always use server file path
        string serverFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        string filePath = serverFilePath;
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] === Loading Pending Items (Safe Mode) ===");
        PrintFormat("[NIGHTRO CLIENT DEBUG] Player: %1 (%2)", playerName, steamID);
        PrintFormat("[NIGHTRO CLIENT DEBUG] Direct server file path: %1", filePath);
        
        ClearItemWidgets();
        
        if (!FileExist(filePath))
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Server file doesn't exist: %1", filePath);
            UpdateEmptyState("Waiting for server data...");
            UpdateStatusText("No server data file found");
            return;
        }
        
        // ⭐ CRITICAL FIX: Safe file reading with validation
        string fileContent = "";
        
        FileHandle fileHandle = OpenFile(filePath, FileMode.READ);
        if (!fileHandle)
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] Cannot open server file: %1", filePath);
            UpdateEmptyState("Error reading server data");
            return;
        }
        
        string line;
        int lineCount = 0;
        while (FGets(fileHandle, line) >= 0 && lineCount < 1000)
        {
            fileContent += line;
            lineCount++;
        }
        CloseFile(fileHandle);
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Successfully read file with %1 lines", lineCount);
        
        if (fileContent == "")
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] File read failed or empty content");
            UpdateEmptyState("No data available");
            UpdateStatusText("Server file is empty");
            return;
        }
        
        // ⭐ CRITICAL FIX: Validate JSON content before parsing
        if (!IsValidJSONContent(fileContent))
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] Invalid JSON content detected, attempting to recreate");
            DeleteFile(filePath); // Delete corrupted file
            UpdateEmptyState("Server data corrupted, requesting refresh...");
            UpdateStatusText("Data corrupted - requesting fresh data");
            RequestServerSync(); // Request fresh data from server
            return;
        }
        
        int maxChars = 800;
        if (fileContent.Length() < maxChars) maxChars = fileContent.Length();
        PrintFormat("[NIGHTRO CLIENT DEBUG] File content (%1 chars): %2", 
            fileContent.Length(), fileContent.Substring(0, maxChars));
        
        // Check if this looks like it has items
        if (fileContent.IndexOf("\"items_to_give\": []") >= 0)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Server file has empty items_to_give array");
        }
        else if (fileContent.IndexOf("\"classname\"") >= 0)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Server file appears to contain items!");
        }
        
        // ⭐ CRITICAL FIX: Safe JSON loading with validation  
        ItemQueue itemQueue = new ItemQueue();
        JsonFileLoader<ItemQueue>.JsonLoadFile(filePath, itemQueue);
        
        if (!itemQueue)
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] Failed to load JSON from server file");
            DeleteFile(filePath);
            UpdateEmptyState("Error loading server data");
            UpdateStatusText("Failed to parse server file");
            RequestServerSync();
            return;
        }
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] JsonLoadFile completed successfully");
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] ItemQueue player_name: '%1'", itemQueue.player_name);
        PrintFormat("[NIGHTRO CLIENT DEBUG] ItemQueue steam_id: '%1'", itemQueue.steam_id);
        PrintFormat("[NIGHTRO CLIENT DEBUG] ItemQueue last_updated: '%1'", itemQueue.last_updated);
        PrintFormat("[NIGHTRO CLIENT DEBUG] ItemQueue items_to_give exists: %1", itemQueue.items_to_give != null);
        
        // Check if this is an empty sync
        if (itemQueue.last_updated.IndexOf("empty") >= 0)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Received empty sync from server");
            UpdateEmptyState("No pending items");
            UpdateStatusText("No items waiting for delivery");
            return;
        }
        
        if (!itemQueue.items_to_give)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] No items_to_give array, creating empty array");
            itemQueue.items_to_give = new array<ref ItemInfo>;
        }
        
        int itemCount = itemQueue.items_to_give.Count();
        PrintFormat("[NIGHTRO CLIENT DEBUG] Items count from server file: %1", itemCount);
        
        if (itemCount == 0)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] No items found in server file");
            UpdateEmptyState("No pending items");
            UpdateStatusText("No items waiting for delivery");
            return;
        }
        
        // Clear and populate items
        m_PendingItems.Clear();
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] === Processing Items From Server File ===");
        for (int i = 0; i < itemCount; i++)
        {
            ItemInfo item = itemQueue.items_to_give.Get(i);
            if (item)
            {
                PrintFormat("[NIGHTRO CLIENT DEBUG] Server Item %1: classname='%2', quantity=%3, status='%4'", 
                    i, item.classname, item.quantity, item.status);
                
                // Accept items with empty status as pending
                if (item.classname != "" && item.quantity > 0)
                {
                    if (item.status == "")
                    {
                        item.status = "pending"; // Set default status
                    }
                    
                    m_PendingItems.Insert(item);
                    PrintFormat("[NIGHTRO CLIENT DEBUG] Added server item to pending list: %1 x%2 (status: %3)", 
                        item.classname, item.quantity, item.status);
                }
                else
                {
                    PrintFormat("[NIGHTRO CLIENT DEBUG] Skipped invalid server item: classname='%1', quantity=%2", 
                        item.classname, item.quantity);
                }
            }
            else
            {
                PrintFormat("[NIGHTRO CLIENT DEBUG] Server Item %1 is null", i);
            }
        }
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Final pending items count from server: %1", m_PendingItems.Count());
        
        if (m_PendingItems.Count() > 0)
        {
            CreateItemWidgets();
            ShowItemsList();
            UpdateStatusText(string.Format("Found %1 items ready for pickup", m_PendingItems.Count()));
        }
        else
        {
            UpdateEmptyState("All items have been processed");
            UpdateStatusText("No items available");
        }
        
        UpdateButtonStates();
    }
    
    // Use the original LoadPendingItems name for compatibility
    void LoadPendingItems()
    {
        LoadPendingItemsWithoutValidation();
    }
    
    // ⭐ NEW: Validate JSON content before parsing
    bool IsValidJSONContent(string content)
    {
        if (content == "" || content.Length() < 10)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Content too short to be valid JSON");
            return false;
        }
        
        // Check for basic JSON structure
        if (content.IndexOf("{") < 0 || content.IndexOf("}") < 0)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Missing basic JSON brackets");
            return false;
        }
        
        // Check for required fields
        if (content.IndexOf("\"steam_id\"") < 0)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Missing required steam_id field");
            return false;
        }
        
        if (content.IndexOf("\"items_to_give\"") < 0)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Missing required items_to_give field");
            return false;
        }
        
        // Check for basic JSON syntax issues
        int openBraces = 0;
        int closeBraces = 0;
        for (int i = 0; i < content.Length(); i++)
        {
            string char = content.Get(i);
            if (char == "{") openBraces++;
            if (char == "}") closeBraces++;
        }
        
        if (openBraces != closeBraces)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Mismatched braces: open=%1, close=%2", openBraces, closeBraces);
            return false;
        }
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] JSON content appears valid");
        return true;
    }
    
    void ClearItemWidgets()
    {
        PrintFormat("[NIGHTRO CLIENT DEBUG] Clearing %1 existing item widgets", m_ItemWidgets.Count());
        
        for (int i = 0; i < m_ItemWidgets.Count(); i++)
        {
            if (m_ItemWidgets.Get(i))
            {
                delete m_ItemWidgets.Get(i);
            }
        }
        m_ItemWidgets.Clear();
    }
    
    void CreateItemWidgets()
    {
        if (!m_ItemContainer) 
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] ItemContainer is null!");
            return;
        }
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] === Creating Item Widgets ===");
        PrintFormat("[NIGHTRO CLIENT DEBUG] Creating %1 item widgets", m_PendingItems.Count());
        
        for (int i = 0; i < m_PendingItems.Count(); i++)
        {
            ItemInfo item = m_PendingItems.Get(i);
            if (!item) 
            {
                PrintFormat("[NIGHTRO CLIENT DEBUG] Item %1 is null, skipping", i);
                continue;
            }
            
            PrintFormat("[NIGHTRO CLIENT DEBUG] Creating widget for item %1: %2", i, item.classname);
            
            NightroItemWidget itemWidget = new NightroItemWidget(m_ItemContainer, item, this);
            if (itemWidget)
            {
                m_ItemWidgets.Insert(itemWidget);
                PrintFormat("[NIGHTRO CLIENT DEBUG] Successfully created widget for: %1", item.classname);
            }
            else
            {
                PrintFormat("[NIGHTRO CLIENT ERROR] Failed to create widget for: %1", item.classname);
            }
        }
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Created %1 item widgets total", m_ItemWidgets.Count());
        
        // Force container update
        if (m_ItemContainer)
        {
            m_ItemContainer.Update();
        }
        if (m_ItemScrollView)
        {
            m_ItemScrollView.Update();
        }
    }
    
    void UpdateEmptyState(string message = "")
    {
        PrintFormat("[NIGHTRO CLIENT DEBUG] UpdateEmptyState called with message: '%1'", message);
        
        if (m_EmptyStatePanel)
        {
            m_EmptyStatePanel.Show(true);
            PrintFormat("[NIGHTRO CLIENT DEBUG] Showing empty state panel");
        }
        
        if (m_EmptyMessage && message != "")
        {
            m_EmptyMessage.SetText(message);
            PrintFormat("[NIGHTRO CLIENT DEBUG] Set empty message to: %1", message);
        }
        
        if (m_ItemScrollView)
        {
            m_ItemScrollView.Show(false);
            PrintFormat("[NIGHTRO CLIENT DEBUG] Hiding item scroll view");
        }
        
        UpdateButtonStates();
    }
    
    void ShowItemsList()
    {
        PrintFormat("[NIGHTRO CLIENT DEBUG] ShowItemsList called");
        
        if (m_EmptyStatePanel)
        {
            m_EmptyStatePanel.Show(false);
            PrintFormat("[NIGHTRO CLIENT DEBUG] Hiding empty state panel");
        }
        
        if (m_EmptyMessage)
        {
            m_EmptyMessage.Show(false);
        }
        
        if (m_ItemScrollView)
        {
            m_ItemScrollView.Show(true);
            PrintFormat("[NIGHTRO CLIENT DEBUG] Showing item scroll view");
        }
    }
    
    void UpdateStatusText(string text)
    {
        if (m_StatusText)
        {
            m_StatusText.SetText(text);
            PrintFormat("[NIGHTRO CLIENT DEBUG] Status text updated to: %1", text);
        }
    }
    
    void UpdateButtonStates()
    {
        if (!m_ReceiveAllButton) return;
        
        bool hasReceivableItems = false;
        int receivableCount = 0;
        
        for (int i = 0; i < m_PendingItems.Count(); i++)
        {
            ItemInfo item = m_PendingItems.Get(i);
            if (item && (item.status == "pending" || item.status == "" || item.status == "inventory_full" || item.status == "territory_failed"))
            {
                hasReceivableItems = true;
                receivableCount++;
            }
        }
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Button state update - receivable items: %1 (%2 total)", receivableCount, m_PendingItems.Count());
        
        m_ReceiveAllButton.Enable(hasReceivableItems);
        if (!hasReceivableItems)
        {
            m_ReceiveAllButton.SetColor(ARGB(255, 100, 100, 100));
            m_ReceiveAllButton.SetText("NO ITEMS AVAILABLE");
        }
        else
        {
            m_ReceiveAllButton.SetColor(ARGB(255, 255, 255, 255));
            m_ReceiveAllButton.SetText(string.Format("RECEIVE ALL (%1)", receivableCount.ToString()));
        }
    }
    
    void OnReceiveItemClicked(ref ItemInfo itemData)
    {
        if (!itemData) 
            return;
        
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player) 
            return;
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] === Receive Item Clicked ===");
        PrintFormat("[NIGHTRO CLIENT DEBUG] Item: %1 x%2", itemData.classname, itemData.quantity);
        
        // ⭐ CRITICAL FIX: Skip LBmaster validation during UI interaction
        string errorMessage;
        bool canReceive = NightroItemGiverUIManager.CanReceiveItemBasic(player, itemData, errorMessage);
        
        if (!canReceive)
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] Cannot receive item: %1", errorMessage);
            GetGame().ChatMP(player, errorMessage, "ColorRed");
            return;
        }
        
        bool success = NightroItemGiverUIManager.GiveItemToPlayer(player, itemData);
        
        if (success)
        {
            string processingMsg = "[NIGHTRO SHOP] Processing delivery request: " + itemData.quantity.ToString() + "x " + itemData.classname;
            PrintFormat("[NIGHTRO CLIENT SUCCESS] %1", processingMsg);
            GetGame().ChatMP(player, processingMsg, "ColorYellow");
            
            // Refresh items after short delay to get server response
            UpdateStatusText("Processing delivery request...");
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(LoadPendingItems, 2000, false);
        }
        else
        {
            string errorMsg = "[NIGHTRO SHOP ERROR] Failed to send delivery request. Please try again.";
            PrintFormat("[NIGHTRO CLIENT ERROR] %1", errorMsg);
            GetGame().ChatMP(player, errorMsg, "ColorRed");
        }
    }
    
    void OnReceiveAllClicked()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player) 
            return;
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] === Receive All Items Clicked ===");
        
        int requestCount = 0;
        
        for (int i = 0; i < m_PendingItems.Count(); i++)
        {
            ItemInfo item = m_PendingItems.Get(i);
            if (!item || item.status == "delivered") 
                continue;
            
            if (item.status == "pending" || item.status == "" || item.status == "inventory_full" || item.status == "territory_failed")
            {
                string errorMessage;
                // ⭐ CRITICAL FIX: Use basic validation only
                if (NightroItemGiverUIManager.CanReceiveItemBasic(player, item, errorMessage))
                {
                    if (NightroItemGiverUIManager.GiveItemToPlayer(player, item))
                    {
                        requestCount++;
                        PrintFormat("[NIGHTRO CLIENT DEBUG] Sent delivery request for: %1", item.classname);
                    }
                    else
                    {
                        PrintFormat("[NIGHTRO CLIENT ERROR] Failed to send request for item: %1", item.classname);
                    }
                }
                else
                {
                    PrintFormat("[NIGHTRO CLIENT ERROR] Cannot request item %1: %2", item.classname, errorMessage);
                }
            }
        }
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Receive all results: %1 delivery requests sent", requestCount);
        
        if (requestCount > 0)
        {
            string resultMsg = "[NIGHTRO SHOP] Processing " + requestCount.ToString() + " delivery requests. Please wait...";
            GetGame().ChatMP(player, resultMsg, "ColorYellow");
            UpdateStatusText("Processing delivery requests...");
            
            // Refresh items after delay to get server response
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(LoadPendingItems, 3000, false);
        }
        else
        {
            string noItemsMsg = "[NIGHTRO SHOP] No items available to receive.";
            GetGame().ChatMP(player, noItemsMsg, "ColorWhite");
            UpdateStatusText("No items available for pickup");
        }
    }
}