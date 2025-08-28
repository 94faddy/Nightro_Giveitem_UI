// ===== ENHANCED ITEMGIVER V3.0 WITH SYNC SYSTEM (PRODUCTION) =====

class AttachmentInfo
{
    string classname;
    int quantity;
    string status;
    ref array<ref AttachmentInfo> attachments;
    
    void AttachmentInfo()
    {
        attachments = new array<ref AttachmentInfo>;
        status = "pending";
    }
}

class ItemInfo
{
    string classname;
    int quantity;
    string status;
    string processed_at;
    string error_message;
    int retry_count;
    string last_retry_at;
    ref array<ref AttachmentInfo> attachments;
    
    void ItemInfo()
    {
        attachments = new array<ref AttachmentInfo>;
        status = "pending";
        processed_at = "";
        error_message = "";
        retry_count = 0;
        last_retry_at = "";
    }
}

class ProcessedItemInfo
{
    string classname;
    int quantity;
    string status;
    string processed_at;
    string error_message;
    ref array<ref AttachmentInfo> attachments;
    
    void ProcessedItemInfo()
    {
        attachments = new array<ref AttachmentInfo>;
        status = "delivered";
        processed_at = "";
        error_message = "";
    }
}

class PlayerStateInfo
{
    string steam_id;
    float last_connect_time;
    float last_respawn_time;
    bool is_ready_for_items;
    bool was_dead_on_connect;
    int connect_count;
    float last_notification_time;
    string last_notification_type;
    
    void PlayerStateInfo()
    {
        steam_id = "";
        last_connect_time = 0;
        last_respawn_time = 0;
        is_ready_for_items = false;
        was_dead_on_connect = false;
        connect_count = 0;
        last_notification_time = 0;
        last_notification_type = "";
    }
}

class ItemQueue
{
    string player_name;
    string steam_id;
    string last_updated;
    ref array<ref ItemInfo> items_to_give;
    ref array<ref ProcessedItemInfo> processed_items;
    ref PlayerStateInfo player_state;
    
    void ItemQueue() 
    { 
        items_to_give = new array<ref ItemInfo>;
        processed_items = new array<ref ProcessedItemInfo>;
        player_state = new PlayerStateInfo();
        player_name = "";
        steam_id = "";
        last_updated = "";
    }
}

class NotificationSettings
{
    string message_template;
    int display_duration_seconds;
    int initial_delay_seconds;
    int respawn_delay_seconds;
    int max_retry_attempts;
    int retry_interval_seconds;
    bool require_territory_check;
    bool enable_ui_system;
    
    // UI-specific notification messages
    string ui_no_group_message;
    string ui_no_plotpole_message;
    string ui_outside_territory_message;
    string ui_inventory_full_message;
    string ui_item_delivered_message;
    string ui_invalid_item_message;
    string ui_player_not_ready_message;

    void NotificationSettings()
    {
        message_template = "[NIGHTRO SHOP] You received: {QUANTITY}x {CLASS_NAME}";
        display_duration_seconds = 8;
        initial_delay_seconds = 0; // No delay for UI-based system
        respawn_delay_seconds = 0; // No delay for UI-based system
        max_retry_attempts = 0; // No retry system for UI
        retry_interval_seconds = 60;
        require_territory_check = true;
        enable_ui_system = true;
        
        // UI-specific messages
        ui_no_group_message = "[NIGHTRO SHOP] You need to join or create a group to receive items!";
        ui_no_plotpole_message = "[NIGHTRO SHOP] Your group needs to place a Plotpole to receive items!";
        ui_outside_territory_message = "[NIGHTRO SHOP] You must be within your group's territory! Distance: {DISTANCE}m";
        ui_inventory_full_message = "[NIGHTRO SHOP] Your inventory is full! Clear space to receive {QUANTITY}x {ITEM_NAME}";
        ui_item_delivered_message = "[NIGHTRO SHOP] Successfully received: {QUANTITY}x {ITEM_NAME}";
        ui_invalid_item_message = "[NIGHTRO SHOP] Invalid item: {ITEM_NAME}";
        ui_player_not_ready_message = "[NIGHTRO SHOP] You must be alive and not unconscious to receive items.";
    }
}

class NightroItemGiverManager
{
    private static const string IG_PROFILE_FOLDER = "$profile:NightroItemGiverData/";
    private static const string IG_PENDING_FOLDER = "pending_items/";
    private static const string IG_DELIVERY_FOLDER = "delivery_requests/";
    private static const string IG_SYNC_FOLDER = "sync_requests/";
    private static const string IG_SETTINGS_FILE = "notification_settings.json";
    private static bool m_IsInitialized = false;
    private static ref NotificationSettings m_NotificationSettings;
    private static ref map<string, ref PlayerStateInfo> m_PlayerStates;

    void NightroItemGiverManager()
    {
        Init();
    }
    
    static void Init()
    {
        if (m_IsInitialized) return;
        
        PrintFormat("[NIGHTRO SERVER DEBUG] Initializing NightroItemGiverManager...");
        
        if (!FileExist(IG_PROFILE_FOLDER)) 
        {
            MakeDirectory(IG_PROFILE_FOLDER);
            PrintFormat("[NIGHTRO SERVER DEBUG] Created directory: %1", IG_PROFILE_FOLDER);
        }
        
        if (!FileExist(IG_PROFILE_FOLDER + IG_PENDING_FOLDER)) 
        {
            MakeDirectory(IG_PROFILE_FOLDER + IG_PENDING_FOLDER);
            PrintFormat("[NIGHTRO SERVER DEBUG] Created directory: %1", IG_PROFILE_FOLDER + IG_PENDING_FOLDER);
        }
        
        if (!FileExist(IG_PROFILE_FOLDER + IG_DELIVERY_FOLDER)) 
        {
            MakeDirectory(IG_PROFILE_FOLDER + IG_DELIVERY_FOLDER);
            PrintFormat("[NIGHTRO SERVER DEBUG] Created directory: %1", IG_PROFILE_FOLDER + IG_DELIVERY_FOLDER);
        }
        
        if (!FileExist(IG_PROFILE_FOLDER + IG_SYNC_FOLDER)) 
        {
            MakeDirectory(IG_PROFILE_FOLDER + IG_SYNC_FOLDER);
            PrintFormat("[NIGHTRO SERVER DEBUG] Created directory: %1", IG_PROFILE_FOLDER + IG_SYNC_FOLDER);
        }

        m_PlayerStates = new map<string, ref PlayerStateInfo>;
        LoadNotificationSettings();
        
        // Register sync system
        if (GetGame().IsServer())
        {
            NightroItemSyncManager.RegisterEvents();
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ProcessDeliveryRequests, 5000, true);
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ProcessSyncRequests, 3000, true);
        }
        
        m_IsInitialized = true;
        PrintFormat("[NIGHTRO SERVER DEBUG] NightroItemGiverManager initialized successfully!");
    }
    
    static void LoadNotificationSettings()
    {
        string settingsPath = IG_PROFILE_FOLDER + IG_SETTINGS_FILE;
        PrintFormat("[NIGHTRO SERVER DEBUG] Loading settings from: %1", settingsPath);
        
        if (FileExist(settingsPath))
        {
            JsonFileLoader<NotificationSettings>.JsonLoadFile(settingsPath, m_NotificationSettings);
            PrintFormat("[NIGHTRO SERVER DEBUG] Settings loaded successfully");
        }
        else
        {
            m_NotificationSettings = new NotificationSettings();
            JsonFileLoader<NotificationSettings>.JsonSaveFile(settingsPath, m_NotificationSettings);
            PrintFormat("[NIGHTRO SERVER DEBUG] Created default settings file");
        }
        
        PrintFormat("[NIGHTRO SERVER DEBUG] UI System enabled: %1", m_NotificationSettings.enable_ui_system);
        PrintFormat("[NIGHTRO SERVER DEBUG] Territory check required: %1", m_NotificationSettings.require_territory_check);
    }

    // SERVER ONLY: Process delivery requests from clients
    static void ProcessDeliveryRequests()
    {
        if (!GetGame().IsServer()) return;
        
        string requestDir = IG_PROFILE_FOLDER + IG_DELIVERY_FOLDER;
        if (!FileExist(requestDir)) return;
        
        // In DayZ, we need to check for specific files rather than list directory
        // We'll check for files with pattern: steamid_timestamp.txt
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (!player || !player.GetIdentity()) continue;
            
            string steamID = player.GetIdentity().GetPlainId();
            
            // Check for request files for this player by trying different timestamps
            float currentTime = GetGame().GetTime();
            for (int j = 0; j < 100; j++) // Check last 100 possible timestamps
            {
                float checkTime = currentTime - (j * 1000); // Check every 1000ms back
                string fileName = steamID + "_" + ((int)checkTime).ToString() + ".txt";
                string filePath = requestDir + fileName;
                
                if (FileExist(filePath))
                {
                    PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Found delivery request file: %1", fileName);
                    
                    // Read request data
                    FileHandle requestFile = OpenFile(filePath, FileMode.READ);
                    if (!requestFile)
                    {
                        DeleteFile(filePath);
                        continue;
                    }
                    
                    string requestData;
                    FGets(requestFile, requestData);
                    CloseFile(requestFile);
                    
                    // Parse request: itemClass|quantity
                    array<string> requestParts = new array<string>;
                    requestData.Split("|", requestParts);
                    
                    if (requestParts.Count() < 2)
                    {
                        DeleteFile(filePath);
                        continue;
                    }
                    
                    string itemClass = requestParts.Get(0);
                    int quantity = requestParts.Get(1).ToInt();
                    
                    PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Processing delivery request: %1x %2 for %3", quantity, itemClass, steamID);
                    
                    // Process the delivery
                    string errorMessage;
                    bool success = NightroItemSyncManager.DeliverItemToPlayer(player, itemClass, quantity, errorMessage);
                    
                    if (!success && errorMessage != "")
                    {
                        GetGame().ChatMP(player, errorMessage, "ColorRed");
                    }
                    
                    // Delete request file
                    DeleteFile(filePath);
                    break; // Process one request per player per cycle
                }
            }
        }
    }

    // SERVER ONLY: Process sync requests from clients
    static void ProcessSyncRequests()
    {
        if (!GetGame().IsServer()) return;
        
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: ProcessSyncRequests() called");
        
        string requestDir = IG_PROFILE_FOLDER + IG_SYNC_FOLDER;
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Checking directory: %1", requestDir);
        
        if (!FileExist(requestDir)) 
        {
            PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Sync request directory doesn't exist, creating...");
            MakeDirectory(requestDir);
            return;
        }
        
        // Get all connected players
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Found %1 connected players", players.Count());
        
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (!player || !player.GetIdentity()) continue;
            
            string steamID = player.GetIdentity().GetPlainId();
            PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Checking sync requests for player: %1", steamID);
            
            // Check for sync request files for this player
            float currentTime = GetGame().GetTime();
            bool foundRequest = false;
            
            for (int j = 0; j < 200; j++) // Check more timestamps
            {
                float checkTime = currentTime - (j * 500); // Check every 500ms back
                string fileName = steamID + "_" + ((int)checkTime).ToString() + ".txt";
                string filePath = requestDir + fileName;
                
                if (FileExist(filePath))
                {
                    PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Found sync request file: %1", fileName);
                    foundRequest = true;
                    
                    // Send current data to client IMMEDIATELY
                    GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NightroItemSyncManager.SyncItemDataToClient, 100, false, player);
                    
                    // Delete request file
                    DeleteFile(filePath);
                    PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Deleted sync request file and queued sync");
                    break; // Process one request per player per cycle
                }
            }
            
            if (!foundRequest)
            {
                // Check if player has items but no sync request - auto sync
                bool hasItems = NightroItemGiverManager.HasPendingItems(player);
                if (hasItems)
                {
                    PrintFormat("[NIGHTRO SYNC DEBUG] SERVER: Player %1 has items but no recent sync request, forcing sync", steamID);
                    GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NightroItemSyncManager.SyncItemDataToClient, 100, false, player);
                }
            }
        }
    }

    static bool HasPendingItems(PlayerBase player)
    {
        if (!player || !player.GetIdentity()) return false;
        
        string steamID = player.GetIdentity().GetPlainId();
        string filePath = IG_PROFILE_FOLDER + IG_PENDING_FOLDER + steamID + ".json";
        
        PrintFormat("[NIGHTRO SERVER DEBUG] Checking pending items for player %1 at: %2", steamID, filePath);
        
        if (!FileExist(filePath)) 
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] No pending items file found for player %1", steamID);
            return false;
        }
        
        ItemQueue itemQueue = new ItemQueue();
        JsonFileLoader<ItemQueue>.JsonLoadFile(filePath, itemQueue);
        
        if (!itemQueue || !itemQueue.items_to_give) 
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] Failed to load item queue or no items array for player %1", steamID);
            return false;
        }
        
        int itemCount = 0;
        for (int i = 0; i < itemQueue.items_to_give.Count(); i++)
        {
            ItemInfo item = itemQueue.items_to_give.Get(i);
            if (item && item.classname != "" && item.quantity > 0 && item.status == "pending")
            {
                itemCount++;
            }
        }
        
        PrintFormat("[NIGHTRO SERVER DEBUG] Player %1 has %2 pending items", steamID, itemCount);
        return itemCount > 0;
    }

    static bool IsPlayerInValidTerritory(PlayerBase player, out float distanceToPlotpole)
    {
        distanceToPlotpole = -1;
        
        if (!player || !player.GetIdentity()) 
        {
            return false;
        }

        // Check if territory validation is enabled
        if (!m_NotificationSettings.require_territory_check)
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] Territory check disabled, allowing access");
            return true;
        }

        LBGroup playerGroup = GetPlayerGroup(player);
        if (!playerGroup)
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] Player %1 has no group", player.GetIdentity().GetPlainId());
            distanceToPlotpole = GetDistanceToNearestPlotpole(player.GetPosition());
            return false;
        }

        if (!GroupHasPlotpole(playerGroup))
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] Group %1 has no plotpole", playerGroup.shortname);
            return false;
        }

        bool isInTerritory = IsPlayerInGroupTerritory(player, playerGroup, distanceToPlotpole);
        PrintFormat("[NIGHTRO SERVER DEBUG] Player %1 in territory: %2, distance: %3m", 
            player.GetIdentity().GetPlainId(), isInTerritory, distanceToPlotpole);
        return isInTerritory;
    }

    static float GetDistanceToNearestPlotpole(vector playerPos)
    {
        float nearestDistance = 999999.0;
        
        #ifdef LBmaster_GroupDLCPlotpole
        foreach (TerritoryFlag flag : TerritoryFlag.all_Flags)
        {
            if (!flag) continue;
            
            float distance = vector.Distance(playerPos, flag.GetPosition());
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
            }
        }
        #endif
        
        return nearestDistance;
    }

    static bool GroupHasPlotpole(LBGroup playerGroup)
    {
        if (!playerGroup) return false;
        
        #ifdef LBmaster_GroupDLCPlotpole
        string tagLower = playerGroup.shortname + "";
        tagLower.ToLower();
        int groupTagHash = tagLower.Hash();
        
        foreach (TerritoryFlag flag : TerritoryFlag.all_Flags)
        {
            if (!flag) continue;
            
            if (flag.ownerGroupTagHash == groupTagHash)
            {
                return true;
            }
        }
        #endif
        
        return false;
    }

    static LBGroup GetPlayerGroup(PlayerBase player)
    {
        if (!player)
            return null;
            
        return player.GetLBGroup();
    }

    static bool IsPlayerInGroupTerritory(PlayerBase player, LBGroup playerGroup, out float distanceToNearestFriendlyFlag)
    {
        distanceToNearestFriendlyFlag = 999999.0;
        
        if (!player || !playerGroup) 
            return false;

        vector playerPos = player.GetPosition();
        string groupTag = playerGroup.shortname;

        #ifdef LBmaster_GroupDLCPlotpole
        string groupTagLower = groupTag;
        groupTagLower.ToLower();
        int groupTagHash = groupTagLower.Hash();
        
        TerritoryFlag nearestFriendlyFlag = TerritoryFlag.FindNearestFlag(playerPos, true, true, groupTagHash);
        
        if (nearestFriendlyFlag)
        {
            distanceToNearestFriendlyFlag = vector.Distance(playerPos, nearestFriendlyFlag.GetPosition());
            
            if (nearestFriendlyFlag.IsInRadius(playerPos))
            {
                return true;
            }
        }
        
        return false;
        #else
        return true;
        #endif
    }

    static void UpdatePlayerState(PlayerBase player, string action)
    {
        if (!player || !player.GetIdentity()) return;
        
        string steamID = player.GetIdentity().GetPlainId();
        string playerName = player.GetIdentity().GetName();
        float currentTime = GetGame().GetTime();
        
        PrintFormat("[NIGHTRO SERVER DEBUG] UpdatePlayerState - Player: %1 (%2), Action: %3", playerName, steamID, action);
        
        if (!m_PlayerStates.Contains(steamID))
        {
            m_PlayerStates.Set(steamID, new PlayerStateInfo());
            PrintFormat("[NIGHTRO SERVER DEBUG] Created new player state for: %1", steamID);
        }
        
        PlayerStateInfo playerState = m_PlayerStates.Get(steamID);
        playerState.steam_id = steamID;
        
        if (action == "connect")
        {
            playerState.last_connect_time = currentTime;
            playerState.connect_count++;
            playerState.is_ready_for_items = true; // Ready immediately for UI system
            playerState.last_notification_time = 0;
            playerState.last_notification_type = "";
            
            if (!player.IsAlive())
            {
                playerState.was_dead_on_connect = true;
                PrintFormat("[NIGHTRO SERVER DEBUG] Player %1 connected while dead", steamID);
            }
            else
            {
                playerState.was_dead_on_connect = false;
                PrintFormat("[NIGHTRO SERVER DEBUG] Player %1 connected alive", steamID);
            }
        }
        else if (action == "respawn")
        {
            playerState.last_respawn_time = currentTime;
            playerState.is_ready_for_items = true; // Ready immediately for UI system
            playerState.was_dead_on_connect = false;
            playerState.last_notification_time = 0;
            playerState.last_notification_type = "";
            PrintFormat("[NIGHTRO SERVER DEBUG] Player %1 respawned", steamID);
        }
    }
    
    static string GetCurrentTimestamp()
    {
        float timestamp = GetGame().GetTime();
        return timestamp.ToString();
    }
    
    static void UpdateItemStatus(ref ItemInfo itemInfo, string status, string errorMessage = "")
    {
        itemInfo.status = status;
        itemInfo.processed_at = GetCurrentTimestamp();
        itemInfo.error_message = errorMessage;
    }

    static void UpdateAttachmentStatus(ref AttachmentInfo attachment, string status)
    {
        attachment.status = status;
    }
    
    static void MoveItemToProcessed(ref ItemQueue itemQueue, ref ItemInfo itemInfo)
    {
        ProcessedItemInfo processedItem = new ProcessedItemInfo();
        processedItem.classname = itemInfo.classname;
        processedItem.quantity = itemInfo.quantity;
        processedItem.status = itemInfo.status;
        processedItem.processed_at = itemInfo.processed_at;
        processedItem.error_message = itemInfo.error_message;
        
        if (itemInfo.attachments && itemInfo.attachments.Count() > 0)
        {
            processedItem.attachments = new array<ref AttachmentInfo>;
            for (int i = 0; i < itemInfo.attachments.Count(); i++)
            {
                AttachmentInfo originalAttachment = itemInfo.attachments.Get(i);
                AttachmentInfo copyAttachment = new AttachmentInfo();
                copyAttachment.classname = originalAttachment.classname;
                copyAttachment.quantity = originalAttachment.quantity;
                copyAttachment.status = originalAttachment.status;
                
                if (originalAttachment.attachments && originalAttachment.attachments.Count() > 0)
                {
                    copyAttachment.attachments = new array<ref AttachmentInfo>;
                    for (int j = 0; j < originalAttachment.attachments.Count(); j++)
                    {
                        AttachmentInfo nestedAttachment = originalAttachment.attachments.Get(j);
                        AttachmentInfo copyNestedAttachment = new AttachmentInfo();
                        copyNestedAttachment.classname = nestedAttachment.classname;
                        copyNestedAttachment.quantity = nestedAttachment.quantity;
                        copyNestedAttachment.status = nestedAttachment.status;
                        copyAttachment.attachments.Insert(copyNestedAttachment);
                    }
                }
                
                processedItem.attachments.Insert(copyAttachment);
            }
        }
        
        itemQueue.processed_items.Insert(processedItem);
    }
    
    static bool IsValidItemClass(string itemName)
    {
        if (!itemName || itemName == "") return false;

        EntityAI testItem = EntityAI.Cast(GetGame().CreateObjectEx(itemName, "0 0 0", ECE_LOCAL | ECE_NOLIFETIME));
        if (!testItem)
        {
            return false;
        }

        GetGame().ObjectDelete(testItem);
        return true;
    }
    
    static bool CanFitInInventory(PlayerBase player, string itemClassname, int quantity)
    {
        if (!player || !player.GetInventory()) return false;
        
        for (int i = 0; i < quantity; i++)
        {
            EntityAI testItem = EntityAI.Cast(GetGame().CreateObjectEx(itemClassname, "0 0 0", ECE_LOCAL | ECE_NOLIFETIME));
            if (!testItem)
            {
                return false;
            }

            InventoryLocation invLocation = new InventoryLocation();
            bool canFit = player.GetInventory().FindFreeLocationFor(testItem, FindInventoryLocationType.ANY, invLocation);
            GetGame().ObjectDelete(testItem);
            
            if (!canFit)
            {
                return false;
            }
        }

        return true;
    }
    
    static bool IsMagazine(string itemName)
    {
        if (itemName == "") return false;
        return GetGame().ConfigIsExisting("CfgMagazines " + itemName);
    }
    
    static bool CanAcceptMagazine(Weapon_Base weapon, string magazineClass)
    {
        if (!weapon || magazineClass == "") return false;

        array<string> compatibleMagazines = {};
        GetGame().ConfigGetTextArray("CfgWeapons " + weapon.GetType() + " magazines", compatibleMagazines);

        return compatibleMagazines.Find(magazineClass) > -1;
    }
    
    static void ValidateWeaponState(Weapon_Base weapon)
    {
        if (!weapon) return;
        
        Magazine currentMag = weapon.GetMagazine(0);
        if (currentMag)
        {
            string magType = currentMag.GetType();
            
            array<string> validMags = {};
            GetGame().ConfigGetTextArray("CfgWeapons " + weapon.GetType() + " magazines", validMags);
            
            if (validMags.Find(magType) == -1)
            {
                weapon.ServerDropEntity(currentMag);
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().ObjectDelete, 1000, false, currentMag);
            }
            else
            {
                currentMag.ServerSetAmmoCount(currentMag.GetAmmoMax());
            }
        }
        
        weapon.ValidateAndRepair();
        weapon.SetSynchDirty();
        weapon.Synchronize();
    }
    
    static void AttachItemsToWeapon(EntityAI parentItem, array<ref AttachmentInfo> attachments, PlayerBase player)
    {
        if (!parentItem || !attachments || !player) return;
        
        for (int i = 0; i < attachments.Count(); i++)
        {
            AttachmentInfo attachment = attachments.Get(i);
            if (!attachment || attachment.classname == "") continue;
            
            if (!IsValidItemClass(attachment.classname))
            {
                UpdateAttachmentStatus(attachment, "failed");
                continue;
            }
            
            for (int j = 0; j < attachment.quantity; j++)
            {
                if (Weapon_Base.Cast(parentItem) && IsMagazine(attachment.classname))
                {
                    Weapon_Base weapon = Weapon_Base.Cast(parentItem);
                    
                    if (CanAcceptMagazine(weapon, attachment.classname))
                    {
                        Magazine existingMag = weapon.GetMagazine(0);
                        if (existingMag)
                        {
                            weapon.ServerDropEntity(existingMag);
                            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().ObjectDelete, 500, false, existingMag);
                        }
                        
                        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(InstallMagazineDelayed, 800, false, weapon, attachment.classname, attachment);
                    }
                    else
                    {
                        EntityAI playerMagItem = player.GetInventory().CreateInInventory(attachment.classname);
                        if (playerMagItem)
                        {
                            Magazine mag = Magazine.Cast(playerMagItem);
                            if (mag) mag.ServerSetAmmoCount(mag.GetAmmoMax());
                            UpdateAttachmentStatus(attachment, "delivered");
                        }
                        else
                        {
                            UpdateAttachmentStatus(attachment, "failed");
                        }
                    }
                }
                else
                {
                    EntityAI attachmentEntity = parentItem.GetInventory().CreateAttachment(attachment.classname);
                    if (attachmentEntity)
                    {
                        UpdateAttachmentStatus(attachment, "delivered");
                        
                        if (attachment.attachments && attachment.attachments.Count() > 0)
                        {
                            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AttachItemsToWeapon, 500, false, attachmentEntity, attachment.attachments, player);
                        }
                    }
                    else
                    {
                        EntityAI playerAttachmentItem = player.GetInventory().CreateInInventory(attachment.classname);
                        if (playerAttachmentItem)
                        {
                            UpdateAttachmentStatus(attachment, "delivered");
                            
                            if (attachment.attachments && attachment.attachments.Count() > 0)
                            {
                                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AttachItemsToWeapon, 500, false, playerAttachmentItem, attachment.attachments, player);
                            }
                        }
                        else
                        {
                            UpdateAttachmentStatus(attachment, "failed");
                        }
                    }
                }
            }
        }
    }
    
    static void InstallMagazineDelayed(Weapon_Base weapon, string magazineClass, ref AttachmentInfo attachment)
    {
        if (!weapon || magazineClass == "" || !attachment) return;
        
        Magazine existingMag = weapon.GetMagazine(0);
        if (existingMag)
        {
            return;
        }
        
        EntityAI newMag = weapon.GetInventory().CreateAttachment(magazineClass);
        if (newMag)
        {
            Magazine magazine = Magazine.Cast(newMag);
            if (magazine)
            {
                magazine.ServerSetAmmoCount(magazine.GetAmmoMax());
                UpdateAttachmentStatus(attachment, "delivered");
                
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ValidateWeaponState, 500, false, weapon);
            }
        }
        else
        {
            UpdateAttachmentStatus(attachment, "failed");
        }
    }
    
    // CRITICAL FIX: Server-side only file creation - Client should NEVER create files
    static void CreatePlayerFileIfNotExists(PlayerBase player)
    {
        // Only run on server side
        if (GetGame().IsClient()) 
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] CreatePlayerFileIfNotExists called on client - IGNORING");
            return;
        }
        
        if (!player || !player.GetIdentity()) return;
        
        string steamID = player.GetIdentity().GetPlainId();
        string playerName = player.GetIdentity().GetName();
        string filePath = IG_PROFILE_FOLDER + IG_PENDING_FOLDER + steamID + ".json";
        
        PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Checking player file for %1 (%2) at: %3", playerName, steamID, filePath);
        
        if (!FileExist(filePath))
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Creating new empty player file for %1 (%2)", playerName, steamID);
            
            ItemQueue playerQueue = new ItemQueue();
            playerQueue.player_name = playerName;
            playerQueue.steam_id = steamID;
            playerQueue.last_updated = "created";
            
            playerQueue.player_state = new PlayerStateInfo();
            playerQueue.player_state.steam_id = steamID;
            
            JsonFileLoader<ItemQueue>.JsonSaveFile(filePath, playerQueue);
            PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Created empty player file for %1", steamID);
        }
        else
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player file already exists for %1 (%2), checking content", playerName, steamID);
            
            // Load existing file and preserve all content
            ItemQueue existingQueue = new ItemQueue();
            JsonFileLoader<ItemQueue>.JsonLoadFile(filePath, existingQueue);
            
            if (!existingQueue)
            {
                PrintFormat("[NIGHTRO SERVER ERROR] SERVER: Failed to load existing player file for %1", steamID);
                return;
            }
            
            bool needsUpdate = false;
            
            // Only update player name if it changed
            if (existingQueue.player_name != playerName)
            {
                existingQueue.player_name = playerName;
                needsUpdate = true;
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Updated player name to %1 for %2", playerName, steamID);
            }
            
            // Ensure player state exists
            if (!existingQueue.player_state)
            {
                existingQueue.player_state = new PlayerStateInfo();
                existingQueue.player_state.steam_id = steamID;
                needsUpdate = true;
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Created missing player_state for %1", steamID);
            }
            else if (existingQueue.player_state.steam_id == "")
            {
                existingQueue.player_state.steam_id = steamID;
                needsUpdate = true;
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Updated player_state steam_id for %1", steamID);
            }
            
            // Only save if something actually changed
            if (needsUpdate)
            {
                JsonFileLoader<ItemQueue>.JsonSaveFile(filePath, existingQueue);
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Updated player file for %1", steamID);
            }
            
            // Debug: Show current file content (SERVER VERSION)
            if (existingQueue.items_to_give)
            {
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player %1 has %2 items in queue:", steamID, existingQueue.items_to_give.Count());
                for (int i = 0; i < existingQueue.items_to_give.Count(); i++)
                {
                    ItemInfo item = existingQueue.items_to_give.Get(i);
                    if (item)
                    {
                        PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: - Item %1: %2 x%3 (status: %4)", i, item.classname, item.quantity, item.status);
                    }
                }
            }
            
            if (existingQueue.processed_items)
            {
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player %1 has %2 processed items", steamID, existingQueue.processed_items.Count());
            }
        }
    }
    
    static string GetCurrentDateTime()
    {
        return "updated";
    }
    
    // Get notification settings for UI system
    static NotificationSettings GetNotificationSettings()
    {
        return m_NotificationSettings;
    }
}

modded class PlayerBase
{
    override void OnConnect()
    {
        super.OnConnect();
        
        PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player connecting: %1 (%2)", GetIdentity().GetName(), GetIdentity().GetPlainId());
        
        NightroItemGiverManager.UpdatePlayerState(this, "connect");
        
        // CRITICAL: Only create file on server side
        NightroItemGiverManager.CreatePlayerFileIfNotExists(this);
        
        // Check if player has items after connection (SERVER SIDE ONLY)
        if (GetGame().IsServer())
        {
            bool hasItems = NightroItemGiverManager.HasPendingItems(this);
            if (hasItems)
            {
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player %1 has pending items, syncing to client", GetIdentity().GetPlainId());
                
                // Send notification about items available
                string notifyMsg = "[NIGHTRO SHOP] You have items waiting! Press Ctrl+I to open the delivery menu.";
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().ChatMP, 3000, false, this, notifyMsg, "ColorYellow");
                
                // Sync data to client
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NightroItemSyncManager.SyncItemDataToClient, 5000, false, this);
            }
            else
            {
                PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player %1 has no pending items, sending empty sync", GetIdentity().GetPlainId());
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NightroItemSyncManager.SyncItemDataToClient, 3000, false, this);
            }
        }
    }

    override void EEOnCECreate()
    {
        super.EEOnCECreate();
        
        if (GetIdentity())
        {
            PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player spawned: %1 (%2)", GetIdentity().GetName(), GetIdentity().GetPlainId());
            NightroItemGiverManager.UpdatePlayerState(this, "respawn");
            
            // Check items after spawn (SERVER SIDE ONLY)
            if (GetGame().IsServer())
            {
                bool hasItems = NightroItemGiverManager.HasPendingItems(this);
                if (hasItems)
                {
                    PrintFormat("[NIGHTRO SERVER DEBUG] SERVER: Player %1 has pending items after spawn", GetIdentity().GetPlainId());
                    
                    string notifyMsg = "[NIGHTRO SHOP] You have items waiting! Press Ctrl+I to open the delivery menu.";
                    GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().ChatMP, 2000, false, this, notifyMsg, "ColorYellow");
                    
                    // Sync data to client
                    GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NightroItemSyncManager.SyncItemDataToClient, 3000, false, this);
                }
                else
                {
                    GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NightroItemSyncManager.SyncItemDataToClient, 2000, false, this);
                }
            }
        }
    }
}

static ref NightroItemGiverManager g_NightroItemGiverManager = new NightroItemGiverManager();