class NightroItemGiverUIManager
{
    private static ref NightroItemGiverMenu m_Menu;
    
    static void Init()
    {
        PrintFormat("[NIGHTRO CLIENT DEBUG] UI Manager initialized");
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
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] Opening item giver menu");
        
        m_Menu = new NightroItemGiverMenu;
        GetGame().GetUIManager().ShowScriptedMenu(m_Menu, NULL);
    }
    
    static void CloseMenu()
    {
        if (m_Menu)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Closing item giver menu");
            GetGame().GetUIManager().HideScriptedMenu(m_Menu);
            m_Menu = NULL;
        }
    }
    
    static bool IsMenuOpen()
    {
        return (m_Menu != NULL);
    }
    
    static void RefreshMenuIfOpen()
    {
        if (m_Menu)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Refreshing open menu");
            m_Menu.LoadPendingItems();
        }
    }
    
    // ⭐ CRITICAL FIX: Basic validation without LBmaster calls for UI
    static bool CanReceiveItemBasic(PlayerBase player, ref ItemInfo itemData, out string errorMessage)
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
        
        // ⭐ CRITICAL FIX: Skip item validation in UI to prevent crashes
        // Server will validate items during actual delivery
        if (!itemData.classname || itemData.classname == "")
        {
            errorMessage = "[NIGHTRO SHOP] Invalid item data.";
            return false;
        }
        
        if (itemData.quantity <= 0)
        {
            errorMessage = "[NIGHTRO SHOP] Invalid item quantity.";
            return false;
        }
        
        // ⭐ CRITICAL FIX: Skip inventory space check in UI to prevent crashes
        // Server will check inventory space during actual delivery
        
        return true;
    }
    
    // ⭐ ORIGINAL: Full validation with LBmaster (still available but not used in UI)
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
        
        // ⭐ PROTECTED: Safe LBmaster calls with null checks
        // Territory check (simplified client-side version)
        LBGroup playerGroup = GetPlayerGroupSafe(player);
        if (!playerGroup)
        {
            errorMessage = "[NIGHTRO SHOP] You need to join or create a group to receive items!";
            return false;
        }
        
        if (!GroupHasPlotpoleSafe(playerGroup))
        {
            errorMessage = "[NIGHTRO SHOP] Your group needs to place a Plotpole to receive items! Create your territory first.";
            return false;
        }
        
        float distanceToPlotpole;
        if (!IsPlayerInGroupTerritorySafe(player, playerGroup, distanceToPlotpole))
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
        
        PrintFormat("[NIGHTRO CLIENT DEBUG] CLIENT: Requesting item delivery: %1x %2", itemData.quantity, itemData.classname);
        
        // Send delivery request to server via sync system
        NightroItemSyncClient.RequestItemDelivery(itemData.classname, itemData.quantity);
        
        return true; // Return true immediately, actual result will come from server
    }
    
    // ⭐ CRITICAL FIX: Cached item validation to prevent repeated CreateObjectEx calls
    private static ref map<string, bool> m_ItemValidityCache;
    
    static bool IsValidItemClass(string itemName)
    {
        if (!itemName || itemName == "") return false;
        
        // Initialize cache if needed
        if (!m_ItemValidityCache)
        {
            m_ItemValidityCache = new map<string, bool>;
        }
        
        // Check cache first
        if (m_ItemValidityCache.Contains(itemName))
        {
            return m_ItemValidityCache.Get(itemName);
        }
        
        // ⭐ CRITICAL FIX: Use config check instead of CreateObjectEx to prevent crashes
        bool isValid = false;
        
        // Check in different config sections
        if (GetGame().ConfigIsExisting("CfgVehicles " + itemName))
        {
            isValid = true;
        }
        else if (GetGame().ConfigIsExisting("CfgWeapons " + itemName))
        {
            isValid = true;
        }
        else if (GetGame().ConfigIsExisting("CfgMagazines " + itemName))
        {
            isValid = true;
        }
        
        // Cache the result
        m_ItemValidityCache.Set(itemName, isValid);
        
        return isValid;
    }
    
    // ⭐ CRITICAL FIX: Simplified inventory check to prevent crashes
    static bool CanFitInInventory(PlayerBase player, string itemClassname, int quantity)
    {
        if (!player || !player.GetInventory()) return false;
        
        // ⭐ CRITICAL FIX: Skip actual object creation in UI, return true
        // Server will perform real inventory check during delivery
        PrintFormat("[NIGHTRO CLIENT DEBUG] Skipping inventory check for UI (server will validate)");
        return true;
    }
    
    // ⭐ CRITICAL FIX: Safe LBmaster calls with comprehensive null checks
    static LBGroup GetPlayerGroupSafe(PlayerBase player)
    {
        if (!player)
            return null;
        
        // Check if LBmaster is properly initialized
        #ifndef LBmaster_GroupDLCPlotpole
        PrintFormat("[NIGHTRO CLIENT DEBUG] LBmaster not available");
        return null;
        #endif
        
        // Additional safety checks
        if (!player.GetIdentity())
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] Player identity not available");
            return null;
        }
        
        LBGroup group = player.GetLBGroup();
        if (!group)
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] Player has no LBGroup");
            return null;
        }
        
        return group;
    }

    static bool GroupHasPlotpoleSafe(LBGroup playerGroup)
    {
        if (!playerGroup) return false;
        
        #ifdef LBmaster_GroupDLCPlotpole
        // Multiple safety checks
        if (!TerritoryFlag) 
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] TerritoryFlag class not available");
            return true; // Allow if system not available
        }
        
        if (!TerritoryFlag.all_Flags) 
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] TerritoryFlag.all_Flags not available");
            return true; // Allow if system not available
        }
        
        if (!playerGroup.shortname)
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] Group shortname not available");
            return false;
        }
        
        string tagLower = playerGroup.shortname + "";
        tagLower.ToLower();
        int groupTagHash = tagLower.Hash();
        
        // Safe iteration with additional checks
        foreach (TerritoryFlag flag : TerritoryFlag.all_Flags)
        {
            if (!flag) continue;
            
            if (flag.ownerGroupTagHash && flag.ownerGroupTagHash == groupTagHash)
            {
                return true;
            }
        }
        return false;
        #else
        PrintFormat("[NIGHTRO CLIENT DEBUG] LBmaster_GroupDLCPlotpole not available");
        return true; // Allow if not available
        #endif
    }

    static bool IsPlayerInGroupTerritorySafe(PlayerBase player, LBGroup playerGroup, out float distanceToNearestFriendlyFlag)
    {
        distanceToNearestFriendlyFlag = 999999.0;
        
        if (!player || !playerGroup) 
            return false;

        #ifdef LBmaster_GroupDLCPlotpole
        // Multiple safety checks
        if (!TerritoryFlag) 
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] TerritoryFlag class not available");
            return true; // Allow if system not available
        }
        
        if (!TerritoryFlag.all_Flags) 
        {
            PrintFormat("[NIGHTRO CLIENT DEBUG] TerritoryFlag.all_Flags not available");
            return true; // Allow if system not available
        }
        
        if (!playerGroup.shortname)
        {
            PrintFormat("[NIGHTRO CLIENT ERROR] Group shortname not available");
            return false;
        }
        
        vector playerPos = player.GetPosition();
        string groupTag = playerGroup.shortname;
        
        string groupTagLower = groupTag;
        groupTagLower.ToLower();
        int groupTagHash = groupTagLower.Hash();
        
        // Manual search for nearest friendly flag (safer approach)
        TerritoryFlag nearestFriendlyFlag = null;
        float nearestDistance = 999999.0;
        
        // Safe iteration through all flags
        foreach (TerritoryFlag flag : TerritoryFlag.all_Flags)
        {
            if (!flag) continue;
            if (!flag.ownerGroupTagHash) continue;
            if (flag.ownerGroupTagHash != groupTagHash) continue;
            
            // Get flag position safely
            vector flagPos = flag.GetPosition();
            if (flagPos == "0 0 0") continue; // Invalid position
            
            float distance = vector.Distance(playerPos, flagPos);
            if (distance >= 0 && distance < nearestDistance)
            {
                nearestDistance = distance;
                nearestFriendlyFlag = flag;
            }
        }
        
        if (nearestFriendlyFlag)
        {
            distanceToNearestFriendlyFlag = nearestDistance;
            
            // Check if player is within territory radius (default 60m for plotpoles)
            if (distanceToNearestFriendlyFlag <= 60.0)
            {
                return true;
            }
        }
        
        return false;
        #else
        PrintFormat("[NIGHTRO CLIENT DEBUG] LBmaster_GroupDLCPlotpole not available");
        return true; // Allow if not available
        #endif
    }
    
    // ⭐ LEGACY: Keep original functions for compatibility
    static LBGroup GetPlayerGroup(PlayerBase player)
    {
        return GetPlayerGroupSafe(player);
    }

    static bool GroupHasPlotpole(LBGroup playerGroup)
    {
        return GroupHasPlotpoleSafe(playerGroup);
    }

    static bool IsPlayerInGroupTerritory(PlayerBase player, LBGroup playerGroup, out float distanceToNearestFriendlyFlag)
    {
        return IsPlayerInGroupTerritorySafe(player, playerGroup, distanceToNearestFriendlyFlag);
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