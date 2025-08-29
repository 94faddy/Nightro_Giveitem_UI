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
    
    // ⭐ NEW: Send data via encoded chat message
    static void SendSyncDataToClient(PlayerBase player, string jsonData)
    {
        if (!player) return;
        
        // Encode data in base64-like format to avoid parsing issues
        string encodedData = EncodeItemData(jsonData);
        
        // Split into chunks if needed (chat message limit)
        int chunkSize = 400; // Safe chunk size
        int totalChunks = Math.Ceil(encodedData.Length() / (float)chunkSize);
        
        if (totalChunks <= 1)
        {
            // Send single message
            string syncMsg = "[NIGHTRO_SYNC_DATA]SINGLE|" + encodedData;
            GetGame().ChatMP(player, syncMsg, "ColorBlue");
        }
        else
        {
            // Send multiple chunks
            for (int i = 0; i < totalChunks; i++)
            {
                int startIdx = i * chunkSize;
                int length = Math.Min(chunkSize, encodedData.Length() - startIdx);
                string chunk = encodedData.Substring(startIdx, length);
                
                string chunkMsg = "[NIGHTRO_SYNC_DATA]CHUNK|" + i.ToString() + "|" + totalChunks.ToString() + "|" + chunk;
                GetGame().ChatMP(player, chunkMsg, "ColorBlue");
                
                // Small delay between chunks
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DoNothing, 100 * i, false);
            }
        }
        
        // Send completion notification
        string notifyMsg = "[NIGHTRO SHOP] Data synchronized! Open menu (Ctrl+I) to see items.";
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().ChatMP, 500, false, player, notifyMsg, "ColorGreen");
    }
    
    static void DoNothing() {} // Helper for delays
    
    // Simple encoding to avoid JSON parsing issues in chat
    static string EncodeItemData(string data)
    {
        // Convert to hex string to avoid special characters
        string encoded = "";
        for (int i = 0; i < data.Length(); i++)
        {
            int charCode = data.Get(i).Hash();
            encoded += charCode.ToString() + ",";
        }
        return encoded;
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

// Client-side sync handler - NEW VERSION
class NightroItemSyncClient
{
    static ref array<string> m_ChunkBuffer;
    static int m_ExpectedChunks = 0;
    static int m_ReceivedChunks = 0;
    
    static void ProcessSyncMessage(string message)
    {
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Processing sync message");
        
        if (message.IndexOf("[NIGHTRO_SYNC_DATA]") != 0)
        {
            PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Not a sync data message");
            return;
        }
        
        // Remove prefix
        string messageData = message.Substring(19, message.Length() - 19);
        
        if (messageData.IndexOf("SINGLE|") == 0)
        {
            // Single message
            string encodedData = messageData.Substring(7, messageData.Length() - 7);
            string jsonData = DecodeItemData(encodedData);
            ProcessReceivedData(jsonData);
        }
        else if (messageData.IndexOf("CHUNK|") == 0)
        {
            // Multi-part message
            ProcessChunk(messageData.Substring(6, messageData.Length() - 6));
        }
    }
    
    static void ProcessChunk(string chunkData)
    {
        array<string> parts = new array<string>;
        chunkData.Split("|", parts);
        
        if (parts.Count() < 3) return;
        
        int chunkIndex = parts.Get(0).ToInt();
        int totalChunks = parts.Get(1).ToInt();
        string chunkContent = parts.Get(2);
        
        // Initialize buffer if needed
        if (!m_ChunkBuffer || m_ChunkBuffer.Count() != totalChunks)
        {
            m_ChunkBuffer = new array<string>;
            for (int i = 0; i < totalChunks; i++)
            {
                m_ChunkBuffer.Insert("");
            }
            m_ExpectedChunks = totalChunks;
            m_ReceivedChunks = 0;
        }
        
        // Store chunk
        m_ChunkBuffer.Set(chunkIndex, chunkContent);
        m_ReceivedChunks++;
        
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Received chunk %1/%2", m_ReceivedChunks, m_ExpectedChunks);
        
        // Check if all chunks received
        if (m_ReceivedChunks >= m_ExpectedChunks)
        {
            string fullData = "";
            for (int j = 0; j < m_ChunkBuffer.Count(); j++)
            {
                fullData += m_ChunkBuffer.Get(j);
            }
            
            string jsonData = DecodeItemData(fullData);
            ProcessReceivedData(jsonData);
            
            // Clear buffer
            m_ChunkBuffer = null;
            m_ExpectedChunks = 0;
            m_ReceivedChunks = 0;
        }
    }
    
    static string DecodeItemData(string encoded)
    {
        string decoded = "";
        array<string> codes = new array<string>;
        encoded.Split(",", codes);
        
        for (int i = 0; i < codes.Count(); i++)
        {
            string code = codes.Get(i);
            if (code != "")
            {
                int charCode = code.ToInt();
                decoded += charCode.AsciiToString();
            }
        }
        
        return decoded;
    }
    
    static void ProcessReceivedData(string jsonData)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity()) return;
        
        string steamID = player.GetIdentity().GetPlainId();
        string clientFilePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Processing received JSON data");
        
        // Ensure directory exists
        string pendingDir = "$profile:NightroItemGiverData/pending_items/";
        if (!FileExist(pendingDir))
        {
            MakeDirectory("$profile:NightroItemGiverData/");
            MakeDirectory(pendingDir);
        }
        
        if (jsonData == "")
        {
            // Empty data
            CreateEmptyClientFile(steamID, player.GetIdentity().GetName(), clientFilePath);
        }
        else
        {
            // Write JSON data to file
            FileHandle fileWrite = OpenFile(clientFilePath, FileMode.WRITE);
            if (fileWrite)
            {
                FPrint(fileWrite, jsonData);
                CloseFile(fileWrite);
                PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Saved sync data to client file");
            }
        }
        
        // Refresh menu if open
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: File sync completed - refreshing UI");
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RefreshUI, 500, false);
    }
    
    static void RefreshUI()
    {
        // This will be called from UI manager
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: UI refresh triggered");
    }
    
    static void CreateEmptyClientFile(string steamID, string playerName, string clientFilePath)
    {
        ItemQueue emptyQueue = new ItemQueue();
        emptyQueue.steam_id = steamID;
        emptyQueue.player_name = playerName;
        emptyQueue.last_updated = "empty_client";
        emptyQueue.items_to_give = new array<ref ItemInfo>;
        emptyQueue.processed_items = new array<ref ProcessedItemInfo>;
        emptyQueue.player_state = new PlayerStateInfo();
        emptyQueue.player_state.steam_id = steamID;
        
        JsonFileLoader<ItemQueue>.JsonSaveFile(clientFilePath, emptyQueue);
        PrintFormat("[NIGHTRO SYNC DEBUG] CLIENT: Created empty client file");
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