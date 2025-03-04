[EntityEditorProps(category: "game/", description: "Opens nearby doors.")]
class TILW_DoorOpenerComponentClass: ScriptComponentClass
{
}

class TILW_DoorOpenerComponent: ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.Auto, desc: "What the control value of nearby doors should be.\n0=Closed, 1=Open", params: "0 1 0.01")]
	protected float m_doorControl;
	
	[Attribute(defvalue: "2", uiwidget: UIWidgets.Auto, desc: "Radius in which entities with DoorComponents are affected.", params: "0 100 0.1")]
	protected float m_radius;
	
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		GetGame().GetCallqueue().Call(PerformDoorQuery, owner);
	}
	
	protected void PerformDoorQuery(IEntity owner)
	{
		GetGame().GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), m_radius, ProcessEntity);
	}
	
	protected bool ProcessEntity(IEntity e)
	{
		if (!e)
			return true;
		
		array<Managed> comps = {};
		e.FindComponents(DoorComponent, comps);
		if (comps.IsEmpty())
			return true;
		
		foreach (Managed c : comps)
		{
			DoorComponent dc = DoorComponent.Cast(c);
			dc.SetControlValue(m_doorControl);
		}
		
		return true;
	}
}