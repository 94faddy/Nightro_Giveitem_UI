class NightroItemWidget: ScriptedWidgetEventHandler
{
    protected Widget m_Root;
    protected ImageWidget m_ItemImage;
    protected TextWidget m_ItemName;
    protected TextWidget m_ItemQuantity;
    protected TextWidget m_ItemStatus;
    protected TextWidget m_AttachmentInfo;
    protected ButtonWidget m_ReceiveButton;
    
    protected ref ItemInfo m_ItemData;
    protected NightroItemGiverMenu m_ParentMenu;
    
    void NightroItemWidget(Widget parent, ref ItemInfo itemData, NightroItemGiverMenu parentMenu)
    {
        m_ItemData = itemData;
        m_ParentMenu = parentMenu;
        
        if (!parent)
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] Parent widget is null!");
            return;
        }
        
        if (!itemData)
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ItemData is null!");
            return;
        }
        
        PrintFormat("[NIGHTRO WIDGET DEBUG] Creating widget for item: %1", itemData.classname);
        
        m_Root = GetGame().GetWorkspace().CreateWidgets("Nightro_Giveitem/gui/layouts/nightro_item_widget.layout", parent);
        
        if (!m_Root)
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] Failed to create root widget!");
            return;
        }
        
        PrintFormat("[NIGHTRO WIDGET DEBUG] Root widget created successfully");
        
        // Find child widgets with detailed logging
        m_ItemImage = ImageWidget.Cast(m_Root.FindAnyWidget("ItemImage"));
        m_ItemName = TextWidget.Cast(m_Root.FindAnyWidget("ItemName"));
        m_ItemQuantity = TextWidget.Cast(m_Root.FindAnyWidget("ItemQuantity"));
        m_ItemStatus = TextWidget.Cast(m_Root.FindAnyWidget("ItemStatus"));
        m_AttachmentInfo = TextWidget.Cast(m_Root.FindAnyWidget("AttachmentInfo"));
        m_ReceiveButton = ButtonWidget.Cast(m_Root.FindAnyWidget("ReceiveButton"));
        
        // Debug widget finding
        PrintFormat("[NIGHTRO WIDGET DEBUG] Widget finding results:");
        PrintFormat("[NIGHTRO WIDGET DEBUG] - ItemImage: %1", m_ItemImage != null);
        PrintFormat("[NIGHTRO WIDGET DEBUG] - ItemName: %1", m_ItemName != null);
        PrintFormat("[NIGHTRO WIDGET DEBUG] - ItemQuantity: %1", m_ItemQuantity != null);
        PrintFormat("[NIGHTRO WIDGET DEBUG] - ItemStatus: %1", m_ItemStatus != null);
        PrintFormat("[NIGHTRO WIDGET DEBUG] - AttachmentInfo: %1", m_AttachmentInfo != null);
        PrintFormat("[NIGHTRO WIDGET DEBUG] - ReceiveButton: %1", m_ReceiveButton != null);
        
        if (m_ReceiveButton)
        {
            m_ReceiveButton.SetHandler(this);
            PrintFormat("[NIGHTRO WIDGET DEBUG] Button handler set successfully");
        }
        else
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ReceiveButton is null, cannot set handler!");
        }
        
        UpdateDisplay();
        
        PrintFormat("[NIGHTRO WIDGET DEBUG] Widget creation completed for: %1", itemData.classname);
    }
    
    void ~NightroItemWidget()
    {
        PrintFormat("[NIGHTRO WIDGET DEBUG] Destroying widget");
        if (m_Root)
        {
            m_Root.Unlink();
        }
    }
    
    void UpdateDisplay()
    {
        if (!m_ItemData) 
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ItemData is null in UpdateDisplay!");
            return;
        }
        
        PrintFormat("[NIGHTRO WIDGET DEBUG] === Updating Display for %1 ===", m_ItemData.classname);
        
        // Update item name
        string displayName = GetItemDisplayName(m_ItemData.classname);
        if (m_ItemName)
        {
            m_ItemName.SetText(displayName);
            m_ItemName.Update();
            PrintFormat("[NIGHTRO WIDGET DEBUG] Set item name to: %1", displayName);
        }
        else
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ItemName widget is null!");
        }
            
        // Update quantity
        if (m_ItemQuantity)
        {
            string quantityText = "Quantity: " + m_ItemData.quantity.ToString();
            m_ItemQuantity.SetText(quantityText);
            m_ItemQuantity.Update();
            PrintFormat("[NIGHTRO WIDGET DEBUG] Set quantity to: %1", quantityText);
        }
        else
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ItemQuantity widget is null!");
        }
            
        // Update status
        if (m_ItemStatus)
        {
            string statusText = "Status: " + GetStatusDisplayText(m_ItemData.status);
            m_ItemStatus.SetText(statusText);
            m_ItemStatus.Update();
            
            // Set status color based on status
            string statusLower = m_ItemData.status;
            statusLower.ToLower();
            
            if (statusLower == "pending" || statusLower == "")
            {
                m_ItemStatus.SetColor(ARGB(255, 255, 255, 0)); // Yellow
            }
            else if (statusLower == "delivered")
            {
                m_ItemStatus.SetColor(ARGB(255, 0, 255, 0)); // Green
            }
            else if (statusLower == "failed" || statusLower == "inventory_full" || statusLower == "territory_failed")
            {
                m_ItemStatus.SetColor(ARGB(255, 255, 100, 100)); // Light Red
            }
            else
            {
                m_ItemStatus.SetColor(ARGB(255, 200, 200, 200)); // Light Gray
            }
            
            PrintFormat("[NIGHTRO WIDGET DEBUG] Set status to: %1", statusText);
        }
        else
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ItemStatus widget is null!");
        }
        
        // Update attachment info
        if (m_AttachmentInfo)
        {
            string attachmentText = "Attachments: ";
            if (m_ItemData.attachments && m_ItemData.attachments.Count() > 0)
            {
                attachmentText += m_ItemData.attachments.Count().ToString() + " items";
            }
            else
            {
                attachmentText += "None";
            }
            m_AttachmentInfo.SetText(attachmentText);
            m_AttachmentInfo.Update();
            PrintFormat("[NIGHTRO WIDGET DEBUG] Set attachment info to: %1", attachmentText);
        }
        else
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] AttachmentInfo widget is null!");
        }
        
        // Set item image
        if (m_ItemImage)
        {
            string imagePath = GetItemImagePath(m_ItemData.classname);
            m_ItemImage.LoadImageFile(0, imagePath);
            m_ItemImage.Update();
            PrintFormat("[NIGHTRO WIDGET DEBUG] Set image path to: %1", imagePath);
        }
        else
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ItemImage widget is null!");
        }
        
        // Update button state
        if (m_ReceiveButton)
        {
            bool canReceive = CanReceiveItem();
            m_ReceiveButton.Enable(canReceive);
            
            if (!canReceive)
            {
                m_ReceiveButton.SetColor(ARGB(255, 100, 100, 100));
                if (m_ItemData.status == "delivered")
                {
                    m_ReceiveButton.SetText("DELIVERED");
                }
                else
                {
                    m_ReceiveButton.SetText("UNAVAILABLE");
                }
            }
            else
            {
                m_ReceiveButton.SetColor(ARGB(255, 255, 255, 255));
                m_ReceiveButton.SetText("RECEIVE");
            }
            
            m_ReceiveButton.Update();
            PrintFormat("[NIGHTRO WIDGET DEBUG] Button updated - canReceive: %1", canReceive);
        }
        else
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ReceiveButton widget is null!");
        }
        
        // Make sure the widget is visible and updated
        if (m_Root)
        {
            m_Root.Show(true);
            m_Root.Update();
            PrintFormat("[NIGHTRO WIDGET DEBUG] Root widget shown and updated");
        }
        
        PrintFormat("[NIGHTRO WIDGET DEBUG] Display update completed for: %1", m_ItemData.classname);
    }
    
    bool CanReceiveItem()
    {
        if (!m_ItemData) 
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] ItemData is null in CanReceiveItem");
            return false;
        }
        
        string status = m_ItemData.status;
        if (status == "")
        {
            status = "pending"; // Default to pending if empty
        }
        
        status.ToLower();
        
        bool canReceive = (status == "pending" || status == "inventory_full" || status == "territory_failed");
        PrintFormat("[NIGHTRO WIDGET DEBUG] CanReceiveItem for %1: status=%2, canReceive=%3", 
            m_ItemData.classname, status, canReceive);
        
        return canReceive;
    }
    
    string GetItemDisplayName(string classname)
    {
        if (classname == "") 
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] Empty classname in GetItemDisplayName");
            return "Unknown Item";
        }
        
        string displayName;
        
        // Try different config paths
        if (GetGame().ConfigGetText("CfgVehicles " + classname + " displayName", displayName) && displayName != "")
        {
            PrintFormat("[NIGHTRO WIDGET DEBUG] Found displayName in CfgVehicles for %1: %2", classname, displayName);
            return displayName;
        }
        
        if (GetGame().ConfigGetText("CfgWeapons " + classname + " displayName", displayName) && displayName != "")
        {
            PrintFormat("[NIGHTRO WIDGET DEBUG] Found displayName in CfgWeapons for %1: %2", classname, displayName);
            return displayName;
        }
        
        if (GetGame().ConfigGetText("CfgMagazines " + classname + " displayName", displayName) && displayName != "")
        {
            PrintFormat("[NIGHTRO WIDGET DEBUG] Found displayName in CfgMagazines for %1: %2", classname, displayName);
            return displayName;
        }
        
        // If no display name found, return classname
        PrintFormat("[NIGHTRO WIDGET DEBUG] No displayName found for %1, using classname", classname);
        return classname;
    }
    
    string GetItemImagePath(string classname)
    {
        if (classname == "") 
        {
            PrintFormat("[NIGHTRO WIDGET ERROR] Empty classname in GetItemImagePath");
            return "set:dayz_inventory image:missing";
        }
        
        // Try to construct image path - use lowercase for image names
        string imageName = classname;
        imageName.ToLower();
        
        string imagePath = "set:dayz_inventory image:" + imageName;
        PrintFormat("[NIGHTRO WIDGET DEBUG] Generated image path for %1: %2", classname, imagePath);
        
        return imagePath;
    }
    
    string GetStatusDisplayText(string status)
    {
        if (status == "") return "Waiting";
        
        string statusLower = status;
        statusLower.ToLower();
        
        if (statusLower == "pending")
        {
            return "Ready";
        }
        else if (statusLower == "delivered")
        {
            return "Delivered";
        }
        else if (statusLower == "failed")
        {
            return "Failed";
        }
        else if (statusLower == "inventory_full")
        {
            return "Inventory Full";
        }
        else if (statusLower == "territory_failed")
        {
            return "Territory Required";
        }
        
        return status;
    }
    
    override bool OnClick(Widget w, int x, int y, int button)
    {
        PrintFormat("[NIGHTRO WIDGET DEBUG] OnClick called, widget: %1", w != null);
        
        if (w == m_ReceiveButton)
        {
            string itemName = "NULL";
            if (m_ItemData) itemName = m_ItemData.classname;
            PrintFormat("[NIGHTRO WIDGET DEBUG] Receive button clicked for: %1", itemName);
            
            if (!m_ItemData)
            {
                PrintFormat("[NIGHTRO WIDGET ERROR] ItemData is null in OnClick!");
                return false;
            }
            
            if (!CanReceiveItem())
            {
                PrintFormat("[NIGHTRO WIDGET DEBUG] Item cannot be received: %1", m_ItemData.classname);
                return false;
            }
            
            if (m_ParentMenu)
            {
                PrintFormat("[NIGHTRO WIDGET DEBUG] Calling parent menu OnReceiveItemClicked");
                m_ParentMenu.OnReceiveItemClicked(m_ItemData);
            }
            else
            {
                PrintFormat("[NIGHTRO WIDGET ERROR] Parent menu is null!");
            }
            return true;
        }
        
        return false;
    }
    
    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w == m_ReceiveButton && CanReceiveItem())
        {
            m_ReceiveButton.SetColor(ARGB(255, 220, 220, 220));
            return true;
        }
        return false;
    }
    
    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w == m_ReceiveButton && CanReceiveItem())
        {
            m_ReceiveButton.SetColor(ARGB(255, 255, 255, 255));
            return true;
        }
        return false;
    }
    
    ItemInfo GetItemData()
    {
        return m_ItemData;
    }
    
    void SetItemData(ref ItemInfo itemData)
    {
        string itemName = "NULL";
        if (itemData) itemName = itemData.classname;
        PrintFormat("[NIGHTRO WIDGET DEBUG] SetItemData called with: %1", itemName);
        m_ItemData = itemData;
        UpdateDisplay();
    }
    
    Widget GetRootWidget()
    {
        return m_Root;
    }
}