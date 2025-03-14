[EntityEditorProps(category: "game/", description: "Crews compartments after initialization with their default passenger.")]
class TILW_VehicleCrewComponentClass: ScriptComponentClass
{
}

class TILW_VehicleCrewComponent: ScriptComponent
{
	
	[Attribute("", UIWidgets.Object, desc: "Defines the vehicles crew - you may drag existing configs into here.", category: "Crew")]
	protected ref TILW_CrewConfig m_crewConfig;
	
	[Attribute("0", UIWidgets.Auto, desc: "Already spawn characters before going ingame - this is used for player characters.", category: "Crew")]
	protected bool m_pregameSpawn;
	
	
	[Attribute("0", UIWidgets.Auto, desc: "Spawn Default Driver", category: "Deprecated")]
	protected bool m_spawnPilot;
	[Attribute("0", UIWidgets.Auto, desc: "Spawn Default Gunner", category: "Deprecated")]
	protected bool m_spawnTurret;
	[Attribute("0", UIWidgets.Auto, desc: "Spawn Default Passengers", category: "Deprecated")]
	protected bool m_spawnCargo;
	
	[Attribute("", UIWidgets.ResourceAssignArray, desc: "If defined, fill seats with these characters instead of the vehicles default characters. \nList of character prefabs, empty resources will become empty seats. \nYou can check the vehicles CompartmentManagerComponent for the primary compartments slot order, secondary compartsments (e. g. BTR gunner) come afterwards.", category: "Deprecated", params: "et")]
	protected ref array<ResourceName> m_customCrew;
	
	[Attribute("", UIWidgets.Auto, desc: "Names of existing waypoints to be assigned after delay", category: "Deprecated")]
	protected ref array<string> m_waypointNames;
	
	[Attribute("5", UIWidgets.Auto, desc: "After how many seconds to assign the waypoints.", category: "Deprecated", params: "0 inf 0.1")]
	protected float m_waypointDelay;
	
	[Attribute("0", UIWidgets.Auto, desc: "If entity is a static turret, prevent the gunner from dismounting, even if unable to engage threats.", category: "Deprecated")]
	protected bool m_noTurretDismount;
	
	[Attribute("0", UIWidgets.Auto, desc: "Put all gunners into a separate group, so that they remain idle (and can engage targets), while only driver + passengers follow a higher priority waypoint.", category: "Deprecated")]
	protected bool m_idleGroup;
	
	[Attribute("0", UIWidgets.Auto, desc: "After how many seconds to spawn the crew - useful for timing otherwise simultaneous VCC spawns.", category: "Deprecated", params: "0 inf 0.1")]
	protected float m_spawnDelay;
	
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;
		
		if (m_crewConfig)
		{
		
			Managed m = GetOwner().FindComponent(SCR_BaseCompartmentManagerComponent);
			if (!m)
				return;
			m_crewConfig.m_cm = SCR_BaseCompartmentManagerComponent.Cast(m);
		}
		
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gm)
			return;
		
		gm.GetOnGameStateChange().Insert(GameStateChange);
		GameStateChange(gm.GetState());
	}
	
	protected void GameStateChange(SCR_EGameModeState state)
	{
		if (!m_pregameSpawn && state != SCR_EGameModeState.GAME)
			return;
		
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gm)
			gm.GetOnGameStateChange().Remove(GameStateChange);
		
		if (m_crewConfig)
			m_crewConfig.SpawnNextGroup(false);
		else
			GetGame().GetCallqueue().CallLater(AddCrew, m_spawnDelay * 1000, false);
	}
	
	protected void AddCrew()
	{
		Managed m = GetOwner().FindComponent(SCR_BaseCompartmentManagerComponent);
		if (!m)
			return;
		SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(m);
		if (!cm)
			return;
		
		array<BaseCompartmentSlot> slots = {};
		cm.GetCompartments(slots);
		Print(slots);
		
		array<ECompartmentType> cTypes = {};
		if (m_spawnPilot)
			cTypes.Insert(ECompartmentType.PILOT);
		if (m_spawnTurret)
			cTypes.Insert(ECompartmentType.TURRET);
		if (m_spawnCargo)
			cTypes.Insert(ECompartmentType.CARGO);
		
		array<AIGroup> groups = {null, null};
		array<bool> boolParams = {m_noTurretDismount, m_idleGroup};
		GetGame().GetCallqueue().Call(SpawnCrew, slots, cTypes, m_customCrew, groups, boolParams, m_waypointNames, m_waypointDelay, this);
	}
	
	static void SpawnCrew(array<BaseCompartmentSlot> slots, array<ECompartmentType> cTypes, array<ResourceName> characters, array<AIGroup> groups, array<bool> boolParams, array<string> waypointNames = null, float wpDelay = 0, TILW_VehicleCrewComponent vcc = null)
	{
		bool noTurretDismount = boolParams[0];
		bool useIdleGroup = boolParams[1];
		
		AIGroup mainGroup = groups[0];
		AIGroup idleGroup = groups[1];
		
		if (slots.IsEmpty() || !characters) // No more slots, or characters have been exhausted (and array has been set to null)
		{
			GetGame().GetCallqueue().CallLater(AddWaypointsStatic, wpDelay * 1000, false, groups[0], waypointNames);
			return;
		}
		
		if (slots[0].IsOccupied())
		{
			slots.RemoveOrdered(0);
			GetGame().GetCallqueue().Call(SpawnCrew, slots, cTypes, characters, groups, boolParams, waypointNames, wpDelay);
			return;
		}
		
		// Spawn character
		IEntity ce;
		
		bool isValidType = cTypes.Contains(slots[0].GetType()); // is this slots type allowed
		if (!characters.IsEmpty() && characters[0] != "" && isValidType)
		{ // character array not empty, use these
			// Custom character
			if (useIdleGroup && slots[0].GetType() == ECompartmentType.TURRET)
				ce = slots[0].SpawnCharacterInCompartment(characters[0], idleGroup);
			else
				ce = slots[0].SpawnCharacterInCompartment(characters[0], mainGroup);
		}
		else if (characters.IsEmpty() && isValidType)
		{ // character array empty (but not null), use default characters
			// Default character
			if (useIdleGroup && slots[0].GetType() == ECompartmentType.TURRET)
				ce = slots[0].SpawnDefaultCharacterInCompartment(idleGroup);
			else
				ce = slots[0].SpawnDefaultCharacterInCompartment(mainGroup);
		}
		
		
		groups[0] = mainGroup;
		groups[1] = idleGroup;
		
		// Prevent turret dismount
		if (ce && noTurretDismount && slots[0].GetType() == ECompartmentType.TURRET) {
			Managed m = ce.FindComponent(SCR_AICombatComponent);
			if (m)
			{
				SCR_AICombatComponent cc = SCR_AICombatComponent.Cast(m);
				cc.m_neverDismountTurret = true;
			}
		}
		
		// Remove from spawn list
		if (slots && !slots.IsEmpty())
			slots.RemoveOrdered(0);
		if (!characters.IsEmpty() && isValidType)
		{ // if character from array was spawned, remove it
			characters.RemoveOrdered(0);
			if (characters.IsEmpty())
				characters = null; // if this was the last character, set to null as sign to stop
		}
		
		// Queue up next character
		GetGame().GetCallqueue().Call(SpawnCrew, slots, cTypes, characters, groups, boolParams, waypointNames, wpDelay);
	}
	
	static void AddWaypointsStatic(AIGroup g, array<string> waypointNames)
	{
		if (!g || !waypointNames || waypointNames.IsEmpty())
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
	
	static void PreventTurretDismount(SCR_BaseCompartmentManagerComponent cm)
	{
		array<IEntity> occupants = new array<IEntity>;
		cm.GetOccupantsOfType(occupants, ECompartmentType.TURRET);
		foreach(IEntity gunner : occupants)
		{
			Managed m = gunner.FindComponent(SCR_AICombatComponent);
			if (!m)
				continue;
			SCR_AICombatComponent combComp = SCR_AICombatComponent.Cast(m);
			combComp.m_neverDismountTurret = true;
		}
	}
}