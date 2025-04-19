[EntityEditorProps(category: "GameScripted/", description: "Deletes entities in certain radius.")]
class PK_EntityDeleterClass : GenericEntityClass
{
}

// The oroginal SCR_PrefabDeleter does not work correctly. Test by placing it over 10+ entities and see how it does not correctly delete all.
// This behavior was confirmed by BI dev on Discord.

// Massive thanks to Blue and Cunnah for bringing this code to life.

//------------------------------------------------------------------------------------------------
class PK_EntityDeleter : GenericEntity
{
	[Attribute(defvalue: "20", uiwidget: UIWidgets.Slider, desc: "Radius in which entities are deleted.\nEntities with the EntityProtectorComponent are ignored, so are entities with hierarchy parents that have it.", "0 1000 1")]
	protected float m_fRadius;

	private ref array<IEntity> QueryEntitiesToRemove;

	//------------------------------------------------------------------------------------------------
	// Find all entities inside QueryEntitiesBySphere and add them to array. Discards any entries where name or parrents name is in list.
	// If only parrent have the specific name, all child entities must have the Hierarchy component.
	// Seems easier to add Hierarchy component, than to rename all entities.

	private bool QueryEntities(IEntity e)
	{

		// Add PK_PrefabDeleterComponent to all entities that should not be deleted.
		// Adding by Prefab details and by Prefab Name both had issues due to Hierarchy not always being added to prefab children. So if you are changing the name, you might as well add a new component. Hopefully an easier way to add components to multiple entities will appear
		if (!e.FindComponent(PK_EntityProtectorComponent) && !GetParent())
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

		// Removes all entities. This is a hack, until the original SCR_PrefabDeleter works correctly
		foreach (IEntity e : QueryEntitiesToRemove)
		{
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

	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(float timeSlice)
	{

		auto origin = GetOrigin();
		auto radiusShape = Shape.CreateSphere(COLOR_YELLOW, ShapeFlags.WIREFRAME | ShapeFlags.ONCE, origin, m_fRadius);

		super._WB_AfterWorldUpdate(timeSlice);

	}

#endif

}
