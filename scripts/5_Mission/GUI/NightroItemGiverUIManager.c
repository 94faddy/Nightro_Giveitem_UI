class NightroItemGiverUIManager
{
    private static ref NightroItemGiverMenu m_Menu;
    
    static void Init()
    {
        // Initialize UI manager
    }
    
    static void ToggleMenu()
    {
        if (m_Menu)
        {
            CloseMenu();
        }
        else
        {
            OpenMenu();
        }
    }
    
    static void OpenMenu()
    {
        if (m_Menu) return;
        
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.IsAlive()) return;
        
        m_Menu = new NightroItemGiverMenu;
        GetGame().GetUIManager().ShowScriptedMenu(m_Menu, NULL);
    }
    
    static void CloseMenu()
    {
        if (m_Menu)
        {
            GetGame().GetUIManager().HideScriptedMenu(m_Menu);
            m_Menu = NULL;
        }
    }
    
    static bool IsMenuOpen()
    {
        return (m_Menu != NULL);
    }
    
    static bool CanReceiveItem(PlayerBase player, ref ItemInfo itemData, out string errorMessage)
    {
        errorMessage = "";
        
        if (!player || !player.GetIdentity() || !itemData)
        {
            errorMessage = "[NIGHTRO SHOP] Invalid player or item data.";
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
        
        if (player.IsInVehicle() && player.GetParent())
        {
            errorMessage = "[NIGHTRO SHOP] You cannot receive items while in a vehicle.";
            return false;
        }
        
        // Check if item class is valid
        if (!IsValidItemClass(itemData.classname))
        {
            errorMessage = "[NIGHTRO SHOP] Invalid item: " + itemData.classname;
            return false;
        }
        
        // Check territory requirements
        LBGroup playerGroup = GetPlayerGroup(player);
        if (!playerGroup)
        {
            errorMessage = "[NIGHTRO SHOP] You need to join or create a group to receive items!";
            return false;
        }
        
        if (!GroupHasPlotpole(playerGroup))
        {
            errorMessage = "[NIGHTRO SHOP] Your group needs to place a Plotpole to receive items! Create your territory first.";
            return false;
        }
        
        float distanceToPlotpole;
        if (!IsPlayerInGroupTerritory(player, playerGroup, distanceToPlotpole))
        {
            errorMessage = "[NIGHTRO SHOP] You are outside your group's territory! Move closer to your Plotpole. Distance: " + ((int)distanceToPlotpole).ToString() + "m";
            return false;
        }
        
        // Check inventory space
        if (!CanFitInInventory(player, itemData.classname, itemData.quantity))
        {
            errorMessage = "[NIGHTRO SHOP] Your inventory is full! Clear space to receive " + itemData.quantity.ToString() + "x " + itemData.classname;
            return false;
        }
        
        return true;
    }
    
    static bool GiveItemToPlayer(PlayerBase player, ref ItemInfo itemData)
    {
        if (!player || !player.GetIdentity() || !itemData) return false;
        
        string steamID = player.GetIdentity().GetPlainId();
        string filePath = "$profile:NightroItemGiverData/pending_items/" + steamID + ".json";
        
        if (!FileExist(filePath)) return false;
        
        ItemQueue itemQueue = new ItemQueue();
        JsonFileLoader<ItemQueue>.JsonLoadFile(filePath, itemQueue);
        
        if (!itemQueue || !itemQueue.items_to_give) return false;
        
        // Find and process the item
        for (int i = 0; i < itemQueue.items_to_give.Count(); i++)
        {
            ItemInfo queueItem = itemQueue.items_to_give.Get(i);
            if (queueItem && queueItem.classname == itemData.classname && queueItem.quantity == itemData.quantity)
            {
                // Give items to player
                int itemsGiven = 0;
                for (int j = 0; j < queueItem.quantity; j++)
                {
                    EntityAI item = player.GetInventory().CreateInInventory(queueItem.classname);
                    if (item)
                    {
                        itemsGiven++;
                        
                        // Handle attachments
                        if (queueItem.attachments && queueItem.attachments.Count() > 0)
                        {
                            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AttachItemsToWeapon, 300, false, item, queueItem.attachments, player);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                
                if (itemsGiven > 0)
                {
                    // Mark as delivered and move to processed
                    queueItem.status = "delivered";
                    queueItem.processed_at = GetCurrentTimestamp();
                    
                    ProcessedItemInfo processedItem = new ProcessedItemInfo();
                    processedItem.classname = queueItem.classname;
                    processedItem.quantity = itemsGiven;
                    processedItem.status = "delivered";
                    processedItem.processed_at = GetCurrentTimestamp();
                    
                    if (queueItem.attachments && queueItem.attachments.Count() > 0)
                    {
                        processedItem.attachments = new array<ref AttachmentInfo>;
                        for (int k = 0; k < queueItem.attachments.Count(); k++)
                        {
                            AttachmentInfo originalAttachment = queueItem.attachments.Get(k);
                            AttachmentInfo copyAttachment = new AttachmentInfo();
                            copyAttachment.classname = originalAttachment.classname;
                            copyAttachment.quantity = originalAttachment.quantity;
                            copyAttachment.status = "delivered";
                            processedItem.attachments.Insert(copyAttachment);
                        }
                    }
                    
                    if (!itemQueue.processed_items)
                        itemQueue.processed_items = new array<ref ProcessedItemInfo>;
                    itemQueue.processed_items.Insert(processedItem);
                    
                    // Remove from pending items
                    itemQueue.items_to_give.RemoveOrdered(i);
                    
                    // Save updated queue
                    JsonFileLoader<ItemQueue>.JsonSaveFile(filePath, itemQueue);
                    
                    return true;
                }
            }
        }
        
        return false;
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
    
    static LBGroup GetPlayerGroup(PlayerBase player)
    {
        if (!player)
            return null;
            
        return player.GetLBGroup();
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
    
    static string GetCurrentTimestamp()
    {
        float timestamp = GetGame().GetTime();
        return timestamp.ToString();
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
                        
                        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(InstallMagazineDelayed, 800, false, weapon, attachment.classname);
                    }
                    else
                    {
                        EntityAI playerMagItem = player.GetInventory().CreateInInventory(attachment.classname);
                        if (playerMagItem)
                        {
                            Magazine mag = Magazine.Cast(playerMagItem);
                            if (mag) mag.ServerSetAmmoCount(mag.GetAmmoMax());
                        }
                    }
                }
                else
                {
                    EntityAI attachmentEntity = parentItem.GetInventory().CreateAttachment(attachment.classname);
                    if (!attachmentEntity)
                    {
                        EntityAI playerAttachmentItem = player.GetInventory().CreateInInventory(attachment.classname);
                    }
                }
            }
        }
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
    
    static void InstallMagazineDelayed(Weapon_Base weapon, string magazineClass)
    {
        if (!weapon || magazineClass == "") return;
        
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
            }
        }
    }
}