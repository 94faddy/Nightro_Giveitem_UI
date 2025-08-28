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
        }
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: - Items count: %1", itemCount);
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: - Last updated: %1", serverQueue.last_updated);
        
        // Update timestamp to indicate fresh sync
        int currentTime = (int)GetGame().GetTime();
        serverQueue.last_updated = "server_sync_" + currentTime.ToString();
        
        // Save back to server file first
        JsonFileLoader<ItemQueue>.JsonSaveFile(serverFilePath, serverQueue);
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Updated server file with new timestamp");
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
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Created empty server file");
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

// Client-side sync handler - SIMPLIFIED
class NightroItemSyncClient
{
    static void ProcessSyncMessage(string message)
    {
        // Legacy function - no longer used with direct file sync
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Legacy sync message processing (not used)");
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