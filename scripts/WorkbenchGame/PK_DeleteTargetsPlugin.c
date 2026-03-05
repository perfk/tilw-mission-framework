#ifdef WORKBENCH

//------------------------------------------------------------------------------------------------
// Deletes all entities inside the selected PK_EntityDeleter's sphere from the World Editor,
// respecting its prefab filter and exclude settings.
// - Locked terrain layers are unlocked automatically before deletion.
// - The entire operation is one undo step: Ctrl+Z restores everything.
// - Since the base terrain layer cannot be saved, reloading the mission also fully restores it.
[WorkbenchPluginAttribute(
	name: "Delete Targets in Editor",
	description: "Deletes all entities the selected PK_EntityDeleter would remove at runtime.\nPress Ctrl+Z to undo, or reload the mission to restore.",
	wbModules: { "WorldEditor" },
	awesomeFontCode: 0xF1F8,
	category: "PK_EntityDeleter"
)]
class PK_DeleteTargetsPlugin : WorldEditorPlugin
{
	// Filled during the query callback, read when deleting
	private ref array<IEntity> m_collected = {};

	// Used inside the callback to skip the deleter entity itself
	private IEntity m_deleterEntity;

	// Per-query filter state, set before QueryEntitiesBySphere
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
			Print("[EntityDeleter] No entity selected — select a PK_EntityDeleter first.", LogLevel.WARNING);
			return;
		}

		IEntitySource selected = api.GetSelectedEntity(0);
		if (!selected || selected.GetClassName() != "PK_EntityDeleter")
		{
			Print("[EntityDeleter] Selected entity is not a PK_EntityDeleter (got: " + selected.GetClassName() + ")", LogLevel.WARNING);
			return;
		}

		m_deleterEntity = api.SourceToEntity(selected);
		if (!m_deleterEntity)
		{
			Print("[EntityDeleter] Could not resolve live entity.", LogLevel.WARNING);
			return;
		}

		PK_EntityDeleter deleter = PK_EntityDeleter.Cast(m_deleterEntity);
		if (!deleter)
		{
			Print("[EntityDeleter] Cast to PK_EntityDeleter failed.", LogLevel.WARNING);
			return;
		}

		m_currentFilter = deleter.WB_GetPrefabFilter();
		m_currentExclude = deleter.WB_GetExcludePrefabs();
		m_currentDeleter = deleter;

		m_collected.Clear();
		api.GetWorld().QueryEntitiesBySphere(m_deleterEntity.GetOrigin(), deleter.WB_GetRadius(), CollectEntity);

		// Also collect hierarchy children of matched entities — mirrors the runtime CollectDescendants pass.
		// QueryEntitiesBySphere only finds the root of a composition instance; children must be traversed explicitly.
		int sphereCount = m_collected.Count();
		for (int i = 0; i < sphereCount; i++)
			CollectDescendants(m_collected[i]);

		if (m_collected.IsEmpty())
		{
			Print("[EntityDeleter] No entities found in sphere.", LogLevel.NORMAL);
			return;
		}

		Print(string.Format("[EntityDeleter] Attempting to delete %1 entities...", m_collected.Count()), LogLevel.NORMAL);

		// Unlock any locked layers first, tracking unique layer keys to avoid redundant calls
		ref array<int> unlockedKeys = {};
		foreach (IEntity e : m_collected)
		{
			IEntitySource src = api.EntityToSource(e);
			if (!src)
				continue;

			int subScene = src.GetSubScene();
			int layerId = src.GetLayerID();

			if (!api.IsEntityLayerLockedHierarchy(subScene, layerId))
				continue;

			int key = subScene * 100000 + layerId;
			if (unlockedKeys.Find(key) != -1)
				continue;

			api.UnlockEntityLayer(subScene, layerId);
			unlockedKeys.Insert(key);
			Print(string.Format("[EntityDeleter] Unlocked layer %1 in sub-scene %2", layerId, subScene), LogLevel.NORMAL);
		}

		// Delete all entities in a single undoable action
		api.BeginEntityAction("PK_EntityDeleter: Preview Deletions");

		int deleted = 0;
		int failed = 0;
		foreach (IEntity e : m_collected)
		{
			IEntitySource src = api.EntityToSource(e);
			if (!src)
			{
				failed++;
				continue;
			}

			if (api.DeleteEntity(src))
				deleted++;
			else
				failed++;
		}

		api.EndEntityAction();

		Print(string.Format("[EntityDeleter] Done. Deleted: %1  Failed: %2  | Ctrl+Z to undo", deleted, failed), LogLevel.NORMAL);
		if (failed > 0)
			Print("[EntityDeleter] Some entities could not be deleted. They may be on a locked parent layer.", LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	// Recursively adds hierarchy children of e to m_collected (skips the deleter itself, avoids duplicates).
	private void CollectDescendants(IEntity e)
	{
		IEntity child = e.GetChildren();
		while (child)
		{
			if (child != m_deleterEntity && m_collected.Find(child) == -1)
				m_collected.Insert(child);
			CollectDescendants(child);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	private bool CollectEntity(IEntity e)
	{
		if (e == m_deleterEntity)
			return true;

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

			bool wouldDelete = (found && !m_currentExclude) || (!found && m_currentExclude);
			if (!wouldDelete)
				return true;
		}

		if (m_currentDeleter && !m_currentDeleter.WB_PassesModeFilter(e, m_deleterEntity.GetOrigin()))
			return true;

		m_collected.Insert(e);
		return true;
	}
}

#endif
