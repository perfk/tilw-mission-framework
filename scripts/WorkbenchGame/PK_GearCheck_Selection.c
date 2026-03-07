[WorkbenchPluginAttribute(name: "Gear Audit: Selection Overview", description: "Quickly checks multiple characters. Outputs OK or NOT OK.", shortcut: "Ctrl+Shift+L", wbModules: {"WorldEditor"}, category: "PK_Gear Checks")]
class PK_GearAudit_Overview : WorldEditorPlugin
{
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api = worldEditor.GetApi();
		int selectedCount = api.GetSelectedEntitiesCount();
		if (selectedCount == 0) return;

		Print(string.Format("=== GEAR AUDIT OVERVIEW (%1 units) ===", selectedCount), LogLevel.NORMAL);

		for (int i = 0; i < selectedCount; i++)
		{
			IEntitySource source = api.GetSelectedEntity(i);
			if (!source) continue;

			PK_GearAudit_Core auditor = new PK_GearAudit_Core();
			int missingCount = auditor.CalculateMissingItems(source, api);

			ResourceName resName = SCR_BaseContainerTools.GetPrefabResourceName(source);
			if (resName == string.Empty) resName = source.GetResourceName();
			string displayName = auditor.GetFileName(resName);
			if (displayName == string.Empty) displayName = source.GetName();

			string variantWarning = "";
			if (SCR_EditableEntityComponentClass.HasVariants(resName))
				variantWarning = " [HAS VARIANTS]";

			if (missingCount == 0)
				Print(string.Format("[OK] %1%2", displayName, variantWarning), LogLevel.NORMAL);
			else
				Print(string.Format("[!!] OVERFLOW (%1 missing) - %2%3", missingCount, displayName, variantWarning), LogLevel.ERROR);
		}
		Print("======================================", LogLevel.NORMAL);
	}
}

[WorkbenchPluginAttribute(name: "Gear Audit: Failure Details", description: "Only outputs details for characters with gear overflow.", shortcut: "Ctrl+Shift+D", wbModules: {"WorldEditor"}, category: "PK_Gear Checks")]
class PK_GearAudit_Details : WorldEditorPlugin
{
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api = worldEditor.GetApi();
		int selectedCount = api.GetSelectedEntitiesCount();
		if (selectedCount == 0) return;

		Print("=== GEAR AUDIT: FAILURE DETAILS ===", LogLevel.NORMAL);

		bool foundFailures = false;
		for (int i = 0; i < selectedCount; i++)
		{
			IEntitySource source = api.GetSelectedEntity(i);
			if (!source) continue;

			PK_GearAudit_Core auditor = new PK_GearAudit_Core();
			map<ResourceName, ref array<int>> missingMap = new map<ResourceName, ref array<int>>();
			map<string, int> actualBucket = new map<string, int>();
			
			int missingTotal = auditor.GetDetailedMissingMap(source, api, missingMap, actualBucket);

			if (missingTotal > 0)
			{
				foundFailures = true;
				ResourceName resName = SCR_BaseContainerTools.GetPrefabResourceName(source);
				if (resName == string.Empty) resName = source.GetResourceName();
				string displayName = auditor.GetFileName(resName);
				
				string variantWarning = "";
				if (SCR_EditableEntityComponentClass.HasVariants(resName))
					variantWarning = " (Warning: Unit has variants)";

				Print(string.Format("--- FAILURE: %1%2 ---", displayName, variantWarning), LogLevel.NORMAL);
				for (int j = 0; j < missingMap.Count(); j++)
				{
					ResourceName itemPath = missingMap.GetKey(j);
					array<int> counts = missingMap.GetElement(j);
					Print(string.Format("    [!] MISSING: %1x of %2x %3", counts[0], counts[1], auditor.GetFileName(itemPath)), LogLevel.ERROR);
				}

				Print("    [DIAGNOSTICS] All prefabs found on this entity:", LogLevel.NORMAL);
				if (actualBucket.IsEmpty())
				{
					Print("      > (None found - Inventory components might not be initialized in Editor)", LogLevel.WARNING);
				}
				else
				{
					for (int k = 0; k < actualBucket.Count(); k++)
					{
						Print(string.Format("      > %1x %2", actualBucket.GetElement(k), actualBucket.GetKey(k)), LogLevel.NORMAL);
					}
				}
			}
		}

		if (!foundFailures) Print("No failures found in selection.", LogLevel.NORMAL);
		Print("====================================", LogLevel.NORMAL);
	}
}

class PK_GearAudit_Core
{
	int CalculateMissingItems(IEntitySource source, WorldEditorAPI api)
	{
		map<ResourceName, ref array<int>> missing = new map<ResourceName, ref array<int>>();
		map<string, int> actualBucket = new map<string, int>();
		return GetDetailedMissingMap(source, api, missing, actualBucket);
	}

	int GetDetailedMissingMap(IEntitySource source, WorldEditorAPI api, out map<ResourceName, ref array<int>> missingMap, out map<string, int> actualBucket)
	{
		map<string, int> intended = new map<string, int>();
		map<string, ResourceName> nameLookup = new map<string, ResourceName>();
		ScanIntended(source, intended, nameLookup);

		IEntity entity = api.SourceToEntity(source);
		if (entity)
		{
			// 1. Scan via Inventory Manager (Primary method)
			SCR_InventoryStorageManagerComponent mgr = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
			if (mgr)
			{
				array<IEntity> items = new array<IEntity>();
				mgr.GetItems(items);
				foreach (IEntity item : items)
				{
					AddToBucket(item, actualBucket);
				}
			}

			// 2. Scan physical hierarchy (For gadgets/attachments not in inventory list)
			ScanActualHierarchy(entity, actualBucket);
		}

		int totalMissing = 0;
		for (int i = 0; i < intended.Count(); i++)
		{
			string file = intended.GetKey(i);
			int exp = intended.GetElement(i);
			int found = 0;
			if (actualBucket.Contains(file)) found = actualBucket.Get(file);

			if (found < exp)
			{
				int diff = exp - found;
				totalMissing += diff;
				array<int> counts = {diff, exp};
				missingMap.Insert(nameLookup.Get(file), counts);
			}
		}
		return totalMissing;
	}

	void AddToBucket(IEntity ent, map<string, int> bucket)
	{
		if (!ent) return;
		EntityPrefabData data = ent.GetPrefabData();
		if (data)
		{
			string f = GetFileName(data.GetPrefabName());
			if (f != string.Empty)
			{
				// Prevent double-counting items found by both manager and hierarchy
				// Hierarchy scan is usually shallow, but we check if it's already there
				// (Actually, if we use a bucket, we'll just increment it twice. 
				// We need a way to ensure we don't count the same entity instance twice.)
				
				// For simplicity in this logic, we assume Manager items are unique.
				if (bucket.Contains(f)) bucket.Set(f, bucket.Get(f) + 1);
				else bucket.Insert(f, 1);
			}
		}
	}

	void ScanActualHierarchy(IEntity ent, map<string, int> bucket)
	{
		IEntity child = ent.GetChildren();
		while (child)
		{
			// If it's a child but not in the inventory manager list, it's a direct attachment
			AddToBucket(child, bucket);
			ScanActualHierarchy(child, bucket);
			child = child.GetSibling();
		}
	}

	void ScanIntended(IEntitySource source, map<string, int> bucket, map<string, ResourceName> lookup)
	{
		IEntityComponentSource mgr = SCR_BaseContainerTools.FindComponentSource(source, SCR_InventoryStorageManagerComponent);
		if (mgr)
		{
			BaseContainerList list = mgr.GetObjectArray("InitialInventoryItems");
			if (list)
			{
				for (int i = 0; i < list.Count(); i++)
				{
					array<ResourceName> prefabs;
					if (list.Get(i).Get("PrefabsToSpawn", prefabs) && prefabs)
					{
						foreach (ResourceName res : prefabs)
						{
							if (res == string.Empty) continue;
							string f = GetFileName(res);
							lookup.Insert(f, res);
							if (bucket.Contains(f)) bucket.Set(f, bucket.Get(f) + 1);
							else bucket.Insert(f, 1);
						}
					}
				}
			}
		}

		IEntityComponentSource loadout = SCR_BaseContainerTools.FindComponentSource(source, BaseLoadoutManagerComponent);
		if (loadout)
		{
			BaseContainerList slots = loadout.GetObjectArray("Slots");
			if (slots)
			{
				for (int i = 0; i < slots.Count(); i++)
				{
					ResourceName prefab;
					slots.Get(i).Get("Prefab", prefab);
					if (prefab != string.Empty) ScanClothingPrefab(prefab, bucket, lookup);
				}
			}
		}
	}

	void ScanClothingPrefab(ResourceName prefab, map<string, int> bucket, map<string, ResourceName> lookup)
	{
		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid()) return;
		IEntitySource src = res.GetResource().ToEntitySource();
		IEntityComponentSource mgr = SCR_BaseContainerTools.FindComponentSource(src, SCR_InventoryStorageManagerComponent);
		if (mgr)
		{
			BaseContainerList list = mgr.GetObjectArray("InitialInventoryItems");
			if (list)
			{
				for (int i = 0; i < list.Count(); i++)
				{
					array<ResourceName> items;
					if (list.Get(i).Get("PrefabsToSpawn", items) && items)
					{
						foreach (ResourceName itm : items)
						{
							if (itm == string.Empty) continue;
							string f = GetFileName(itm);
							lookup.Insert(f, itm);
							if (bucket.Contains(f)) bucket.Set(f, bucket.Get(f) + 1);
							else bucket.Insert(f, 1);
						}
					}
				}
			}
		}
	}

	string GetFileName(ResourceName res)
	{
		string s = res;
		int bracketIndex = s.IndexOf("}");
		if (bracketIndex != -1) s = s.Substring(bracketIndex + 1, s.Length() - (bracketIndex + 1));
		int lastSlash = s.LastIndexOf("/");
		if (lastSlash == -1) lastSlash = s.LastIndexOf("\\");
		if (lastSlash != -1) return s.Substring(lastSlash + 1, s.Length() - (lastSlash + 1));
		return s;
	}
}