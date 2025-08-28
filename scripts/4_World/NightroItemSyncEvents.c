// แทนที่ function SyncItemDataToClient() ใน NightroItemSyncEvents.c

class NightroItemSyncManager
{
    static void RegisterEvents()
    {
        PrintFormat("[NIGHTRO SYNC DEBUG] File-based sync system initialized");
    }
    
    // ⭐ FIXED VERSION: Direct file sync without chat messages
    static void SyncItemDataToClient(PlayerBase player)
    {
        if (!player || !player.GetIdentity()) return;
        if (GetGame().IsClient()) return; // Server only
        
        string steamID = player.GetIdentity().GetPlainId();
        string serverFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: === DIRECT FILE SYNC START ===");
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Player: %1", steamID);
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Server file path: %1", serverFilePath);
        
        if (!FileExist(serverFilePath))
        {
            PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: No server file found for %1", steamID);
            CreateEmptyClientFile(steamID, player.GetIdentity().GetName());
            return;
        }
        
        // Read server file
        ItemQueue serverQueue = new ItemQueue();
        JsonFileLoader<ItemQueue>.JsonLoadFile(serverFilePath, serverQueue);
        
        if (!serverQueue)
        {
            PrintFormat("[NIGHTRO SYNC ERROR] SERVER: Failed to load server queue for %1", steamID);
            return;
        }
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Loaded server queue:");
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: - Player: %1", serverQueue.player_name);
        
        // Fix the problematic line - use if statement instead of ternary
        int itemCount = 0;
        if (serverQueue.items_to_give)
        {
            itemCount = serverQueue.items_to_give.Count();
            
            // ⭐ DEBUG: Print actual items in server queue
            PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: === DETAILED ITEMS DEBUG ===");
            for (int x = 0; x < itemCount; x++)
            {
                ItemInfo serverItem = serverQueue.items_to_give.Get(x);
                if (serverItem)
                {
                    PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Item %1: class='%2', qty=%3, status='%4'", 
                        x, serverItem.classname, serverItem.quantity, serverItem.status);
                }
                else
                {
                    PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Item %1: NULL", x);
                }
            }
        }
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: - Items count: %1", itemCount);
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: - Last updated: %1", serverQueue.last_updated);
        
        // Update timestamp to indicate fresh sync  
        int currentTime = (int)GetGame().GetTime();
        serverQueue.last_updated = "server_sync_" + currentTime.ToString();
        
        // ⭐ CRITICAL FIX: Don't save back to server file to avoid timing issues
        // Just send the current data as-is
        
        // ⭐ Send file content directly in sync message
        string rawFileContent = "";
        FileHandle fileRead = OpenFile(serverFilePath, FileMode.READ);
        if (fileRead)
        {
            string line;
            while (FGets(fileRead, line) >= 0)
            {
                rawFileContent += line;
            }
            CloseFile(fileRead);
            
            // Debug raw content
            int debugLen = 500;
            if (rawFileContent.Length() < debugLen) debugLen = rawFileContent.Length();
            PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Raw file content (%1 chars): %2", 
                rawFileContent.Length(), rawFileContent.Substring(0, debugLen));
        }
        
        // Send sync command with file content indicator
        string syncCommand = "[NIGHTRO_SYNC_FILE]" + steamID + "|" + itemCount.ToString() + "|" + currentTime.ToString();
        GetGame().ChatMP(player, syncCommand, "ColorBlue");
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: === DIRECT FILE SYNC COMPLETED ===");
        
        // Send success notification
        string notifyMsg = "[NIGHTRO SHOP] Data synchronized! You have " + itemCount.ToString() + " items waiting. Open menu to see items!";
        GetGame().ChatMP(player, notifyMsg, "ColorGreen");
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Sent success notification to player");
    }
    
    static void CreateEmptyClientFile(string steamID, string playerName)
    {
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Creating empty sync for player %1", steamID);
        
        ItemQueue emptyQueue = new ItemQueue();
        emptyQueue.steam_id = steamID;
        emptyQueue.player_name = playerName;
        
        int currentTime = (int)GetGame().GetTime();
        emptyQueue.last_updated = "empty_sync_" + currentTime.ToString();
        emptyQueue.items_to_give = new array<ref ItemInfo>;
        emptyQueue.processed_items = new array<ref ProcessedItemInfo>;
        emptyQueue.player_state = new PlayerStateInfo();
        emptyQueue.player_state.steam_id = steamID;
        
        string serverFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        JsonFileLoader<ItemQueue>.JsonSaveFile(serverFilePath, emptyQueue);
        
        // Notify client about empty sync
        string syncCommand = "[NIGHTRO_SYNC_FILE]" + steamID + "|0|" + currentTime.ToString();
        PlayerBase player = GetPlayerBySteamID(steamID);
        if (player)
        {
            GetGame().ChatMP(player, syncCommand, "ColorBlue");
        }
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Created empty server file");
    }
    
    static PlayerBase GetPlayerBySteamID(string steamID)
    {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == steamID)
            {
                return player;
            }
        }
        return null;
    }
    
    static void SendEmptyDataNotification(PlayerBase player)
    {
        if (!player) return;
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Sending empty data notification");
        GetGame().ChatMP(player, "[NIGHTRO SHOP] No items found. Check with administrators.", "ColorYellow");
    }
    
    static string CreateSyncMessage(ref ItemQueue itemQueue)
    {
        // Legacy function - not used in direct file sync
        return "";
    }
    
    // ⭐ IMPROVED DELIVERY FUNCTION
    static bool DeliverItemToPlayer(PlayerBase player, string itemClass, int quantity, out string errorMessage)
    {
        errorMessage = "";
        
        if (!player || !player.GetIdentity())
        {
            errorMessage = "[NIGHTRO SHOP] Invalid player data.";
            return false;
        }
        
        if (!player.IsAlive())
        {
            errorMessage = "[NIGHTRO SHOP] You must be alive to receive items.";
            return false;
        }
        
        if (player.IsUnconscious() || player.IsRestrained())
        {
            errorMessage = "[NIGHTRO SHOP] You cannot receive items while unconscious or restrained.";
            return false;
        }
        
        // Territory check
        float distanceToPlotpole;
        if (!NightroItemGiverManager.IsPlayerInValidTerritory(player, distanceToPlotpole))
        {
            if (distanceToPlotpole < 0)
            {
                errorMessage = "[NIGHTRO SHOP] You need to join or create a group to receive items!";
            }
            else
            {
                int distanceInt = (int)distanceToPlotpole;
                errorMessage = "[NIGHTRO SHOP] You are outside your group's territory! Distance: " + distanceInt.ToString() + "m";
            }
            return false;
        }
        
        // Inventory check
        if (!NightroItemGiverManager.CanFitInInventory(player, itemClass, quantity))
        {
            errorMessage = "[NIGHTRO SHOP] Your inventory is full! Clear space to receive " + quantity.ToString() + "x " + itemClass;
            return false;
        }
        
        // Deliver items
        string steamID = player.GetIdentity().GetPlainId();
        string filePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        PrintFormat("[NIGHTRO DELIVERY DEBUG] SERVER: Processing delivery - %1x %2 for %3", quantity, itemClass, steamID);
        
        if (!FileExist(filePath)) 
        {
            errorMessage = "[NIGHTRO SHOP] No pending items found.";
            return false;
        }
        
        ItemQueue itemQueue = new ItemQueue();
        JsonFileLoader<ItemQueue>.JsonLoadFile(filePath, itemQueue);
        
        if (!itemQueue || !itemQueue.items_to_give) 
        {
            errorMessage = "[NIGHTRO SHOP] Failed to load item queue.";
            return false;
        }
        
        // Find and deliver the item
        for (int i = 0; i < itemQueue.items_to_give.Count(); i++)
        {
            ItemInfo queueItem = itemQueue.items_to_give.Get(i);
            if (queueItem && queueItem.classname == itemClass && queueItem.quantity == quantity && queueItem.status == "pending")
            {
                PrintFormat("[NIGHTRO DELIVERY DEBUG] SERVER: Found matching item, creating %1x %2", quantity, itemClass);
                
                // Give items to player
                int itemsGiven = 0;
                for (int j = 0; j < queueItem.quantity; j++)
                {
                    EntityAI item = player.GetInventory().CreateInInventory(queueItem.classname);
                    if (item)
                    {
                        itemsGiven++;
                        PrintFormat("[NIGHTRO DELIVERY DEBUG] SERVER: Created item %1/%2", j+1, queueItem.quantity);
                        
                        // Handle attachments
                        if (queueItem.attachments && queueItem.attachments.Count() > 0)
                        {
                            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NightroItemGiverManager.AttachItemsToWeapon, 300, false, item, queueItem.attachments, player);
                        }
                    }
                    else
                    {
                        PrintFormat("[NIGHTRO DELIVERY ERROR] SERVER: Failed to create item %1/%2", j+1, queueItem.quantity);
                        break;
                    }
                }
                
                if (itemsGiven > 0)
                {
                    // Update item status
                    NightroItemGiverManager.UpdateItemStatus(queueItem, "delivered");
                    
                    // Move to processed
                    NightroItemGiverManager.MoveItemToProcessed(itemQueue, queueItem);
                    
                    // Remove from pending
                    itemQueue.items_to_give.RemoveOrdered(i);
                    
                    // Update file
                    int currentTime = (int)GetGame().GetTime();
                    itemQueue.last_updated = "delivered_" + currentTime.ToString();
                    JsonFileLoader<ItemQueue>.JsonSaveFile(filePath, itemQueue);
                    
                    PrintFormat("[NIGHTRO DELIVERY DEBUG] SERVER: Successfully delivered %1x %2 to %3", itemsGiven, itemClass, steamID);
                    
                    // Notify client of successful delivery
                    string successMsg = "[NIGHTRO SHOP SUCCESS] Successfully received: " + itemsGiven.ToString() + "x " + itemClass;
                    GetGame().ChatMP(player, successMsg, "ColorGreen");
                    
                    // Sync updated data back to client immediately
                    GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SyncItemDataToClient, 1000, false, player);
                    
                    return true;
                }
                else
                {
                    errorMessage = "[NIGHTRO SHOP] Failed to create items in inventory.";
                    return false;
                }
            }
        }
        
        errorMessage = "[NIGHTRO SHOP] Item not found in queue or already processed.";
        PrintFormat("[NIGHTRO DELIVERY DEBUG] SERVER: Item not found - %1x %2", quantity, itemClass);
        return false;
    }
}

// Client-side sync handler - SIMPLIFIED VERSION
class NightroItemSyncClient
{
    static void ProcessSyncMessage(string message)
    {
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Processing sync message: %1", message);
        
        if (message.IndexOf("[NIGHTRO_SYNC_FILE]") != 0)
        {
            PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Not a file sync message");
            return;
        }
        
        // Parse sync message format: [NIGHTRO_SYNC_FILE]steamid|itemcount|timestamp
        string messageData = message.Substring(19, message.Length() - 19); // Remove "[NIGHTRO_SYNC_FILE]"
        array<string> parts = new array<string>;
        messageData.Split("|", parts);
        
        if (parts.Count() < 3)
        {
            PrintFormat("[NIGHTRO SYNC ERROR] CLIENT: Invalid sync message format - need 3 parts, got %1", parts.Count());
            return;
        }
        
        string steamID = parts.Get(0);
        int itemCount = parts.Get(1).ToInt();
        string timestamp = parts.Get(2);
        
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity() || player.GetIdentity().GetPlainId() != steamID)
        {
            PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Sync message not for current player");
            return;
        }
        
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Processing file sync for %1 items", itemCount);
        
        // ⭐ WAIT and then read server file directly
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ReadServerFileDirectly, 500, false, steamID, player.GetIdentity().GetName(), timestamp, itemCount);
        
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Scheduled delayed file read");
    }
    
    static void ReadServerFileDirectly(string steamID, string playerName, string timestamp, int expectedItems)
    {
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: === READING SERVER FILE DIRECTLY ===");
        
        string serverFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        string clientFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        // Ensure client directory exists
        string pendingDir = "$profile:NightroItemGiverData/pending_items/";
        if (!FileExist(pendingDir))
        {
            MakeDirectory("$profile:NightroItemGiverData/");
            MakeDirectory(pendingDir);
        }
        
        // Check if server file exists
        if (!FileExist(serverFilePath))
        {
            PrintFormat("[NIGHTRO SYNC ERROR] CLIENT: Server file does not exist: %1", serverFilePath);
            CreateEmptyClientFile(steamID, playerName, clientFilePath);
            return;
        }
        
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Server file exists, reading directly: %1", serverFilePath);
        
        // Read raw file content for debug
        string rawContent = "";
        FileHandle rawFile = OpenFile(serverFilePath, FileMode.READ);
        if (rawFile)
        {
            string line;
            while (FGets(rawFile, line) >= 0)
            {
                rawContent += line;
            }
            CloseFile(rawFile);
            
            int debugLen = 1000;
            if (rawContent.Length() < debugLen) debugLen = rawContent.Length();
            PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Direct raw server file (%1 chars): %2", 
                rawContent.Length(), rawContent.Substring(0, debugLen));
        }
        
        // Read using JsonFileLoader
        ItemQueue serverQueue = new ItemQueue();
        JsonFileLoader<ItemQueue>.JsonLoadFile(serverFilePath, serverQueue);
        
        if (!serverQueue)
        {
            PrintFormat("[NIGHTRO SYNC ERROR] CLIENT: Failed to parse server file");
            CreateEmptyClientFile(steamID, playerName, clientFilePath);
            return;
        }
        
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: === DIRECT SERVER FILE ANALYSIS ===");
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Player: %1", serverQueue.player_name);
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Steam ID: %1", serverQueue.steam_id);
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Last updated: %1", serverQueue.last_updated);
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Items array exists: %1", serverQueue.items_to_give != null);
        
        int actualCount = 0;
        if (serverQueue.items_to_give)
        {
            actualCount = serverQueue.items_to_give.Count();
            PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Direct items count: %1", actualCount);
            
            for (int i = 0; i < actualCount; i++)
            {
                ItemInfo item = serverQueue.items_to_give.Get(i);
                if (item)
                {
                    PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Direct Item %1: class='%2', qty=%3, status='%4'", 
                        i, item.classname, item.quantity, item.status);
                }
            }
        }
        
        // Update timestamp for client version
        serverQueue.last_updated = "direct_client_sync_" + timestamp;
        
        // Save as client file (this IS the same file but with updated timestamp)
        JsonFileLoader<ItemQueue>.JsonSaveFile(clientFilePath, serverQueue);
        
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Saved client file with %1 items", actualCount);
        
        // If menu is open, it should now be able to read the items
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: File sync completed - menu can now load items");
    }
    
    static void CreateEmptyClientFile(string steamID, string playerName, string clientFilePath)
    {
        ItemQueue emptyQueue = new ItemQueue();
        emptyQueue.steam_id = steamID;
        emptyQueue.player_name = playerName;
        emptyQueue.last_updated = "empty_client_direct";
        emptyQueue.items_to_give = new array<ref ItemInfo>;
        emptyQueue.processed_items = new array<ref ProcessedItemInfo>;
        emptyQueue.player_state = new PlayerStateInfo();
        emptyQueue.player_state.steam_id = steamID;
        
        JsonFileLoader<ItemQueue>.JsonSaveFile(clientFilePath, emptyQueue);
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Created empty direct client file");
    }
    
    static void RequestItemDelivery(string itemClass, int quantity)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity()) return;
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] CLIENT: Requesting delivery of %1x %2", quantity, itemClass);
        
        // Create request via file system
        string steamID = player.GetIdentity().GetPlainId();
        int currentTime = (int)GetGame().GetTime();
        string requestFilePath = "$profile:NightroItemGiverData/delivery_requests/" + steamID + "_" + currentTime.ToString() + ".txt";
        
        // Ensure directory exists
        string dirPath = "$profile:NightroItemGiverData/delivery_requests/";
        if (!FileExist(dirPath))
        {
            MakeDirectory(dirPath);
        }
        
        // Create request file
        FileHandle requestFile = OpenFile(requestFilePath, FileMode.WRITE);
        if (requestFile)
        {
            FPrintln(requestFile, itemClass + "|" + quantity.ToString());
            CloseFile(requestFile);
            PrintFormat("[NIGHTRO CLIENT DEBUG] CLIENT: Created delivery request file");
        }
    }
}