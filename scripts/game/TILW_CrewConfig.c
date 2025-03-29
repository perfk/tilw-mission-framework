[BaseContainerProps(configRoot: true), BaseContainerCustomStringTitleField("Crew Config")]
class TILW_CrewConfig
{
	[Attribute("", UIWidgets.Object, desc: "AI groups spawned in the vehicle.")]
	protected ref array<ref TILW_CrewGroup> m_crewGroups;
	
	[Attribute("0", UIWidgets.Auto, desc: "If entity is a static turret, prevent gunner AI from dismounting, even if unable to engage threats.")]
	bool m_noTurretDismount;
	
	[Attribute("1", UIWidgets.Auto, desc: "If a group has waypoints assigned, spawn gunners in a separate group.")]
	bool m_useIdleGroups;
	
	void SpawnCrew(SCR_BaseCompartmentManagerComponent cm)
	{
		array<BaseCompartmentSlot> compartmentSlots = {};
		cm.GetCompartments(compartmentSlots);
		SpawnNextGroup(compartmentSlots, 0);
	}
	
	void SpawnNextGroup(array<BaseCompartmentSlot> slots, int groupIndex)
	{
		if (m_crewGroups.Count() > groupIndex)
			m_crewGroups[groupIndex].Spawn(this, slots, groupIndex);
	}
	
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Group")]
class TILW_CrewGroup
{
	[Attribute("", UIWidgets.Object, desc: "Crew stations are used to assign the groups characters to certain seat types.")]
	protected ref array<ref TILW_CrewStation> m_crewStations;
	
	[Attribute("", UIWidgets.Auto, desc: "Names of existing waypoints to be assigned to the group", category: "Waypoints")]
	ref array<string> m_waypointNames;
	
	[Attribute("0", UIWidgets.Auto, desc: "After how many seconds to assign these waypoints", category: "Waypoints", params: "0 inf 0.1")]
	protected float m_waypointDelay;
	
	[Attribute("", UIWidgets.Auto, desc: "Optional. Display name, used to set the name of player groups. ONLY DISPLAYED IN MP.", category: "Group")]
	string m_displayName; // only works in mp
	
	[Attribute("", UIWidgets.Auto, desc: "Optional. Unique entity name for this group, to be used when creating a group with members outside of this vehicle. Groups with the same name are essentially merged.\nONLY SET THIS ON A MISSION LEVEL!", category: "Group")]
	string m_entityName;
	
	[Attribute("1", UIWidgets.Auto, desc: "Whether this group should be spawned.", category: "Group")]
	protected bool m_enabled;
	
	TILW_CrewConfig m_cc;
	
	void Spawn(TILW_CrewConfig cc, array<BaseCompartmentSlot> slots, int groupIndex)
	{
		if (!m_enabled)
		{
			cc.SpawnNextGroup(slots, groupIndex + 1);
			return;
		}
		m_cc = cc;
		SpawnNextStation(slots, groupIndex, 0, null);
	}
	
	void SpawnNextStation(array<BaseCompartmentSlot> slots, int groupIndex, int stationIndex, AIGroup aiGroup)
	{
		if (m_crewStations.Count() > stationIndex)
			GetGame().GetCallqueue().Call(m_crewStations[stationIndex].Spawn, this, slots, groupIndex, stationIndex, aiGroup);
		else
		{
			// Finished spawning group
			GetGame().GetCallqueue().CallLater(AssignWaypoints, m_waypointDelay * 1000, false, aiGroup, m_waypointNames, false, true);
			m_cc.SpawnNextGroup(slots, groupIndex + 1);
		}
	}
	
	static void AssignWaypoints(AIGroup g, array<string> waypointNames, bool clearExisting = false, bool initial = false)
	{
		if (!g || !waypointNames)
			return;
		if (clearExisting || initial)
		{
			array<AIWaypoint> wps = {};
			g.GetWaypoints(wps);
			if (initial && wps.Count() > 0)
				return;
			if (clearExisting)
				foreach (AIWaypoint wp: wps)
					g.RemoveWaypoint(wp);
		}
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
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Station")]
class TILW_CrewStation
{
	[Attribute("1", UIWidgets.Auto, desc: "Driver", category: "Compartments")]
	protected bool m_allowPilot;
	[Attribute("1", UIWidgets.Auto, desc: "Gunner", category: "Compartments")]
	protected bool m_allowTurret;
	[Attribute("1", UIWidgets.Auto, desc: "Passengers", category: "Compartments")]
	protected bool m_allowCargo;
	
	[Attribute("", UIWidgets.ResourceAssignArray, desc: "List of character prefabs to spawn, empty resources will become empty seats. If not defined, all qualifying slots are filled with the vehicles default characters.", category: "Crew", params: "et")]
	protected ref array<ResourceName> m_characters;
	
	[Attribute("1", UIWidgets.Auto, desc: "Whether this station should be spawned.")]
	protected bool m_enabled;
	
	protected TILW_CrewGroup m_cg;
	
	void Spawn(TILW_CrewGroup cg, array<BaseCompartmentSlot> slots, int groupIndex, int stationIndex, AIGroup aiGroup)
	{
		if (!m_enabled)
		{
			cg.SpawnNextStation(slots, groupIndex, stationIndex + 1, aiGroup);
			return;
		}
		m_cg = cg;
		GetGame().GetCallqueue().Call(ProcessSlot, slots, groupIndex, stationIndex, 0, aiGroup);
	}
	
	protected void ProcessSlot(array<BaseCompartmentSlot> slots, int groupIndex, int stationIndex, int characterIndex, AIGroup aiGroup)
	{
		// Pick next slot
		BaseCompartmentSlot slot = GetNextSlot(slots);
		slots.RemoveItemOrdered(slot);
		
		// Check if finished (no more slots available, or no characters left to spawn)
		if (!slot || (!m_characters.IsEmpty() && characterIndex > m_characters.Count() - 1))
		{
			m_cg.SpawnNextStation(slots, groupIndex, stationIndex + 1, aiGroup);
			return;
		}
		
		// Decide on AI group
		AIGroup g = aiGroup;
		bool spawnIdle = (slot.GetType() == ECompartmentType.TURRET) && m_cg.m_cc.m_useIdleGroups && !m_cg.m_waypointNames.IsEmpty();
		if (spawnIdle)
			g = null;
		else if (!g && m_cg.m_entityName != "")
		{
			g = aiGroup;
			IEntity e = GetGame().GetWorld().FindEntityByName(m_cg.m_entityName);
			if (e)
				g = SCR_AIGroup.Cast(e);
		}
		
		// Spawn character
		IEntity ce;
		if (m_characters.IsEmpty())
			ce = slot.SpawnDefaultCharacterInCompartment(g);
		else
		{
			if (m_characters[0] != "")
				ce = slot.SpawnCharacterInCompartment(m_characters[characterIndex], g);
		}
		
		// Maybe prevent dismount
		if (ce && m_cg.m_cc.m_noTurretDismount && slot.GetType() == ECompartmentType.TURRET) {
			SCR_AICombatComponent cc = SCR_AICombatComponent.Cast(ce.FindComponent(SCR_AICombatComponent));
			if (cc)
				cc.m_neverDismountTurret = true;
		}
		
		// Configure new group and save it for later
		if (!aiGroup && ce && g && !spawnIdle)
		{
			if (m_cg.m_entityName != "")
				g.SetName(m_cg.m_entityName);
			
			SCR_AIGroup scr_g = SCR_AIGroup.Cast(g);
			if (scr_g && m_cg.m_displayName != "")
			{
				scr_g.SetCustomName(m_cg.m_displayName, 0);
				g = scr_g;
			}
			
			aiGroup = g;
		}
		
		GetGame().GetCallqueue().Call(ProcessSlot, slots, groupIndex, stationIndex, characterIndex + 1, aiGroup);
	}
	 
	protected BaseCompartmentSlot GetNextSlot(array<BaseCompartmentSlot> slots)
	{
		array<ECompartmentType> cTypes = {};
		if (m_allowPilot)
			cTypes.Insert(ECompartmentType.PILOT);
		if (m_allowTurret)
			cTypes.Insert(ECompartmentType.TURRET);
		if (m_allowCargo)
			cTypes.Insert(ECompartmentType.CARGO);
		
		foreach (BaseCompartmentSlot slot: slots)
		{
			if (slot && cTypes.Contains(slot.GetType()) && !slot.IsOccupied() && slot.IsCompartmentAccessible() && (!m_characters.IsEmpty() || !slot.GetDefaultOccupantPrefab().IsEmpty()))
				return slot;
		}
		return null;
	}
}