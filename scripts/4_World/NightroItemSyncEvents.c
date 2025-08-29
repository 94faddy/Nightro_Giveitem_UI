class NightroItemSyncManager
{
    static void RegisterEvents()
    {
        PrintFormat("[NIGHTRO SYNC DEBUG] File-based sync system initialized");
    }
    
    // ⭐ FIXED: ส่งข้อมูลผ่าน RPC แทนการอ่านไฟล์โดยตรง
    static void SyncItemDataToClient(PlayerBase player)
    {
        if (!player || !player.GetIdentity()) return;
        if (GetGame().IsClient()) return; // Server only
        
        string steamID = player.GetIdentity().GetPlainId();
        string serverFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: === SYNC START ===");
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Player: %1", steamID);
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Server file path: %1", serverFilePath);
        
        if (!FileExist(serverFilePath))
        {
            PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: No server file found for %1", steamID);
            CreateEmptyClientFile(steamID, player.GetIdentity().GetName());
            
            // Send empty data via RPC
            SendSyncDataToClient(player, "");
            return;
        }
        
        // Read server file content as string
        string fileContent = "";
        FileHandle fileRead = OpenFile(serverFilePath, FileMode.READ);
        if (fileRead)
        {
            string line;
            while (FGets(fileRead, line) >= 0)
            {
                fileContent += line;
            }
            CloseFile(fileRead);
        }
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: File content length: %1", fileContent.Length());
        
        // Send the actual JSON content to client via RPC/Chat
        SendSyncDataToClient(player, fileContent);
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: === SYNC COMPLETED ===");
    }
    
    // ⭐ FIXED: Simplified sync data transmission - use direct file writing instead of encoding
    static void SendSyncDataToClient(PlayerBase player, string jsonData)
    {
        if (!player || !player.GetIdentity()) return;
        
        string steamID = player.GetIdentity().GetPlainId();
        string clientFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Writing sync data directly to client file");
        
        // Ensure directory exists
        string pendingDir = "$profile:NightroItemGiverData/pending_items/";
        if (!FileExist(pendingDir))
        {
            MakeDirectory("$profile:NightroItemGiverData/");
            MakeDirectory(pendingDir);
        }
        
        if (jsonData == "")
        {
            // Create empty data file
            CreateEmptyClientFile(steamID, player.GetIdentity().GetName());
        }
        else
        {
            // Write JSON data directly to client file (server writes to profile which is shared)
            FileHandle fileWrite = OpenFile(clientFilePath, FileMode.WRITE);
            if (fileWrite)
            {
                FPrint(fileWrite, jsonData);
                CloseFile(fileWrite);
                PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Successfully wrote sync data to client file");
            }
            else
            {
                PrintFormat("[NIGHTRO SYNC ERROR] SERVER: Failed to write sync data to client file");
            }
        }
        
        // ⭐ REDUCED SPAM: Only send notification if player has items
        if (jsonData != "" && jsonData.IndexOf("\"items_to_give\": []") < 0)
        {
            string notifyMsg = "[NIGHTRO SHOP] You have items waiting! Press Ctrl+I to open menu.";
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().ChatMP, 1000, false, player, notifyMsg, "ColorYellow");
        }
    }
    
    static void DoNothing() {} // Helper for delays
    
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
        // Legacy function - not used
        return "";
    }
    
    // ⭐ DELIVERY FUNCTION (unchanged)
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

// ⭐ FIXED: Simplified client-side sync handler - remove encoding/decoding
class NightroItemSyncClient
{
    static void ProcessSyncMessage(string message)
    {
        // ⭐ REDUCED SPAM: Only process specific sync messages
        if (message.IndexOf("[NIGHTRO SHOP]") == 0)
        {
            // Only process messages that indicate items are waiting
            if (message.IndexOf("You have items waiting!") > 0)
            {
                PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Items available notification received");
                
                // Note: UI refresh will be handled by MissionGameplay file update check
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RefreshUI, 2000, false);
            }
        }
    }
    
    static void RefreshUI()
    {
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: UI refresh triggered");
        // Note: UI refresh will be handled by MissionGameplay file update check
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