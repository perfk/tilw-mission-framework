// In-game diagnostic script to identify gear overflow in characters.
// Execute via: PK_GearCheck_InGame.ScanAllCharacters();

class PK_GearCheck_InGame
{
	static int s_ScannedCount = 0;
	static int s_FailureCount = 0;

	static void ScanAllCharacters()
	{
		Print("========== STARTING LIVE GEAR AUDIT ==========", LogLevel.NORMAL);
		
		s_ScannedCount = 0;
		s_FailureCount = 0;

		// Use QueryEntitiesBySphere to find all characters in the world safely
		GetGame().GetWorld().QueryEntitiesBySphere("0 0 0", 100000, FilterCharacter, null, EQueryEntitiesFlags.ALL);

		Print("========== LIVE GEAR AUDIT SUMMARY ==========", LogLevel.NORMAL);
		Print(string.Format("Scanned %1 characters. Found %2 units with gear overflow/missing items.", s_ScannedCount, s_FailureCount), LogLevel.NORMAL);
		Print("===============================================", LogLevel.NORMAL);
	}

	static bool FilterCharacter(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (character)
		{
			s_ScannedCount++;
			if (AuditCharacter(character))
				s_FailureCount++;
		}
		return true; // continue query
	}

	static bool AuditCharacter(SCR_ChimeraCharacter character)
	{
		ResourceName charPrefab = character.GetPrefabData().GetPrefabName();
		if (charPrefab == string.Empty) return false;

		// 1. Build INTENDED Bucket from Prefab Hierarchy
		map<string, int> intended = new map<string, int>();
		map<string, ResourceName> nameLookup = new map<string, ResourceName>();
		BuildIntendedRequirements(charPrefab, intended, nameLookup);

		// 2. Build ACTUAL Bucket from Live Entity
		map<string, int> actual = new map<string, int>();
		BuildActualInventory(character, actual);

		// 3. Comparison
		bool hasIssue = false;
		string charName = character.GetName();
		if (charName == string.Empty) charName = GetFileName(charPrefab);

		for (int i = 0; i < intended.Count(); i++)
		{
			string file = intended.GetKey(i);
			int exp = intended.GetElement(i);
			int found = 0;
			if (actual.Contains(file)) found = actual.Get(file);

			if (found < exp)
			{
				if (!hasIssue)
				{
					Print(string.Format("--- FAILURE: %1 ---", charName), LogLevel.NORMAL);
					hasIssue = true;
				}
				Print(string.Format("    [!] MISSING: %1x of %2x %3", exp - found, exp, GetFileName(nameLookup.Get(file))), LogLevel.ERROR);
			}
		}

		return hasIssue;
	}

	static void BuildIntendedRequirements(ResourceName resName, map<string, int> bucket, map<string, ResourceName> lookup)
	{
		Resource res = Resource.Load(resName);
		if (!res || !res.IsValid()) return;
		
		IEntitySource source = res.GetResource().ToEntitySource();
		while (source)
		{
			// A. Items in Inventory Manager
			// Manual component count search since IEntitySource.FindComponent is Workbench-only
			int compCount = source.GetComponentCount();
			for (int c = 0; c < compCount; c++)
			{
				IEntityComponentSource comp = source.GetComponent(c);
				if (comp && comp.GetClassName() == "SCR_InventoryStorageManagerComponent")
				{
					BaseContainerList list = comp.GetObjectArray("InitialInventoryItems");
					if (list)
					{
						for (int i = 0; i < list.Count(); i++)
						{
							array<ResourceName> prefabs;
							if (list.Get(i).Get("PrefabsToSpawn", prefabs) && prefabs)
							{
								foreach (ResourceName p : prefabs)
								{
									if (p == string.Empty) continue;
									string f = GetFileName(p);
									lookup.Insert(f, p);
									if (bucket.Contains(f)) bucket.Set(f, bucket.Get(f) + 1);
									else bucket.Insert(f, 1);
								}
							}
						}
					}
				}
				
				if (comp && comp.GetClassName() == "BaseLoadoutManagerComponent")
				{
					BaseContainerList slots = comp.GetObjectArray("Slots");
					if (slots)
					{
						for (int i = 0; i < slots.Count(); i++)
						{
							ResourceName prefab;
							slots.Get(i).Get("Prefab", prefab);
							if (prefab != string.Empty)
							{
								string f = GetFileName(prefab);
								lookup.Insert(f, prefab);
								if (bucket.Contains(f)) bucket.Set(f, bucket.Get(f) + 1);
								else bucket.Insert(f, 1);
								
								// Recursive check for clothing defaults
								BuildIntendedRequirements(prefab, bucket, lookup);
							}
						}
					}
				}
			}
			
			source = source.GetAncestor();
		}
	}

	static void BuildActualInventory(IEntity ent, map<string, int> bucket)
	{
		if (!ent) return;

		// 1. Primary: Use Inventory Manager
		SCR_InventoryStorageManagerComponent mgr = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
		if (mgr)
		{
			array<IEntity> items = new array<IEntity>();
			mgr.GetItems(items);
			foreach (IEntity item : items)
			{
				AddToBucket(item, bucket);
			}
		}

		// 2. Secondary: Hierarchy scan for gadgets
		ScanHierarchyRecursive(ent, bucket);
	}

	static void ScanHierarchyRecursive(IEntity ent, map<string, int> bucket)
	{
		IEntity child = ent.GetChildren();
		while (child)
		{
			// Check if this child is already in the bucket? 
			// In-game hierarchy includes items in pockets, so we only add if it's NOT an inventory item
			// to avoid double counting from the Manager call above.
			InventoryItemComponent itemComp = InventoryItemComponent.Cast(child.FindComponent(InventoryItemComponent));
			
			// If it's a prefab but NOT considered an inventory item (likely a gadget or attachment)
			if (!itemComp)
			{
				AddToBucket(child, bucket);
			}
			
			ScanHierarchyRecursive(child, bucket);
			child = child.GetSibling();
		}
	}

	static void AddToBucket(IEntity ent, map<string, int> bucket)
	{
		if (!ent) return;
		EntityPrefabData data = ent.GetPrefabData();
		if (data)
		{
			string f = GetFileName(data.GetPrefabName());
			if (f != string.Empty)
			{
				if (bucket.Contains(f)) bucket.Set(f, bucket.Get(f) + 1);
				else bucket.Insert(f, 1);
			}
		}
	}

	static string GetFileName(ResourceName res)
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