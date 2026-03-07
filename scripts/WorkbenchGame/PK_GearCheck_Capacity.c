[WorkbenchPluginAttribute(name: "Gear Audit: Capacity Analysis", description: "Analyzes gear overflow based on weight and volume capacity.", shortcut: "Ctrl+Shift+K", wbModules: {"WorldEditor"}, category: "PK_Gear Checks")]
class PK_GearCheck_Capacity : WorldEditorPlugin
{
	ref map<ResourceName, float> m_WeightCache = new map<ResourceName, float>();
	ref map<ResourceName, float> m_VolumeCache = new map<ResourceName, float>();
	WorldEditorAPI m_Api;

	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor) { Print("WorldEditor not available", LogLevel.ERROR); return; }
		m_Api = worldEditor.GetApi();
		if (!m_Api) { Print("WorldEditorAPI not available", LogLevel.ERROR); return; }

		int selectedCount = m_Api.GetSelectedEntitiesCount();
		if (selectedCount == 0)
		{
			Print("Please select one or more character entities.", LogLevel.WARNING);
			return;
		}

		Print(string.Format("========== CAPACITY-BASED GEAR ANALYSIS (%1 entities) ==========", selectedCount), LogLevel.NORMAL);

		for (int i = 0; i < selectedCount; i++)
		{
			IEntitySource source = m_Api.GetSelectedEntity(i);
			if (!source) continue;
			
			string charName = source.GetName();
			if (charName == string.Empty) charName = source.GetResourceName();
			Print(string.Format("--- Character: %1 ---", charName), LogLevel.NORMAL);

			// Map: Container -> (Item -> [Count|Purpose])
			map<ResourceName, ref map<ResourceName, string>> intendedMap = new map<ResourceName, ref map<ResourceName, string>>();
			ScanIntendedLoadout(source, intendedMap);

			IEntity charEntity = m_Api.SourceToEntity(source);
			if (!charEntity)
			{
				Print("  [ERROR] Could not access character entity instance.", LogLevel.ERROR);
				continue;
			}

			if (intendedMap.IsEmpty())
			{
				Print("  [INFO] No intended inventory items found.", LogLevel.NORMAL);
				continue;
			}

			for (int c = 0; c < intendedMap.Count(); c++)
			{
				ResourceName intendedContainerRes = intendedMap.GetKey(c);
				map<ResourceName, string> itemsData = intendedMap.GetElement(c);

				BaseInventoryStorageComponent storage = FindStorageOnEntity(charEntity, intendedContainerRes);
				
				string title = intendedContainerRes;
				if (title == "Character/General") title = "Direct Character Inventory (Root)";
				
				if (!storage)
				{
					Print(string.Format("  [CONTAINER] %1 (NOT FOUND ON CHARACTER)", title), LogLevel.WARNING);
					continue;
				}

				float maxWeight = 0;
				float maxVolume = 0;
				
				SCR_UniversalInventoryStorageComponent uniStorage = SCR_UniversalInventoryStorageComponent.Cast(storage);
				if (uniStorage)
					maxWeight = uniStorage.GetMaxLoad();
				
				maxVolume = storage.GetMaxVolumeCapacity() / 1000.0;

				Print(string.Format("  [CONTAINER] %1 (Max: %2kg / %3L)", title, maxWeight.ToString(1, 2), maxVolume.ToString(1, 2)), LogLevel.NORMAL);

				float totalWeight = 0;
				float totalVolume = 0;

				for (int j = 0; j < itemsData.Count(); j++)
				{
					ResourceName itemRes = itemsData.GetKey(j);
					string dataStr = itemsData.GetElement(j);
					
					array<string> parts = new array<string>();
					dataStr.Split("|", parts, true);
					int count = parts[0].ToInt();
					string purpose = parts[1];

					float w, v;
					GetItemPropertiesFromPrefab(itemRes, w, v);
					
					totalWeight += w * count;
					totalVolume += v * count;
					
					string purposeLog = "";
					if (purpose != "0") purposeLog = string.Format(" [Purpose: %1]", purpose);
					
					Print(string.Format("    + %1x %2 (Weight: %3kg, Vol: %4L)%5", count, GetFileName(itemRes), w.ToString(1, 2), v.ToString(1, 2), purposeLog), LogLevel.NORMAL);
				}

				string weightStatus = "[OK]";
				LogLevel weightLevel = LogLevel.NORMAL;
				if (maxWeight > 0 && totalWeight > maxWeight) { weightStatus = "[!] WEIGHT OVERFLOW"; weightLevel = LogLevel.ERROR; }
				
				string volumeStatus = "[OK]";
				LogLevel volumeLevel = LogLevel.NORMAL;
				if (maxVolume > 0 && totalVolume > maxVolume) { volumeStatus = "[!] VOLUME OVERFLOW"; volumeLevel = LogLevel.ERROR; }

				Print(string.Format("    > Result Weight: %1kg / %2kg %3", totalWeight.ToString(1, 2), maxWeight.ToString(1, 2), weightStatus), weightLevel);
				Print(string.Format("    > Result Volume: %1L / %2L %3", totalVolume.ToString(1, 2), maxVolume.ToString(1, 2), volumeStatus), volumeLevel);
			}
		}

		m_WeightCache.Clear();
		m_VolumeCache.Clear();
		Print("==================================================================", LogLevel.NORMAL);
	}

	BaseInventoryStorageComponent FindStorageOnEntity(IEntity entity, ResourceName targetPrefab)
	{
		if (targetPrefab == "Character/General")
			return BaseInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));

		array<Managed> components = new array<Managed>();
		entity.FindComponents(BaseInventoryStorageComponent, components);
		
		string nTarget = GetFileName(targetPrefab);

		foreach (Managed m : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(m);
			if (!storage) continue;
			
			ResourceName actualPrefab = storage.GetOwner().GetPrefabData().GetPrefabName();
			if (GetFileName(actualPrefab) == nTarget || SCR_BaseContainerTools.IsKindOf(actualPrefab, targetPrefab)) 
				return storage;
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			BaseInventoryStorageComponent s = FindStorageOnEntity(child, targetPrefab);
			if (s) return s;
			child = child.GetSibling();
		}

		return null;
	}

	void GetItemPropertiesFromPrefab(ResourceName res, out float weight, out float volume)
	{
		if (m_WeightCache.Contains(res))
		{
			weight = m_WeightCache.Get(res);
			volume = m_VolumeCache.Get(res);
			return;
		}

		weight = 0;
		volume = 0;

		Resource resource = Resource.Load(res);
		if (resource && resource.IsValid())
		{
			IEntitySource entSrc = resource.GetResource().ToEntitySource();
			if (entSrc)
			{
				IEntityComponentSource compSource = SCR_BaseContainerTools.FindComponentSource(entSrc, "InventoryItemComponent");
				if (!compSource) compSource = SCR_BaseContainerTools.FindComponentSource(entSrc, "SCR_InventoryItemComponent");
				
				if (compSource)
				{
					BaseContainer attrCont = compSource.GetObject("Attributes");
					if (attrCont)
					{
						BaseContainer physCont = attrCont.GetObject("ItemPhysAttributes");
						if (!physCont) physCont = attrCont.GetObject("ItemPhysicalAttributes");
						
						if (physCont)
						{
							physCont.Get("Weight", weight);
							float rawVol = 0;
							if (physCont.Get("ItemVolume", rawVol))
								volume = rawVol / 1000.0;
						}
					}
				}
			}
		}

		m_WeightCache.Insert(res, weight);
		m_VolumeCache.Insert(res, volume);
	}

	void ScanIntendedLoadout(IEntitySource source, map<ResourceName, ref map<ResourceName, string>> containerMap)
	{
		IEntityComponentSource mgr = SCR_BaseContainerTools.FindComponentSource(source, SCR_InventoryStorageManagerComponent);
		if (mgr)
		{
			BaseContainerList list = mgr.GetObjectArray("InitialInventoryItems");
			if (list)
			{
				for (int i = 0; i < list.Count(); i++)
				{
					BaseContainer entry = list.Get(i);
					ResourceName target;
					entry.Get("TargetStorage", target);
					if (target == string.Empty) target = "Character/General";

					int purpose = 0;
					entry.Get("TargetPurpose", purpose);

					array<ResourceName> prefabs;
					if (entry.Get("PrefabsToSpawn", prefabs) && prefabs)
					{
						foreach (ResourceName item : prefabs)
						{
							AddItemToMap(containerMap, target, item, purpose);
						}
					}
				}
			}
		}

		IEntityComponentSource loadoutMgr = SCR_BaseContainerTools.FindComponentSource(source, BaseLoadoutManagerComponent);
		if (loadoutMgr)
		{
			BaseContainerList slots = loadoutMgr.GetObjectArray("Slots");
			if (slots)
			{
				for (int i = 0; i < slots.Count(); i++)
				{
					BaseContainer slot = slots.Get(i);
					ResourceName clothingPrefab;
					slot.Get("Prefab", clothingPrefab);
					if (clothingPrefab != string.Empty)
						ScanPrefabManager(clothingPrefab, containerMap);
				}
			}
		}
	}

	void ScanPrefabManager(ResourceName prefab, map<ResourceName, ref map<ResourceName, string>> containerMap)
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
					BaseContainer entry = list.Get(i);
					int purpose = 0;
					entry.Get("TargetPurpose", purpose);

					array<ResourceName> items;
					if (entry.Get("PrefabsToSpawn", items) && items)
					{
						foreach (ResourceName item : items)
						{
							AddItemToMap(containerMap, prefab, item, purpose);
						}
					}
				}
			}
		}
	}

	void AddItemToMap(map<ResourceName, ref map<ResourceName, string>> m, ResourceName container, ResourceName item, int purpose)
	{
		if (item == string.Empty) return;
		
		if (!m.Contains(container))
		{
			map<ResourceName, string> itemMap = new map<ResourceName, string>();
			m.Insert(container, itemMap);
		}
		
		map<ResourceName, string> items = m.Get(container);
		if (items.Contains(item))
		{
			string existing = items.Get(item);
			array<string> parts = new array<string>();
			existing.Split("|", parts, true);
			int count = parts[0].ToInt() + 1;
			items.Set(item, string.Format("%1|%2", count, purpose));
		}
		else
		{
			items.Insert(item, string.Format("1|%1", purpose));
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