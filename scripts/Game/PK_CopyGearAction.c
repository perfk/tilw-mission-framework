// PK_CopyGearAction.c
class PK_CopyGearAction : ScriptedUserAction
{
    // Static variable to store the copied gear (shared across all instances)
    static ref array<ResourceName> s_aCopiedGear = {};
    
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        // Clear previous gear list
        s_aCopiedGear.Clear();
        
        // Get the inventory storage manager from the target entity
        SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(pOwnerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
        
        if (!inventoryManager)
        {
            Print("No inventory manager found on target entity", LogLevel.WARNING);
            return;
        }
        
        // Get all items from the inventory
        array<IEntity> items = {};
        inventoryManager.GetItems(items);
        
        Print("========== COPYING GEAR ==========", LogLevel.NORMAL);
        Print("Total items found: " + items.Count(), LogLevel.NORMAL);
        
        // Store each item's prefab resource name
        foreach (IEntity item : items)
        {
            if (!item)
                continue;
                
            EntityPrefabData prefabData = item.GetPrefabData();
            if (!prefabData)
                continue;
                
            ResourceName itemResource = prefabData.GetPrefabName();
            s_aCopiedGear.Insert(itemResource);
            
            Print("  - " + itemResource, LogLevel.NORMAL);
        }
        
        Print("========== GEAR COPIED ==========", LogLevel.NORMAL);
        Print("Total gear items saved: " + s_aCopiedGear.Count(), LogLevel.NORMAL);
        
        // Notify the player
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
        {
            hintManager.ShowCustomHint("Gear copied: " + s_aCopiedGear.Count() + " items", "Gear System", 5);
        }
    }
    
    override bool CanBeShownScript(IEntity user)
    {
        return true;
    }
    
    override bool CanBePerformedScript(IEntity user)
    {
        return true;
    }
}