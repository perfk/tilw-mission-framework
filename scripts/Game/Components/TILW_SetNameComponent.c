[EntityEditorProps(category: "game/", description: "Allows you to set the entities name.")]
class TILW_SetNameComponentClass: ScriptComponentClass
{
}

class TILW_SetNameComponent: ScriptComponent
{
	[Attribute("", UIWidgets.Auto, "Set the name of the entity")]
	protected string m_setName;
	
	void TILW_SetNameComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (Replication.IsServer() && m_setName != "" && GetGame().InPlayMode())
			owner.SetName(m_setName);
	}
}