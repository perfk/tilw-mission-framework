[WorkbenchPluginAttribute(name: "Check Gear (Final)", description: "Compares a pre-defined loadout file with a selected character.", shortcut: "Ctrl+Shift+G", wbModules: {"WorldEditor"})]
class PK_CheckGearPlugin : WorldEditorPlugin
{
	const string BEFORE_LOADOUT_FILE = "final_loadout.txt";

	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor) { Print("WorldEditor not available", LogLevel.ERROR); return; }
		WorldEditorAPI api = worldEditor.GetApi();
		if (!api) { Print("WorldEditorAPI not available", LogLevel.ERROR); return; }

		// --- 1. Get Selected Entity ---
		if (api.GetSelectedEntitiesCount() != 1)
		{
			Print("Please select exactly one character entity (the final child).", LogLevel.WARNING);
            return;
		}
		
		IEntitySource source = api.GetSelectedEntity(0);
		Print(string.Format("========== ANALYZING CHARACTER: %1 ==========", source.GetResourceName()), LogLevel.NORMAL);

		// --- 2. Get the "BEFORE" list from the final_loadout.txt file ---
		Print("--- Reading 'Before' list from " + BEFORE_LOADOUT_FILE + " ---", LogLevel.NORMAL);
		map<ResourceName, int> beforeItems = new map<ResourceName, int>();
		array<string> beforeLines = SCR_FileIOHelper.ReadFileContent(BEFORE_LOADOUT_FILE);
		
		if (!beforeLines || beforeLines.IsEmpty())
		{
			Print(string.Format("Error: Could not read '%1' or it is empty.", BEFORE_LOADOUT_FILE), LogLevel.ERROR);
			return;
		}
		
		foreach (string line : beforeLines)
		{
			line.TrimInPlace();
			if (line == string.Empty) continue;
			
			ResourceName resName = line;
			if (beforeItems.Contains(resName))
				beforeItems.Set(resName, beforeItems.Get(resName) + 1);
			else
				beforeItems.Insert(resName, 1);
		}
		
		// Print the "Before" list
		for (int i = 0; i < beforeItems.Count(); i++)
		{
			Print(string.Format("  - (Should Spawn) %1x %2", beforeItems.GetElement(i), beforeItems.GetKey(i)), LogLevel.NORMAL);
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
		for (int i = 0; i < afterItems.Count(); i++)
		{
			Print(string.Format("  - (Actually Spawned) %1x %2", afterItems.GetElement(i), afterItems.GetKey(i)), LogLevel.NORMAL);
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

		if (discardedCount == 0)
		{
			Print("No items were discarded. 'Before' and 'After' lists match.", LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("A total of %1 items were discarded due to overflow or other errors.", discardedCount), LogLevel.WARNING);
		}
	}
}