#ifdef WORKBENCH

//------------------------------------------------------------------------------------------------
// Reduces the current World Editor selection to only entities that any PK_EntityDeleter
// in the scene would delete at game start — including their prefab filter settings.
//
// Workflow:
//   1. Box-select terrain entities in the area (non-editable only — do NOT include the
//      PK_EntityDeleter, as mixing editable + non-editable in one selection causes an error)
//   2. Run this plugin — it finds all PK_EntityDeleters in the scene automatically
//   3. Press H to hide the remaining selection
[WorkbenchPluginAttribute(
	name: "Remove entities not in EntityDeleters list",
	description: "Keeps only PK_EntityDeleter targets in the current selection.\n\nNOTE: Due to a World Editor bug, manual box-selections may miss some entities (especially on locked layers). If targets remain visible, try a larger selection or use the 'Delete Targets in Editor' plugin temporarily.\n\n1. Box-select terrain entities in the area.\n2. Run this plugin.\n3. Press H to hide.",
	wbModules: { "WorldEditor" },
	awesomeFontCode: 0xF0B0,
	category: "PK_EntityDeleter"
)]
class PK_FilterSelectionPlugin : WorldEditorPlugin
{
	// All entities confirmed to be deleted by any PK_EntityDeleter in the scene
	private ref array<IEntity> m_deleteTargets = {};

	// All PK_EntityDeleter live entities — used in the callback to skip them
	private ref array<IEntity> m_deleterEntities = {};

	// Per-query filter state, set before each QueryEntitiesBySphere call
	private ref array<ResourceName> m_currentFilter = {};
	private bool m_currentExclude = false;
	private PK_EntityDeleter m_currentDeleter;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
			return;

		if (api.GetSelectedEntitiesCount() == 0)
		{
			Print("[EntityDeleter] Nothing selected. Box-select terrain entities in the area first.", LogLevel.WARNING);
			return;
		}

		// --- Step 1: find all PK_EntityDeleters in the scene ---

		m_deleterEntities.Clear();
		m_deleteTargets.Clear();

		int editorEntityCount = api.GetEditorEntityCount();
		for (int i = 0; i < editorEntityCount; i++)
		{
			IEntitySource src = api.GetEditorEntity(i);
			if (!src || src.GetClassName() != "PK_EntityDeleter")
				continue;

			IEntity delEnt = api.SourceToEntity(src);
			if (!delEnt)
				continue;

			PK_EntityDeleter deleter = PK_EntityDeleter.Cast(delEnt);
			if (!deleter)
				continue;

			m_deleterEntities.Insert(delEnt);

			// Load this deleter's filter settings into the shared callback fields
			m_currentFilter = deleter.WB_GetPrefabFilter();
			m_currentExclude = deleter.WB_GetExcludePrefabs();
			m_currentDeleter = deleter;

			api.GetWorld().QueryEntitiesBySphere(delEnt.GetOrigin(), deleter.WB_GetRadius(), CollectTarget);
		}

		if (m_deleterEntities.IsEmpty())
		{
			Print("[EntityDeleter] No PK_EntityDeleter entities found in the scene.", LogLevel.WARNING);
			return;
		}

		// Also collect hierarchy children of matched entities — mirrors the runtime CollectDescendants pass.
		int targetCount = m_deleteTargets.Count();
		for (int i = 0; i < targetCount; i++)
			CollectDescendants(m_deleteTargets[i]);

		// --- Step 2: Unlock layers for all targets ---
		// Helps the editor correctly identify the entities for selection filtering.
		ref array<int> unlockedKeys = {};
		foreach (IEntity e : m_deleteTargets)
		{
			IEntitySource src = api.EntityToSource(e);
			if (!src) continue;
			int subScene = src.GetSubScene();
			int layerId = src.GetLayerID();
			if (!api.IsEntityLayerLockedHierarchy(subScene, layerId)) continue;
			int key = subScene * 100000 + layerId;
			if (unlockedKeys.Find(key) != -1) continue;
			api.UnlockEntityLayer(subScene, layerId);
			unlockedKeys.Insert(key);
		}

		// --- Step 3: snapshot the selection, then remove non-targets ---

		int selCount = api.GetSelectedEntitiesCount();
		ref array<IEntitySource> snapshot = {};
		for (int i = 0; i < selCount; i++)
		{
			IEntitySource src = api.GetSelectedEntity(i);
			if (src)
				snapshot.Insert(src);
		}

		int removed = 0;
		foreach (IEntitySource src : snapshot)
		{
			IEntity ent = api.SourceToEntity(src);
			
			// If it's not in our target list, it shouldn't be selected
			bool isTarget = false;
			foreach (IEntity target : m_deleteTargets)
			{
				if (ent == target) { isTarget = true; break; }
			}
			
			if (!isTarget)
			{
				api.RemoveFromEntitySelection(src);
				removed++;
			}
		}

		int remaining = api.GetSelectedEntitiesCount();
		Print(string.Format("[EntityDeleter] Found %1 PK_EntityDeleter(s), %2 total targets in world.", m_deleterEntities.Count(), m_deleteTargets.Count()), LogLevel.NORMAL);
		Print(string.Format("[EntityDeleter] Filtered current selection: Removed %1 entities. %2 deletion targets remain — press H to hide.", removed, remaining), LogLevel.NORMAL);

		if (remaining == 0)
			Print("[EntityDeleter] No deletion targets were in the selection. Try box-selecting a larger area.", LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	// Recursively adds hierarchy children of e to m_deleteTargets (skips deleters, avoids duplicates).
	private void CollectDescendants(IEntity e)
	{
		IEntity child = e.GetChildren();
		while (child)
		{
			if (m_deleterEntities.Find(child) == -1 && m_deleteTargets.Find(child) == -1)
				m_deleteTargets.Insert(child);
			CollectDescendants(child);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	private bool CollectTarget(IEntity e)
	{
		// Skip the PK_EntityDeleter entities themselves
		if (m_deleterEntities.Find(e) != -1)
			return true;

		// Skip protected entities
		if (e.FindComponent(PK_EntityProtectorComponent))
			return true;

		// Apply prefab filter (mirrors PK_EntityDeleter runtime logic)
		if (m_currentFilter && m_currentFilter.Count() > 0)
		{
			EntityPrefabData pd = e.GetPrefabData();
			if (!pd)
				return true; 

			ResourceName entityPrefab = pd.GetPrefabName();
			bool found = false;
			foreach (ResourceName filterPrefab : m_currentFilter)
			{
				if (entityPrefab == filterPrefab)
				{
					found = true;
					break;
				}
			}

			// Only collect if this entity would actually be deleted
			bool wouldDelete = (found && !m_currentExclude) || (!found && m_currentExclude);
			if (!wouldDelete)
				return true;
		}

		// Apply selection mode filter
		if (m_currentDeleter && !m_currentDeleter.WB_PassesModeFilter(e, m_currentDeleter.GetOrigin()))
			return true;

		// Avoid duplicates (entity inside multiple deleters' spheres)
		bool alreadyAdded = false;
		foreach (IEntity target : m_deleteTargets) { if (target == e) { alreadyAdded = true; break; } }
		if (!alreadyAdded) m_deleteTargets.Insert(e);

		return true;
	}
}

#endif
