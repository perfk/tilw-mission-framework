/*
===========================================
FUNCTIONAL SCRIPT (SINGLE PREFAB ONLY)
===========================================
This script performs a "Before/After" inventory analysis on a character prefab.
It reads the intended inventory directly from the prefab data and compares it
to the inventory of a temporarily spawned entity to identify items that are
discarded due to inventory overflow or other issues.

NOTE: This script CANNOT read items from inherited parent prefabs due to API limitations.

The key was discovering that the 'PrefabsToSpawn' property had to be read
as an 'array<ResourceName>' type using the .Get() method.
*/

[WorkbenchPluginAttribute(name: "Check Gear (Single Prefab)", description: "Compares a single prefab's intended inventory with its spawned state.", shortcut: "Ctrl+Shift+L", wbModules: {"WorldEditor"})]
class PK_GearCheck_Future : WorldEditorPlugin
{
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor) { Print("WorldEditor not available", LogLevel.ERROR); return; }
		WorldEditorAPI api = worldEditor.GetApi();
		if (!api) { Print("WorldEditorAPI not available", LogLevel.ERROR); return; }

		// --- 1. Get Selected Entity ---
		if (api.GetSelectedEntitiesCount() != 1)
		{
			Print("Please select exactly one character entity.", LogLevel.WARNING);
            return;
		}
		
		IEntitySource source = api.GetSelectedEntity(0);
		Print(string.Format("========== ANALYZING CHARACTER: %1 ==========", source.GetResourceName()), LogLevel.NORMAL);

		// --- 2. Get the "BEFORE" list from raw prefab data ---
		Print("--- Reading 'Before' list from Prefab Data... ---", LogLevel.NORMAL);
		map<ResourceName, int> beforeItems = new map<ResourceName, int>();
		
		IEntityComponentSource invManagerSource = SCR_BaseContainerTools.FindComponentSource(source, SCR_InventoryStorageManagerComponent);
		if (!invManagerSource)
		{
			Print("[DEBUG] Failed to find SCR_InventoryStorageManagerComponent source.", LogLevel.WARNING);
		}
		else
		{
			BaseContainerList initialItemsList = invManagerSource.GetObjectArray("InitialInventoryItems");
			if (initialItemsList)
			{
				for (int i = 0; i < initialItemsList.Count(); i++)
				{
					BaseContainer itemConfig = initialItemsList.Get(i);
					if (!itemConfig) continue;

					array<ResourceName> prefabsToSpawnArray;
					if (itemConfig.Get("PrefabsToSpawn", prefabsToSpawnArray) && prefabsToSpawnArray)
					{
						for (int j = 0; j < prefabsToSpawnArray.Count(); j++)
						{
							ResourceName resName = prefabsToSpawnArray.Get(j);
							if (resName && resName != string.Empty)
							{
								if (beforeItems.Contains(resName))
									beforeItems.Set(resName, beforeItems.Get(resName) + 1);
								else
									beforeItems.Insert(resName, 1);
							}
						}
					}
				}
			}
		}
		
		// Print the "Before" list
		if (beforeItems.IsEmpty())
		{
			Print("  'Before' list is empty. No items found in raw prefab data.", LogLevel.WARNING);
		}
		else
		{
			Print(string.Format("--- 'Before' list contains %1 unique items: ---", beforeItems.Count()), LogLevel.NORMAL);
			for (int i = 0; i < beforeItems.Count(); i++)
			{
				Print(string.Format("  - (Should Spawn) %1x %2", beforeItems.GetElement(i), beforeItems.GetKey(i)), LogLevel.NORMAL);
			}
		}


		// --- 3. Get the "AFTER" list by creating a temporary entity ---
		Print("--- Reading 'After' list from Initialized Entity... ---", LogLevel.NORMAL);
		map<ResourceName, int> afterItems = new map<ResourceName, int>();
		
		IEntity entity = api.SourceToEntity(source);
		if (entity)
		{
			SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
			if (invManager)
			{
				array<IEntity> allSpawnedItems = new array<IEntity>();
				invManager.GetItems(allSpawnedItems);

				foreach(IEntity item : allSpawnedItems)
				{
					ResourceName resName = item.GetPrefabData().GetPrefabName();
					if (afterItems.Contains(resName))
						afterItems.Set(resName, afterItems.Get(resName) + 1);
					else
						afterItems.Insert(resName, 1);
				}
			}
		}

		// Print the "After" list
		if (!afterItems.IsEmpty())
		{
			Print(string.Format("--- 'After' list contains %1 unique items: ---", afterItems.Count()), LogLevel.NORMAL);
			for (int i = 0; i < afterItems.Count(); i++)
			{
				Print(string.Format("  - (Actually Spawned) %1x %2", afterItems.GetElement(i), afterItems.GetKey(i)), LogLevel.NORMAL);
			}
		}


		// --- 4. FINAL ANALYSIS: Compare the lists ---
		Print("", LogLevel.NORMAL);
		Print("--- DISCARDED ITEM ANALYSIS ---", LogLevel.NORMAL);
		
		int discardedCount = 0;
		for (int i = 0; i < beforeItems.Count(); i++)
		{
			ResourceName resName = beforeItems.GetKey(i);
			int beforeCount = beforeItems.GetElement(i);
			int afterCount = 0;
			if (afterItems.Contains(resName))
				afterCount = afterItems.Get(resName);
			
			int diff = beforeCount - afterCount;
			if (diff > 0)
			{
				Print(string.Format("  DISCARDED: %1x %2", diff, resName), LogLevel.ERROR);
				discardedCount += diff;
			}
		}

		if (discardCount == 0)
		{
			Print("No items were discarded from this specific prefab. (NOTE: Inherited items are not included in this check).", LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("A total of %1 items were discarded due to overflow or other errors.", discardedCount), LogLevel.WARNING);
		}
	}
}