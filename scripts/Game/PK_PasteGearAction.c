// PK_PasteGearAction.c
class PK_PasteGearAction : ScriptedUserAction
{
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        // Check if we have any copied gear
        if (!PK_CopyGearAction.s_aCopiedGear || PK_CopyGearAction.s_aCopiedGear.Count() == 0)
        {
            Print("No gear has been copied yet!", LogLevel.WARNING);
            
            SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
            if (hintManager)
            {
                hintManager.ShowCustomHint("No gear copied! Use Copy Gear action first.", "Gear System", 5);
            }
            return;
        }
        
        // Get the inventory storage manager from the target entity
        SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(pOwnerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
        
        if (!inventoryManager)
        {
            Print("No inventory manager found on target entity", LogLevel.WARNING);
            return;
        }
        
        Print("========== PASTING GEAR ==========", LogLevel.NORMAL);
        Print("Attempting to add " + PK_CopyGearAction.s_aCopiedGear.Count() + " items", LogLevel.NORMAL);
        
        int successCount = 0;
        int failCount = 0;
        
        // Spawn and add each item to the target's inventory
        foreach (ResourceName itemResource : PK_CopyGearAction.s_aCopiedGear)
        {
            if (itemResource.IsEmpty())
                continue;
            
            // Load the item resource
            Resource resource = Resource.Load(itemResource);
            if (!resource || !resource.IsValid())
            {
                Print("  Failed to load resource: " + itemResource, LogLevel.WARNING);
                failCount++;
                continue;
            }
            
            // Spawn the item in the world temporarily
            EntitySpawnParams spawnParams = new EntitySpawnParams();
            spawnParams.TransformMode = ETransformMode.WORLD;
            pOwnerEntity.GetTransform(spawnParams.Transform);
            
            IEntity spawnedItem = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
            
            if (!spawnedItem)
            {
                Print("  Failed to spawn item: " + itemResource, LogLevel.WARNING);
                failCount++;
                continue;
            }
            
            // Try to insert the item into the inventory
            bool inserted = inventoryManager.TryInsertItem(spawnedItem);
            
            if (inserted)
            {
                Print("  Successfully added: " + itemResource, LogLevel.NORMAL);
                successCount++;
            }
            else
            {
                Print("  Failed to insert into inventory: " + itemResource, LogLevel.WARNING);
                // Delete the item if it couldn't be inserted
                SCR_EntityHelper.DeleteEntityAndChildren(spawnedItem);
                failCount++;
            }
        }
        
        Print("========== GEAR PASTE COMPLETE ==========", LogLevel.NORMAL);
        Print("Success: " + successCount + " | Failed: " + failCount, LogLevel.NORMAL);
        
        // Notify the player
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
        {
            hintManager.ShowCustomHint("Gear applied: " + successCount + " items (" + failCount + " failed)", "Gear System", 5);
        }
    }
    
    override bool CanBeShownScript(IEntity user)
    {
        return true;
    }
    
    override bool CanBePerformedScript(IEntity user)
    {
        // Only show this action if gear has been copied
        return PK_CopyGearAction.s_aCopiedGear && PK_CopyGearAction.s_aCopiedGear.Count() > 0;
    }
}