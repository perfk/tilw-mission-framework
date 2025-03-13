//! TILW_BaseTerm provides inverting functionality. Do not use this directly.
[BaseContainerProps(configRoot: true), BaseContainerCustomStringTitleField("Crew Config")]
class TILW_CrewConfig
{
	[Attribute("", UIWidgets.Object, desc: "AI groups spawned in the vehicle. You could e. g. separate the gunner from the rest, or the crew and multiple squads.")]
	protected ref array<ref TILW_CrewGroup> m_crewGroups;
	
	[Attribute("0", UIWidgets.Auto, desc: "If entity is a static turret, prevent gunner AI from dismounting, even if unable to engage threats.")]
	bool m_noTurretDismount;
	
	SCR_BaseCompartmentManagerComponent m_cm;
	
	void SpawnNextGroup(bool removePrevious = true)
	{
		if (removePrevious)
			m_crewGroups.RemoveOrdered(0);
		if (!m_crewGroups.IsEmpty())
			m_crewGroups[0].Spawn(this);
	}
	
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Crew Group")]
class TILW_CrewGroup
{
	[Attribute("1", UIWidgets.Auto, desc: "Driver", category: "Compartments")]
	protected bool m_allowPilot;
	[Attribute("1", UIWidgets.Auto, desc: "Gunner", category: "Compartments")]
	protected bool m_allowTurret;
	[Attribute("1", UIWidgets.Auto, desc: "Passengers", category: "Compartments")]
	protected bool m_allowCargo;
	
	[Attribute("", UIWidgets.ResourceAssignArray, desc: "List of character prefabs to spawn, empty resources will become empty seats. If not defined, the vehicles default characters are used. \nYou can check the vehicles CompartmentManagerComponent for the primary compartments slot order, secondary compartsments (e. g. BTR gunner) come afterwards.", category: "Crew", params: "et")]
	protected ref array<ResourceName> m_characters;
	
	[Attribute("", UIWidgets.Auto, desc: "Names of existing waypoints to be assigned to the group", category: "Waypoints")]
	protected ref array<string> m_waypointNames;
	
	[Attribute("0", UIWidgets.Auto, desc: "After how many seconds to assign these waypoints", category: "Waypoints", params: "0 inf 0.1")]
	protected float m_waypointDelay;
	
	protected TILW_CrewConfig m_cc;
	protected AIGroup m_aiGroup = null;
	protected ref array<BaseCompartmentSlot> m_validSlots = {};
	
	void Spawn(TILW_CrewConfig cc)
	{
		m_cc = cc;
		SetFreeCompartmentsOfTypes();
		GetGame().GetCallqueue().Call(ProcessSlot);
	}
	
	
	protected void ProcessSlot()
	{
		if (m_validSlots.IsEmpty())
		{
			// Finished spawning
			GetGame().GetCallqueue().CallLater(AssignWaypoints, m_waypointDelay * 1000, false, m_aiGroup, m_waypointNames);
			m_cc.SpawnNextGroup();
			return;
		}
		
		BaseCompartmentSlot slot = m_validSlots[0];
		m_validSlots.RemoveOrdered(0);
		
		if (slot && !slot.IsOccupied())
		{
			IEntity ce;
			if (m_characters.IsEmpty())
				ce = slot.SpawnDefaultCharacterInCompartment(m_aiGroup);
			else
			{
				if (m_characters[0] != "")
					ce = slot.SpawnCharacterInCompartment(m_characters[0], m_aiGroup);
				m_characters.RemoveOrdered(0);
			}
			
			if (ce && m_cc.m_noTurretDismount && slot.GetType() == ECompartmentType.TURRET) {
				Managed m = ce.FindComponent(SCR_AICombatComponent);
				if (m)
				{
					SCR_AICombatComponent cc = SCR_AICombatComponent.Cast(m);
					cc.m_neverDismountTurret = true;
				}
			}
		}
		
		GetGame().GetCallqueue().Call(ProcessSlot);
		
	}
	
	static void AssignWaypoints(AIGroup g, array<string> waypointNames)
	{
		if (!g || !waypointNames)
			return;
		foreach (string name : waypointNames)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(name);
			if (!e)
				continue;
			AIWaypoint wp = AIWaypoint.Cast(e);
			if (!wp)
				continue;
			g.AddWaypoint(wp);
		}
	}
	
	protected void SetFreeCompartmentsOfTypes()
	{
		array<ECompartmentType> cTypes = {};
		if (m_allowPilot)
			cTypes.Insert(ECompartmentType.PILOT);
		if (m_allowTurret)
			cTypes.Insert(ECompartmentType.TURRET);
		if (m_allowCargo)
			cTypes.Insert(ECompartmentType.CARGO);
		
		array<BaseCompartmentSlot> compartmentSlots = {}; 
		m_cc.m_cm.GetCompartments(compartmentSlots);
		foreach (BaseCompartmentSlot slot: compartmentSlots)
		{
			if (slot && cTypes.Contains(slot.GetType()) && !slot.IsOccupied() && slot.IsCompartmentAccessible() && (!m_characters.IsEmpty() || !slot.GetDefaultOccupantPrefab().IsEmpty()))
				m_validSlots.InsertAt(slot, 0);
		}
	}
}