enum EJIPState
{
	Deny,
	Auto,
	Manual
} 

modded class TILW_MissionFrameworkEntity
{
	[Attribute("0", UIWidgets.ComboBox, "What is the JIP behavior?", "", ParamEnumArray.FromEnum(EJIPState), category: "JIP")]
	EJIPState m_eJIPState;	
	
	[Attribute("0", UIWidgets.Auto, "How many seconds until JIP is deined. 0 = never", "0 999999 0", category: "JIP")]
	int m_denyJIPTime;
	
	[Attribute("30", UIWidgets.Auto, "The minimum distance for JIP to be available.", category: "JIP")]
	protected int m_minDistance;
	
	protected ref map<IEntity, string> m_JIP_List = new map<IEntity, string>();
	
	[RplProp()]
	WorldTimestamp m_startTime;
	
	override protected void GameModeStart(SCR_EGameModeState state)
	{
		super.GameModeStart(state);
		
		if(state == SCR_EGameModeState.GAME && !m_startTime)
		{
			ChimeraWorld world = GetGame().GetWorld();
			m_startTime = world.GetLocalTimestamp();
			Replication.BumpMe();
		}
	}
	
	void TryJIP(SCR_PlayerController playerController)
	{
		int playerId = playerController.GetPlayerId();
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if(!player)
			return;
		
		string uid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if(m_JIP_List.Contains(player))
		{
			string lastUID = m_JIP_List.Get(player);
			if(lastUID == uid)
				return playerController.SetJIP(false);
		}

		ChimeraWorld world = GetGame().GetWorld();
		WorldTimestamp currentTime = world.GetLocalTimestamp();
		if(currentTime.LessEqual(m_startTime.PlusSeconds(120 * 1000)))
			playerController.SetJIP(false);

		if(m_denyJIPTime)
		{
			if(currentTime.Greater(currentTime.PlusSeconds(m_denyJIPTime)))
				playerController.SetJIP(false, "JIP deny time reached!");
		}
		
		SCR_PlayerControllerGroupComponent groupComp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		SCR_AIGroup group = groupComp.GetPlayersGroup();
		if(!group)
			return playerController.SetJIP(false);

		IEntity targetEntity;
		IEntity leader = group.GetLeaderEntity();
		if(!leader || leader == player || !EntityUtils.IsPlayer(leader))
		{
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			foreach(AIAgent agent : agents)
			{
				IEntity	entity = agent.GetControlledEntity();
				if(!entity)
					continue;
				
				if(entity == leader)
					continue;
				
				if(!EntityUtils.IsPlayer(entity))
					continue;
				
				targetEntity = entity;
				break;
			}
		}
		else
		{
			targetEntity = leader;
		}
		
		if(!targetEntity)
			return playerController.SetJIP(false, "No players found.");
		
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(targetEntity);
		if(vehicle)
		{
			playerController.GetInVehicle(vehicle);
			GetGame().GetCallqueue().CallLater(CheckPlayerJIPed, 1000, false, playerId);
			return;
		}

		float distance = vector.Distance(player.GetOrigin(), targetEntity.GetOrigin());
		if (distance < m_minDistance)
			return playerController.SetJIP(false);

		vector transform[4];
		targetEntity.GetTransform(transform);
		
    	vector behindPosition = transform[3] - (transform[2] * 0.5);
		
		playerController.Teleport(behindPosition);
		m_JIP_List.Insert(player, uid);
	}
	
	protected void CheckPlayerJIPed(int playerId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if(!player)
			return;

		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(player);
		if(vehicle)
		{
			string uid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
			m_JIP_List.Insert(player, uid);
		}
	}
}
