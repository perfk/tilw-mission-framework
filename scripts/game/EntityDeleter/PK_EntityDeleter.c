[EntityEditorProps(
    category: "GameScripted/",
    description: "Deletes entities in a certain radius. \n- Use the Prefab Filter Picker tool to build a filter list.\n- Right-click the filter list to copy/paste between entities.\n- Ctrl+Click prefabs in the picker tool to add/remove from the filter list."
)]
class PK_EntityDeleterClass : GenericEntityClass
{
}

// The oroginal SCR_PrefabDeleter does not work correctly. Test by placing it over 10+ entities and see how it does not correctly delete all.
// This behavior was confirmed by BI dev on Discord.

// Massive thanks to Blue and Cunnah for bringing this code to life.

//! Controls which entities touched by the sphere are treated as deletion targets.
enum PK_EEntityDeleterMode
{
	ANY_OVERLAP    = 0, //! Bounding box touches the sphere — default, backwards compatible
	CENTER_INSIDE  = 1, //! Bounding box centre must be inside the sphere
	MAJORITY_INSIDE = 2, //! At least 5 of 8 bounding box corners must be inside the sphere
	FULLY_INSIDE   = 3  //! All 8 bounding box corners must be inside the sphere
}

//------------------------------------------------------------------------------------------------
class PK_EntityDeleter : GenericEntity
{
	[Attribute("www.youtube.com/watch?v=D-Bb66hJnqE", UIWidgets.Object, 
	desc: "INSTRUCTIONS:\n\n- Use the Prefab Picker tool to build a prefab list.\n- Right-click the prefab list to copy/paste between entities.\n- Ctrl+Click prefabs in the picker tool to add/remove from the filter list.", 
	category: "Entity Deleter")]
	string m_HowToUsePrefabPickerTool;
	
	[Attribute(defvalue: "20", uiwidget: UIWidgets.Slider, desc: "Radius in which entities are deleted.\nEntities with the EntityProtectorComponent are ignored, so are entities with hierarchy parents that have it.", "0 1000 1", category: "Entity Deleter")]
	protected float m_fRadius;

	[Attribute("", UIWidgets.ResourceAssignArray, desc: "List of prefab resources used to filter which entities are deleted. If empty, all entities in the area are deleted", category: "Entity Deleter", params: "et")]
	protected ref array<ResourceName> m_prefabFilterList ;
	
	[Attribute("0", UIWidgets.Auto, desc: "If enabled, entities whose prefabs are NOT in the list will be deleted. If disabled, only entities whose prefabs are in the list will be deleted.", category: "Entity Deleter")]
	bool m_excludePrefabsInList;

	[Attribute("0", UIWidgets.Auto, desc: "Show red markers in the World Editor on every entity that will be deleted. Useful for previewing — turn off when not needed to reduce viewport noise.", category: "Entity Deleter")]
	bool m_wbShowMarkers;

	[Attribute("0", UIWidgets.ComboBox, desc: "Controls which entities inside the sphere are treated as targets.\n- Any Overlap: bounding box touches sphere (default, backwards compatible)\n- Center Inside: bounding box centre must be inside sphere\n- Majority Inside: at least 5 of 8 corners inside sphere\n- Fully Inside: all 8 corners inside sphere", category: "Entity Deleter", enums: ParamEnumArray.FromEnum(PK_EEntityDeleterMode))]
	protected PK_EEntityDeleterMode m_eSelectionMode;

	private ref array<IEntity> QueryEntitiesToRemove;

	//------------------------------------------------------------------------------------------------
	// Recursively adds all hierarchy children of e to list (avoids duplicates).
	private void CollectDescendants(IEntity e, array<IEntity> list)
	{
		IEntity child = e.GetChildren();
		while (child)
		{
			if (list.Find(child) == -1)
				list.Insert(child);
			CollectDescendants(child, list);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	// Returns true if the entity passes the configured selection-mode filter.
	// ANY_OVERLAP always returns true (QueryEntitiesBySphere already handled the broad overlap test).
	// Other modes check the world-space AABB against the sphere.
	private bool PassesModeFilter(IEntity e, vector sphereOrigin)
	{
		if (m_eSelectionMode == PK_EEntityDeleterMode.ANY_OVERLAP)
			return true;

		vector mins, maxs;
		e.GetWorldBounds(mins, maxs);

		if (m_eSelectionMode == PK_EEntityDeleterMode.CENTER_INSIDE)
		{
			vector center = (mins + maxs) * 0.5;
			return vector.Distance(center, sphereOrigin) <= m_fRadius;
		}

		// MAJORITY_INSIDE or FULLY_INSIDE: test all 8 AABB corners
		float rSq = m_fRadius * m_fRadius;
		int inside = 0;
		vector corners[8];
		corners[0] = Vector(mins[0], mins[1], mins[2]);
		corners[1] = Vector(maxs[0], mins[1], mins[2]);
		corners[2] = Vector(mins[0], maxs[1], mins[2]);
		corners[3] = Vector(maxs[0], maxs[1], mins[2]);
		corners[4] = Vector(mins[0], mins[1], maxs[2]);
		corners[5] = Vector(maxs[0], mins[1], maxs[2]);
		corners[6] = Vector(mins[0], maxs[1], maxs[2]);
		corners[7] = Vector(maxs[0], maxs[1], maxs[2]);
		for (int ic = 0; ic < 8; ic++)
		{
			float dx = corners[ic][0] - sphereOrigin[0];
			float dy = corners[ic][1] - sphereOrigin[1];
			float dz = corners[ic][2] - sphereOrigin[2];
			if (dx * dx + dy * dy + dz * dz <= rSq)
				inside++;
		}

		if (m_eSelectionMode == PK_EEntityDeleterMode.MAJORITY_INSIDE)
			return inside >= 5;

		return inside == 8; // FULLY_INSIDE
	}

	//------------------------------------------------------------------------------------------------
	// Find all entities inside QueryEntitiesBySphere and add them to array. Discards any entries where name or parrents name is in list.
	// If only parrent have the specific name, all child entities must have the Hierarchy component.
	// Seems easier to add Hierarchy component, than to rename all entities.

	private bool QueryEntities(IEntity e)
	{

		// Add PK_PrefabDeleterComponent to all entities that should not be deleted.
		// Adding by Prefab details and by Prefab Name both had issues due to Hierarchy not always being added to prefab children. So if you are changing the name, you might as well add a new component. Hopefully an easier way to add components to multiple entities will appear
		if (!e.FindComponent(PK_EntityProtectorComponent) && !GetParent() && PassesModeFilter(e, GetOrigin()))
			QueryEntitiesToRemove.Insert(e);
		return true;

	}


	//------------------------------------------------------------------------------------------------
	override void EOnActivate(IEntity owner)
	{
		QueryEntitiesToRemove = {};
		// server only
		RplComponent rplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (rplComponent && !rplComponent.IsMaster())
			return;

		BaseWorld world = GetWorld();

		world.QueryEntitiesBySphere(owner.GetOrigin(), m_fRadius, QueryEntities);

		if (m_prefabFilterList.Count() > 0)
		{
			array<IEntity> filteredEntities = {};
			foreach (IEntity e : QueryEntitiesToRemove)
			{
				EntityPrefabData pd = e.GetPrefabData();
				if (!pd)
					continue;

				// Use GetPrefabName() — returns the full ResourceName including GUID.
				// The old code used GetPrefab().GetName() which returns a class name, not a file path.
				ResourceName prefabName = pd.GetPrefabName();
				bool found = false;
				foreach (ResourceName allowedPrefab : m_prefabFilterList)
				{
					if (prefabName == allowedPrefab)
					{
						found = true;
						break;
					}
				}

				if (found && !m_excludePrefabsInList)
					filteredEntities.Insert(e);
				else if (!found && m_excludePrefabsInList)
					filteredEntities.Insert(e);
			}
			QueryEntitiesToRemove = filteredEntities;
		}

		// For each entity to delete, also explicitly collect all hierarchy children.
		// Compositions whose hierarchy IS preserved at runtime won't have children deleted
		// by DeleteEntityAndChildren alone, because the helper only works on script-tracked children.
		int preExpandCount = QueryEntitiesToRemove.Count();
		for (int i = 0; i < preExpandCount; i++)
			CollectDescendants(QueryEntitiesToRemove[i], QueryEntitiesToRemove);

		// Removes all entities. IsDeleted() guard handles cases where a parent deletion
		// already removed a child that is also in the list.
		foreach (IEntity e : QueryEntitiesToRemove)
		{
			if (!e.IsDeleted())
				SCR_EntityHelper.DeleteEntityAndChildren(e);
		}

		// destroy self
		delete owner;
	}


	//------------------------------------------------------------------------------------------------
	// constructor
	//! \param[in] src
	//! \param[in] parent
	void PK_EntityDeleter(IEntitySource src, IEntity parent)
	{
		if (!GetGame().InPlayMode())
			return;

		SetEventMask(EntityEvent.INIT);
	}
	

	

#ifdef WORKBENCH

	// Accessors for WB plugins — protected fields can't be read from outside the class
	float WB_GetRadius() { return m_fRadius; }
	bool WB_GetExcludePrefabs() { return m_excludePrefabsInList; }
	ref array<ResourceName> WB_GetPrefabFilter() { return m_prefabFilterList; }
	bool WB_PassesModeFilter(IEntity e, vector sphereOrigin) { return PassesModeFilter(e, sphereOrigin); }

	// True if e's world AABB touches the deletion sphere (ANY_OVERLAP semantics).
	// Uses exact AABB-sphere distance so entities partially inside are caught even if GetOrigin() is outside.
	bool WB_OverlapsSphere(IEntity e)
	{
		vector center = GetOrigin();
		vector mins, maxs;
		e.GetWorldBounds(mins, maxs);

		float sqDist = 0;
		float d;

		d = 0; if (center[0] < mins[0]) d = mins[0] - center[0]; else if (center[0] > maxs[0]) d = center[0] - maxs[0]; sqDist += d * d;
		d = 0; if (center[1] < mins[1]) d = mins[1] - center[1]; else if (center[1] > maxs[1]) d = center[1] - maxs[1]; sqDist += d * d;
		d = 0; if (center[2] < mins[2]) d = mins[2] - center[2]; else if (center[2] > maxs[2]) d = center[2] - maxs[2]; sqDist += d * d;

		return sqDist <= m_fRadius * m_fRadius;
	}

	// Full spatial check for any mode — use this instead of WB_PassesModeFilter when the sphere query hasn't pre-filtered.
	bool WB_InDeletionZone(IEntity e)
	{
		if (m_eSelectionMode == PK_EEntityDeleterMode.ANY_OVERLAP)
			return WB_OverlapsSphere(e);
		return PassesModeFilter(e, GetOrigin());
	}

	// Cached positions of entities that will be deleted, rebuilt periodically
	private ref array<vector> m_wbMarkedPositions = {};

	// Temp buffer used inside the query callback before prefab filter is applied
	private ref array<IEntity> m_wbTempEntities = {};

	// Throttle: re-query every 30 frames, or whenever origin/radius changes
	private int m_wbFrameCounter = 30;
	private vector m_wbLastPos;
	private float m_wbLastRadius = -1;

	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		vector origin = GetOrigin();

		// Draw the deletion zone sphere (existing behaviour)
		Shape.CreateSphere(COLOR_YELLOW, ShapeFlags.WIREFRAME | ShapeFlags.ONCE, origin, m_fRadius);

		if (m_wbShowMarkers)
		{
			// Re-query when the entity moves, radius changes, or every 30 frames
			m_wbFrameCounter++;
			if (m_wbFrameCounter >= 30 || vector.Distance(origin, m_wbLastPos) > 0.01 || m_fRadius != m_wbLastRadius)
			{
				m_wbFrameCounter = 0;
				m_wbLastPos = origin;
				m_wbLastRadius = m_fRadius;
				WB_RefreshPreview(origin);
			}

			// Draw a red wireframe sphere at each entity that will be deleted
			ShapeFlags markerFlags = ShapeFlags.WIREFRAME | ShapeFlags.ONCE | ShapeFlags.TRANSP | ShapeFlags.NOZWRITE | ShapeFlags.NOCULL;
			int deleteColor = ARGB(220, 255, 50, 50);
			foreach (vector pos : m_wbMarkedPositions)
			{
				Shape.CreateSphere(deleteColor, markerFlags, pos, 0.5);
			}
		}

		super._WB_AfterWorldUpdate(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	// Rebuilds m_wbMarkedPositions by querying the world and applying the same filters as runtime.
	private void WB_RefreshPreview(vector origin)
	{
		m_wbTempEntities.Clear();
		m_wbMarkedPositions.Clear();

		GetWorld().QueryEntitiesBySphere(origin, m_fRadius, WB_CollectEntity);

		// Apply prefab filter (mirrors EOnActivate logic)
		if (m_prefabFilterList.Count() > 0)
		{
			foreach (IEntity e : m_wbTempEntities)
			{
				EntityPrefabData pd = e.GetPrefabData();
				if (!pd)
					continue;

				ResourceName entityPrefab = pd.GetPrefabName();
				bool found = false;
				foreach (ResourceName filterPrefab : m_prefabFilterList)
				{
					if (entityPrefab == filterPrefab)
					{
						found = true;
						break;
					}
				}

				if (found && !m_excludePrefabsInList)
					m_wbMarkedPositions.Insert(e.GetOrigin());
				else if (!found && m_excludePrefabsInList)
					m_wbMarkedPositions.Insert(e.GetOrigin());
			}
		}
		else
		{
			foreach (IEntity e : m_wbTempEntities)
				m_wbMarkedPositions.Insert(e.GetOrigin());
		}
	}

	//------------------------------------------------------------------------------------------------
	// Query callback: collects entities that pass the basic filter (mirrors QueryEntities runtime logic).
	private bool WB_CollectEntity(IEntity e)
	{
		if (e == this)
			return true;
		if (e.FindComponent(PK_EntityProtectorComponent))
			return true;
		if (GetParent()) // mirrors runtime: only collect if the deleter itself has no parent
			return true;
		if (!PassesModeFilter(e, GetOrigin()))
			return true;

		m_wbTempEntities.Insert(e);
		return true;
	}

#endif

}